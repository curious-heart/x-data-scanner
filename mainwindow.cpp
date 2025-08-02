#include <QMessageBox>
#include <QModbusRtuSerialMaster>

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "literal_strings/literal_strings.h"
#include "common_tools/common_tool_func.h"
#include "sysconfigs/sysconfigs.h"

const char* g_main_th_local_log_fn = "./main_th_local.log";

extern bool g_data_scanning_now;

const quint16 g_hv_st_code_idle = 0x11;
const quint16 g_hv_st_code_hs = 0x22;
const quint16 g_hv_st_code_input1_fb_abn = 0xE1;
const quint16 g_hv_st_code_curr_fb_abn = 0xE2;
const quint16 g_hv_st_code_volt_fb_abn = 0xE3;
const quint16 g_hv_st_code_mult_abn = 0xE4;

static const char gs_addr_byte = 0x01;
static const char gs_pb_sport_data_start_flag = 0x01;

/*
5.1 关机命令上传
   电源板：0x01  0x00  0x00  0x01
    上位机：0x01  0x00  0x00  0x01
*/
static const char gs_dif_byte_pwr_off = 0x01;
static const int gs_pwr_off_msg_len = 4;

/*
5.2 休眠状态控制
上位机：0x01  0x??  0x??  0x02
电源板：0x01  0x??  0x??  0x02
0x??  0x??：0x00  0x00 表示唤醒；0x00  0x01 表示休眠；
*/
static const char gs_dif_byte_wkup_slp = 0x02;
static const int gs_wkup_slp_msg_len = 4;
static const quint16 gs_wkup_val = 0, gs_slp_val = 1;

/*
5.3 电机转速控制
上位机：0x01  0x??  0x??  0x03
电源板：0x01  0x??  0x??  0x03
0x??  0x??：0x00  0x00 表示停转；0x??  0x?? 非零值表示速度；
电机速度单位：转/分，高位在前，低位在后。
*/
static const char gs_dif_byte_motor_rpm = 0x03;
static const int gs_motor_rpm_msg_len = 4;

/*
5.4 电池信息读取
上位机：0x01  0x00  0x00  0x04
电源板：0x01  0x??  0x??  0x??  0x??  0x??  0x04
            ----------  ----------  ----
               电压值     电流值     电量百分比
高位在前，低位在后；
电流值如果是正值，表面真正充电。
*/
static const char gs_dif_byte_pwr_st = 0x04;
static const int gs_pwr_st_msg_len = 7;
static const quint16 gs_pwr_st_chk_val = 0;

static const int gs_sport_read_try_cnt = 3;
static const int gs_pb_sport_read_buf_size = 256;

MainWindow::MainWindow(QString sw_about_str, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow), m_cfg_recorder(this)
{
    ui->setupUi(this);

    ui->swAboutLbl->setText(sw_about_str);

    m_rec_ui_cfg_fin.clear();
    m_rec_ui_cfg_fout.clear();
    m_cfg_recorder.load_configs_to_ui(this, m_rec_ui_cfg_fin, m_rec_ui_cfg_fout);

    /*this widget should be newed first since it loads sys settings.*/
    m_syssettings_widget = new SysSettingsWidget(&m_cfg_recorder, this);
    m_syssettings_widget->load_ui_settings();
    m_syssettings_widget->hide();

    m_stacked_widget = new QStackedWidget(this);
    ui->mainPartVBoxLayot->addWidget(m_stacked_widget);

    m_self_chk_widget = new SelfCheckWidget(this);
    m_stacked_widget->addWidget(m_self_chk_widget);

    m_login_widget = new LoginWidget(this);
    m_login_widget->hide();

    m_scan_widget = new ScanWidget(&m_cfg_recorder, this);
    m_scan_widget->hide();

    m_camera_widget = new CameraWidget(this);
    m_camera_widget->hide();

    m_mainmenubtns_widget = new MainmenuBtnsWidget(this);
    ui->buttonsHBoxLayout->addWidget(m_mainmenubtns_widget);

    load_widgets_ui_settings();

    connect(m_login_widget, &LoginWidget::login_chk_passed_sig,
            this, &MainWindow::login_chk_passed_sig_hdlr, Qt::QueuedConnection);

    connect(m_mainmenubtns_widget, &MainmenuBtnsWidget::go_to_syssettings_widget_sig,
            this, &MainWindow::go_to_syssettings_widget_sig_hdlr, Qt::QueuedConnection);
    connect(m_mainmenubtns_widget, &MainmenuBtnsWidget::go_to_scan_widget_sig,
            this, &MainWindow::go_to_scan_widget_sig_hdlr, Qt::QueuedConnection);
    connect(m_mainmenubtns_widget, &MainmenuBtnsWidget::go_to_camera_widget_sig,
            this, &MainWindow::go_to_camera_widget_sig_hdlr, Qt::QueuedConnection);

    connect(m_scan_widget, &ScanWidget::mb_regs_read_ret_sig,
            this, &MainWindow::mb_regs_read_ret_sig_hdlr, Qt::QueuedConnection);
    connect(m_scan_widget, &ScanWidget::hv_op_finish_sig,
            this, &MainWindow::hv_op_finish_sig_hdlr, Qt::QueuedConnection);
    connect(m_scan_widget, &ScanWidget::detector_self_chk_ret_sig,
            this, &MainWindow::detector_self_chk_ret_sig_hdlr, Qt::QueuedConnection);
    connect(this, &MainWindow::scan_widget_disp_sig,
            m_scan_widget, &ScanWidget::scan_widget_disp_sig_hdlr, Qt::QueuedConnection);

    connect(&m_pb_monitor_timer, &QTimer::timeout, this, &MainWindow::pb_monitor_timer_hdlr,
            Qt::QueuedConnection);
    connect(this, &MainWindow::pb_monitor_check_st, this, &MainWindow::pb_monitor_check_st_hdlr,
            Qt::QueuedConnection);

    m_pb_self_chk_timer.setSingleShot(true);
    connect(&m_pb_self_chk_timer, &QTimer::timeout, this, &MainWindow::pb_self_chk_to_timer_hdlr,
            Qt::QueuedConnection);

    connect(&m_hv_monitor_timer, &QTimer::timeout, this, &MainWindow::hv_monitor_timer_sig_hdlr,
            Qt::QueuedConnection);

    connect(this, &MainWindow::self_check_hv_rechk_sig,
            this, &MainWindow::self_check_hv_rechk_sig_hdlr,Qt::QueuedConnection);
    connect(this, &MainWindow::self_check_item_ret_sig,
            m_self_chk_widget, &SelfCheckWidget::self_check_item_ret_sig_hdlr, Qt::QueuedConnection);
    connect(this, &MainWindow::check_next_item_sig, this, &MainWindow::self_check_next_item_hdlr,
            Qt::QueuedConnection);
    connect(this, &MainWindow::self_check_finished_sig,
            this, &MainWindow::self_check_finished_sig_hdlr,Qt::QueuedConnection);

    setup_sport_parameters();
    connect(&m_pb_sport, &QSerialPort::readyRead,
            this, &MainWindow::pbSportReadyReadHdlr, Qt::QueuedConnection);
    connect(this, &MainWindow::parse_sport_data_sig, this, &MainWindow::parse_pb_sport_data, Qt::QueuedConnection);

    connect(m_mainmenubtns_widget, &MainmenuBtnsWidget::send_pb_power_off_sig,
            this, &MainWindow::send_pb_power_off_sig_hdlr);

    setup_hv_conn_client();

    m_scan_widget->setup_tools(m_hv_conn_device);

    self_check(g_sys_configs_block.enable_self_check);

    refresh_storage_st();
}

void MainWindow::self_check(bool go_check)
{
    if(!m_pb_sport_open) open_sport();
    if(m_hv_conn_state != QModbusDevice::ConnectedState) hv_connect();

    if(go_check)
    {
        emit check_next_item_sig(true);
    }
    else
    {
        emit self_check_finished_sig(true);
    }
}

void MainWindow::self_check_next_item_hdlr(bool start, bool last_ret)
{
    static MainWindow::self_chk_ret_e_t chk_st = MainWindow::SELF_CHK_END;
    static int hdlr_idx = 0;
    static bool ret = true;
    if(start)
    {
        hdlr_idx = 0;
        ret = true;
        chk_st = MainWindow::SELF_CHK_END;
    }
    else if(hdlr_idx >= m_check_hdlrs.count())
    {
        hdlr_idx = 0;
        bool final_ret = ret;
        ret = true;

        emit self_check_finished_sig(final_ret);
        return;
    }

    bool sub_ret;
    if(MainWindow::SELF_CHK_END == chk_st)
    {
        chk_st = (this->*m_check_hdlrs[hdlr_idx])(&sub_ret);

        if(MainWindow::SELF_CHK_END == chk_st)
        {
            ret = ret && sub_ret;
            ++hdlr_idx;
        }
        //else: pending. do nothing now, wait next call.
    }
    else //last time pending, now end.
    {
        ret = ret && last_ret;
        ++hdlr_idx;
        chk_st = MainWindow::SELF_CHK_END;
        emit check_next_item_sig();
    }
}

MainWindow::self_chk_ret_e_t MainWindow::pwr_st_check(bool * result)
{
    MainWindow::self_chk_ret_e_t ret = MainWindow::SELF_CHK_END;

    emit self_check_item_ret_sig(SelfCheckWidget::SELF_CHECK_PWR, SelfCheckWidget::SELF_CHECKING);

    if(g_sys_configs_block.skip_pwr_self_chk)
    {
        emit self_check_item_ret_sig(SelfCheckWidget::SELF_CHECK_PWR,
                                    SelfCheckWidget::SELF_CHECK_PASS);
        emit check_next_item_sig();
        if(result) *result = true;
        return ret;
    }

    if(!pb_monitor_check_st_hdlr())
    {
        emit self_check_item_ret_sig(SelfCheckWidget::SELF_CHECK_PWR,
                                     SelfCheckWidget::SELF_CHECK_FAIL);
        emit check_next_item_sig();
        if(result) *result = false;
        return ret;
    }

    ret = MainWindow::SELF_CHK_PENDING;
    m_pb_self_chk_timer.start(g_sys_configs_block.pb_self_chk_to_ms);
    m_pb_is_self_checking = true;

    return ret;
}

MainWindow::self_chk_ret_e_t MainWindow::x_ray_source_st_check(bool * result)
{
    bool chk_ret = true;
    bool chk_finished = false;

    emit self_check_item_ret_sig(SelfCheckWidget::SELF_CHECK_XRAY, SelfCheckWidget::SELF_CHECKING);
    if(g_sys_configs_block.skip_x_src_self_chk)
    {
        chk_ret = true;
        chk_finished = true;
    }
    else
    {
        if(HV_SELF_CHK_CONN_DONE == m_hv_self_chk_stg)
        {
            if(QModbusDevice::ConnectedState == m_hv_conn_state)
            {
                m_scan_widget->hv_send_op_cmd(HV_OP_READ_REGS);
                m_hv_self_chk_stg = HV_SELF_CHK_WAITING_READ;
            }
            else
            {
                chk_ret = false;
                chk_finished = true;
            }
        }
    }

    if(chk_finished)
    {
        emit self_check_item_ret_sig(SelfCheckWidget::SELF_CHECK_XRAY,
                                     chk_ret ? SelfCheckWidget::SELF_CHECK_PASS
                                             : SelfCheckWidget::SELF_CHECK_FAIL);
        m_hv_self_chk_stg = HV_SELF_CHK_FINISHED;

        emit check_next_item_sig(false, chk_ret);
        if(!result) *result = chk_ret;
        return MainWindow::SELF_CHK_END;
    }
    else
    {
        return MainWindow::SELF_CHK_PENDING;
    }
}

MainWindow::self_chk_ret_e_t MainWindow::detector_st_check(bool * result)
{
    emit self_check_item_ret_sig(SelfCheckWidget::SELF_CHECK_DETECTOR, SelfCheckWidget::SELF_CHECKING);

    if(g_sys_configs_block.skip_detector_self_chk)
    {
        emit self_check_item_ret_sig(SelfCheckWidget::SELF_CHECK_DETECTOR,
                                    SelfCheckWidget::SELF_CHECK_PASS);
        emit check_next_item_sig();

        if(result) *result = true;
        return MainWindow::SELF_CHK_END;
    }
    else
    {
        m_scan_widget->detector_self_check();
        return MainWindow::SELF_CHK_PENDING;
    }
}
MainWindow::self_chk_ret_e_t MainWindow::storage_st_check(bool * result)
{
    bool ret = true;
    emit self_check_item_ret_sig(SelfCheckWidget::SELF_CHECK_STORAGE, SelfCheckWidget::SELF_CHECKING);
    if(!g_sys_configs_block.skip_storage_self_chk)
    {
        storage_space_info_s_t storage_info;
        get_total_storage_amount(storage_info);
        ret = (storage_info.total > 0);
    }
    emit self_check_item_ret_sig(SelfCheckWidget::SELF_CHECK_STORAGE,
                                 ret ? SelfCheckWidget::SELF_CHECK_PASS
                                     : SelfCheckWidget::SELF_CHECK_FAIL);
    emit check_next_item_sig(false, ret);
    if(result) *result = ret;
    return MainWindow::SELF_CHK_END;
}

void MainWindow::self_check_hv_rechk_sig_hdlr()
{
    x_ray_source_st_check();
}

MainWindow::~MainWindow()
{
    m_pb_monitor_timer.stop();
    close_sport();

    m_hv_monitor_timer.stop();
    m_hv_reconn_wait_timer.stop();
    hv_disconnect();

    rec_widgets_ui_settings();

    delete ui;
}

void MainWindow::refresh_storage_st()
{
    storage_space_info_s_t storage_info;
    get_total_storage_amount(storage_info);

    qint64 unit_val;
    QString unit_str = trans_bytes_cnt_unit(storage_info.total, &unit_val);
    QString aval_str = QString::number(storage_info.total_ava / unit_val) + " " + unit_str;
    QString total_str = QString::number(storage_info.total / unit_val) + " " + unit_str;
    QString disp_str = QString("%1 %2/%3").arg(g_str_storage_space, aval_str, total_str);
    ui->storageCapLbl->setText(disp_str);
}

void MainWindow::self_check_finished_sig_hdlr(bool result)
{
    m_self_check_passed = result;

    if(result)
    {
        goto_login_widget();
    }
    if(g_sys_configs_block.enable_pb_monitor)
    {
        m_pb_monitor_timer.start(g_sys_configs_block.pb_monitor_period_ms);
    }
    if(g_sys_configs_block.enable_hv_monitor)
    {
        m_hv_monitor_timer.start(g_sys_configs_block.hv_monitor_period_ms);
    }
}

void MainWindow::goto_login_widget()
{
    if(m_stacked_widget->indexOf(m_login_widget) < 0)
    {
        m_stacked_widget->addWidget(m_login_widget);
    }
    m_stacked_widget->setCurrentWidget(m_login_widget);
}

void MainWindow::goto_syssettings_widget()
{
    if(m_stacked_widget->indexOf(m_syssettings_widget) < 0)
    {
        m_stacked_widget->addWidget(m_syssettings_widget);
    }
    m_stacked_widget->setCurrentWidget(m_syssettings_widget);
}

void MainWindow::go_to_scan_widget()
{
    if(m_stacked_widget->indexOf(m_scan_widget) < 0)
    {
        m_stacked_widget->addWidget(m_scan_widget);
    }
    m_stacked_widget->setCurrentWidget(m_scan_widget);
    emit scan_widget_disp_sig();
}

void MainWindow::go_to_camera_widget()
{
    if(m_stacked_widget->indexOf(m_camera_widget) < 0)
    {
        m_stacked_widget->addWidget(m_camera_widget);
    }
    m_stacked_widget->setCurrentWidget(m_camera_widget);
}

void MainWindow::go_to_syssettings_widget_sig_hdlr()
{
    goto_syssettings_widget();
}

void MainWindow::go_to_scan_widget_sig_hdlr()
{
    go_to_scan_widget();
}

void MainWindow::go_to_camera_widget_sig_hdlr()
{
    go_to_camera_widget();
}

void MainWindow::login_chk_passed_sig_hdlr()
{
    go_to_scan_widget();
}

void MainWindow::detector_self_chk_ret_sig_hdlr(bool ret)
{
    emit self_check_item_ret_sig(SelfCheckWidget::SELF_CHECK_DETECTOR,
                                 ret ? SelfCheckWidget::SELF_CHECK_PASS
                                     : SelfCheckWidget::SELF_CHECK_FAIL);
    emit check_next_item_sig(false, ret);
}

void MainWindow::hv_monitor_timer_sig_hdlr()
{
    if(QModbusDevice::ConnectedState == m_hv_conn_state)
    {
        m_scan_widget->hv_send_op_cmd(HV_OP_READ_REGS);
    }
}

void MainWindow::mb_regs_read_ret_sig_hdlr(mb_reg_val_map_t reg_val_map)
{
    qint16 st_code = reg_val_map[State];
    QString disp_str = g_str_hv_st;
    QString stylesheet;

    if(g_hv_st_code_idle == st_code || g_hv_st_code_hs == st_code)
    {
        disp_str += hv_work_st_str(st_code);
        stylesheet =  "QLabel { color : black; }";
    }
    else
    {
        disp_str += QString::number(st_code, 16).toUpper().rightJustified(2, '0');
        stylesheet =  "QLabel { color : red; }";
    }
    ui->hvConnLbl->setStyleSheet(stylesheet);
    ui->hvConnLbl->setText(disp_str);
}

void MainWindow::hv_op_finish_sig_hdlr(bool ret, QString /*err_str*/)
{
    if(HV_SELF_CHK_WAITING_READ == m_hv_self_chk_stg)
    {
        emit self_check_item_ret_sig(SelfCheckWidget::SELF_CHECK_XRAY,
                                     ret ? SelfCheckWidget::SELF_CHECK_PASS
                                         : SelfCheckWidget::SELF_CHECK_FAIL);
        m_hv_self_chk_stg = HV_SELF_CHK_FINISHED;

        emit check_next_item_sig(false, ret);
    }

    if(!ret)
    {
        ui->hvConnLbl->setStyleSheet("QLabel { color : red; }");
        ui->hvConnLbl->setText(g_str_hv_op_error);
    }
}

QString MainWindow::hv_work_st_str(quint16 st_reg_val)
{
    static bool ls_first = true;
    typedef struct
    {
        quint16 val; QString str;
    }st_val_to_str_s_t;
    static const st_val_to_str_s_t ls_st_val_to_str_arr[] =
    {
        {g_hv_st_code_idle, "空闲"},
        {g_hv_st_code_hs, "散热"},
        {g_hv_st_code_input1_fb_abn, "input1状态反馈异常"},
        {g_hv_st_code_curr_fb_abn, "电流反馈异常"},
        {g_hv_st_code_volt_fb_abn, "电压反馈异常"},
        {g_hv_st_code_mult_abn, "多个异常同时发生"},
    };
    static QMap<quint16, QString> ls_st_val_to_str_map;

    if(ls_first)
    {
        for(int i = 0; i < ARRAY_COUNT(ls_st_val_to_str_arr); ++i)
        {
            ls_st_val_to_str_map.insert(ls_st_val_to_str_arr[i].val,
                                        ls_st_val_to_str_arr[i].str);
        }
        ls_first = false;
    }

    QString st_str;
    if(ls_st_val_to_str_map.contains(st_reg_val))
    {
        st_str = ls_st_val_to_str_map[st_reg_val];
    }
    else
    {
        st_str = QString(g_str_unkonw_st) + ":0x" + QString::number(st_reg_val, 16).toUpper();
    }
    return st_str;
}

void MainWindow::pb_monitor_timer_hdlr()
{
    if(!g_sys_configs_block.disable_monitor_during_scan || !g_data_scanning_now)
    {
        emit pb_monitor_check_st();
    }
}

void MainWindow::pb_self_chk_to_timer_hdlr()
{
    m_pb_is_self_checking = false;

    emit self_check_item_ret_sig(SelfCheckWidget::SELF_CHECK_PWR,
                                 SelfCheckWidget::SELF_CHECK_FAIL);

    emit check_next_item_sig(false, false);
}

#define CHECK_SPORT_AND_OPEN(...) \
if(!m_pb_sport_open && !open_sport())\
{\
    DIY_LOG(LOG_ERROR, "sport not open, and open failed."); \
    return __VA_ARGS__;\
}

bool MainWindow::pb_monitor_check_st_hdlr()
{
    QString log_str;
    LOG_LEVEL log_lvl = LOG_INFO;
    bool set_ok;
    static char data_arr[] = {gs_addr_byte,
                    (char)((gs_pwr_st_chk_val >> 8) & 0xFF), (char)(gs_pwr_st_chk_val& 0xFF),
                    gs_dif_byte_pwr_st};
    int byte_cnt = ARRAY_COUNT(data_arr);
    //QString charging_str = g_str_unkonw_st, bat_lvl_str = g_str_unkonw_st;

    CHECK_SPORT_AND_OPEN(false);

    set_ok = write_to_sport(data_arr, byte_cnt, g_sys_configs_block.pb_monitor_log);
    if(!set_ok)
    {
        log_lvl = LOG_ERROR;
        log_str += "write serial port error.\n";
        DIY_LOG(log_lvl, log_str);
    }
    return set_ok;
}

bool MainWindow::write_to_sport(char* data_arr, qint64 byte_cnt, bool log_rw)
{
    bool set_ok = false;
    CHECK_SPORT_AND_OPEN(set_ok);

    QString log_str;
    LOG_LEVEL log_lvl;

    qint64 bytes_written;
    bytes_written = m_pb_sport.write(data_arr, byte_cnt);
    log_lvl = LOG_INFO;
    log_str = QString("Write data to %1: ").arg(g_sys_configs_block.pb_sport_params.com_port_s)
            + QByteArray::fromRawData(data_arr, byte_cnt).toHex(' ').toUpper() + "\n";
    if(bytes_written < byte_cnt)
    {
        log_str += QString("Write data to %1 error: expect write %2 bytes, "
                          "but in fact written %3 bytes.")
                    .arg(g_sys_configs_block.pb_sport_params.com_port_s).arg(byte_cnt).arg(bytes_written);
        log_lvl = LOG_ERROR;
    }
    else
    {
        set_ok = true;
    }
    if(log_rw)
    {
        DIY_LOG(log_lvl, log_str);
    }
    return set_ok;
}

/*buf_size is also the expected read bytes cnt.*/
bool MainWindow::read_from_sport(char* read_data, qint64 buf_size, bool log_rw)
{
    bool ret = true;
    QString log_str;
    LOG_LEVEL log_lvl = LOG_INFO;

    CHECK_SPORT_AND_OPEN(false);

    int idx = 0;
    qint64 bytes_read_this_op = 0, total_bytes_read = 0;
    while(idx < gs_sport_read_try_cnt && total_bytes_read < buf_size)
    {
        bytes_read_this_op = m_pb_sport.read(&read_data[total_bytes_read], buf_size - total_bytes_read);
        total_bytes_read += bytes_read_this_op;
        ++idx;
    }
    log_str += QString("bytes read : ");
    log_str += QByteArray::fromRawData(read_data, qMin(total_bytes_read, buf_size)).toHex(' ').toUpper() + "\n";
    if(total_bytes_read != buf_size)
    {
        log_lvl = LOG_ERROR;
        log_str += QString("read from %1 %2 bytes, unequal to expected %3.\n")
                    .arg(g_sys_configs_block.pb_sport_params.com_port_s).arg(total_bytes_read).arg(buf_size);
        ret = false;
    }

    if(log_rw)
    {
        DIY_LOG(log_lvl, log_str);
    }
    return ret;
}

void MainWindow::setup_sport_parameters()
{
    m_pb_sport.setPortName(g_sys_configs_block.pb_sport_params.com_port_s);
    m_pb_sport.setBaudRate((QSerialPort::BaudRate)g_sys_configs_block.pb_sport_params.boudrate);
    m_pb_sport.setDataBits((QSerialPort::DataBits)g_sys_configs_block.pb_sport_params.databits);
    m_pb_sport.setParity((QSerialPort::Parity)g_sys_configs_block.pb_sport_params.parity);
    m_pb_sport.setStopBits((QSerialPort::StopBits)g_sys_configs_block.pb_sport_params.stopbits);
    m_pb_sport.setFlowControl(QSerialPort::NoFlowControl);

    m_pb_sport_read_buf.hd_idx = 0;
    m_pb_sport_read_buf.len_from_hd = 0;
    m_pb_sport_read_buf.buf = QByteArray(gs_pb_sport_read_buf_size, 0);

    m_min_pb_sport_msg_len = std::min({gs_pwr_off_msg_len, gs_wkup_slp_msg_len, gs_motor_rpm_msg_len, gs_pwr_st_msg_len});
}

bool MainWindow::open_sport()
{
    QString sport_info_str = QString("name: %1, baudrate: %2, databits: %3,"
                                     "parity: %4, stopbits: %5")
                            .arg(g_sys_configs_block.pb_sport_params.com_port_s)
                            .arg(g_sys_configs_block.pb_sport_params.boudrate)
                            .arg(g_sys_configs_block.pb_sport_params.databits)
                            .arg(g_sys_configs_block.pb_sport_params.parity)
                            .arg(g_sys_configs_block.pb_sport_params.stopbits);
    if(!m_pb_sport_open)
    {
        if(m_pb_sport.open(QIODevice::ReadWrite))
        {
            m_pb_sport_open = true;
            ui->pbConnLbl->setText(g_str_connected);
            DIY_LOG(LOG_INFO, QString("%1: %2").arg(gs_str_sport_open_succeed, sport_info_str));
        }
        else
        {
            m_pb_sport_open = false;
            ui->pbConnLbl->setText(g_str_disconnected);
            DIY_LOG(LOG_ERROR, QString("%1: %2").arg(gs_str_sport_open_fail, sport_info_str));
        }
    }
    return m_pb_sport_open;
}

bool MainWindow::close_sport()
{
    m_pb_sport.close();

    m_pb_sport_open = false;
    return true;
}

void MainWindow::pbSportReadyReadHdlr()
{
    int buf_size = m_pb_sport_read_buf.buf.size();

    QByteArray sport_data;
    sport_data = m_pb_sport.readAll();

    if(g_sys_configs_block.pb_monitor_log)
    {
        DIY_LOG(LOG_INFO, QString("sport read data: %1").arg(QString(sport_data.toHex(' ').toUpper())));
    }

    int end_pos = m_pb_sport_read_buf.hd_idx + m_pb_sport_read_buf.len_from_hd;
    if(end_pos + sport_data.size() > buf_size)
    {
        //move data to start pos.
        if(m_pb_sport_read_buf.hd_idx != 0)
        {
            Q_ASSERT(m_pb_sport_read_buf.hd_idx + m_pb_sport_read_buf.len_from_hd <= buf_size);

            memmove(m_pb_sport_read_buf.buf.data(),
                    &m_pb_sport_read_buf.buf.constData()[m_pb_sport_read_buf.hd_idx],
                    m_pb_sport_read_buf.len_from_hd);
            m_pb_sport_read_buf.hd_idx = 0;
            end_pos = m_pb_sport_read_buf.len_from_hd;
        }

        if(end_pos + sport_data.size() > buf_size)
        {
            DIY_LOG(LOG_ERROR, QString("buf size is %1, data cnt in buf is %2, received data cnt is %3,"
                                       "buffer override! Discard all buffer data and received data.")
                                        .arg(buf_size).arg(m_pb_sport_read_buf.len_from_hd)
                                        .arg(sport_data.size()));
            clear_pb_sport_data_buf();
            return;
        }
        m_pb_sport_read_buf.buf.insert(end_pos, sport_data);
    }
    else
    {
        m_pb_sport_read_buf.buf.insert(end_pos, sport_data);
    }
    m_pb_sport_read_buf.len_from_hd += sport_data.size();
    if(g_sys_configs_block.pb_monitor_log)
    {
        DIY_LOG(LOG_INFO, QString("now buf data is: %1").
                          arg(QString(m_pb_sport_read_buf.buf
                             .mid(m_pb_sport_read_buf.hd_idx, m_pb_sport_read_buf.len_from_hd)
                             .toHex(' ').toUpper())));
    }

    emit parse_sport_data_sig();
}

void MainWindow::parse_pb_sport_data()
{
    int buf_size = m_pb_sport_read_buf.buf.size();
    int hd_idx = m_pb_sport_read_buf.hd_idx, data_len,
        end_pos = hd_idx + m_pb_sport_read_buf.len_from_hd;
    QByteArray &buf = m_pb_sport_read_buf.buf;

    Q_ASSERT(end_pos <= buf_size);

    bool finished = false, update_buf_idx = true;
    while((hd_idx < end_pos) && !finished)
    {
        while(hd_idx < end_pos && (gs_pb_sport_data_start_flag != buf[hd_idx])) ++hd_idx;

        if(hd_idx >= end_pos)
        {
            DIY_LOG(LOG_ERROR, QString("can't find flag 0x%1 in buf. discard all received data.")
                                   .arg(QString::number(gs_pb_sport_data_start_flag, 16).toUpper().rightJustified(2, '0')));
            clear_pb_sport_data_buf();
            update_buf_idx = false;
            break;
        }

        data_len = end_pos - hd_idx;
        if(data_len < m_min_pb_sport_msg_len)
        {
            DIY_LOG(LOG_INFO, QString("buf data len %1 is less than min valid msg len %2. do not process")
                              .arg(data_len).arg(m_min_pb_sport_msg_len));
            break;
        }
        /* the pb sport msg format is not very reasonable: if there happens msg-split or sticky by driver,
           diffrent kinds of msg may not be distinguished.
           so here we can only take some assumptions: if the length of msg exceeds min valid len, take it as
           long valid msg.
           this part of code DEPENDS on msge definition (list in the beginning of this file). if there is any
           change or adding to msg, check below code carefully and do the accordingly modification as necessary!
         */

        /* firstly, check if it is the longes msg.*/
        if(data_len > m_min_pb_sport_msg_len)
        {
            if(data_len < gs_pwr_st_msg_len)
            {
                DIY_LOG(LOG_INFO, QString("data len in buf is %1, we think it as incomplete power-battery-st msg, "
                                          "and do not process it untill it is completed").arg(data_len));
                break;
            }

            if(gs_dif_byte_pwr_st == buf[hd_idx + gs_pwr_st_msg_len - 1])
            {
                //this is power-battery-st msg.
                proc_pb_pwr_bat_st_msg(buf.mid(hd_idx, gs_pwr_st_msg_len));
                hd_idx += gs_pwr_st_msg_len;

                continue;
            }

            /*else: think it as sticked msg.*/
            DIY_LOG(LOG_WARN, QString("data in buf is longer than power-battery-st msg, but it does not "
                                      "end with %1. so we think it is a sticked msges.")
                                  .arg(QString::number(gs_dif_byte_pwr_st, 16).toUpper().rightJustified(2, '0')));
        }

        /* now, check if its a short msg. currently, all short msgs are of the same length. so the code
           can be simpler...
        */
        char diff_byte = buf[hd_idx + m_min_pb_sport_msg_len - 1];
        switch(diff_byte)
        {
        case gs_dif_byte_pwr_off:
            proc_pb_pwr_off_msg(buf.mid(hd_idx, gs_pwr_off_msg_len));
            hd_idx += gs_pwr_off_msg_len;
            break;

        case gs_dif_byte_wkup_slp:
            proc_pb_wkup_msg(buf.mid(hd_idx, gs_wkup_slp_msg_len));
            hd_idx += gs_wkup_slp_msg_len;
            break;

        case gs_dif_byte_motor_rpm:
            proc_pb_motor_msg(buf.mid(hd_idx, gs_motor_rpm_msg_len));
            hd_idx += gs_motor_rpm_msg_len;
            break;

        default:
            DIY_LOG(LOG_WARN, QString("diff byte of data is %1, not a valid short msg.")
                                  .arg(QString::number(diff_byte, 16).toUpper().rightJustified(2, '0')));
            if(data_len == m_min_pb_sport_msg_len)
            {
                //may be an incomplete long msg. wait future data.
                DIY_LOG(LOG_INFO, QString("this maybe an implete power-battery-st msge. wait future data." ));
                finished = true;
            }
            else
            { // data_len > m_min_pb_sport_msg_len. flow from above long msg check.
                DIY_LOG(LOG_WARN, QString("this part is neither a shor or a long msg."
                                          " discard the long part."));
                hd_idx += gs_pwr_st_msg_len;
            }
            break;
        }
    }

    if(update_buf_idx)
    {
        m_pb_sport_read_buf.hd_idx = hd_idx;
        if(m_pb_sport_read_buf.hd_idx >= buf_size)
        {
            DIY_LOG(LOG_WARN, "this should not happen: hd_idx exceeds buf limit!!!");
            clear_pb_sport_data_buf();
        }
        else
        {
            m_pb_sport_read_buf.len_from_hd = end_pos - hd_idx;
            if(m_pb_sport_read_buf.len_from_hd < 0)
            {
                DIY_LOG(LOG_WARN, "this should not happen: counted len is less than 0!!!");
                m_pb_sport_read_buf.len_from_hd = 0;
            }
        }
    }
}

void MainWindow::proc_pb_pwr_off_msg(QByteArray msg)
{
    QString log_str = QString("recerive power off msg, power off. ")
                      + msg.toHex(' ').toUpper();
    DIY_LOG(LOG_INFO, log_str);

    LOCAL_DIY_LOG(LOG_INFO, g_main_th_local_log_fn, log_str);

    if(APP_EXIT_APP_POWER_OFF != m_exit_mode)
    {
        m_exit_mode = APP_EXIT_HD_POWER_OFF;
    }
    QCoreApplication::exit((int)m_exit_mode);
}

void MainWindow::proc_pb_wkup_msg(QByteArray msg)
{
    quint16 wk_slp_v = (quint16)msg[1] * 256 + (quint16)msg[2];
    DIY_LOG(LOG_INFO, QString("wake/sleep st is %1").arg(wk_slp_v));
}

void MainWindow::proc_pb_motor_msg(QByteArray msg)
{
    quint16 rpm_v = (quint16)msg[1] * 256 + (quint16)msg[2];
    DIY_LOG(LOG_INFO, QString("motor speed is % rpm.").arg(rpm_v));
}

void MainWindow::proc_pb_pwr_bat_st_msg(QByteArray msg)
{
    quint16 volt_val, current_val;
    quint8 bat_pct;
    int val_byte_idx = 1;
    QByteArray &read_data = msg;
    QString charging_str, bat_lvl_str;

    volt_val = (quint16)read_data[val_byte_idx] * 256 + (quint16)read_data[val_byte_idx + 1];
    val_byte_idx += 2;

    *((char*)(&current_val) + 1) = read_data[val_byte_idx];
    *(char*)(&current_val) = read_data[val_byte_idx + 1];
    val_byte_idx += 2;
    charging_str = current_val > 0 ? g_str_charging : "";

    bat_pct = (quint8)read_data[val_byte_idx];
    val_byte_idx += 1;
    bat_lvl_str = QString::number(bat_pct) + "%";

    ui->batImgLbl->setText(charging_str);
    ui->batLvlDispLbl->setText(bat_lvl_str);

    if(m_pb_is_self_checking)
    {
        m_pb_self_chk_timer.stop();
        m_pb_is_self_checking = false;

        emit self_check_item_ret_sig(SelfCheckWidget::SELF_CHECK_PWR,
                                     SelfCheckWidget::SELF_CHECK_PASS);

        emit check_next_item_sig(false, true);
    }
}

void MainWindow::clear_pb_sport_data_buf()
{
    m_pb_sport_read_buf.hd_idx = m_pb_sport_read_buf.len_from_hd = 0;
}

void MainWindow::setup_hv_conn_client()
{
    m_hv_conn_device = new QModbusRtuSerialMaster(this);

    m_hv_conn_device->setConnectionParameter(QModbusDevice::SerialPortNameParameter,
                                g_sys_configs_block.x_ray_mb_conn_params.serial_params.com_port_s);
    m_hv_conn_device->setConnectionParameter(QModbusDevice::SerialBaudRateParameter,
                                g_sys_configs_block.x_ray_mb_conn_params.serial_params.boudrate);
    m_hv_conn_device->setConnectionParameter(QModbusDevice::SerialDataBitsParameter,
                                g_sys_configs_block.x_ray_mb_conn_params.serial_params.databits);
    m_hv_conn_device->setConnectionParameter(QModbusDevice::SerialParityParameter,
                                g_sys_configs_block.x_ray_mb_conn_params.serial_params.parity);
    m_hv_conn_device->setConnectionParameter(QModbusDevice::SerialStopBitsParameter,
                                g_sys_configs_block.x_ray_mb_conn_params.serial_params.stopbits);
    m_hv_conn_device->setTimeout(g_sys_configs_block.x_ray_mb_conn_params.resp_wait_time_ms);

    connect(m_hv_conn_device, &QModbusClient::errorOccurred,
            this, &MainWindow::hv_conn_error_sig_handler, Qt::QueuedConnection);
    connect(m_hv_conn_device, &QModbusClient::stateChanged,
            this, &MainWindow::hv_conn_state_changed_sig_handler, Qt::QueuedConnection);

    m_hv_reconn_wait_timer.setSingleShot(true);
    connect(&m_hv_reconn_wait_timer, &QTimer::timeout,
            this, &MainWindow::hv_reconn_wait_timer_sig_handler, Qt::QueuedConnection);
}
void MainWindow::hv_connect()
{
    if(m_hv_conn_device)
    {
        m_hv_conn_device->connectDevice();
    }
}

void MainWindow::hv_disconnect()
{
    if(m_hv_conn_device)
    {
        m_hv_conn_device->disconnectDevice();
    }
}

void MainWindow::hv_conn_error_sig_handler(QModbusDevice::Error error)
{
    /*The strings below are in the same order of enum QModbusDevice::Error.*/
    static const char* err_str[] =
    {
        "No errors have occurred.",
        "An error occurred during a read operation.",
        "An error occurred during a write operation.",
        "An error occurred when attempting to open the backend.",
        "An error occurred when attempting to set a configuration parameter.",
        "A timeout occurred during I/O. An I/O operation did not finish within a given time frame.",
        "A Modbus specific protocol error occurred.",
        "The reply was aborted due to a disconnection of the device.",
        "An unknown error occurred.",
    };
    QString curr_str;

    curr_str = (error < 0 || error >= ARRAY_COUNT(err_str)) ?
                    QString("%1:%2").arg(g_str_modbus_exceptional_error, QString::number(error))
                    : err_str[error];

    if((error < 0) || (QModbusDevice::NoError != error))
    {
        DIY_LOG(LOG_ERROR, curr_str);
    }
    QString stylesheet = (QModbusDevice::NoError == error) ? "QLabel { color : black; }"
                                                : "QLabel { color : red; }";
    ui->hvConnLbl->setStyleSheet(stylesheet);
    ui->hvConnLbl->setText(QString::number(error));
}

void MainWindow::hv_conn_state_changed_sig_handler(QModbusDevice::State state)
{
    /*The following strings are in the same order with enum QModbusDevice::State*/
    static const char* state_str[] =
    {
        "The device is disconnected.",
        "The device is being connected.",
        "The device is connected to the Modbus network.",
        "The device is being closed.",
    };

    m_hv_conn_state = state;

    QString curr_str = (state < 0 || (int)state >= ARRAY_COUNT(state_str)) ?
                QString("%1:%2").arg(g_str_modbus_unkonwn_state, QString::number(state))
              : state_str[state];
    LOCAL_DIY_LOG(LOG_INFO, g_main_th_local_log_fn, curr_str);
    DIY_LOG(LOG_INFO, curr_str);

    QString lbl_str, stylesheet;
    if(QModbusDevice::ConnectedState == state)
    {
        lbl_str = g_str_connected;
        stylesheet = "QLabel { color : darkgreen; }";
    }
    else
    {
        lbl_str = g_str_disconnected;
        stylesheet = "QLabel { color : red; }";
        if(QModbusDevice::UnconnectedState == state
            && g_sys_configs_block.enable_hv_auto_reconn)
        {
            m_hv_reconn_wait_timer.start(g_sys_configs_block.mb_reconnect_wait_ms);
        }
    }
    ui->hvConnLbl->setStyleSheet(stylesheet);
    ui->hvConnLbl->setText(lbl_str);

    if(HV_SELF_CHK_WAITING_CONN == m_hv_self_chk_stg
            && (QModbusDevice::ConnectedState == m_hv_conn_state
                || QModbusDevice::UnconnectedState == m_hv_conn_state))
    {
        m_hv_self_chk_stg = HV_SELF_CHK_CONN_DONE;
        emit self_check_hv_rechk_sig();
    }
}

void MainWindow::hv_reconn_wait_timer_sig_handler()
{
    hv_connect();
}

void MainWindow::load_widgets_ui_settings()
{
    m_scan_widget->load_ui_settings();
    m_syssettings_widget->load_ui_settings();
}

void MainWindow::rec_widgets_ui_settings()
{
    m_scan_widget->rec_ui_settings();
    m_syssettings_widget->rec_ui_settings();
}

void MainWindow::send_pb_power_off_sig_hdlr()
{
    bool set_ok;
    static char data_arr[] = {gs_addr_byte, 0, 0, gs_dif_byte_pwr_off};
    int byte_cnt = ARRAY_COUNT(data_arr);
    LOG_LEVEL log_lvl = LOG_INFO;
    QString log_str;

    set_ok = write_to_sport(data_arr, byte_cnt, g_sys_configs_block.pb_monitor_log);
    if(!set_ok)
    {
        log_lvl = LOG_ERROR;
        log_str += "write serial port error.\n";
    }
    else
    {
        log_str += "send pb_power_off cmd ok";
    }
    LOCAL_DIY_LOG(log_lvl, g_main_th_local_log_fn, log_str);
}
