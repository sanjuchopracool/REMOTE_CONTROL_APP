// Copyright (C) 2013 BlackBerry Limited. All rights reserved.
// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "device.h"

#include <QBluetoothDeviceInfo>
#include <QBluetoothUuid>

#include <QDebug>
#include <QMetaObject>
#include <QTimer>

#if QT_CONFIG(permissions)
#include <QPermissions>

#include <QGuiApplication>
#endif

#include <QBluetoothUuid>

using namespace Qt::StringLiterals;

namespace {
const QUuid tx_uuid("{6e400003-b5a3-f393-e0a9-e50e24dcca9e}");
const QUuid rx_uuid("{6e400002-b5a3-f393-e0a9-e50e24dcca9e}");
}
Device::Device()
{
    //! [les-devicediscovery-1]
    discoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);
    discoveryAgent->setLowEnergyDiscoveryTimeout(25000);
    connect(discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
            this, &Device::addDevice);
    connect(discoveryAgent, &QBluetoothDeviceDiscoveryAgent::errorOccurred,
            this, &Device::deviceScanError);
    connect(discoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished,
            this, &Device::deviceScanFinished);
    connect(discoveryAgent, &QBluetoothDeviceDiscoveryAgent::canceled,
            this, &Device::deviceScanFinished);
    //! [les-devicediscovery-1]

    setUpdate(u"Search"_s);
    m_data_timer = new QTimer(this);
    connect(m_data_timer, &QTimer::timeout, this, &Device::writeData);
    m_data_timer->start(1000);
}

Device::~Device()
{
    qDeleteAll(devices);
    qDeleteAll(m_services);
    qDeleteAll(m_characteristics);
    devices.clear();
    m_services.clear();
    m_characteristics.clear();
}

void Device::startDeviceDiscovery()
{
    qDeleteAll(devices);
    devices.clear();
    emit devicesUpdated();

    //! [les-devicediscovery-2]
    discoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
    //! [les-devicediscovery-2]

    if (discoveryAgent->isActive()) {
        setUpdate(u"Stop"_s);
        m_deviceScanState = true;
        Q_EMIT stateChanged();
    }
}

void Device::stopDeviceDiscovery()
{
    if (discoveryAgent->isActive())
        discoveryAgent->stop();
}

//! [les-devicediscovery-3]
void Device::addDevice(const QBluetoothDeviceInfo &info)
{
    if (info.coreConfigurations() & QBluetoothDeviceInfo::LowEnergyCoreConfiguration) {
        // // it should have ESP32 in the name
        // if (!info.name().contains("ESP32")) {
        //     return;
        // }

        auto devInfo = new DeviceInfo(info);
        auto it = std::find_if(devices.begin(), devices.end(),
                               [devInfo](DeviceInfo *dev) {
                                   return devInfo->getAddress() == dev->getAddress();
                               });
        if (it == devices.end()) {
            devices.append(devInfo);
        } else {
            auto oldDev = *it;
            *it = devInfo;
            delete oldDev;
        }
        emit devicesUpdated();
    }
}
//! [les-devicediscovery-3]

void Device::deviceScanFinished()
{
    m_deviceScanState = false;
    emit stateChanged();
    if (devices.isEmpty())
        setUpdate(u"No Low Energy devices found..."_s);
    else
        setUpdate(u"Done! Scan Again!"_s);
}

QVariant Device::getDevices()
{
    return QVariant::fromValue(devices);
}

QVariant Device::getServices()
{
    return QVariant::fromValue(m_services);
}

QVariant Device::getCharacteristics()
{
    return QVariant::fromValue(m_characteristics);
}

QString Device::getUpdate()
{
    return m_message;
}

void Device::scanServices(const QString &address)
{
    // We need the current device for service discovery.

    for (auto d: std::as_const(devices)) {
        if (auto device = qobject_cast<DeviceInfo *>(d)) {
            if (device->getAddress() == address) {
                currentDevice.setDevice(device->getDevice());
                break;
            }
        }
    }

    if (!currentDevice.getDevice().isValid()) {
        qWarning() << "Not a valid device";
        return;
    }

    qDeleteAll(m_characteristics);
    m_characteristics.clear();
    emit characteristicsUpdated();
    qDeleteAll(m_services);
    m_services.clear();
    emit servicesUpdated();

    setUpdate(u"Back\n(Connecting to device...)"_s);

    if (controller && m_previousAddress != currentDevice.getAddress()) {
        controller->disconnectFromDevice();
        delete controller;
        controller = nullptr;
    }

    //! [les-controller-1]
    if (!controller) {
        // Connecting signals and slots for connecting to LE services.
        controller = QLowEnergyController::createCentral(currentDevice.getDevice(), this);
        connect(controller, &QLowEnergyController::connected,
                this, &Device::deviceConnected);
        connect(controller, &QLowEnergyController::errorOccurred, this, &Device::errorReceived);
        connect(controller, &QLowEnergyController::disconnected,
                this, &Device::deviceDisconnected);
        connect(controller, &QLowEnergyController::serviceDiscovered,
                this, &Device::addLowEnergyService);
        connect(controller, &QLowEnergyController::discoveryFinished,
                this, &Device::serviceScanDone);
    }

    if (isRandomAddress())
        controller->setRemoteAddressType(QLowEnergyController::RandomAddress);
    else
        controller->setRemoteAddressType(QLowEnergyController::PublicAddress);
    controller->connectToDevice();
    //! [les-controller-1]

    m_previousAddress = currentDevice.getAddress();
}

void Device::addLowEnergyService(const QBluetoothUuid &serviceUuid)
{
    //! [les-service-1]
    QLowEnergyService *service = controller->createServiceObject(serviceUuid);
    if (!service) {
        qWarning() << "Cannot create service for uuid";
        return;
    }

    //! [les-service-1]
    auto serv = new ServiceInfo(service);
    m_services.append(serv);

    emit servicesUpdated();
}
//! [les-service-1]

void Device::serviceScanDone()
{
    setUpdate(u"Back\n(Service scan done!)"_s);
    // force UI in case we didn't find anything
    if (m_services.isEmpty())
        emit servicesUpdated();
}

void Device::connectToService(const QString &uuid)
{
    QLowEnergyService *service = nullptr;
    for (auto s: std::as_const(m_services)) {
        auto serviceInfo = qobject_cast<ServiceInfo *>(s);
        if (!serviceInfo)
            continue;

        if (serviceInfo->getUuid() == uuid) {
            service = serviceInfo->service();
            break;
        }
    }

    m_rx_tx_service = service;
    qDebug() << "SANJAY SERVICE =>>>>>>>>>>>" << m_rx_tx_service;
    if (!m_rx_tx_service)
        return;

    connect(m_rx_tx_service,
            &QLowEnergyService::characteristicChanged,
            this,
            [this](const QLowEnergyCharacteristic &ch, const QByteArray &data) {
                qDebug() << "CH name = " << ch.name() << " uuid = " << ch.uuid() << data;

                auto characteristic = m_rx_tx_service->characteristic(QBluetoothUuid(rx_uuid));
                if (characteristic.isValid()) {
                    // qDebug() << "CH name = " << characteristic.name()
                    //          << " uuid = " << characteristic.uuid() << data;
                    auto writeDes = characteristic.descriptor(
                        QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);
                    if (writeDes.isValid()) {
                        m_rx_tx_service->writeDescriptor(writeDes, "Hello");
                    } else {
                        // qDebug() << "Invalid write des!";
                        m_rx_tx_service
                            ->writeCharacteristic(characteristic,
                                                  "Hello Sanjay Chopra",
                                                  QLowEnergyService::WriteMode::WriteWithResponse);
                    }
                }
            });

    connect(m_rx_tx_service,
            &QLowEnergyService::stateChanged,
            this,
            [this](QLowEnergyService::ServiceState newState) {
                qDebug() << "SANJAY new State = " << newState;
            });

    connect(m_rx_tx_service,
            &QLowEnergyService::characteristicRead,
            this,
            [this](const QLowEnergyCharacteristic &info, const QByteArray &value) {
                qDebug() << "SANJAY Ch read = " << info.uuid() << " value = " << value;
            });

    connect(m_rx_tx_service,
            &QLowEnergyService::characteristicWritten,
            this,
            [this](const QLowEnergyCharacteristic &info, const QByteArray &value) {
                qDebug() << "SANJAY Ch written = " << info.uuid() << " value = " << value;
            });

    connect(m_rx_tx_service,
            &QLowEnergyService::descriptorWritten,
            this,
            [this](const QLowEnergyDescriptor &info, const QByteArray &value) {
                qDebug() << "SANJAY ds written = " << info.uuid() << " value = " << value;
            });

    connect(m_rx_tx_service,
            &QLowEnergyService::descriptorRead,
            this,
            [this](const QLowEnergyDescriptor &info, const QByteArray &value) {
                qDebug() << "SANJAY ds read = " << info.uuid() << " value = " << value;
            });

    // connect(m_service, &QLowEnergyService::stateChanged, this, &DeviceHandler::serviceStateChanged);
    // connect(m_service, &QLowEnergyService::characteristicChanged, this, &DeviceHandler::updateHeartRateValue);
    // connect(m_service, &QLowEnergyService::descriptorWritten, this, &DeviceHandler::confirmedDescriptorWrite);

    qDeleteAll(m_characteristics);
    m_characteristics.clear();
    emit characteristicsUpdated();

    if (m_rx_tx_service->state() == QLowEnergyService::RemoteService) {
        //! [les-service-3]
        connect(m_rx_tx_service,
                &QLowEnergyService::stateChanged,
                this,
                &Device::serviceDetailsDiscovered);
        m_rx_tx_service->discoverDetails();
        setUpdate(u"Back\n(Discovering details...)"_s);
        //! [les-service-3]
        qDebug() << "SANJAY SERVICE 2 =>>>>>>>>>>>" << m_rx_tx_service;
        return;
    }

    //discovery already done
    const QList<QLowEnergyCharacteristic> chars = m_rx_tx_service->characteristics();
    for (const QLowEnergyCharacteristic &ch : chars) {
        auto cInfo = new CharacteristicInfo(ch);
        m_characteristics.append(cInfo);
    }

    qDebug() << "SANJAY =>>>>>>>>>>>" << m_rx_tx_service;
    QTimer::singleShot(0, this, &Device::characteristicsUpdated);
}

void Device::deviceConnected()
{
    setUpdate(u"Back\n(Discovering services...)"_s);
    connected = true;
    //! [les-service-2]
    controller->discoverServices();
    //! [les-service-2]
}

void Device::errorReceived(QLowEnergyController::Error /*error*/)
{
    qWarning() << "Error: " << controller->errorString();
    setUpdate(u"Back\n(%1)"_s.arg(controller->errorString()));
}

void Device::setUpdate(const QString &message)
{
    m_message = message;
    emit updateChanged();
}

void Device::disconnectFromDevice()
{
    // UI always expects disconnect() signal when calling this signal
    // TODO what is really needed is to extend state() to a multi value
    // and thus allowing UI to keep track of controller progress in addition to
    // device scan progress

    if (controller->state() != QLowEnergyController::UnconnectedState)
        controller->disconnectFromDevice();
    else
        deviceDisconnected();
}

void Device::deviceDisconnected()
{
    qWarning() << "Disconnect from device";
    connected = false;
    emit disconnected();
}

void Device::serviceDetailsDiscovered(QLowEnergyService::ServiceState newState)
{
    if (newState != QLowEnergyService::RemoteServiceDiscovered) {
        // do not hang in "Scanning for characteristics" mode forever
        // in case the service discovery failed
        // We have to queue the signal up to give UI time to even enter
        // the above mode
        if (newState != QLowEnergyService::RemoteServiceDiscovering) {
            QMetaObject::invokeMethod(this, "characteristicsUpdated",
                                      Qt::QueuedConnection);
        }
        qDebug() << "SANJAY SERVICE 3 =>>>>>>>>>>>" << m_rx_tx_service;
        return;
    }

    auto service = qobject_cast<QLowEnergyService *>(sender());
    if (!service) {
        qDebug() << "SANJAY SERVICE 4 =>>>>>>>>>>>" << m_rx_tx_service;
        return;
    }

    //! [les-chars]
    const QList<QLowEnergyCharacteristic> chars = service->characteristics();
    for (const QLowEnergyCharacteristic &ch : chars) {
        auto cInfo = new CharacteristicInfo(ch);
        m_characteristics.append(cInfo);
    }
    //! [les-chars]

    for (CharacteristicInfo *characteristicInfo : m_characteristics) {
        auto characteristic = characteristicInfo->getCharacteristic();
        if (characteristic.isValid()) {
            qDebug() << "Characterstic Name = " << characteristic.name() << " "
                     << characteristic.uuid() << " " << characteristic.value();
            for (const auto &info : characteristic.descriptors()) {
                qDebug() << " valid = " << info.isValid() << " value = " << info.value()
                         << " name = " << info.name() << " type = " << info.type()
                         << " uuid = " << info.uuid();
            }
        }

        auto m_notificationDesc = characteristic.descriptor(
            QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);
        if (m_notificationDesc.isValid()) {
            qDebug() << "Control Point Descriptor";
            m_rx_tx_service->writeDescriptor(m_notificationDesc, QByteArray::fromHex("0100"));
        }
    }
    emit characteristicsUpdated();
}

void Device::writeData()
{
    return;
    qDebug() << Q_FUNC_INFO << connected;
    if (controller) {
        qDebug() << controller->state();
    }
    if (connected && controller->state() == QLowEnergyController::DiscoveredState) {
        if (m_rx_tx_service) {
            for (const auto &info : m_rx_tx_service->characteristics()) {
                auto types = info.properties();
                qDebug() << QString("Permission 0x%1").arg(types, 8, 16, QLatin1Char('0'))
                         << info.name() << " uuid = " << info.uuid() << "value = " << info.value();
            }
            QLowEnergyCharacteristic characteristic = m_rx_tx_service->characteristic(
                QBluetoothUuid(tx_uuid));
            if (characteristic.isValid()) {
                for (const auto &info : characteristic.descriptors()) {
                    qDebug() << "Tx valid = " << info.isValid() << " value = " << info.value()
                             << " name = " << info.name() << " type = " << info.type()
                             << " uuid = " << info.uuid();
                }
            }

            characteristic = m_rx_tx_service->characteristic(QBluetoothUuid(rx_uuid));
            if (characteristic.isValid()) {
                for (const auto &info : characteristic.descriptors()) {
                    qDebug() << "RX valid = " << info.isValid() << " value = " << info.value()
                             << " name = " << info.name() << " type = " << info.type()
                             << " uuid = " << info.uuid();
                }
            }
            auto m_notificationDesc = characteristic.descriptor(
                QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);
            if (m_notificationDesc.isValid()) {
                qDebug() << "Control Point Descriptor";
                m_rx_tx_service->writeDescriptor(m_notificationDesc, QByteArray::fromHex("0100"));
            }
        }
    }
}

void Device::deviceScanError(QBluetoothDeviceDiscoveryAgent::Error error)
{
    if (error == QBluetoothDeviceDiscoveryAgent::PoweredOffError) {
        setUpdate(u"The Bluetooth adaptor is powered off, power it on before doing discovery."_s);
    } else if (error == QBluetoothDeviceDiscoveryAgent::InputOutputError) {
        setUpdate(u"Writing or reading from the device resulted in an error."_s);
    } else {
        static QMetaEnum qme = discoveryAgent->metaObject()->enumerator(
                    discoveryAgent->metaObject()->indexOfEnumerator("Error"));
        setUpdate(u"Error: "_s + QLatin1StringView(qme.valueToKey(error)));
    }

    m_deviceScanState = false;
    emit stateChanged();
}

bool Device::state()
{
    return m_deviceScanState;
}

bool Device::hasControllerError() const
{
    return (controller && controller->error() != QLowEnergyController::NoError);
}

bool Device::isRandomAddress() const
{
    return randomAddress;
}

void Device::setRandomAddress(bool newValue)
{
    randomAddress = newValue;
    emit randomAddressChanged();
}
