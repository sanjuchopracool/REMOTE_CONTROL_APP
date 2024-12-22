#include "controllerobject.h"
#include <QReadLocker>
#include <QReadWriteLock>
#include <QWriteLocker>
#include <device.h>

namespace {
enum Field { Roll = 0, Pitch = 2, Throttle = 4, Yaw = 6 };
constexpr int double_to_int16_factor = 1000;
QReadWriteLock lock;
} // namespace
ControllerObject::ControllerObject(QObject *parent)
    : QObject{parent}
{
    // send 4 int16
    // A E T R
    // A -> ROLL
    // E -> PITCH
    // T -> Throttle
    // R -> Yaw
    m_data.resize(sizeof(int16_t) * 4, 0);
    //rudder

    m_data_timer = new QTimer(this);
    connect(m_data_timer, &QTimer::timeout, this, [this]() {
        QReadLocker locker(&lock);
        emit dataUpdated(m_data);
    });

    m_data_timer->start(50);
}

ControllerObject::~ControllerObject()
{
    m_data_timer->stop();
}

void ControllerObject::leftStickMoved(double x, double y)
{
    QWriteLocker locker(&lock);
    setData(Yaw, x);
    setData(Throttle, y);
}

void ControllerObject::rightStickMoved(double x, double y)
{
    QWriteLocker locker(&lock);
    setData(Roll, x);
    setData(Pitch, y);
}

void ControllerObject::setData(int field, double data)
{
    int16_t xVal = data * double_to_int16_factor;
    int8_t low = xVal & 0xFF;
    int8_t high = (xVal >> 8);
    m_data[field] = low;
    m_data[field + 1] = high;
}
