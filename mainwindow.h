#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QTimer>
#include <QtSerialPort/QSerialPort>
#include <QModbusClient>

#include <selfcheckwidget.h>
#include <loginwidget.h>
#include <scanwidget.h>
#include <syssettingswidget.h>
#include <mainmenubtnswidget.h>

#include "config_recorder/uiconfigrecorder.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QString sw_about_str = "", QWidget *parent = nullptr);
    ~MainWindow();

    SelfCheckWidget * m_self_chk_widget;

    void setup_sport_parameters();
    bool open_sport();
    bool close_sport();
    bool write_to_sport(char* data_arr, qint64 byte_cnt, bool log_rw);
    bool read_from_sport(char* read_data, qint64 buf_size, bool log_rw);
    QString hv_work_st_str(quint16 st_reg_val);

private:
    Ui::MainWindow *ui;
    QStackedWidget *m_stacked_widget;

    UiConfigRecorder m_cfg_recorder;
    qobj_ptr_set_t m_rec_ui_cfg_fin, m_rec_ui_cfg_fout;

    LoginWidget * m_login_widget;
    ScanWidget * m_scan_widget;
    SysSettingsWidget * m_syssettings_widget;

    MainmenuBtnsWidget * m_mainmenubtns_widget;

    bool m_self_check_passed = false;

    QTimer m_pb_monitor_timer;
    QSerialPort m_pb_sport;
    bool m_pb_sport_open = false;

    using CheckHandler = bool (MainWindow::*)();
    QVector<CheckHandler> m_check_hdlrs =
    {
        &MainWindow::pwr_st_check,
        &MainWindow::x_ray_source_st_check,
        &MainWindow::detector_st_check,
        &MainWindow::storage_st_check,
    };
    void self_check(bool go_check = true);
    bool pwr_st_check();
    bool x_ray_source_st_check();
    bool detector_st_check();
    bool storage_st_check();
    void self_check_next_item_hdlr(bool start = false);

    void goto_login_widget();
    void go_to_scan_widget();
    void goto_syssettings_widget();

    QModbusClient * m_hv_conn_device = nullptr;
    QTimer m_hv_reconn_wait_timer;
    QModbusDevice::State m_hv_conn_state = QModbusDevice::UnconnectedState;
    bool m_hv_op_for_self_chk = false;
    void setup_hv_conn_client();
    void hv_connect();
    void hv_disconnect();

    void load_widgets_ui_settings();
    void rec_widgets_ui_settings();

public slots:
    void self_check_finished_sig_hdlr(bool result);
    void login_chk_passed_sig_hdlr();
    void pb_monitor_timer_hdlr();
    bool pb_monitor_check_st_hdlr();

    void hv_conn_error_sig_handler(QModbusDevice::Error error);
    void hv_conn_state_changed_sig_handler(QModbusDevice::State state);
    void hv_reconn_wait_timer_sig_handler();

    void go_to_syssettings_widget_sig_hdlr();
    void go_to_scan_widget_sig_hdlr();
    void mb_regs_read_ret_sig_hdlr(mb_reg_val_map_t reg_val_map);
    void hv_op_finish_sig_hdlr(bool ret, QString err_str = "");

    void detector_self_chk_ret_sig_hdlr(bool ret);

signals:
    void check_next_item_sig(bool start = false);
    void self_check_item_ret_sig(SelfCheckWidget::self_check_type_e_t, bool ret);
    void self_check_finished_sig(bool result);
    void pb_monitor_check_st();
};
#endif // MAINWINDOW_H
