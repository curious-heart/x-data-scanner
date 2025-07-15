#ifndef SCANWIDGET_H
#define SCANWIDGET_H

#include <QWidget>
#include <QTimer>
#include <QFile>
#include <QMutex>

#include "recvscanneddata.h"
#include "config_recorder/uiconfigrecorder.h"

namespace Ui {
class ScanWidget;
}

typedef quint16 gray_pixel_data_type;

typedef enum
{
    COLLECT_CMD_SW_BTN,
    COLLECT_CMD_PHY_KEY,
}src_of_collect_cmd_e_t;

typedef struct
{
    QVector<QVector<gray_pixel_data_type>> lines;
    int line_len;
    bool refreshed;
}gray_lines_s_t;

typedef enum
{
    DISPLAY_IMG_REAL,
    DISPLAY_IMG_LAYFULL,
}gray_img_disp_type_e_t;

typedef struct
{
    int valid_line_cnt;
    QImage img_buffer;
}display_buf_img_s_t;

class ScanWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ScanWidget(UiConfigRecorder * cfg_recorder = nullptr, QWidget *parent = nullptr);
    ~ScanWidget();
    void rec_ui_settings();
    void load_ui_settings();

private:
    Ui::ScanWidget *ui;

    RecvScannedData *recv_data_worker;
    QThread *recv_data_workerThread;

    QQueue<recv_data_with_notes_s_t> dataQueue;
    QMutex queueMutex;

    int m_curr_line_pt_cnt;
    QVector<gray_pixel_data_type> m_ch1_data_vec, m_ch2_data_vec;
    gray_lines_s_t m_gray_img_lines;

    QImage m_local_img, m_local_img_8bit;
    QImage m_local_layfull_img, m_local_layfull_img_8bit;
    display_buf_img_s_t m_display_buf_img;

    QTimer m_scan_dura_timer;

    QTimer m_expo_to_coll_delay_timer;

    QString m_curr_sc_txt_file_name;
    QFile m_curr_sc_txt_file;
    QTextStream m_curr_sc_txt_stream;

    UiConfigRecorder * m_cfg_recorder = nullptr;
    qobj_ptr_set_t m_rec_ui_cfg_fin, m_rec_ui_cfg_fout;

    void start_collect(src_of_collect_cmd_e_t cmd_src = COLLECT_CMD_SW_BTN);
    void stop_collect(src_of_collect_cmd_e_t cmd_src = COLLECT_CMD_SW_BTN);
    void setup_sc_data_rec_file(QString &curr_path, QString &curr_date_str);
    void close_sc_data_file_rec();
    void clear_gray_img_lines();
    int split_data_into_channels(QByteArray& ori_data,
                                  QVector<gray_pixel_data_type> &dv_ch1, QVector<gray_pixel_data_type> &dv_ch2,
                                  quint64 &pkt_idx);
    void clear_recv_data_queue();

    void record_gray_img_line();
    void add_line_to_display(QVector<gray_pixel_data_type> &line);
    void generate_gray_img(gray_img_disp_type_e_t disp_type);
    void display_gray_img(gray_img_disp_type_e_t disp_type);
    void save_local_imgs(gray_img_disp_type_e_t disp_type);

    QString log_disp_prepender_str();
    bool chk_mk_pth_and_warn(QString &pth_str);
    void reset_display_img_buffer();
    void clear_display_img();

    void btns_refresh();

private slots:
    void handleNewDataReady();
    void recv_worker_report_sig_hdlr(LOG_LEVEL lvl, QString report_str,
                                collect_rpt_evt_e_t evt = COLLECT_RPT_EVT_IGNORE);
    void scn_dura_timeout_hdlr();
    void expo_to_coll_delay_timer_hdlr();

    void on_dataCollStartPbt_clicked();

    void on_dataCollStopPbt_clicked();

    void recv_data_finished_sig_hdlr();

signals:
    void start_collect_sc_data_sig(QString ip, quint16 port, int connTimeout);
    void stop_collect_sc_data_sig();
};

#endif // SCANWIDGET_H
