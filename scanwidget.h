#ifndef SCANWIDGET_H
#define SCANWIDGET_H

#include <QWidget>
#include <QTimer>
#include <QFile>
#include <QMutex>
#include <QModbusClient>

#include "recvscanneddata.h"
#include "config_recorder/uiconfigrecorder.h"
#include "common_tools/common_tool_func.h"
#include "hv_ops_internal.h"
#include "modbus_regs.h"

namespace Ui {
class ScanWidget;
}

typedef enum
{
    COLLECT_CMD_NONE = 0,
    COLLECT_CMD_SW_BTN,
    COLLECT_CMD_PHY_KEY,
    COLLECT_CMD_REMOTE,
    COLLECT_CMD_CALI_BG,
    COLLECT_CMD_CALI_STRE_FACTOR,
}src_of_collect_cmd_e_t;
Q_DECLARE_METATYPE(src_of_collect_cmd_e_t)

typedef struct
{
    QVector<QVector<gray_pixel_data_type>> lines;
    int line_len;
    bool refreshed;
    gray_pixel_data_type min_v, max_v;
}gray_lines_s_t;

typedef enum
{
    DISPLAY_IMG_REAL,
    DISPLAY_IMG_LAYFULL,
}gray_img_disp_type_e_t;

typedef struct
{
    int buf_line_cnt, curr_work_img_ind, disp_line_start_idx, disp_area_height, total_line_cnt;
    QImage img_buffer, img_buffer2;
    QVector<gray_pixel_data_type> img_line_min_vs, img_line_max_vs, img_line_min_vs2, img_line_max_vs2;
}display_buf_img_s_t;

class ScanWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ScanWidget(UiConfigRecorder * cfg_recorder = nullptr, QWidget *parent = nullptr);
    ~ScanWidget();
    void rec_ui_settings();
    void load_ui_settings();

    void setup_tools(QModbusClient * modbus_device);

    void start_scan(src_of_collect_cmd_e_t cmd_src = COLLECT_CMD_SW_BTN); //control x ray and collect.
    void stop_scan(src_of_collect_cmd_e_t cmd_src = COLLECT_CMD_SW_BTN); //control x ray and collect.
    void detector_self_check();

    bool hv_send_op_cmd(hv_op_enum_t op);

private:
    Ui::ScanWidget *ui;

    QModbusClient * m_hv_device = nullptr;

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
    QImage m_disp8bit_img;

    QTimer m_scan_dura_timer;

    bool m_detector_self_chk = false;

    QTimer m_expo_to_coll_delay_timer;

    QString m_curr_sc_txt_file_name;
    QFile m_curr_sc_txt_file;
    QTextStream m_curr_sc_txt_stream;

    UiConfigRecorder * m_cfg_recorder = nullptr;
    qobj_ptr_set_t m_rec_ui_cfg_fin, m_rec_ui_cfg_fout;

    QVector<gray_pixel_data_type> m_scan_bg_value_loaded, m_scan_bg_value_for_work;
    QVector<double> m_scan_stre_factor_value_loaded, m_scan_stre_factor_value_for_work;
    bool m_pt_per_row_changed = true;

    hv_op_enum_t m_hv_curr_op = HV_OP_NULL;
    mb_reg_val_map_t m_regs_read_result;

    bool m_scan_cmd_proc = false;

    bool m_cali_bg_data_now = false, m_cali_stre_factor_data_now = false;
    int m_cali_bg_line_idx = 0, m_cali_stre_factor_line_idx = 0;

    src_of_collect_cmd_e_t m_curr_collect_cmd = COLLECT_CMD_NONE;

    //just collect.
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
    void add_line_to_display(QVector<gray_pixel_data_type> &line, gray_pixel_data_type min_v, gray_pixel_data_type max_v);
    void generate_gray_img(gray_img_disp_type_e_t disp_type);
    void display_gray_img(gray_img_disp_type_e_t disp_type);
    void save_local_imgs(gray_img_disp_type_e_t disp_type);

    QString log_disp_prepender_str();
    bool chk_mk_pth_and_warn(QString &pth_str);
    void reset_display_img_buffer();

    void adjust_bg_data_size(QVector<gray_pixel_data_type> &tgt,
                             const QVector<gray_pixel_data_type>& data, int tgt_size);
    void adjust_stre_factor_data_size(QVector<double> &tgt, const QVector<double>& data, int tgt_size);
    void load_cali_datum_from_file();
    void gen_cali_datum_to_file(src_of_collect_cmd_e_t cali_type);
    void reload_cali_datum();
    void calibrate_px_line(QVector<gray_pixel_data_type> &line);
    void reset_cali_ctrl_vars();
    void print_cali_data_to_log();
    QString get_cali_data_str(src_of_collect_cmd_e_t cali_type);

    void proc_pt_per_row_cnt_related_work();

    void btns_refresh();

    bool hv_construct_mb_du(hv_op_enum_t op, QModbusDataUnit &mb_du);
    void mb_rw_reply_received(hv_op_enum_t op, QModbusReply* mb_reply,
                              void (ScanWidget::*finished_sig_handler)(),
                              bool sync, bool error_notify);

private slots:
    void handleNewDataReady();
    void recv_worker_report_sig_hdlr(LOG_LEVEL lvl, QString report_str,
                                collect_rpt_evt_e_t evt = COLLECT_RPT_EVT_IGNORE);
    void scan_dura_timeout_hdlr();
    void expo_to_coll_delay_timer_hdlr();

    void on_dataCollStartPbt_clicked();

    void on_dataCollStopPbt_clicked();

    void recv_data_finished_sig_hdlr();

    void on_ptPerRowSpinBox_valueChanged(int arg1);

    void mb_op_finished_sig_handler();
    void mb_rw_error_sig_handler(QModbusDevice::Error error);

    void on_scanLockChkBox_stateChanged(int arg1);

    void on_caliBgBtn_clicked();

    void on_caliStreFactorBtn_clicked();

public slots:
    void scan_widget_disp_sig_hdlr();

signals:
    void start_collect_sc_data_sig();
    void stop_collect_sc_data_sig();
    void detector_handshake_sig();
    void mb_regs_read_ret_sig(mb_reg_val_map_t reg_val_map);
    void hv_op_finish_sig(bool ret, QString err_str = "");
    void detector_self_chk_ret_sig(bool ret);
    void gen_cali_datum_to_file_sig(src_of_collect_cmd_e_t cali_type);
};

#endif // SCANWIDGET_H
