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

#include <controllerobject.h>

using namespace Qt::StringLiterals;

namespace {
const QBluetoothUuid service_uuid("{6e400001-b5a3-f393-e0a9-e50e24dcca9e}");
const QBluetoothUuid rx_uuid("{6e400003-b5a3-f393-e0a9-e50e24dcca9e}");
const QBluetoothUuid tx_uuid("{6e400002-b5a3-f393-e0a9-e50e24dcca9e}");
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

    m_controler_object = new ControllerObject(this);
    connect(m_controler_object, &ControllerObject::dataUpdated, this, &Device::writeData);
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
    if (serviceUuid != service_uuid)
        return;

    //! [les-service-1]
    QLowEnergyService *service = controller->createServiceObject(serviceUuid);
    if (!service) {
        qWarning() << "Cannot create service for uuid";
        return;
    }

    // filter servicec here based on type
    //! [les-service-1]
    auto serv = new ServiceInfo(service);
    m_services.append(serv);
    emit servicesUpdated();
}
//! [les-service-1]

void Device::serviceScanDone()
{
    setUpdate(u"\n(Service scan done!)"_s);
    // force UI in case we didn't find anything
    if (m_services.isEmpty()) {
        setUpdate(u"\n(Could not find the right service)"_s);
        emit servicesUpdated();
    } else {
        connectToService(m_services.first()->getUuid());
    }
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
    if (!m_rx_tx_service)
        return;

    connect(m_rx_tx_service,
            &QLowEnergyService::characteristicChanged,
            this,
            [this](const QLowEnergyCharacteristic &ch, const QByteArray &data) {
                if (ch.uuid() == rx_uuid) {
                    qDebug() << "DATA REC = " << data;
                }
            });

    // connect(m_rx_tx_service,
    //         &QLowEnergyService::stateChanged,
    //         this,
    //         [this](QLowEnergyService::ServiceState newState) {
    //             qDebug() << "SANJAY new State = " << newState;
    //         });

    // connect(m_rx_tx_service,
    //         &QLowEnergyService::characteristicRead,
    //         this,
    //         [this](const QLowEnergyCharacteristic &info, const QByteArray &value) {
    //             qDebug() << "SANJAY Ch read = " << info.uuid() << " value = " << value;
    //         });

    // connect(m_rx_tx_service,
    //         &QLowEnergyService::characteristicWritten,
    //         this,
    //         [this](const QLowEnergyCharacteristic &info, const QByteArray &value) {
    //             qDebug() << "SANJAY Ch written = " << info.uuid() << " value = " << value;
    //         });

    // connect(m_rx_tx_service,
    //         &QLowEnergyService::descriptorWritten,
    //         this,
    //         [this](const QLowEnergyDescriptor &info, const QByteArray &value) {
    //             qDebug() << "SANJAY ds written = " << info.uuid() << " value = " << value;
    //         });

    // connect(m_rx_tx_service,
    //         &QLowEnergyService::descriptorRead,
    //         this,
    //         [this](const QLowEnergyDescriptor &info, const QByteArray &value) {
    //             qDebug() << "SANJAY ds read = " << info.uuid() << " value = " << value;
    //         });

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
        return;
    }

    //discovery already done
    const QList<QLowEnergyCharacteristic> chars = m_rx_tx_service->characteristics();
    for (const QLowEnergyCharacteristic &ch : chars) {
        auto cInfo = new CharacteristicInfo(ch);
        m_characteristics.append(cInfo);
    }

    QTimer::singleShot(0, this, &Device::characteristicsUpdated);
}

void Device::deviceConnected()
{
    setUpdate(u"Back\n(Discovering services...)"_s);
    connected = true;
    emit currentDeviceChanged();

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

    connected = false;
    emit rxTxConnectionChanged();
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
        return;
    }

    auto service = qobject_cast<QLowEnergyService *>(sender());
    if (!service) {
        return;
    }

    //! [les-chars]
    const QList<QLowEnergyCharacteristic> chars = service->characteristics();
    for (const QLowEnergyCharacteristic &ch : chars) {
        if (ch.uuid() == tx_uuid || ch.uuid() == rx_uuid) {
            auto cInfo = new CharacteristicInfo(ch);
            m_characteristics.append(cInfo);

            if (ch.uuid() == tx_uuid) {
                m_tx_characteric = m_characteristics.last()->getCharacteristic();
            }
        }
    }
    //! [les-chars]

    if (m_characteristics.size() != 2) {
        setUpdate("Missing Rx or Tx characterstics");
    } else {
        for (CharacteristicInfo *characteristicInfo : m_characteristics) {
            auto characteristic = characteristicInfo->getCharacteristic();
            auto m_notificationDesc = characteristic.descriptor(
                QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);
            if (m_notificationDesc.isValid()) {
                m_rx_tx_service->writeDescriptor(m_notificationDesc, QByteArray::fromHex("0100"));
            }
        }

        // actual connected
        emit rxTxConnectionChanged();
    }
    emit characteristicsUpdated();
}

void Device::writeData(QByteArray data)
{
    if (connected && m_rx_tx_service && controller
        && controller->state() == QLowEnergyController::DiscoveredState) {
        if (m_tx_characteric.isValid()) {
            qDebug() << " T ";
            m_rx_tx_service->writeCharacteristic(m_tx_characteric,
                                                 data,
                                                 QLowEnergyService::WriteMode::WriteWithoutResponse);
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

bool Device::rxTxConnected() const
{
    return connected && m_characteristics.size() == 2;
}

QString Device::connectedDeviceName() const
{
    return currentDevice.getName();
}

QString Device::connectedDeviceId() const
{
    return currentDevice.getAddress();
}
