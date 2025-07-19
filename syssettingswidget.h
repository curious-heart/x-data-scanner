#ifndef SYSSETTINGSWIDGET_H
#define SYSSETTINGSWIDGET_H

#include <QWidget>

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

private slots:
    void on_pushButton_clicked();

private:
    Ui::SysSettingsWidget *ui;

    UiConfigRecorder * m_cfg_recorder = nullptr;
    qobj_ptr_set_t m_rec_ui_cfg_fin, m_rec_ui_cfg_fout;

    void setup_scan_params_limit_on_ui();

    void setup_hv_params_convert_factors();
    void set_hv_params_limit_on_ui();
};

#endif // SYSSETTINGSWIDGET_H
