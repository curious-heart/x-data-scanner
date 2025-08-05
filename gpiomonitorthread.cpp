#include "literal_strings/literal_strings.h"
#include "gpiomonitorthread.h"
#include "logger/logger.h"
#include "sysconfigs/sysconfigs.h"

#include <QSocketNotifier>
#include <QDateTime>

#ifdef Q_OS_UNIX
#include <fcntl.h>
#include <unistd.h>
#endif

GpioMonitorThread::GpioMonitorThread(QObject *parent)
    : QThread(parent) {}

GpioMonitorThread::~GpioMonitorThread()
{
    cleanupAndExit();
}

void GpioMonitorThread::cleanupAndExit()
{
#ifdef Q_OS_UNIX
    // 关闭文件
    if (lightFile.isOpen())
    {
        lightFile.close();
    }
    // 关闭 GPIO fd
    if (fdLeft >= 0)
    {
        close(fdLeft);
        fdLeft = -1;
    }
    if (fdRight >= 0)
    {
        close(fdRight);
        fdRight = -1;
    }
    /*
    unexportGpio(g_sys_configs_block.btn_gpio_cfg.left_btn_gpio);
    unexportGpio(g_sys_configs_block.btn_gpio_cfg.right_btn_gpio);
    unexportGpio(g_sys_configs_block.btn_gpio_cfg.light_ctrl_gpio);
    */

    DIY_LOG(LOG_INFO, "gpio monitor thread cleaned and to exit.");
#endif
}

bool GpioMonitorThread::exportGpio(int gpio)
{
    QString gpioPath = QString("/sys/class/gpio/gpio%1").arg(gpio);
    if (QFile::exists(gpioPath))
    {
        DIY_LOG(LOG_INFO, QString("gpio %1 already exists").arg(gpio));
        return true;
    }

    QFile exportFile("/sys/class/gpio/export");
    if (!exportFile.open(QIODevice::WriteOnly))
    {
        DIY_LOG(LOG_ERROR, QString("open %1 error, may need root priveledge.")
                                   .arg(exportFile.fileName()));
        return false;
    }
    exportFile.write(QString::number(gpio).toUtf8());
    exportFile.close();

    // 等待目录出现
    for (int i = 0; i < 50; ++i)
    { // 最多等 500ms
        if (QFile::exists(gpioPath))
        {
            DIY_LOG(LOG_INFO, QString("gpio %1 created ok!").arg(gpio));
            return true;
        }
        msleep(10);
    }

    DIY_LOG(LOG_ERROR, QString("create %1 fails.").arg(gpio));
    return false;
}

bool GpioMonitorThread::unexportGpio(int gpio)
{
    QString gpioPath = QString("/sys/class/gpio/gpio%1").arg(gpio);
    if (!QFile::exists(gpioPath))
    {
        DIY_LOG(LOG_INFO, QString("gpio %1 already unexported.").arg(gpio));
        return true; // 已释放
    }

    QFile unexportFile("/sys/class/gpio/unexport");
    if (!unexportFile.open(QIODevice::WriteOnly))
    {
        DIY_LOG(LOG_ERROR, QString("open %1 error, may need root priveledge.")
                                   .arg(unexportFile.fileName()));
        return false;
    }
    unexportFile.write(QString::number(gpio).toUtf8());
    unexportFile.close();
    return true;
}

void GpioMonitorThread::run()
{
#ifdef Q_OS_UNIX
    initSuccess = false;

    fdLeft  = openGpioInput(g_sys_configs_block.btn_gpio_cfg.left_btn_gpio, "both");
    if (fdLeft < 0) { DIY_LOG(LOG_ERROR, "init fdLeft fails."); cleanupAndExit(); return; }

    fdRight = openGpioInput(g_sys_configs_block.btn_gpio_cfg.right_btn_gpio, "both");
    if (fdRight < 0) { DIY_LOG(LOG_ERROR, "init fdRight fails."); cleanupAndExit(); return; }

    int lightGpio = g_sys_configs_block.btn_gpio_cfg.light_ctrl_gpio;
    if (!exportGpio(lightGpio))
    {
        DIY_LOG(LOG_ERROR, "export light gpio fails.");
        cleanupAndExit();
        return;
    }

    /*
    QFile dirFile(QString("/sys/class/gpio/gpio%1/direction").arg(lightGpio));
    if(!dirFile.open(QIODevice::WriteOnly))
    {
        DIY_LOG(LOG_ERROR, QString("open dirFile %1 fails.").arg(dirFile.fileName()));
        cleanupAndExit();
        return;
    }
    dirFile.write("out");
    dirFile.close();
    DIY_LOG(LOG_INFO, QString("write dirFile %1 succeeds.").arg(dirFile.fileName()));
    */

    QString lightPath = QString("/sys/class/gpio/gpio%1/value")
                            .arg(g_sys_configs_block.btn_gpio_cfg.light_ctrl_gpio);
    lightFile.setFileName(lightPath);
    if(!lightFile.open(QIODevice::WriteOnly))
    {
        DIY_LOG(LOG_ERROR, QString("open light file %1 fails.").arg(lightPath));
        cleanupAndExit();
        return;
    }
    DIY_LOG(LOG_INFO, QString("open light file %1 succeeds.").arg(lightPath));

    initSuccess = true;

    notifierLeft = new QSocketNotifier(fdLeft, QSocketNotifier::Exception, this);
    notifierRight = new QSocketNotifier(fdRight, QSocketNotifier::Exception, this);

    connect(notifierLeft, &QSocketNotifier::activated,
            this, &GpioMonitorThread::leftBtnActivatedHdlr);
    connect(notifierRight, &QSocketNotifier::activated,
            this, &GpioMonitorThread::rightBtnActivatedHdlr);

    connect(&leftSmoothTimer, &QTimer::timeout,
            this, &GpioMonitorThread::leftSmoothTimeoutHdlr);
    connect(&rightSmoothTimer, &QTimer::timeout,
            this, &GpioMonitorThread::rightSmoothTimeoutHdlr);

    DIY_LOG(LOG_INFO, "gpio monitor thread started.");

    exec();
#endif
}

int GpioMonitorThread::openGpioInput(int gpio, const char */*edgeMode*/)
{
#ifdef Q_OS_UNIX
   if(!exportGpio(gpio))
    {
        return -1;
    }

    QString pth_str = QString("/sys/class/gpio/gpio%1").arg(gpio);

    /*
    QFile dirFile(pth_str + "/direction");
    if(dirFile.open(QIODevice::WriteOnly))
    {
        dirFile.write("in");
        dirFile.close();

        DIY_LOG(LOG_INFO, QString("write dirFile %1 succeeds.").arg(dirFile.fileName()));
    }
    else
    {
        DIY_LOG(LOG_ERROR, QString("open dirFile %1 fails.").arg(dirFile.fileName()));
        return -1;
    }

    QFile edgeFile(pth_str + "/edge");
    if(edgeFile.open(QIODevice::WriteOnly))
    {
        edgeFile.write(edgeMode);
        edgeFile.close();

        DIY_LOG(LOG_INFO, QString("write edgeFile %1 succeeds.").arg(edgeFile.fileName()));
    }
    else
    {
        DIY_LOG(LOG_INFO, QString("open edgeFile %1 fails.").arg(edgeFile.fileName()));
        return -1;
    }
    */

    QString val_str = pth_str + "/value";
    int fd = open(val_str.toStdString().c_str(), O_RDONLY);
    if (fd < 0)
    {
        DIY_LOG(LOG_ERROR, QString("open %1 error, fd: %2").arg(val_str).arg(fd));
    }
    else
    {
        DIY_LOG(LOG_INFO, QString("open %1 succeeds, fd: %2.").arg(val_str).arg(fd));
    }

    lseek(fd, 0, SEEK_SET);
    char buf[8] = {0};
    ::read(fd, buf, sizeof(buf));

    return fd;

#else
    return -1;
#endif
}

int GpioMonitorThread::readGpioValue(int fd)
{
#ifdef Q_OS_UNIX
    /*
    lseek(fd, 0, SEEK_SET);
    char buf[2] = {0};
    if (read(fd, buf, sizeof(buf)) > 0)
    {
        return buf[0] - '0';
    }
    */

    char buf[8] = {0};
    lseek(fd, 0, SEEK_SET);
    if (::read(fd, buf, sizeof(buf)) <= 0)
    {
        DIY_LOG(LOG_ERROR,  "Failed to read GPIO value");
        return -1; // 读取失败
    }
    return atoi(buf);
#endif
    return -1;
}

void GpioMonitorThread::handleButtonEvent(bool isLeft)
{
    static int lastLeftVal = -1;
    static int lastRightVal = -1;
    int &lastVal = isLeft ? lastLeftVal : lastRightVal;

    QString left_right_str = isLeft ? g_str_left_en : g_str_right_en;

    int val = readGpioValue(isLeft ? fdLeft : fdRight);

    if(lastVal == val)
    {
        return;
    }
    lastVal = val;

    DIY_LOG(val < 0 ? LOG_ERROR : LOG_INFO,
            QString("%1 btn gpio val: %2").arg(left_right_str).arg(val));

    if (val < 0) return;

    const int pressVal   = g_sys_configs_block.btn_gpio_cfg.btn_press_val;
    const int releaseVal = g_sys_configs_block.btn_gpio_cfg.btn_release_val;

    QString pre_rel_str;
    if(pressVal == val) pre_rel_str = g_str_pressed_en;
    else if(releaseVal == val) pre_rel_str = g_str_released_en;
    else pre_rel_str = g_str_unknown_en;
    DIY_LOG(LOG_INFO, QString("that is, %1 btn %2.").arg(left_right_str, pre_rel_str));

    // 右键双击
    if(!isLeft)
    {
        if (pressVal == val)
        {
            qint64 now = QDateTime::currentMSecsSinceEpoch();
            if (clickCountRight == 0)
            {
                lastPressTimeRight = now;
                clickCountRight    = 1;
            }
            else if (clickCountRight == 1 &&
                       now - lastPressTimeRight <=
                           g_sys_configs_block.btn_gpio_cfg.dbl_click_btn_int_ms)
            {
                clickCountRight = 2;
            }
            else
            {
                clickCountRight    = 1;
                lastPressTimeRight = now;
            }
        }
        else if (releaseVal == val)
        {
            qint64 now = QDateTime::currentMSecsSinceEpoch();
            if (clickCountRight == 2 &&
                now - lastPressTimeRight <=
                    g_sys_configs_block.btn_gpio_cfg.dbl_click_btn_int_ms)
            {
                DIY_LOG(LOG_INFO, "right button double clicked!");

                toggleLight();
                clickCountRight = 0;
            }
            else if (clickCountRight >= 1 &&
                       now - lastPressTimeRight >
                           g_sys_configs_block.btn_gpio_cfg.dbl_click_btn_int_ms)
            {
                clickCountRight = 0;
            }
        }
    }

    QString another_key = !isLeft ? g_str_left_en : g_str_right_en;
    int another_val = readGpioValue(!isLeft ? fdLeft : fdRight);
    QString another_pre_rel_str;
    if(pressVal == another_val) another_pre_rel_str = g_str_pressed_en;
    else if(releaseVal == another_val) another_pre_rel_str = g_str_released_en;
    else another_pre_rel_str = g_str_unknown_en;
    DIY_LOG(LOG_INFO, QString("now another btn(%1) is %2, that is %3.")
                          .arg(another_key).arg(another_val).arg(another_pre_rel_str));

    if(!scanActive && pressVal == val && pressVal == another_val)
    {
        scanActive = true;
        emit btn_trigger_scan_sig(true);

        DIY_LOG(LOG_INFO, "btn_trigger_scan_sig: start");
    }
    else if (scanActive && (releaseVal == val || releaseVal == another_val))
    {
        scanActive = false;
        emit btn_trigger_scan_sig(false);

        DIY_LOG(LOG_INFO, "btn_trigger_scan_sig: stop");
    }
}

void GpioMonitorThread::toggleLight()
{
    lightOn = !lightOn;
    int val = lightOn ? g_sys_configs_block.btn_gpio_cfg.light_on_val
                      : g_sys_configs_block.btn_gpio_cfg.light_off_val;
    QString on_off_str = lightOn ? "on" : "off";

    DIY_LOG(LOG_INFO, QString("try turning light %1:%2").arg(on_off_str).arg(val));

    if(lightFile.isOpen())
    {
        lightFile.write(QString::number(val).toUtf8());
        lightFile.flush();
        emit turn_light_on_off_sig(val);

        DIY_LOG(LOG_INFO, QString("turn light %1 succeedss").arg(on_off_str));
    }
    else
    {
        DIY_LOG(LOG_ERROR, "lightFile is not opend!");
    }
}

void GpioMonitorThread::leftBtnActivatedHdlr(QSocketDescriptor /*socket*/, QSocketNotifier::Type /*type*/)
{
    notifierLeft->setEnabled(false);
    handleButtonEvent(true);
    leftSmoothTimer.start(g_sys_configs_block.btn_gpio_cfg.btn_smooth_dur_ms);
}

void GpioMonitorThread::rightBtnActivatedHdlr(QSocketDescriptor /*socket*/, QSocketNotifier::Type /*type*/)
{
    notifierRight->setEnabled(false);
    handleButtonEvent(false);
    rightSmoothTimer.start(g_sys_configs_block.btn_gpio_cfg.btn_smooth_dur_ms);
}

void GpioMonitorThread::leftSmoothTimeoutHdlr()
{
    notifierLeft->setEnabled(true);
}

void GpioMonitorThread::rightSmoothTimeoutHdlr()
{
    notifierRight->setEnabled(true);
}
