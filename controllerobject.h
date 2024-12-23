#ifndef CONTROLLEROBJECT_H
#define CONTROLLEROBJECT_H

#include <QObject>
#include <QTimer>

class ControllerObject : public QObject
{
    Q_OBJECT
public:
    explicit ControllerObject(QObject *parent = nullptr);
    ~ControllerObject();
    const QByteArray &data() const;

    Q_PROPERTY(bool throttleInverted READ configThrottleInverted WRITE configInvertThrottle NOTIFY
                   configChanged FINAL)
    Q_PROPERTY(bool steeringInverted READ configSteeringInverted WRITE configInvertThrottle NOTIFY
                   configChanged FINAL)

    Q_PROPERTY(uint8_t throttleFrontPercentage READ configThrottlePercentageFront WRITE
                   configSetThrottlePrecentageFront NOTIFY configChanged FINAL)

    Q_PROPERTY(uint8_t throttleBackPercentage READ configThrottlePercentageBack WRITE
                   configSetThrottlePrecentageBack NOTIFY configChanged FINAL)

    Q_PROPERTY(uint8_t steeringPercentage READ configSteeringPercentage WRITE
                   configSetSteeringPrecentage NOTIFY configChanged FINAL)

signals:
    void dataUpdated(QByteArray, bool withRespons = false);
    void configChanged();

public slots:
    void leftStickMoved(double x, double y);
    void rightStickMoved(double x, double y);
    void sendConfig();

    bool configThrottleInverted() const { return m_invert_throttle; }
    void configInvertThrottle(bool flag);

    bool configSteeringInverted() const { return m_invert_steering; }
    void configInvertSteering(bool flag);

    uint8_t configThrottlePercentageFront() const { return m_throttle_front_percentage; }
    void configSetThrottlePrecentageFront(uint8_t front);

    uint8_t configThrottlePercentageBack() const { return m_throttle_back_percentage; }
    void configSetThrottlePrecentageBack(uint8_t back);

    uint8_t configSteeringPercentage() const { return m_steering_percentage; }
    void configSetSteeringPrecentage(uint8_t factor);

private:
    void setData(int field, double data);

protected:
    QByteArray m_data;
    QTimer *m_data_timer = nullptr;

    bool m_invert_throttle = true;
    bool m_invert_steering = false;
    uint8_t m_steering_percentage = 100;
    uint8_t m_throttle_front_percentage = 50;
    uint8_t m_throttle_back_percentage = 30;
};

#endif // CONTROLLEROBJECT_H
