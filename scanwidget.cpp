#include <cstring>
#include <QMessageBox>
#include <QByteArray>

#include "scanwidget.h"
#include "ui_scanwidget.h"

#include "sysconfigs/sysconfigs.h"
#include "common_tools/common_tool_func.h"
#include "syssettings.h"
#include "literal_strings/literal_strings.h"

bool g_data_scanning_now = false;

static const char* gs_str_sc_data_txt_file_type = ".txt";
static const char* gs_str_img_raw_type = ".raw";
static const char* gs_str_img_png_type = ".png";
static const char* gs_str_8bit_img_apx = "-8bit";

#undef RECV_DATA_NOTE_E
#define RECV_DATA_NOTE_E(e) #e
static const char* gs_recv_data_note_str [] = {RECV_DATA_NOTES};

ScanWidget::ScanWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ScanWidget)
{
    ui->setupUi(this);

    ui->ptPerRowSpinBox->setMaximum(g_sys_configs_block.max_pt_number);

    m_scan_dura_timer.setSingleShot(true);
    m_scan_dura_timer.setInterval(g_sys_settings_blk.max_scan_dura_sec * 1000);
    connect(&m_scan_dura_timer, &QTimer::timeout,
            this, &ScanWidget::scn_dura_timeout_hdlr, Qt::QueuedConnection);

    qRegisterMetaType<collect_rpt_evt_e_t>();

    recv_data_worker = new RecvScannedData(&dataQueue, &queueMutex);
    recv_data_workerThread = new QThread(this);
    recv_data_worker->moveToThread(recv_data_workerThread);
    connect(recv_data_workerThread, &QThread::finished,
            recv_data_worker, &QObject::deleteLater);

    connect(this, &ScanWidget::start_collect_sc_data_sig,
            recv_data_worker, &RecvScannedData::start_collect_sc_data_hdlr, Qt::QueuedConnection);
    connect(this, &ScanWidget::stop_collect_sc_data_sig,
            recv_data_worker, &RecvScannedData::stop_collect_sc_data_hdlr, Qt::QueuedConnection);
    connect(recv_data_worker, &RecvScannedData::new_data_ready_sig,
            this, &ScanWidget::handleNewDataReady, Qt::QueuedConnection);
    connect(recv_data_worker, &RecvScannedData::recv_worker_report_sig,
            this, &ScanWidget::recv_worker_report_sig_hdlr, Qt::QueuedConnection);
    connect(recv_data_worker, &RecvScannedData::recv_data_finished_sig,
            this, &ScanWidget::recv_data_finished_sig_hdlr, Qt::QueuedConnection);

    connect(&m_expo_to_coll_delay_timer, &QTimer::timeout,
            this, &ScanWidget::expo_to_coll_delay_timer_hdlr, Qt::QueuedConnection);

    int disp_pt_per_row = ui->ptPerRowSpinBox->value();
    if(disp_pt_per_row > g_sys_configs_block.max_pt_number || disp_pt_per_row < 0)
    {
        disp_pt_per_row = g_sys_configs_block.max_pt_number;
    }
    m_display_buf_img.img_buffer = QImage(disp_pt_per_row, g_sys_configs_block.scrn_h,
                               QImage::Format_Grayscale16);
    clear_display_img();

    recv_data_workerThread->start();

    btns_refresh();
}

ScanWidget::~ScanWidget()
{
    m_scan_dura_timer.stop();

    recv_data_workerThread->quit();
    recv_data_workerThread->wait();
    recv_data_workerThread->deleteLater();

    delete ui;
}

void ScanWidget::clear_display_img()
{
    QColor bgColor = this->palette().color(QPalette::Window);
    m_display_buf_img.img_buffer.fill(bgColor.rgb());
    m_display_buf_img.valid_line_cnt = 0;
}

void ScanWidget::start_collect(src_of_collect_cmd_e_t /*cmd_src*/)
{
    clear_gray_img_lines();
    clear_display_img();
    ui->grayImgLbl->clear();

    QString curr_date_str = common_tool_get_curr_date_str();
    QString curr_path = g_sys_settings_blk.img_save_path + "/" + curr_date_str;
    setup_sc_data_rec_file(curr_path, curr_date_str);

    QString ip = g_sys_configs_block.data_src_ip;
    quint16 port = (quint16)g_sys_configs_block.data_src_port;

    emit start_collect_sc_data_sig(ip, port, g_sys_settings_blk.conn_data_src_timeout_sec);
    g_data_scanning_now = true;

    m_scan_dura_timer.start();

    btns_refresh();
}

void ScanWidget::stop_collect(src_of_collect_cmd_e_t /*cmd_src*/)
{
    if(!g_data_scanning_now) return;

    m_scan_dura_timer.stop();

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

void ScanWidget::handleNewDataReady()
{
    if(!g_data_scanning_now) return;

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

    if(NORMAL == packet.notes)
    {
        quint64 pkt_idx;
        m_curr_line_pt_cnt
                = split_data_into_channels(packet.data, m_ch1_data_vec, m_ch2_data_vec, pkt_idx);

        record_gray_img_line();
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
    if(disp_pt_per_row > g_sys_configs_block.max_pt_number)
    {//normally this branch would never be reached. just for assurance...
        disp_pt_per_row = g_sys_configs_block.max_pt_number;
    }
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

    default:
        break;
    }
}

void ScanWidget::expo_to_coll_delay_timer_hdlr()
{
    start_collect();
}

void ScanWidget::scn_dura_timeout_hdlr()
{
    stop_collect();
}

void ScanWidget::clear_gray_img_lines()
{
    for (auto &line: m_gray_img_lines.lines) line.clear();

    m_gray_img_lines.lines.clear();
    m_gray_img_lines.refreshed = false;
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
    m_gray_img_lines.lines.append(line);
    m_gray_img_lines.line_len = m_curr_line_pt_cnt;
    m_gray_img_lines.refreshed = true;

    add_line_to_display(line);
}

void ScanWidget::add_line_to_display(QVector<gray_pixel_data_type> &line)
{
    int img_disp_area_height = m_display_buf_img.img_buffer.height();
    int bytesPerLine = m_display_buf_img.img_buffer.bytesPerLine();  // 注意：不是 width * 2，包含对齐
    int target_line_idx;

    uchar *data = m_display_buf_img.img_buffer.bits();

    if (line.count() > m_display_buf_img.img_buffer.width())
    {
        DIY_LOG(LOG_ERROR, "this line is too long");
        return;
    }

    if(m_display_buf_img.valid_line_cnt >= img_disp_area_height)
    {
        // 上移一行（height - 1 行拷贝到顶部）
        memmove(data, data + bytesPerLine, bytesPerLine * (img_disp_area_height - 1));

        target_line_idx = img_disp_area_height - 1;
    }
    else
    {
        target_line_idx = m_display_buf_img.valid_line_cnt;
        ++m_display_buf_img.valid_line_cnt;
    }

    // 拷贝新行到底部
    memcpy(data + bytesPerLine * target_line_idx, line.constData(),
               line.count()*sizeof(gray_pixel_data_type));

    QRect valid_area = QRect(0, 0, m_display_buf_img.img_buffer.width(),
                                   m_display_buf_img.valid_line_cnt);
    QImage disp_img = convertGrayscale16To8(m_display_buf_img.img_buffer, valid_area);
    QPixmap scaled = QPixmap::fromImage(disp_img)
                .scaled(ui->grayImgLbl->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    ui->grayImgLbl->setPixmap(scaled);
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
    start_collect();
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
}

void ScanWidget::on_dataCollStopPbt_clicked()
{
    stop_collect();
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
    ui->dataCollStartPbt->setEnabled(!g_data_scanning_now);
    ui->dataCollStopPbt->setEnabled(g_data_scanning_now);
}
