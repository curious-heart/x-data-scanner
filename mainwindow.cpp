#include <QMessageBox>
#include <QModbusRtuSerialMaster>

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "common_tools/common_tool_func.h"
#include "literal_strings/literal_strings.h"
#include "sysconfigs/sysconfigs.h"

extern bool g_data_scanning_now;

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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    /*this widget should be newed first since it loads sys settings.*/
    m_syssettings_widget = new SysSettingsWidget(this);
    m_syssettings_widget->hide();

    m_stacked_widget = new QStackedWidget(this);
    ui->mainPartVBoxLayot->addWidget(m_stacked_widget);

    m_self_chk_widget = new SelfCheckWidget(this);
    m_stacked_widget->addWidget(m_self_chk_widget);

    m_login_widget = new LoginWidget(this);
    m_login_widget->hide();

    m_scan_widget = new ScanWidget(this);
    m_scan_widget->hide();

    m_mainmenubtns_widget = new MainmenuBtnsWidget(this);
    ui->buttonsHBoxLayout->addWidget(m_mainmenubtns_widget);

    connect(m_login_widget, &LoginWidget::login_chk_passed_sig,
            this, &MainWindow::login_chk_passed_sig_hdlr, Qt::QueuedConnection);

    connect(&m_pb_monitor_timer, &QTimer::timeout, this, &MainWindow::pb_monitor_timer_hdlr,
            Qt::QueuedConnection);
    connect(this, &MainWindow::pb_monitor_check_st, this, &MainWindow::pb_monitor_check_st_hdlr,
            Qt::QueuedConnection);

    connect(this, &MainWindow::self_check_item_ret_sig,
            m_self_chk_widget, &SelfCheckWidget::self_check_item_ret_sig_hdlr, Qt::QueuedConnection);
    connect(this, &MainWindow::check_next_item_sig, this, &MainWindow::self_chk,
            Qt::QueuedConnection);

    m_self_check_finish_timer.setSingleShot(true);
    connect(&m_self_check_finish_timer, &QTimer::timeout,
            this, &MainWindow::goto_login_widget, Qt::QueuedConnection);

    if(g_sys_configs_block.enable_self_check)
    {
        QTimer::singleShot(0, this, [this]() { self_chk(true); });
    }
    else
    {
        m_self_check_finish_timer.start(1000);
    }
}

void MainWindow::self_chk(bool start)
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

        if(final_ret)
        {
            m_self_check_finish_timer.start(1000);
        }
        return;
    }

    bool sub_ret = (this->*m_check_hdlrs[hdlr_idx])();
    ret = ret && sub_ret;

    ++hdlr_idx;
}

bool MainWindow::pwr_st_check()
{
    bool ret = open_sport();
    emit self_check_item_ret_sig(SelfCheckWidget::SELF_CHECK_PWR, ret);

    emit check_next_item_sig();

    return ret;
}

bool MainWindow::x_ray_source_st_check()
{
    bool ret = true;
    QThread::msleep(800);
    emit self_check_item_ret_sig(SelfCheckWidget::SELF_CHECK_XRAY, ret);

    emit check_next_item_sig();
    return ret;
}
bool MainWindow::detector_st_check()
{
    bool ret = true;
    QThread::msleep(800);
    emit self_check_item_ret_sig(SelfCheckWidget::SELF_CHECK_DETECTOR, ret);

    emit check_next_item_sig();
    return ret;
}
bool MainWindow::storage_st_check()
{
    bool ret = true;
    QThread::msleep(800);
    emit self_check_item_ret_sig(SelfCheckWidget::SELF_CHECK_STORAGE, ret);

    emit check_next_item_sig();
    return ret;
}

MainWindow::~MainWindow()
{
    close_sport();
    m_pb_monitor_timer.stop();

    delete ui;
}

void MainWindow::self_check_finished_sig_hdlr(bool result)
{
    if(result)
    {
        if(m_stacked_widget->indexOf(m_login_widget) < 0)
        {
            m_stacked_widget->addWidget(m_login_widget);
        }
        m_stacked_widget->setCurrentWidget(m_login_widget);

        m_pb_monitor_timer.start(g_sys_configs_block.pb_monitor_period_ms);
    }
}

void MainWindow::goto_login_widget()
{
    if(m_stacked_widget->indexOf(m_login_widget) < 0)
    {
        m_stacked_widget->addWidget(m_login_widget);
    }
    m_stacked_widget->setCurrentWidget(m_login_widget);

    m_pb_monitor_timer.start(g_sys_configs_block.pb_monitor_period_ms);
}

void MainWindow::login_chk_passed_sig_hdlr()
{
    if(m_stacked_widget->indexOf(m_scan_widget) < 0)
    {
        m_stacked_widget->addWidget(m_scan_widget);
    }
    m_stacked_widget->setCurrentWidget(m_scan_widget);
}

QString hv_work_st_str(quint16 st_reg_val)
{
    static bool ls_first = true;
    typedef struct
    {
        quint16 val; QString str;
    }st_val_to_str_s_t;
    static const st_val_to_str_s_t ls_st_val_to_str_arr[] =
    {
        {0x11, "空闲"},
        {0x22, "散热"},
        {0xE1, "input1状态反馈异常"},
        {0xE2, "电流反馈异常"},
        {0xE3, "电压反馈异常"},
        {0xE4, "多个异常同时发生"},
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

void MainWindow::pb_monitor_check_st_hdlr()
{
    QString log_str;
    LOG_LEVEL log_lvl = LOG_INFO;
    bool set_ok;
    char data_arr[] = {gs_addr_byte,
                    (char)((gs_pwr_st_chk_val >> 8) & 0xFF), (char)(gs_pwr_st_chk_val& 0xFF),
                    gs_dif_byte_pwr_st};
    int byte_cnt = ARRAY_COUNT(data_arr);

    set_ok = write_to_sport(data_arr, byte_cnt, true, g_sys_configs_block.pb_monitor_log);
    if(set_ok)
    {
        quint16 volt_val, bat_pct;
        qint16 current_val;
        int val_byte_idx = 1;
        char read_data[gs_pwr_st_msg_len];
        bool read_ret;
        read_ret = read_from_sport(read_data, gs_pwr_st_msg_len,
                                   g_sys_configs_block.pb_monitor_log);
        do
        {
            if(!read_ret)
            {
                log_lvl = LOG_ERROR;
                log_str += "read serial port error.\n";
                break;
            }

            if(read_data[0] != gs_addr_byte
                    || read_data[gs_pwr_st_msg_len - 1] != gs_dif_byte_pwr_st)
            {
                log_lvl = LOG_ERROR;
                log_str += "bytes array format error.\n";
                break;
            }
            volt_val = (quint16)read_data[val_byte_idx] * 256 + (quint16)read_data[val_byte_idx + 1];
            val_byte_idx += 2;

            *((char*)(&current_val) + 1) = read_data[val_byte_idx];
            *(char*)(&current_val) = read_data[val_byte_idx + 1];
            val_byte_idx += 2;
            QString charging_str = current_val > 0 ? g_str_charging : "";
            ui->batImgLbl->setText(charging_str);

            bat_pct = (quint16)read_data[val_byte_idx] * 256 + (quint16)read_data[val_byte_idx + 1];
            val_byte_idx += 2;
            ui->batLvlDispLbl->setText(QString::number(bat_pct) + "%");
            break;
        }while(true);

        if(g_sys_configs_block.pb_monitor_log && !log_str.isEmpty())
        {
            DIY_LOG(log_lvl, log_str);
        }
    }
}

#define CHECK_SPORT_OPEN(ret, silent) \
if(!m_pb_sport_open)\
{\
    if(!silent) QMessageBox::critical(this, "", g_str_plz_conn_dev_firstly);\
    return ret;\
}

bool MainWindow::write_to_sport(char* data_arr, qint64 byte_cnt, bool silent, bool log_rw)
{
    bool set_ok = false;
    CHECK_SPORT_OPEN(set_ok, silent);

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

bool MainWindow::open_sport()
{
    m_pb_sport.setPortName(g_sys_configs_block.pb_sport_params.com_port_s);
    m_pb_sport.setBaudRate((QSerialPort::BaudRate)g_sys_configs_block.pb_sport_params.boudrate);
    m_pb_sport.setDataBits((QSerialPort::DataBits)g_sys_configs_block.pb_sport_params.databits);
    m_pb_sport.setParity((QSerialPort::Parity)g_sys_configs_block.pb_sport_params.parity);
    m_pb_sport.setStopBits((QSerialPort::StopBits)g_sys_configs_block.pb_sport_params.stopbits);
    m_pb_sport.setFlowControl(QSerialPort::NoFlowControl);

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
    return true;
}

void MainWindow::setup_modbus_client()
{
    m_modbus_device = new QModbusRtuSerialMaster(this);

    m_modbus_device->setConnectionParameter(QModbusDevice::SerialPortNameParameter,
                                g_sys_configs_block.x_ray_mb_conn_params.serial_params.com_port_s);
    m_modbus_device->setConnectionParameter(QModbusDevice::SerialBaudRateParameter,
                                g_sys_configs_block.x_ray_mb_conn_params.serial_params.boudrate);
    m_modbus_device->setConnectionParameter(QModbusDevice::SerialDataBitsParameter,
                                g_sys_configs_block.x_ray_mb_conn_params.serial_params.databits);
    m_modbus_device->setConnectionParameter(QModbusDevice::SerialParityParameter,
                                g_sys_configs_block.x_ray_mb_conn_params.serial_params.parity);
    m_modbus_device->setConnectionParameter(QModbusDevice::SerialStopBitsParameter,
                                g_sys_configs_block.x_ray_mb_conn_params.serial_params.stopbits);
    m_modbus_device->setTimeout(g_sys_configs_block.x_ray_mb_conn_params.resp_wait_time_ms);

    connect(m_modbus_device, &QModbusClient::errorOccurred,
            this, &MainWindow::modbus_error_sig_handler, Qt::QueuedConnection);
    connect(m_modbus_device, &QModbusClient::stateChanged,
            this, &MainWindow::modbus_state_changed_sig_handler, Qt::QueuedConnection);
}

void MainWindow::modbus_error_sig_handler(QModbusDevice::Error error)
{}

void MainWindow::modbus_state_changed_sig_handler(QModbusDevice::State state)
{}
