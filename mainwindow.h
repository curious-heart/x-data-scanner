#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QTimer>
#include <QtSerialPort/QSerialPort>

#include <selfcheckwidget.h>
#include <loginwidget.h>
#include <scanwidget.h>
#include <syssettingswidget.h>
#include <mainmenubtnswidget.h>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    SelfCheckWidget * m_self_chk_widget;

    bool open_sport();
    bool close_sport();
    bool write_to_sport(char* data_arr, qint64 byte_cnt, bool silent, bool log_rw);
    bool read_from_sport(char* read_data, qint64 buf_size, bool log_rw);

private:
    Ui::MainWindow *ui;
    QStackedWidget *m_stacked_widget;

    LoginWidget * m_login_widget;
    ScanWidget * m_scan_widget;
    SysSettingsWidget * m_syssettings_widget;

    MainmenuBtnsWidget * m_mainmenubtns_widget;

    QTimer m_pb_monitor_timer;
    QSerialPort m_pb_sport;
    bool m_pb_sport_open = false;

    void start_self_chk();

public slots:
    void self_check_finished_sig_hdlr(bool result);
    void login_chk_passed_sig_hdlr();
    void pb_monitor_timer_hdlr();
    void pb_monitor_check_st_hdlr();

signals:
    void pb_monitor_check_st();
};
#endif // MAINWINDOW_H
