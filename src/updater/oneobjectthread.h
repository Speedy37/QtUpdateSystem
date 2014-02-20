#ifndef ONEOBJECTTHREAD_H
#define ONEOBJECTTHREAD_H

#include <QThread>

class OneObjectThread : public QThread
{
    Q_OBJECT
public:
    OneObjectThread(QObject * parent = 0) : QThread(parent)
    {
        m_time = 200;
    }

    void manage(QObject *object)
    {
        object->moveToThread(this);
        connect(this, &OneObjectThread::destroying, object, &QObject::deleteLater);
        connect(object, &QObject::destroyed, [this]() {
            quit();
        });
        connect(this, &QThread::finished, this, &QThread::deleteLater);
    }

    void setMaxWaitTime(unsigned long time)
    {
        m_time = time;
    }

    ~OneObjectThread()
    {
        emit destroying();
        wait(m_time);
    }
signals:
    void destroying();
private:
    unsigned long m_time;
};

#endif // ONEOBJECTTHREAD_H
