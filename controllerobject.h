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

signals:
    void dataUpdated(QByteArray);

public slots:
    void leftStickMoved(double x, double y);
    void rightStickMoved(double x, double y);

private:
    void setData(int field, double data);

protected:
    QByteArray m_data;
    QTimer *m_data_timer = nullptr;
};

#endif // CONTROLLEROBJECT_H
