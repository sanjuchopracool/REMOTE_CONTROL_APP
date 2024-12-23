#include "controllerobject.h"
#include <QReadLocker>
#include <QReadWriteLock>
#include <QWriteLocker>
#include <device.h>

namespace {
enum Field { Throttle = 0, Steering = 2 };
enum Flags {
    Invert_Throttle = 0x01,
    Invert_Steering = 0x02,
};

constexpr int double_to_int16_factor = 10000;
QReadWriteLock lock;
} // namespace
ControllerObject::ControllerObject(QObject *parent)
    : QObject{parent}
{
    // send 2 int16
    // throttle and steering
    m_data.resize(sizeof(int16_t) * 2, 0);
    //rudder

    m_data_timer = new QTimer(this);
    connect(m_data_timer, &QTimer::timeout, this, [this]() {
        QReadLocker locker(&lock);
        emit dataUpdated(m_data);
    });

    m_data_timer->start(20);
}

ControllerObject::~ControllerObject()
{
    m_data_timer->stop();
}

void ControllerObject::leftStickMoved(double x, double y)
{
    QWriteLocker locker(&lock);
    setData(Steering, x);
}

void ControllerObject::rightStickMoved(double x, double y)
{
    QWriteLocker locker(&lock);
    setData(Throttle, y);
}

void ControllerObject::sendConfig()
{
    // sending one dummy also to differentiate between data and config packet
    QByteArray configData(5, 0);
    uint8_t flags = 0x00;
    if (m_invert_throttle) {
        flags |= Invert_Throttle;
    }

    if (m_invert_steering) {
        flags |= Invert_Steering;
    }

    configData[0] = flags;
    configData[1] = m_steering_percentage;
    configData[2] = m_throttle_front_percentage;
    configData[3] = m_throttle_back_percentage;
    qDebug() << "Sending Config";
    emit dataUpdated(configData, true);
}

void ControllerObject::configInvertThrottle(bool flag)
{
    if (flag != m_invert_throttle) {
        m_invert_throttle = flag;
        emit configChanged();
    }
}

void ControllerObject::configInvertSteering(bool flag)
{
    if (flag != m_invert_steering) {
        m_invert_steering = flag;
        emit configChanged();
    }
}

void ControllerObject::configSetThrottlePrecentageFront(uint8_t front)
{
    if (front <= 100 && front != m_throttle_front_percentage) {
        m_throttle_front_percentage = front;
        emit configChanged();
    }
}

void ControllerObject::configSetThrottlePrecentageBack(uint8_t back)
{
    if (back <= 100 && back != m_throttle_back_percentage) {
        m_throttle_back_percentage = back;
        emit configChanged();
    }
}

void ControllerObject::configSetSteeringPrecentage(uint8_t factor)
{
    if (factor <= 100 && factor != m_steering_percentage) {
        m_steering_percentage = factor;
        emit configChanged();
    }
}

void ControllerObject::setData(int field, double data)
{
    int16_t xVal = data * double_to_int16_factor;
    int8_t low = xVal & 0xFF;
    int8_t high = (xVal >> 8);
    m_data[field] = low;
    m_data[field + 1] = high;
}
