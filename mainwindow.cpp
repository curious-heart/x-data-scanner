#include <QMessageBox>
#include <QModbusRtuSerialMaster>

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "literal_strings/literal_strings.h"
#include "common_tools/common_tool_func.h"
#include "sysconfigs/sysconfigs.h"

extern bool g_data_scanning_now;

const quint16 g_hv_st_code_idle = 0x11;
const quint16 g_hv_st_code_hs = 0x22;
const quint16 g_hv_st_code_input1_fb_abn = 0xE1;
const quint16 g_hv_st_code_curr_fb_abn = 0xE2;
const quint16 g_hv_st_code_volt_fb_abn = 0xE3;
const quint16 g_hv_st_code_mult_abn = 0xE4;

static const char gs_addr_byte = 0x01;

static const char gs_dif_byte_motor_rpm = 0x03;
static const qint64 gs_motor_rpm_msg_len = 4;

static const char gs_dif_byte_pwr_st = 0x04;
static const qint64 gs_pwr_st_msg_len = 8;
static const quint16 gs_pwr_st_chk_val = 0;

static const char gs_dif_byte_wkup_slp = 0x02;
static const qint64 gs_wkup_slp_msg_len = 4;
static const quint16 gs_wkup_val = 0, gs_slp_val = 1;

static const int gs_sport_read_try_cnt = 3;

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

void MainWindow::self_check_next_item_hdlr(bool start)
{
    static int hdlr_idx = 0;
    static bool ret = true;
    if(start)
    {
        hdlr_idx = 0;
        ret = true;
    }
    else if(hdlr_idx >= m_check_hdlrs.count())
    {
        hdlr_idx = 0;
        bool final_ret = ret;
        ret = true;

        emit self_check_finished_sig(final_ret);
        return;
    }

    bool sub_ret = (this->*m_check_hdlrs[hdlr_idx])();
    ret = ret && sub_ret;

    ++hdlr_idx;
}

bool MainWindow::pwr_st_check()
{
    bool ret;

    emit self_check_item_ret_sig(SelfCheckWidget::SELF_CHECK_PWR, SelfCheckWidget::SELF_CHECKING);

    ret = g_sys_configs_block.skip_pwr_self_chk ? true : pb_monitor_check_st_hdlr();

    emit self_check_item_ret_sig(SelfCheckWidget::SELF_CHECK_PWR,
                                 ret ? SelfCheckWidget::SELF_CHECK_PASS
                                     : SelfCheckWidget::SELF_CHECK_FAIL);

    emit check_next_item_sig();

    return ret;
}

bool MainWindow::x_ray_source_st_check()
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

        emit check_next_item_sig();
    }

    return true;
}

bool MainWindow::detector_st_check()
{
    emit self_check_item_ret_sig(SelfCheckWidget::SELF_CHECK_DETECTOR, SelfCheckWidget::SELF_CHECKING);
    if(g_sys_configs_block.skip_detector_self_chk)
    {
        emit self_check_item_ret_sig(SelfCheckWidget::SELF_CHECK_DETECTOR,
                                    SelfCheckWidget::SELF_CHECK_PASS);
        emit check_next_item_sig();
    }
    else
    {
        m_scan_widget->detector_self_check();
    }
    return true;
}
bool MainWindow::storage_st_check()
{
    bool ret = true;
    emit self_check_item_ret_sig(SelfCheckWidget::SELF_CHECK_STORAGE, SelfCheckWidget::SELF_CHECKING);
    if(g_sys_configs_block.skip_storage_self_chk)
    {
        storage_space_info_s_t storage_info;
        get_total_storage_amount(storage_info);
        ret = (storage_info.total > 0);
    }
    emit self_check_item_ret_sig(SelfCheckWidget::SELF_CHECK_STORAGE,
                                 ret ? SelfCheckWidget::SELF_CHECK_PASS
                                     : SelfCheckWidget::SELF_CHECK_FAIL);
    emit check_next_item_sig();
    return ret;
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
    m_pb_monitor_timer.start(g_sys_configs_block.pb_monitor_period_ms);
    m_hv_monitor_timer.start(g_sys_configs_block.hv_monitor_period_ms);
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
    emit check_next_item_sig();
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

        emit check_next_item_sig();
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

#define CHECK_SPORT_AND_OPEN(...) \
if(!m_pb_sport_open && !open_sport())\
{\
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
    QString charging_str = g_str_unkonw_st, bat_lvl_str = g_str_unkonw_st;

    CHECK_SPORT_AND_OPEN(false);

    set_ok = write_to_sport(data_arr, byte_cnt, g_sys_configs_block.pb_monitor_log);
    if(set_ok)
    {
        quint16 volt_val, bat_pct;
        qint16 current_val;
        int val_byte_idx = 1;
        char read_data[gs_pwr_st_msg_len];
        set_ok = read_from_sport(read_data, gs_pwr_st_msg_len,
                                   g_sys_configs_block.pb_monitor_log);
        do
        {
            if(!set_ok)
            {
                log_lvl = LOG_ERROR;
                log_str += "read serial port error.\n";
                break;
            }

            if(read_data[0] != gs_addr_byte
                    || read_data[gs_pwr_st_msg_len - 1] != gs_dif_byte_pwr_st)
            {
                log_lvl = LOG_ERROR;
                log_str += "bytes array format error:\n";
                log_str += QByteArray(read_data, sizeof(read_data)).toHex(' ').toUpper() + "\n";
                set_ok = false;
                break;
            }
            volt_val = (quint16)read_data[val_byte_idx] * 256 + (quint16)read_data[val_byte_idx + 1];
            val_byte_idx += 2;

            *((char*)(&current_val) + 1) = read_data[val_byte_idx];
            *(char*)(&current_val) = read_data[val_byte_idx + 1];
            val_byte_idx += 2;
            charging_str = current_val > 0 ? g_str_charging : "";

            bat_pct = (quint16)read_data[val_byte_idx] * 256 + (quint16)read_data[val_byte_idx + 1];
            val_byte_idx += 2;
            bat_lvl_str = QString::number(bat_pct) + "%";
            break;
        }while(true);

    }
    else
    {
        log_lvl = LOG_ERROR;
        log_str += "write serial port error.\n";
    }

    ui->batImgLbl->setText(charging_str);
    ui->batLvlDispLbl->setText(bat_lvl_str);

    if(g_sys_configs_block.pb_monitor_log && !log_str.isEmpty())
    {
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
    log_str += QByteArray::fromRawData(read_data, qMin(total_bytes_read, buf_size)).toHex(' ').toUpper();
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
        if(QModbusDevice::UnconnectedState == state)
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
