#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QTimer>
#include <QtSerialPort/QSerialPort>
#include <QModbusClient>

#include "selfcheckwidget.h"
#include "loginwidget.h"
#include "scanwidget.h"
#include "camerawidget.h"
#include "syssettingswidget.h"
#include "mainmenubtnswidget.h"

#include "config_recorder/uiconfigrecorder.h"

#include "serialsniffer/serialsniffer.h"
#include "gpiomonitorthread.h"

#include "remotedbgopthreadworker.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

typedef struct
{
    QByteArray buf;
    int hd_idx, len_from_hd;
}sport_read_buffer_s_t;

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

    typedef enum
    {
        SELF_CHK_END,
        SELF_CHK_PENDING,
    }self_chk_ret_e_t;
    Q_ENUM(self_chk_ret_e_t)

    typedef enum
    {
        HV_SELF_CHK_WAITING_CONN,
        HV_SELF_CHK_CONN_DONE,
        HV_SELF_CHK_WAITING_READ,
        HV_SELF_CHK_FINISHED,
    }hv_self_chk_stg_e_t;
    Q_ENUM(hv_self_chk_stg_e_t)

private:
    Ui::MainWindow *ui;
    QStackedWidget *m_stacked_widget;

    UiConfigRecorder m_cfg_recorder;
    qobj_ptr_set_t m_rec_ui_cfg_fin, m_rec_ui_cfg_fout;

    LoginWidget * m_login_widget;
    ScanWidget * m_scan_widget;
    CameraWidget * m_camera_widget;
    SysSettingsWidget * m_syssettings_widget;

    MainmenuBtnsWidget * m_mainmenubtns_widget;

    bool m_self_check_passed = false;

    QTimer m_pb_monitor_timer;
    QSerialPort m_pb_sport;
    bool m_pb_sport_open = false;
    sport_read_buffer_s_t  m_pb_sport_read_buf;
    int m_min_pb_sport_msg_len;
    app_exit_mode_e_t m_exit_mode = APP_EXIT_NORMAL;

    QTimer m_pb_self_chk_timer;
    bool m_pb_is_self_checking = false;

    QTimer m_hv_monitor_timer;

    using CheckHandler = self_chk_ret_e_t (MainWindow::*)(bool * result);
    QVector<CheckHandler> m_check_hdlrs =
    {
        &MainWindow::pwr_st_check,
        &MainWindow::x_ray_source_st_check,
        &MainWindow::detector_st_check,
        &MainWindow::storage_st_check,
    };
    void self_check(bool go_check = true);
    self_chk_ret_e_t pwr_st_check(bool * result = nullptr);
    self_chk_ret_e_t x_ray_source_st_check(bool * result = nullptr);
    self_chk_ret_e_t detector_st_check(bool * result = nullptr);
    self_chk_ret_e_t storage_st_check(bool * result = nullptr);
    void self_check_next_item_hdlr(bool start = false, bool last_ret = true);

    void goto_login_widget();
    void go_to_scan_widget();
    void go_to_camera_widget();
    void goto_syssettings_widget();

    QModbusClient * m_hv_conn_device = nullptr;
    QTimer m_hv_reconn_wait_timer;
    QModbusDevice::State m_hv_conn_state = QModbusDevice::UnconnectedState;
    hv_self_chk_stg_e_t m_hv_self_chk_stg = HV_SELF_CHK_WAITING_CONN;
    bool m_hv_op_for_self_chk = false;
    void setup_hv_conn_client();
    void hv_connect();
    void hv_disconnect();

    void load_widgets_ui_settings();
    void rec_widgets_ui_settings();

    void refresh_storage_st();

    void clear_pb_sport_data_buf();
    void parse_pb_sport_data();
    void proc_pb_pwr_off_msg(QByteArray msg);
    void proc_pb_wkup_msg(QByteArray msg);
    void proc_pb_motor_msg(QByteArray msg);
    void proc_pb_pwr_bat_st_msg(QByteArray msg);

    SerialSniffer * m_serial_sniffer = nullptr;

    GpioMonitorThread * m_gpio_monitor = nullptr;
    QThread * m_gpio_monitor_hdlr = nullptr;

    RemoteDbgOpThreadWorker * m_dbg_th_worker = nullptr;
    QThread* m_dbg_th_hdlr = nullptr;

    void updateRemoteDbgThread(bool enabled, quint16 local_port);
    void exit_rmg_dbg_thread(bool over = false);

public slots:
    void self_check_finished_sig_hdlr(bool result);
    void login_chk_passed_sig_hdlr();
    void pb_monitor_timer_hdlr();
    void pb_self_chk_to_timer_hdlr();
    bool pb_monitor_check_st_hdlr();

    void pbSportReadyReadHdlr();
    void hv_conn_error_sig_handler(QModbusDevice::Error error);
    void hv_conn_state_changed_sig_handler(QModbusDevice::State state);
    void hv_reconn_wait_timer_sig_handler();

    void go_to_syssettings_widget_sig_hdlr();
    void go_to_scan_widget_sig_hdlr();
    void go_to_camera_widget_sig_hdlr();
    void hv_op_finish_sig_hdlr(bool ret, QString err_str = "");

    void self_check_hv_rechk_sig_hdlr();
    void detector_self_chk_ret_sig_hdlr(bool ret);

    void hv_monitor_timer_sig_hdlr();
    void mb_regs_read_ret_sig_hdlr(mb_reg_val_map_t reg_val_map);

    void send_pb_power_off_sig_hdlr();

    void btn_trigger_scan_sig_hdlr(bool start);
    void turn_light_on_off_sig_hdlr(int /*on_off_val*/);

    void rmt_dbg_enabled_sig_hdlr(bool enable);
    void rmt_scan_sig_hdlr(bool start, const QString &peer_ip, quint16 peer_port, const QString &cmd_str);
    void motor_speed_set_sig_hdlr(int speed);
    void pb_slp_wkp_sig_sig_hdlr(bool wkp); //true-wkp, false-slp

signals:
    void self_check_hv_rechk_sig();
    void check_next_item_sig(bool start = false, bool last_ret = true);
    void self_check_item_ret_sig(SelfCheckWidget::self_check_type_e_t,
                                 SelfCheckWidget::self_check_stage_e_t st);
    void self_check_finished_sig(bool result);
    void pb_monitor_check_st();

    void scan_widget_disp_sig();

    void parse_sport_data_sig();
};
#endif // MAINWINDOW_H
