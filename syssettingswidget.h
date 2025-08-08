#ifndef SYSSETTINGSWIDGET_H
#define SYSSETTINGSWIDGET_H

#include <QWidget>
#include <QButtonGroup>

#include "config_recorder/uiconfigrecorder.h"

namespace Ui {
class SysSettingsWidget;
}

class SysSettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SysSettingsWidget(UiConfigRecorder * cfg_recorder = nullptr, QWidget *parent = nullptr);
    ~SysSettingsWidget();

    void load_ui_settings();
    void rec_ui_settings();
    bool get_sysettings_from_ui(bool succ_silent = true);
    bool check_expo_and_scan_time(bool succ_silent);
    bool rmt_dbg_enabled();
    quint16 rmt_dbg_local_port();

private slots:
    void on_pushButton_clicked();

    void on_rmtDbgEnableRBtn_toggled(bool checked);

    void on_caliModecheckBox_toggled(bool checked);

    void on_caliBgSpinBox_valueChanged(int arg1);

    void on_caliStreFactorSpinBox_valueChanged(int arg1);

    void on_motorSpeedPBtn_clicked();

    void on_pbSleepPBtn_clicked();

    void on_pbWakeupPBtn_clicked();

private:
    Ui::SysSettingsWidget *ui;
    QButtonGroup * m_rmt_dbg_rbtn_grp = nullptr;

    UiConfigRecorder * m_cfg_recorder = nullptr;
    qobj_ptr_set_t m_rec_ui_cfg_fin, m_rec_ui_cfg_fout;

    void setup_scan_params_limit_on_ui();

    void setup_hv_params_convert_factors();
    void set_hv_params_limit_on_ui();

signals:
    void rmt_dbg_enabled_sig(bool enable);
    void motor_speed_set_sig(int speed);
    void pb_slp_wkp_sig(bool wkp);
};

#endif // SYSSETTINGSWIDGET_H
