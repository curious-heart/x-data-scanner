#ifndef GPIOMONITORTHREAD_H
#define GPIOMONITORTHREAD_H

#include <QThread>
#include <QFile>
#include <QTimer>
#include <QSocketNotifier>

class GpioMonitorThread : public QObject
{
    Q_OBJECT
public:
    explicit GpioMonitorThread(QObject *parent = nullptr);
    ~GpioMonitorThread();
    bool isInitSuccess() const { return initSuccess; }

signals:
    void turn_light_on_off_sig(int on_off_val);
    void btn_trigger_scan_sig(bool start);

private:
    bool exportGpio(int gpio);      // 导出 GPIO
    bool unexportGpio(int gpio);    // 释放 GPIO
    int openGpioInput(int gpio, const char *edgeMode);
    int readGpioValue(int fd);
    void handleButtonEvent(bool isLeft);
    void toggleLight();

    void cleanupAndExit();

private:
    int fdLeft{-1}, fdRight{-1};
    QFile lightFile;

    qint64 lastPressTimeRight{0};
    int clickCountRight{0};
    bool lightOn{false};

    bool scanActive{false};

    bool initSuccess{false};

    QSocketNotifier *notifierLeft = nullptr, *notifierRight = nullptr;
    QTimer *leftSmoothTimer, *rightSmoothTimer;
    QTimer *gpio_scan_timer;

    void check_thread_id(QString pos);

public slots:
    void thread_started_rpt();

private slots:
    void leftSmoothTimeoutHdlr();
    void rightSmoothTimeoutHdlr();
    void leftBtnActivatedHdlr(QSocketDescriptor socket, QSocketNotifier::Type type);
    void rightBtnActivatedHdlr(QSocketDescriptor socket, QSocketNotifier::Type type);
    void scan_gpio_keys();
};

#endif // GPIOMONITORTHREAD_H
