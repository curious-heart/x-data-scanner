#include <cstring>
#include <algorithm>
#include <QMessageBox>
#include <QByteArray>

#include "scanwidget.h"
#include "ui_scanwidget.h"

#include "sysconfigs/sysconfigs.h"
#include "syssettings.h"
#include "literal_strings/literal_strings.h"

bool g_data_scanning_now = false;

static const char* gs_str_sc_data_txt_file_type = ".txt";
static const char* gs_str_img_raw_type = ".raw";
static const char* gs_str_img_png_type = ".png";
static const char* gs_str_8bit_img_apx = "-8bit";

static const char* gs_scan_cali_file_path = "./scan_cali_datum";
static const char* gs_scan_bg_value_fn = "scan_bg_data";
static const char* gs_scan_bg_value_txt_fn = "scan_bg_data.txt";
static const char* gs_scan_stre_factor_value_fn = "scan_stre_factor_data";
static const char* gs_scan_stre_factor_value_txt_fn = "scan_stre_factor_data.txt";

#undef RECV_DATA_NOTE_E
#define RECV_DATA_NOTE_E(e) #e
static const char* gs_recv_data_note_str [] = {RECV_DATA_NOTES};

ScanWidget::ScanWidget(UiConfigRecorder * cfg_recorder, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ScanWidget), m_cfg_recorder(cfg_recorder)
{
    qRegisterMetaType<src_of_collect_cmd_e_t>("src_of_collect_cmd_e_t");
    qRegisterMetaType<collect_rpt_evt_e_t>("collect_rpt_evt_e_t");
    qRegisterMetaType<hv_op_enum_t>("hv_op_enum_t");

    ui->setupUi(this);

    m_rec_ui_cfg_fin.clear(); m_rec_ui_cfg_fout.clear();
    m_rec_ui_cfg_fout << ui->scanLockChkBox;
    ui->scanLockChkBox->setChecked(!g_sys_configs_block.scan_without_x);

    ui->ptPerRowSpinBox->setMinimum(1);
    ui->ptPerRowSpinBox->setMaximum(g_sys_configs_block.max_pt_number);
    ui->ptPerRowSpinBox->setValue(g_sys_configs_block.max_pt_number);

    load_ui_settings();

    m_scan_dura_timer.setSingleShot(true);
    connect(&m_scan_dura_timer, &QTimer::timeout,
            this, &ScanWidget::scan_dura_timeout_hdlr, Qt::QueuedConnection);


    QString rmt_ip = g_sys_configs_block.data_src_ip;
    quint16 rmt_port = (quint16)g_sys_configs_block.data_src_port;
    recv_data_worker = new RecvScannedData(&dataQueue, &queueMutex,
                                       rmt_ip, rmt_port, g_sys_settings_blk.conn_data_src_timeout_sec);
    recv_data_workerThread = new QThread(this);
    recv_data_worker->moveToThread(recv_data_workerThread);
    connect(recv_data_workerThread, &QThread::finished,
            recv_data_worker, &QObject::deleteLater);

    connect(this, &ScanWidget::start_collect_sc_data_sig,
            recv_data_worker, &RecvScannedData::start_collect_sc_data_hdlr, Qt::QueuedConnection);
    connect(this, &ScanWidget::stop_collect_sc_data_sig,
            recv_data_worker, &RecvScannedData::stop_collect_sc_data_hdlr, Qt::QueuedConnection);
    connect(this, &ScanWidget::detector_handshake_sig,
            recv_data_worker, &RecvScannedData::handshake_comm, Qt::QueuedConnection);

    connect(recv_data_worker, &RecvScannedData::new_data_ready_sig,
            this, &ScanWidget::handleNewDataReady, Qt::QueuedConnection);
    connect(recv_data_worker, &RecvScannedData::recv_worker_report_sig,
            this, &ScanWidget::recv_worker_report_sig_hdlr, Qt::QueuedConnection);
    connect(recv_data_worker, &RecvScannedData::recv_data_finished_sig,
            this, &ScanWidget::recv_data_finished_sig_hdlr, Qt::QueuedConnection);

    m_expo_to_coll_delay_timer.setSingleShot(true);
    connect(&m_expo_to_coll_delay_timer, &QTimer::timeout,
            this, &ScanWidget::expo_to_coll_delay_timer_hdlr, Qt::QueuedConnection);

    m_expo_dura_timer.setSingleShot(true);
    connect(&m_expo_dura_timer, &QTimer::timeout, this, &ScanWidget::expo_dura_timer_sig_hdlr,
            Qt::QueuedConnection);

    connect(this, &ScanWidget::gen_cali_datum_to_file_sig,
            this, &ScanWidget::gen_cali_datum_to_file, Qt::QueuedConnection);

    clear_gray_img_lines();

    if(!mkpth_if_not_exists(gs_scan_cali_file_path))
    {
        DIY_LOG(LOG_WARN, QString("make cali data dir %1 fails").arg(gs_scan_cali_file_path));
    }
    load_cali_datum_from_file();

    proc_pt_per_row_cnt_related_work();
    m_display_buf_img.img_line_min_vs.resize(g_sys_configs_block.scrn_h);
    m_display_buf_img.img_line_max_vs.resize(g_sys_configs_block.scrn_h);
    m_display_buf_img.img_line_min_vs2.resize(g_sys_configs_block.scrn_h);
    m_display_buf_img.img_line_max_vs2.resize(g_sys_configs_block.scrn_h);

    recv_data_workerThread->start();

    reset_cali_ctrl_vars();

    btns_refresh();
}

void ScanWidget::setup_tools(QModbusClient * modbus_device)
{
    m_hv_device = modbus_device;
}

ScanWidget::~ScanWidget()
{
    m_scan_dura_timer.stop();
    m_expo_to_coll_delay_timer.stop();
    m_expo_dura_timer.stop();

    recv_data_workerThread->quit();
    recv_data_workerThread->wait();
    recv_data_workerThread->deleteLater();

    delete ui;
}

void ScanWidget::proc_pt_per_row_cnt_related_work()
{
    reload_cali_datum();
    reset_display_img_buffer();

    m_pt_per_row_changed = false;
}

void ScanWidget::reset_display_img_buffer()
{
    scan_widget_disp_sig_hdlr();

    int disp_pt_per_row = ui->ptPerRowSpinBox->value();
    if(m_pt_per_row_changed)
    {
        m_display_buf_img.img_buffer = QImage(disp_pt_per_row, g_sys_configs_block.scrn_h,
                                   QImage::Format_Grayscale16);
        m_display_buf_img.img_buffer2 = QImage(disp_pt_per_row, g_sys_configs_block.scrn_h,
                                   QImage::Format_Grayscale16);

    }

    if(m_pt_per_row_changed || m_disp8bit_img.height() != m_display_buf_img.disp_area_height)
    {
        m_disp8bit_img = QImage(disp_pt_per_row, m_display_buf_img.disp_area_height,
                                   QImage::Format_Grayscale8);
    }
    m_disp8bit_img.fill(this->palette().color(QPalette::Window));

    m_display_buf_img.buf_line_cnt = 0;
    m_display_buf_img.curr_work_img_ind = 0;
    m_display_buf_img.disp_line_start_idx = 0;
    m_display_buf_img.total_line_cnt = 0;
}

void ScanWidget::adjust_bg_data_size(QVector<gray_pixel_data_type> &tgt,
                         const QVector<gray_pixel_data_type>& data, int tgt_size)
{
    if(data.size() >= tgt_size)
    {
        tgt = data.mid(0, tgt_size);
    }
    else
    {
        tgt = data;
        tgt.resize(tgt_size);
        std::fill(tgt.begin() + data.size(), tgt.end(), g_sys_configs_block.def_scan_bg_value);
    }
}
void ScanWidget::adjust_stre_factor_data_size(QVector<double> &tgt, const QVector<double>& data,
                                              int tgt_size)
{
    if(data.size() >= tgt_size)
    {
        tgt = data.mid(0, tgt_size);
    }
    else
    {
        tgt = data;
        tgt.resize(tgt_size);
        std::fill(tgt.begin() + data.size(), tgt.end(), g_sys_configs_block.def_scan_stre_factor_value);
    }
}

void ScanWidget::reset_cali_ctrl_vars()
{
    m_cali_bg_data_now = m_cali_stre_factor_data_now = false;
    m_cali_bg_line_idx = m_cali_stre_factor_line_idx = 0;
}

void ScanWidget::gen_cali_datum_to_file(src_of_collect_cmd_e_t cali_type)
{
    if(COLLECT_CMD_CALI_BG != cali_type && COLLECT_CMD_CALI_STRE_FACTOR != cali_type)
    {
        DIY_LOG(LOG_WARN, QString("wrong calibration type %1. should be %2 or %3")
                  .arg(cali_type).arg(COLLECT_CMD_CALI_BG).arg(COLLECT_CMD_CALI_STRE_FACTOR));
        return;
    }

    DIY_LOG(LOG_INFO, QString("now generate cali file of type: %1").arg((int)cali_type));

    int line_num = m_gray_img_lines.lines.size();
    if(0 == line_num)
    {
        DIY_LOG(LOG_ERROR, "error in cali-data-generation: line num is 0!");
        return;
    }

    for(int row = 0; row < line_num; ++row)
    {
        if(m_gray_img_lines.lines[row].size() != m_gray_img_lines.line_len)
        {
            DIY_LOG(LOG_ERROR, QString("Data Format error: the %1 line length is %2, "
                                       "but it should be %3.").arg(row)
                                    .arg(m_gray_img_lines.lines[row].size())
                                    .arg(m_gray_img_lines.line_len));
            return;
        }
    }

    DIY_LOG(LOG_INFO, "the collected cali value:");
    QVector<QString> val_strings(line_num, "");
    QString avg_str;
    double pt_v_dbl;
    QVector<double> temp_v(m_gray_img_lines.line_len, 0);
    for(int col = 0; col < m_gray_img_lines.line_len; ++col)
    {
        for(int row = 0; row < line_num; ++row)
        {
            pt_v_dbl = m_gray_img_lines.lines[row][col];
            temp_v[col] += pt_v_dbl;
            val_strings[row] += QString::number(pt_v_dbl) + "\t";
        }
        temp_v[col] = temp_v[col] / line_num;
        avg_str += QString::number(temp_v[col]) + "\t";
    }

    for(int row = 0; row < line_num; ++row)
    {
        DIY_LOG(LOG_INFO, val_strings[row]);
    }
    DIY_LOG(LOG_INFO, QString("the averaged cal value:\n%1").arg(avg_str));

    QString cali_file_fn, cali_txt_file_fn ;
    if(COLLECT_CMD_CALI_BG == cali_type)
    {
        m_scan_bg_value_loaded.resize(m_gray_img_lines.line_len);
        for(int col = 0; col < m_gray_img_lines.line_len; ++col)
        {
            m_scan_bg_value_loaded[col] = (gray_pixel_data_type)(temp_v[col]);
        }

        cali_file_fn = QString(gs_scan_cali_file_path) + "/" + gs_scan_bg_value_fn;
        cali_txt_file_fn = QString(gs_scan_cali_file_path) + "/" + gs_scan_bg_value_txt_fn;
    }
    else// (COLLECT_CMD_CALI_STRE_FACTOR == caliy_type)
    {
        m_scan_stre_factor_value_loaded.resize(m_gray_img_lines.line_len);
        for(int col = 0; col < m_gray_img_lines.line_len; ++col)
        {
            m_scan_stre_factor_value_loaded[col] = (temp_v[col] ? (g_12bitpx_max_v / temp_v[col]) : g_12bitpx_max_v);
        }
        cali_file_fn = QString(gs_scan_cali_file_path) + "/" + gs_scan_stre_factor_value_fn;
        cali_txt_file_fn = QString(gs_scan_cali_file_path) + "/" + gs_scan_stre_factor_value_txt_fn;
    }

    QFile bg_file(cali_file_fn);
    if(bg_file.open(QIODevice::WriteOnly))
    {
        QDataStream bg_out(&bg_file);
        bg_out.setVersion(QDataStream::Qt_5_15);
        if(COLLECT_CMD_CALI_BG == cali_type)
        {
            bg_out << m_scan_bg_value_loaded;
        }
        else
        {
            bg_out << m_scan_stre_factor_value_loaded;
        }
        bg_file.close();

        DIY_LOG(LOG_INFO, QString("calibration data file %1 is generated.").arg(cali_file_fn));
    }
    else
    {
        DIY_LOG(LOG_ERROR, QString("generate calibration file error: open file %1 error. ")
                               .arg(cali_file_fn));
    }

    if(COLLECT_CMD_CALI_BG == cali_type)
    {
        m_scan_bg_value_for_work = m_scan_bg_value_loaded;
    }
    else
    {
        m_scan_stre_factor_value_for_work = m_scan_stre_factor_value_loaded;
    }
    QString cal_data_txt_str = get_cali_data_str(cali_type);
    QFile txt_file(cali_txt_file_fn);
    if(txt_file.open(QIODevice::WriteOnly))
    {
        QTextStream txt_out(&txt_file);
        txt_out << cal_data_txt_str;

        txt_file.close();
    }
    else
    {
        DIY_LOG(LOG_ERROR, QString("write calibration txt file error: open file %1 error. ")
                               .arg(cali_txt_file_fn));
    }
}

void ScanWidget::load_cali_datum_from_file()
{
    static bool ls_init_load = true;

    if(ls_init_load)
    {
        int disp_pt_per_row = ui->ptPerRowSpinBox->value();

        QFile bg_file(QString(gs_scan_cali_file_path) + "/" + gs_scan_bg_value_fn);
        if(!bg_file.open(QIODevice::ReadOnly))
        {
            m_scan_bg_value_loaded.resize(disp_pt_per_row);
            m_scan_bg_value_loaded.fill(g_sys_configs_block.def_scan_bg_value);
        }
        else
        {
            QDataStream bg_in(&bg_file);
            bg_in.setVersion(QDataStream::Qt_5_15);

            QVector<gray_pixel_data_type> tmp_bg;
            bg_in >> tmp_bg;
            bg_file.close();

            adjust_bg_data_size(m_scan_bg_value_loaded, tmp_bg, disp_pt_per_row);
        }

        QFile stre_factor_file(QString(gs_scan_cali_file_path) + "/" + gs_scan_stre_factor_value_fn);
        if(!stre_factor_file.open(QIODevice::ReadOnly))
        {
            m_scan_stre_factor_value_loaded.resize(disp_pt_per_row);
            m_scan_stre_factor_value_loaded.fill(g_sys_configs_block.def_scan_stre_factor_value);
        }
        else
        {
            QDataStream stre_in(&stre_factor_file);
            stre_in.setVersion(QDataStream::Qt_5_15);

            QVector<double> tmp_stre;
            stre_in >> tmp_stre;
            stre_factor_file.close();

            adjust_stre_factor_data_size(m_scan_stre_factor_value_loaded, tmp_stre,
                                         disp_pt_per_row);
        }

        ls_init_load = false;
    }
}

void ScanWidget::reload_cali_datum()
{
    if(!m_pt_per_row_changed) return;

    int disp_pt_per_row = ui->ptPerRowSpinBox->value();

    //asuming m_scan_bg_value_for_work and m_scan_stre_factor_value_for_work
    //are always the same size.
    if(disp_pt_per_row < m_scan_bg_value_for_work.size())
    {
        m_scan_bg_value_for_work.resize(disp_pt_per_row);
        m_scan_stre_factor_value_for_work.resize(disp_pt_per_row);
    }
    else
    {
        adjust_bg_data_size(m_scan_bg_value_for_work, m_scan_bg_value_loaded, disp_pt_per_row);
        adjust_stre_factor_data_size(m_scan_stre_factor_value_for_work,
                                     m_scan_stre_factor_value_loaded, disp_pt_per_row);
    }
    print_cali_data_to_log();
}

void ScanWidget::print_cali_data_to_log()
{
    QString cal_str;

    cal_str = get_cali_data_str(COLLECT_CMD_CALI_BG);
    DIY_LOG(LOG_INFO, QString("the background calibration data: %1").arg(cal_str));

    cal_str = get_cali_data_str(COLLECT_CMD_CALI_STRE_FACTOR);
    DIY_LOG(LOG_INFO, QString("the stretch-factor calibration data: %1").arg(cal_str));
}

QString ScanWidget::get_cali_data_str(src_of_collect_cmd_e_t cali_type)
{
    QString cal_str;

    if(COLLECT_CMD_CALI_BG == cali_type)
    {
        for(int col = 0; col < m_scan_bg_value_for_work.size(); ++col)
        {
            gray_pixel_data_type pv;
            pv = m_scan_bg_value_for_work[col];
            cal_str += QString::number(pv) + " ";
        }
    }
    else
    {
        for(int col = 0; col < m_scan_stre_factor_value_for_work.size(); ++col)
        {
            double pv;
            pv = m_scan_stre_factor_value_for_work[col];
            cal_str += QString::number(pv) + " ";
        }
    }
    return cal_str;
}

void ScanWidget::start_scan(src_of_collect_cmd_e_t cmd_src)
{
    DIY_LOG(LOG_INFO, QString("start scan, cmd: %1").arg(cmd_src));

    if(m_scan_cmd_proc)
    {
        DIY_LOG(LOG_WARN, "last start scan is still in proc. discard this cmd.");
        return;
    }
    m_scan_cmd_proc = true;

    m_curr_collect_cmd = cmd_src;

    if(COLLECT_CMD_CALI_BG == cmd_src)
    {
        m_cali_bg_data_now = true;
        m_cali_bg_line_idx = 0;
    }
    else if(COLLECT_CMD_CALI_STRE_FACTOR == cmd_src)
    {
        m_cali_stre_factor_data_now = true;
        m_cali_stre_factor_line_idx = 0;
    }

    if(g_sys_configs_block.scan_without_x)
    {
        start_collect(cmd_src);
    }
    else
    {
        if(ui->scanLockChkBox->isChecked())
        {
            DIY_LOG(LOG_WARN, "scan is locked, ignore the start scan cmd");
            ui->grayImgLbl->setText(g_str_plz_unlock_first);
            m_scan_cmd_proc = false;
            m_curr_collect_cmd = COLLECT_CMD_NONE;
            reset_cali_ctrl_vars();
            btns_refresh();
            return;
        }

        hv_send_op_cmd(HV_OP_START_EXPO);
        m_expo_to_coll_delay_timer.start(g_sys_settings_blk.expo_to_coll_delay_ms);
    }

    m_curr_expo_dura_s = (int)(g_sys_settings_blk.hv_params.expo_dura_ms / 1000);
    m_expo_dura_timer.start((int)(g_sys_settings_blk.hv_params.expo_dura_ms));
}

void ScanWidget::stop_scan(src_of_collect_cmd_e_t cmd_src)
{
    DIY_LOG(LOG_INFO, QString("stop scan, cmd: %1").arg(cmd_src));
    if(!m_scan_cmd_proc)
    {
        DIY_LOG(LOG_WARN, "not in scan proc. discard this cmd.");
        return;
    }

    if(!g_sys_configs_block.scan_without_x)
    {
        hv_send_op_cmd(HV_OP_STOP_EXPO);
        m_expo_to_coll_delay_timer.stop();
    }
    stop_collect(cmd_src);

    if((m_cali_bg_data_now && (COLLECT_CMD_CALI_BG != cmd_src))
        || (m_cali_stre_factor_data_now && (COLLECT_CMD_CALI_STRE_FACTOR != cmd_src)))
    {
        gen_cali_datum_to_file(m_curr_collect_cmd);
    }

    reset_cali_ctrl_vars();
    m_curr_collect_cmd = COLLECT_CMD_NONE;

    m_scan_cmd_proc = false;
    btns_refresh();

    if(m_expo_dura_timer.isActive())
    {
        int actual_run_time = m_expo_dura_timer.interval() - m_expo_dura_timer.remainingTime();
        m_expo_dura_timer.stop();

        emit fpga_pwr_on_off_sig(false, (quint16)(actual_run_time / 1000));
    }
}

void ScanWidget::detector_self_check()
{
    emit detector_handshake_sig();
    m_detector_self_chk = true;
}

void ScanWidget::start_collect(src_of_collect_cmd_e_t cmd_src)
{
    DIY_LOG(LOG_INFO, QString("start_collect cmd: %1").arg(cmd_src));

    ui->grayImgLbl->clear();
    ui->grayImgLbl->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    ui->ptPerRowSpinBox->setDisabled(true);

    clear_recv_data_queue();

    proc_pt_per_row_cnt_related_work();

    clear_gray_img_lines();

    QString curr_date_str = common_tool_get_curr_date_str();
    QString curr_path = g_sys_settings_blk.img_save_path + "/" + curr_date_str;
    setup_sc_data_rec_file(curr_path, curr_date_str);

    emit start_collect_sc_data_sig();
    g_data_scanning_now = true;

    if(g_sys_configs_block.scan_without_x)
    {
        m_scan_dura_timer.start(g_sys_settings_blk.max_scan_dura_sec * 1000);
    }
    else
    {
        m_scan_dura_timer.start((int)g_sys_settings_blk.hv_params.expo_dura_ms);
    }

    btns_refresh();
}

void ScanWidget::stop_collect(src_of_collect_cmd_e_t cmd_src)
{
    DIY_LOG(LOG_INFO, QString("stop_collect cmd: %1").arg(cmd_src));
    m_scan_dura_timer.stop();

    if(!g_data_scanning_now) return;

    emit stop_collect_sc_data_sig();
}

void ScanWidget::setup_sc_data_rec_file(QString &curr_path, QString &curr_date_str)
{
    QString err_str, curr_file_path;

    if(!chk_mk_pth_and_warn(curr_path))
    {
        return;
    }

    m_curr_sc_txt_file_name = curr_date_str + gs_str_sc_data_txt_file_type;

    curr_file_path = curr_path + "/" + m_curr_sc_txt_file_name;
    m_curr_sc_txt_file.setFileName(curr_file_path);
    if(!m_curr_sc_txt_file.isOpen())
    {
        if(!m_curr_sc_txt_file.open(QFile::WriteOnly | QFile::Append))
        {
            err_str = QString("%1%2:%3").arg(g_str_create_file, g_str_fail, curr_file_path);
            DIY_LOG(LOG_ERROR, err_str);
            QMessageBox::critical(this, "Error", err_str);
            return;
        }
    }
    m_curr_sc_txt_stream.setDevice(&m_curr_sc_txt_file);
}

void ScanWidget::close_sc_data_file_rec()
{
    if(m_curr_sc_txt_file.isOpen())
    {
        m_curr_sc_txt_stream.flush();
        m_curr_sc_txt_file.close();
    }
}

QString ScanWidget::log_disp_prepender_str()
{
    return (common_tool_get_curr_date_str() + "," + common_tool_get_curr_time_str() + ",");
}

void ScanWidget::clear_recv_data_queue()
{
    {
        QMutexLocker locker(&queueMutex);
        dataQueue.clear();
    }
}

void ScanWidget::handleNewDataReady()
{
    recv_data_with_notes_s_t packet;
    {
        QMutexLocker locker(&queueMutex);
        if(dataQueue.isEmpty()) return;
        packet = dataQueue.dequeue();
    }
    QString data_str = log_disp_prepender_str();

    data_str += QString(gs_recv_data_note_str[packet.notes]) + ":";
    data_str += packet.data.toHex(' ').toUpper();

    data_str += "\n";
    if(m_curr_sc_txt_file.isOpen()) {m_curr_sc_txt_stream << data_str;}

    if(m_detector_self_chk && (HANDSHAKE_ACK == packet.notes))
    {
        emit detector_self_chk_ret_sig(true);
        m_detector_self_chk = false;
        return;
    }

    if(!g_data_scanning_now) return;
    if(NORMAL == packet.notes)
    {
        quint64 pkt_idx;
        m_curr_line_pt_cnt
                = split_data_into_channels(packet.data, m_ch1_data_vec, m_ch2_data_vec, pkt_idx);

        record_gray_img_line();

        if(m_cali_bg_data_now) ++m_cali_bg_line_idx;
        if(m_cali_stre_factor_data_now) ++m_cali_stre_factor_line_idx;
        if((m_cali_bg_data_now && (m_cali_bg_line_idx >= g_sys_settings_blk.cali_bg_line_cnt))
        || (m_cali_stre_factor_data_now &&
                (m_cali_stre_factor_line_idx >= g_sys_settings_blk.cali_stre_factor_line_cnt)))
        {
            src_of_collect_cmd_e_t cmd = m_curr_collect_cmd;
            stop_scan(m_curr_collect_cmd);
            emit gen_cali_datum_to_file_sig(cmd);
            return;
        }
    }
}

int ScanWidget::split_data_into_channels(QByteArray& ori_data,
                              QVector<gray_pixel_data_type> &dv_ch1, QVector<gray_pixel_data_type> &dv_ch2,
                              quint64 &pkt_idx)
{
    /*two channels, all_bytes_cnt_per_pt * 4 bits per point for each channel.*/
    static const int all_bytes_cnt_per_pt = g_sys_configs_block.all_bytes_per_pt,
            suffix_bytes_cnt = g_sys_configs_block.pkt_idx_byte_cnt;
    static const int all_hbs_cnt_per_pt = all_bytes_cnt_per_pt * 2;
    static const bool odd_hb_per_chpt = ((all_bytes_cnt_per_pt % 2) == 1);

    int disp_pt_per_row = ui->ptPerRowSpinBox->value();
    if(disp_pt_per_row <= 0) return 0;

    int disp_bytes_per_row = disp_pt_per_row * all_bytes_cnt_per_pt;
    int ori_bytes_cnt = ori_data.count();

    if(ori_bytes_cnt < suffix_bytes_cnt)
    {
        DIY_LOG(LOG_ERROR, QString("received bytes too few: %1").arg(ori_bytes_cnt));
        return 0;
    }
    pkt_idx = 0;
    for(int idx = ori_bytes_cnt - suffix_bytes_cnt; idx < ori_bytes_cnt; ++idx)
    {
        pkt_idx = (pkt_idx << 8) + (quint64)(ori_data.at(idx));
    }

    int ori_data_bytes_cnt = (ori_bytes_cnt - suffix_bytes_cnt);

    if(dv_ch1.size() < disp_pt_per_row) dv_ch1.resize(disp_pt_per_row);
    if(dv_ch2.size() < disp_pt_per_row) dv_ch2.resize(disp_pt_per_row);
    dv_ch1.fill(0); dv_ch2.fill(0);

    unsigned char byte;
    QVector<gray_pixel_data_type> * curr_dv = nullptr;
    for(int idx = 0, hb_idx = 0, hb_idx_in_pt = 0, pt_idx = 0;
        idx < disp_bytes_per_row; ++idx)
    {
        byte = idx < ori_data_bytes_cnt ? ori_data[idx] : 0xFF;
        pt_idx = idx / all_bytes_cnt_per_pt;

        hb_idx = idx * 2;
        hb_idx_in_pt = hb_idx % all_hbs_cnt_per_pt;

        curr_dv = (hb_idx_in_pt < all_bytes_cnt_per_pt) ? &dv_ch1 : &dv_ch2;

        if(odd_hb_per_chpt)
        {
            if(hb_idx_in_pt < all_bytes_cnt_per_pt - 1 ||
                    hb_idx_in_pt > all_bytes_cnt_per_pt) //whole byte
            {
                (*curr_dv)[pt_idx] <<= 8;
                (*curr_dv)[pt_idx] += byte;
            }
            else //half byte
            {
                dv_ch1[pt_idx] <<= 4;
                dv_ch1[pt_idx] += (byte >> 4);

                dv_ch2[pt_idx] <<= 4;
                dv_ch2[pt_idx] += (byte & 0x0F);
            }
        }
        else
        {
            (*curr_dv)[pt_idx] <<= 8;
            (*curr_dv)[pt_idx] += byte;
        }
    }

    return disp_pt_per_row;
}

void ScanWidget::recv_worker_report_sig_hdlr(LOG_LEVEL lvl, QString report_str,
                                         collect_rpt_evt_e_t evt )
{
    lvl = VALID_LOG_LVL(lvl) ? lvl : LOG_ERROR;

    QString disp_str = log_disp_prepender_str() + QString(g_log_level_strs[lvl]) + ",";

    disp_str += report_str;
    disp_str += "\n";
    if(m_curr_sc_txt_file.isOpen()) {m_curr_sc_txt_stream << disp_str;}

    switch(evt)
    {
    case COLLECT_RPT_EVT_CONNECTED:
        ui->scanStLbl->setText(g_str_connected);
        break;

    case COLLECT_RPT_EVT_DISCONNECTED:
        ui->scanStLbl->setText(g_str_disconnected);

        close_sc_data_file_rec();
        break;

    case COLLECT_RPT_EVT_HANDSHAKE_TIMEOUT:
        if(m_detector_self_chk)
        {
            emit detector_self_chk_ret_sig(false);
            m_detector_self_chk = false;
        }
        break;

    default:
        break;
    }
}

void ScanWidget::expo_to_coll_delay_timer_hdlr()
{
    start_collect(m_curr_collect_cmd);
}

void ScanWidget::scan_dura_timeout_hdlr()
{
    stop_scan(m_curr_collect_cmd);
}

void ScanWidget::clear_gray_img_lines()
{
    for (auto &line: m_gray_img_lines.lines) line.clear();

    m_gray_img_lines.lines.clear();
    m_gray_img_lines.refreshed = false;
    m_gray_img_lines.line_len = 0;
    m_gray_img_lines.max_v = m_gray_img_lines.min_v = 0;
}

void ScanWidget::record_gray_img_line()
{
    QVector<gray_pixel_data_type> line;
    quint32 px_sum;
    line.resize(m_curr_line_pt_cnt);
    for(int idx = 0; idx < m_curr_line_pt_cnt; ++idx)
    {
        px_sum = (quint32)(m_ch1_data_vec[idx]) + (quint32)(m_ch2_data_vec[idx]);
        line[idx] = px_sum/2;
    }
    if(!m_cali_bg_data_now && !m_cali_stre_factor_data_now)
    {
        calibrate_px_line(line);
    }

    m_gray_img_lines.lines.append(line);
    m_gray_img_lines.line_len = m_curr_line_pt_cnt;
    m_gray_img_lines.refreshed = true;

    auto mmpair = std::minmax_element(line.begin(), line.end());
    if(*mmpair.first < m_gray_img_lines.min_v) m_gray_img_lines.min_v = *mmpair.first;
    if(*mmpair.second > m_gray_img_lines.max_v) m_gray_img_lines.max_v = *mmpair.second;

    add_line_to_display(line, *mmpair.first, *mmpair.second);
}

void ScanWidget::calibrate_px_line(QVector<gray_pixel_data_type> &line)
{
    std::transform(line.begin(), line.end(), m_scan_bg_value_for_work.begin(), line.begin(),
               [](gray_pixel_data_type x, gray_pixel_data_type y)
                    { return (x > y) ? (x - y) : 0; });

    std::transform(line.begin(), line.end(),
               m_scan_stre_factor_value_for_work.begin(),
               line.begin(),
               [](gray_pixel_data_type val, double factor) -> gray_pixel_data_type
               {
                   double result = val * factor;
                   if (result < 0.0) return 0;
                   if (result > g_12bitpx_max_v) return g_12bitpx_max_v;
                   return static_cast<gray_pixel_data_type>(result);
               });
}

void ScanWidget::add_line_to_display(QVector<gray_pixel_data_type> &line, gray_pixel_data_type min_v, gray_pixel_data_type max_v)
{
    int img_buffer_height = m_display_buf_img.img_buffer.height();
    int bytesPerLine = m_display_buf_img.img_buffer.bytesPerLine();  // 注意：不是 width * 2，包含对齐
    int target_line_idx;
    gray_pixel_data_type work_area_min_v, work_area_max_v;

    if (line.count() > m_display_buf_img.img_buffer.width())
    {
        DIY_LOG(LOG_ERROR, "this line is too long");
        return;
    }

    Q_ASSERT(m_display_buf_img.disp_area_height <= img_buffer_height);

    QImage * curr_buf, * alt_buf;
    QVector<gray_pixel_data_type> *curr_minv, *curr_maxv, *alt_minv, *alt_maxv;
    if(0 == m_display_buf_img.curr_work_img_ind)
    {
        curr_buf = &m_display_buf_img.img_buffer;
        alt_buf = &m_display_buf_img.img_buffer2;

        curr_minv = &m_display_buf_img.img_line_min_vs;
        curr_maxv = &m_display_buf_img.img_line_max_vs;
        alt_minv = &m_display_buf_img.img_line_min_vs2;
        alt_maxv = &m_display_buf_img.img_line_max_vs2;
    }
    else
    {
        curr_buf = &m_display_buf_img.img_buffer2;
        alt_buf = &m_display_buf_img.img_buffer;

        curr_minv = &m_display_buf_img.img_line_min_vs2;
        curr_maxv = &m_display_buf_img.img_line_max_vs2;
        alt_minv = &m_display_buf_img.img_line_min_vs;
        alt_maxv = &m_display_buf_img.img_line_max_vs;
    }

    QImage disp_img;
    int disp_img_height;
    if(m_display_buf_img.buf_line_cnt < img_buffer_height)
    {//just copy the line into buffer.
        uchar *data = curr_buf->bits();
        target_line_idx = m_display_buf_img.buf_line_cnt;
        memcpy(data + bytesPerLine * target_line_idx, line.constData(),
                   line.count()*sizeof(gray_pixel_data_type));

        (*curr_minv)[target_line_idx] = min_v;
        (*curr_maxv)[target_line_idx] = max_v;

        if(m_display_buf_img.buf_line_cnt < m_display_buf_img.disp_area_height)
        {
            disp_img_height = m_display_buf_img.buf_line_cnt + 1;
        }
        else
        {
            ++m_display_buf_img.disp_line_start_idx;
            disp_img_height = m_display_buf_img.disp_area_height;
        }
        disp_img = QImage(curr_buf->bits() + m_display_buf_img.disp_line_start_idx * bytesPerLine,
                          curr_buf->width(), disp_img_height,
                          bytesPerLine, QImage::Format_Grayscale16);
        work_area_min_v = *(std::min_element(curr_minv->begin() + m_display_buf_img.disp_line_start_idx,
                                             curr_minv->begin() + disp_img_height));
        work_area_max_v = *(std::max_element(curr_maxv->begin() + m_display_buf_img.disp_line_start_idx,
                                             curr_maxv->begin() + disp_img_height));

        ++m_display_buf_img.buf_line_cnt;
    }
    else
    {
        //switch buffer
        memcpy(alt_buf->bits(),
               curr_buf->constBits() + bytesPerLine * (m_display_buf_img.disp_line_start_idx + 1),
               bytesPerLine * (m_display_buf_img.disp_area_height - 1));
        target_line_idx = m_display_buf_img.disp_area_height - 1;
        memcpy(alt_buf->bits() + bytesPerLine * target_line_idx,
               line.constData(), line.count()*sizeof(gray_pixel_data_type));

        disp_img = QImage(alt_buf->bits(),
                          alt_buf->width(), m_display_buf_img.disp_area_height,
                          bytesPerLine, QImage::Format_Grayscale16);

        std::copy(curr_minv->begin() + m_display_buf_img.disp_line_start_idx + 1,
                  curr_minv->end(), alt_minv->begin());
        std::copy(curr_maxv->begin() + m_display_buf_img.disp_line_start_idx + 1,
                  curr_maxv->end(), alt_maxv->begin());
        (*alt_minv)[target_line_idx] = min_v;
        (*alt_maxv)[target_line_idx] = max_v;

        work_area_min_v = *(std::min_element(alt_minv->begin(),
                                             alt_minv->begin() + m_display_buf_img.disp_area_height));
        work_area_max_v = *(std::max_element(alt_maxv->begin(),
                                             alt_maxv->begin() + m_display_buf_img.disp_area_height));


        m_display_buf_img.buf_line_cnt = m_display_buf_img.disp_area_height;
        m_display_buf_img.disp_line_start_idx = 0;
        if(0 == m_display_buf_img.curr_work_img_ind)
        {
            m_display_buf_img.curr_work_img_ind = 1;
        }
        else
        {
            m_display_buf_img.curr_work_img_ind = 0;
        }
    }
    ++m_display_buf_img.total_line_cnt;

    pixel_mmpairt_s_t mmpair;
    mmpair.min_v = work_area_min_v; mmpair.max_v = work_area_max_v;

    if((m_display_buf_img.total_line_cnt <= g_sys_settings_blk.ini_disp_img_line_cnt)
        || ((m_display_buf_img.total_line_cnt - g_sys_settings_blk.ini_disp_img_line_cnt)
            % g_sys_settings_blk.merg_disp_img_line_cnt == 0))
    {
        QImage disp8bit_img = convertGrayscale16To8(disp_img, &mmpair);
        memcpy(m_disp8bit_img.bits(), disp8bit_img.bits(), disp8bit_img.bytesPerLine() * disp8bit_img.height());
        QPixmap scaled = QPixmap::fromImage(m_disp8bit_img)
                    .scaled(ui->grayImgLbl->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        ui->grayImgLbl->setPixmap(scaled);
    }
}

static inline void pt_data_to_image(QVector<QVector<gray_pixel_data_type>> &data,
                                    QImage &img, int width, int height)
{
    for (int y = 0; y < height; ++y)
    {
        const QVector<gray_pixel_data_type>& row = data[y];
        uchar* line = img.scanLine(y);
        for (int x = 0; x < width; ++x)
        {
            quint16 gray = static_cast<quint16>(row[x]);
            // 写入16位灰度（低字节在前）
            line[2 * x]     = gray & 0xFF;         // LSB
            line[2 * x + 1] = (gray >> 8) & 0xFF;  // MSB
        }
    }
}

void ScanWidget::generate_gray_img(gray_img_disp_type_e_t disp_type)
{
    static gray_pixel_data_type append_val = (1 << g_sys_configs_block.all_bytes_per_pt * 4) - 1;
    static bool real_img_gened = false, layfull_img_gened = false;

    if(m_gray_img_lines.refreshed)
    {
        real_img_gened = layfull_img_gened = false;
    }

    if(DISPLAY_IMG_REAL == disp_type)
    {
        if(!real_img_gened && m_gray_img_lines.lines.size() > 0)
        {
            gray_lines_s_t img_lines = m_gray_img_lines;
            for (auto &line: img_lines.lines)
            {
                int extra_cnt;
                extra_cnt = img_lines.line_len - line.size();
                if(extra_cnt > 0)
                {
                    int pos = line.size();
                    line.insert(pos, extra_cnt, append_val);
                }
            }
            int width = img_lines.line_len,
                height = img_lines.lines.size();
            QImage img(width, height, QImage::Format_Grayscale16);
            pt_data_to_image(img_lines.lines, img, width, height);
            m_local_img = img;

            real_img_gened = true;
        }
    }
    else
    {//DISPLAY_IMG_LAYFULL
        if(!layfull_img_gened && m_gray_img_lines.lines.size())
        {
            int width = g_sys_configs_block.scrn_w,
                height = g_sys_configs_block.scrn_h;
            QImage img(width, height, QImage::Format_Grayscale16);

            gray_lines_s_t img_lines = m_gray_img_lines;
            //fill each line with 0xFF so that they are all of lenth width.
            for (auto &line: img_lines.lines)
            {
                int pre_cnt, post_cnt;
                pre_cnt = (width - line.size())/2;
                post_cnt = width - line.size() - pre_cnt;
                if(pre_cnt <= 0)
                {
                    if(pre_cnt < 0) line.remove(width, line.size() - width);
                    continue;
                }
                line.insert(0, pre_cnt, append_val);
                if(post_cnt > 0) line.insert(line.size(), post_cnt, append_val);
            }

            //strench vertically.
            int total_extra_line_cnt = height - img_lines.lines.size();
            if(total_extra_line_cnt > 0)
            {
                int extra_line_cnt_per_line = total_extra_line_cnt / img_lines.lines.size();
                QVector<int> added_cnt_v(img_lines.lines.size(), extra_line_cnt_per_line);
                for(int idx = 0; idx < total_extra_line_cnt - extra_line_cnt_per_line * img_lines.lines.size();
                    ++idx)
                {
                    added_cnt_v[idx] += 1;
                }
                for(int cnt_idx = 0, ori_idx = 0, added_cnt = 0; added_cnt < total_extra_line_cnt; ++cnt_idx)
                {
                    QVector<gray_pixel_data_type> &ori_line = img_lines.lines[ori_idx];
                    int added_cnt_this_line = added_cnt_v[cnt_idx];
                    img_lines.lines.insert(ori_idx + 1, added_cnt_this_line, ori_line);

                    added_cnt += added_cnt_this_line;
                    ori_idx += added_cnt_this_line + 1;
                }
            }
            else if(total_extra_line_cnt < 0)
            {
                img_lines.lines.remove(height, img_lines.lines.size() - height);
            }

            pt_data_to_image(img_lines.lines, img, width, height);

            m_local_layfull_img = img;

            layfull_img_gened = true;
        }
    }
    m_gray_img_lines.refreshed = false;
}

void ScanWidget::display_gray_img(gray_img_disp_type_e_t disp_type)
{
    QPixmap scaled;

    ui->grayImgLbl->clear();
    ui->grayImgLbl->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);

    if(DISPLAY_IMG_REAL == disp_type)
    {
        if(m_local_img.isNull())
        {
            DIY_LOG(LOG_WARN, "local_img is NULL");
            return;
        }

        m_local_img_8bit = convertGrayscale16To8(m_local_img);
        scaled = QPixmap::fromImage(m_local_img_8bit)
                    .scaled(ui->grayImgLbl->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    else
    {
        if(m_local_layfull_img.isNull())
        {
            DIY_LOG(LOG_WARN, "local_layfull_img is NULL");
            return;
        }

        m_local_layfull_img_8bit = convertGrayscale16To8(m_local_layfull_img);
        scaled = QPixmap::fromImage(m_local_layfull_img_8bit)
                    .scaled(ui->grayImgLbl->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    ui->grayImgLbl->setPixmap(scaled);
}

void ScanWidget::save_local_imgs(gray_img_disp_type_e_t disp_type)
{
    QString curr_date_str = common_tool_get_curr_date_str();
    QString curr_path = g_sys_settings_blk.img_save_path + "/" + curr_date_str;

    if(!chk_mk_pth_and_warn(curr_path))
    {
        return;
    }
    QString curr_dt_str = common_tool_get_curr_dt_str();
    QImage * op_img_ptr = (DISPLAY_IMG_REAL == disp_type) ? &m_local_img : &m_local_layfull_img;
    QImage * op_img_8bit_ptr = (DISPLAY_IMG_REAL == disp_type) ? &m_local_img_8bit : &m_local_layfull_img_8bit;

    op_img_ptr->save(curr_path + "/" + curr_dt_str + gs_str_img_png_type);
    op_img_8bit_ptr->save(curr_path + "/" + curr_dt_str + gs_str_8bit_img_apx + gs_str_img_png_type);

    QFile raw_data_file(curr_path + "/" + curr_dt_str + gs_str_img_raw_type);
    if(raw_data_file.open(QIODevice::WriteOnly))
    {
        raw_data_file.write(reinterpret_cast<const char *>(op_img_ptr->constBits()),
                            op_img_ptr->sizeInBytes());
        raw_data_file.close();
    }
}

void ScanWidget::on_dataCollStartPbt_clicked()
{
    start_scan();
}

void ScanWidget::recv_data_finished_sig_hdlr()
{
    g_data_scanning_now = false;

    close_sc_data_file_rec();

    if(m_gray_img_lines.refreshed)
    {
        generate_gray_img(DISPLAY_IMG_REAL);
        display_gray_img(DISPLAY_IMG_REAL);
        save_local_imgs(DISPLAY_IMG_REAL);
    }
    btns_refresh();

    ui->ptPerRowSpinBox->setEnabled(true);
}

void ScanWidget::on_dataCollStopPbt_clicked()
{
    stop_scan();
}

bool ScanWidget::chk_mk_pth_and_warn(QString &pth_str)
{
    if(!mkpth_if_not_exists(pth_str))
    {
        QString err_str = QString("%1%2:%3").arg(g_str_create_folder, g_str_fail, pth_str);
        DIY_LOG(LOG_ERROR, err_str);
        QMessageBox::critical(this, "Error", err_str);
        return false;
    }
    return true;
}

void ScanWidget::btns_refresh()
{
    bool can_scan = (!g_data_scanning_now &&
                     (g_sys_configs_block.scan_without_x || !ui->scanLockChkBox->isChecked()));
    ui->dataCollStartPbt->setEnabled(can_scan);
    ui->dataCollStopPbt->setEnabled(g_data_scanning_now);
    ui->scanLockChkBox->setEnabled(!g_data_scanning_now && !g_sys_configs_block.scan_without_x);

    ui->caliBgBtn->setEnabled(can_scan);
    ui->caliStreFactorBtn->setEnabled(can_scan);
    ui->caliBgBtn->setVisible(g_sys_settings_blk.cali_mode_now);
    ui->caliStreFactorBtn->setVisible(g_sys_settings_blk.cali_mode_now);
}

void ScanWidget::rec_ui_settings()
{
    if(m_cfg_recorder)
    {
        m_cfg_recorder->record_ui_configs(this, m_rec_ui_cfg_fin, m_rec_ui_cfg_fout);
    }
}

void ScanWidget::load_ui_settings()
{
    if(m_cfg_recorder)
    {
        m_cfg_recorder->load_configs_to_ui(this, m_rec_ui_cfg_fin, m_rec_ui_cfg_fout);
    }
}

void ScanWidget::on_ptPerRowSpinBox_valueChanged(int /*arg1*/)
{
    m_pt_per_row_changed = true;
}


void ScanWidget::on_scanLockChkBox_stateChanged(int checked)
{
    emit fpga_pwr_on_off_sig((Qt::Checked == checked) ? false : true);

    if(Qt::Checked != checked)
    {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "", g_str_conf_if_power_on_fpga,
                    QMessageBox::Yes | QMessageBox::No);
        if(QMessageBox::No == reply)
        {
            emit fpga_pwr_on_off_sig(false);
        }
    }

    btns_refresh();
}

void ScanWidget::scan_widget_disp_sig_hdlr()
{
    m_display_buf_img.disp_area_height = ui->grayImgLbl->height();
    if(m_display_buf_img.disp_area_height > g_sys_configs_block.scrn_h)
    {
        m_display_buf_img.disp_area_height = g_sys_configs_block.scrn_h;
    }
    btns_refresh();
}

void ScanWidget::on_caliBgBtn_clicked()
{
    start_scan(COLLECT_CMD_CALI_BG);
}


void ScanWidget::on_caliStreFactorBtn_clicked()
{
    start_scan(COLLECT_CMD_CALI_STRE_FACTOR);
}

void ScanWidget::expo_dura_timer_sig_hdlr()
{
    emit fpga_pwr_on_off_sig(false, (quint16)m_curr_expo_dura_s);
}
