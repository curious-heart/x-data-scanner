#include <QMessageBox>

#include "literal_strings/literal_strings.h"
#include "syssettingswidget.h"
#include "ui_syssettingswidget.h"

#include "syssettings.h"
#include "sysconfigs/sysconfigs.h"

static const char* gs_def_img_save_path = "./scan_images";
static const char* gs_def_cam_photo_save_path = "./camera_photos";
static const char* gs_def_stitched_img_save_path = "./stitched_images";

sys_settings_block_s_t g_sys_settings_blk;

SysSettingsWidget::SysSettingsWidget(UiConfigRecorder * cfg_recorder, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SysSettingsWidget), m_cfg_recorder(cfg_recorder)
{
    ui->setupUi(this);

    m_rmt_dbg_rbtn_grp = new QButtonGroup(this);
    m_rmt_dbg_rbtn_grp->addButton(ui->rmtDbgEnableRBtn);
    m_rmt_dbg_rbtn_grp->addButton(ui->rmtDbgDisableRBtn);

    m_rec_ui_cfg_fin.clear(); m_rec_ui_cfg_fout.clear();

    setup_scan_params_limit_on_ui();

    setup_hv_params_convert_factors();
    set_hv_params_limit_on_ui();

    load_ui_settings();

    ui->recvdLineLimtLbl->setVisible(g_sys_configs_block.limit_recvd_line_number);
    ui->recvdLineLimitSpinBox->setVisible(g_sys_configs_block.limit_recvd_line_number);
}

SysSettingsWidget::~SysSettingsWidget()
{
    delete ui;
}

void SysSettingsWidget::on_pushButton_clicked()
{
    get_sysettings_from_ui(false);
    rec_ui_settings();
}

void SysSettingsWidget::setup_scan_params_limit_on_ui()
{
    ui->maxScanDuraSpinBox->setMinimum(g_sys_configs_block.scan_dura_allowed_min_sec);
    ui->maxScanDuraSpinBox->setMaximum(g_sys_configs_block.scan_dura_allowed_max_sec);

    ui->connDataSrcTimeoutSpinBox->setMinimum(g_sys_configs_block.conn_data_src_tmo_allowed_min_sec);
    ui->connDataSrcTimeoutSpinBox->setMaximum(g_sys_configs_block.conn_data_src_tmo_allowed_max_sec);

    ui->expoCollDelaySpinBox->setMinimum(g_sys_configs_block.expo_to_coll_min_allowed_delay_ms);
    ui->expoCollDelaySpinBox->setMaximum(g_sys_configs_block.expo_to_coll_max_allowed_delay_ms);

    ui->iniDisplayLineCntSpinBox->setMinimum(g_sys_configs_block.ini_disp_img_line_cnt_allowed_min);
    ui->iniDisplayLineCntSpinBox->setMaximum(g_sys_configs_block.ini_disp_img_line_cnt_allowed_max);

    ui->mergeDispLineCntSpinBox->setMinimum(g_sys_configs_block.merg_disp_img_line_cnt_allowed_min);
    ui->mergeDispLineCntSpinBox->setMaximum(g_sys_configs_block.merg_disp_img_line_cnt_allowed_max);

    ui->caliBgSpinBox->setMinimum(g_sys_configs_block.cali_bg_line_cnt_allowed_min);
    ui->caliBgSpinBox->setMaximum(g_sys_configs_block.cali_bg_line_cnt_allowed_max);

    ui->caliStreFactorSpinBox->setMinimum(g_sys_configs_block.cali_stre_factor_line_cnt_allowed_min);
    ui->caliStreFactorSpinBox->setMaximum(g_sys_configs_block.cali_stre_factor_line_cnt_allowed_max);

    ui->recvdLineLimitSpinBox->setMinimum(g_sys_configs_block.recvd_line_number_limit_allowed_min);
    ui->recvdLineLimitSpinBox->setMaximum(g_sys_configs_block.recvd_line_number_limit_allowed_max);
}

void SysSettingsWidget::setup_hv_params_convert_factors()
{
    //internal(sw): current is mA
    switch(g_sys_configs_block.ui_current_unit)
    {
        case MB_TUBE_CURRENT_UNIT_UA:
            ui->tubeCurrentLbl->setText(QString("%1（%2）").arg(g_str_tube_current, g_str_current_unit_ua));
            g_sys_settings_blk.hv_params.ui_to_sw_current_factor = 0.001;
            break;

        case MB_TUBE_CURRENT_UNIT_MA:
            ui->tubeCurrentLbl->setText(QString("%1（%2）").arg(g_str_tube_current, g_str_current_unit_ma));
            g_sys_settings_blk.hv_params.ui_to_sw_current_factor = 1;
            break;

        default: //MB_TUBE_CURRENT_UNIT_A
            ui->tubeCurrentLbl->setText(QString("%1（%2）").arg(g_str_tube_current, g_str_current_unit_a));
            g_sys_settings_blk.hv_params.ui_to_sw_current_factor = 1000;
            break;

    }

    //internal(sw): current is mA
    switch(g_sys_configs_block.mb_tube_current_intf_unit)
    {
        case MB_TUBE_CURRENT_UNIT_UA:
            g_sys_settings_blk.hv_params.sw_to_dev_current_factor = 1000;
            break;

        case MB_TUBE_CURRENT_UNIT_MA:
            g_sys_settings_blk.hv_params.sw_to_dev_current_factor = 1;
            break;

        default: //MB_TUBE_CURRENT_UNIT_A
            g_sys_settings_blk.hv_params.sw_to_dev_current_factor = 0.001;
            break;

    }

    //internal(sw): dura is ms
    switch(g_sys_configs_block.ui_mb_dura_unit)
    {
        case MB_DURA_UNIT_MS:
            ui->expoDuraLbl->setText(QString("%1（%2）").arg(g_str_expo_dura, g_str_dura_unit_ms));
            g_sys_settings_blk.hv_params.ui_to_sw_dura_factor = 1;
            break;

        case MB_DURA_UNIT_SEC:
            ui->expoDuraLbl->setText(QString("%1（%2）").arg(g_str_expo_dura, g_str_dura_unit_s));
            g_sys_settings_blk.hv_params.ui_to_sw_dura_factor = 1000;
            break;

        default: //MB_DURA_UNIT_MIN
            ui->expoDuraLbl->setText(QString("%1（%2）").arg(g_str_expo_dura, g_str_dura_unit_min));
            g_sys_settings_blk.hv_params.ui_to_sw_dura_factor = 60 * 1000;
            break;
    }

    //internal(sw): dura is ms
    switch(g_sys_configs_block.mb_dura_intf_unit)
    {
        case MB_DURA_UNIT_MS:
            g_sys_settings_blk.hv_params.sw_to_dev_dura_factor = 1;
            break;

        case MB_DURA_UNIT_SEC:
            g_sys_settings_blk.hv_params.sw_to_dev_dura_factor = 0.001;
            break;

        default: //MB_DURA_UNIT_MIN
            g_sys_settings_blk.hv_params.sw_to_dev_dura_factor = 1/(60*1000);
            break;
    }

    g_sys_settings_blk.hv_params.ui_to_sw_volt_factor = 1;
    g_sys_settings_blk.hv_params.sw_to_dev_volt_factor = 1;

    //extra sw to dev factor
    g_sys_settings_blk.hv_params.sw_to_dev_volt_factor
        *= g_sys_configs_block.sw_to_dev_extra_factor_volt;
    g_sys_settings_blk.hv_params.sw_to_dev_current_factor
        *= g_sys_configs_block.sw_to_dev_extra_factor_current;
    g_sys_settings_blk.hv_params.sw_to_dev_dura_factor
        *= g_sys_configs_block.sw_to_dev_extra_factor_dura;
}

void SysSettingsWidget::set_hv_params_limit_on_ui()
{
    ui->tubeVoltKvSpinBox->setMinimum(g_sys_configs_block.tube_volt_kv_min);
    ui->tubeVoltKvSpinBox->setMaximum(g_sys_configs_block.tube_volt_kv_max);

    //config(sw): current is mA
    switch(g_sys_configs_block.ui_current_unit)
    {
        case MB_TUBE_CURRENT_UNIT_UA:
            ui->tubeCurrentSpinBox->setMinimum(g_sys_configs_block.tube_current_ma_min * 1000);
            ui->tubeCurrentSpinBox->setMaximum(g_sys_configs_block.tube_current_ma_max * 1000);
            ui->tubeCurrentSpinBox->setSingleStep(1);
            ui->tubeCurrentSpinBox->setDecimals(0);
            break;

        case MB_TUBE_CURRENT_UNIT_MA:
            ui->tubeCurrentSpinBox->setMinimum(g_sys_configs_block.tube_current_ma_min);
            ui->tubeCurrentSpinBox->setMaximum(g_sys_configs_block.tube_current_ma_max);
            ui->tubeCurrentSpinBox->setSingleStep(1);
            ui->tubeCurrentSpinBox->setDecimals(0);
            break;

        default: //MB_TUBE_CURRENT_UNIT_A
            ui->tubeCurrentSpinBox->setMinimum(g_sys_configs_block.tube_current_ma_min / 1000);
            ui->tubeCurrentSpinBox->setMaximum(g_sys_configs_block.tube_current_ma_max / 1000);
            ui->tubeCurrentSpinBox->setDecimals(2);
            break;

    }

    //config(sw): dura is ms
    switch(g_sys_configs_block.ui_mb_dura_unit)
    {
        case MB_DURA_UNIT_MS:
            ui->expoDuraSpinBox->setMinimum(g_sys_configs_block.dura_ms_min);
            ui->expoDuraSpinBox->setMaximum(g_sys_configs_block.dura_ms_max);
            ui->expoDuraSpinBox->setSingleStep(1);
            ui->expoDuraSpinBox->setDecimals(0);
            break;

        case MB_DURA_UNIT_SEC:
            ui->expoDuraSpinBox->setMinimum(g_sys_configs_block.dura_ms_min / 1000);
            ui->expoDuraSpinBox->setMaximum(g_sys_configs_block.dura_ms_max / 1000);
            ui->expoDuraSpinBox->setSingleStep(1);
            ui->expoDuraSpinBox->setDecimals(0);
            break;

        default: //MB_DURA_UNIT_MIN
            ui->expoDuraSpinBox->setMinimum(g_sys_configs_block.dura_ms_min / (1000*60));
            ui->expoDuraSpinBox->setMaximum(g_sys_configs_block.dura_ms_max / (1000*60));
            ui->expoDuraSpinBox->setSingleStep(0.1);
            ui->expoDuraSpinBox->setDecimals(2);
            break;
    }

}

void SysSettingsWidget::rec_ui_settings()
{
    if(m_cfg_recorder)
    {
        m_cfg_recorder->record_ui_configs(this, m_rec_ui_cfg_fin, m_rec_ui_cfg_fout);
    }
}

void SysSettingsWidget::load_ui_settings()
{
    if(m_cfg_recorder)
    {
        m_cfg_recorder->load_configs_to_ui(this, m_rec_ui_cfg_fin, m_rec_ui_cfg_fout);
    }
    get_sysettings_from_ui();
}

bool SysSettingsWidget::get_sysettings_from_ui(bool succ_silent)
{
    bool ret;

    g_sys_settings_blk.max_scan_dura_sec = ui->maxScanDuraSpinBox->value();
    g_sys_settings_blk.conn_data_src_timeout_sec = ui->connDataSrcTimeoutSpinBox->value();
    g_sys_settings_blk.expo_to_coll_delay_ms = ui->expoCollDelaySpinBox->value();

    g_sys_settings_blk.img_save_path = gs_def_img_save_path;
    g_sys_settings_blk.cam_photo_save_path = gs_def_cam_photo_save_path;
    g_sys_settings_blk.stitched_img_save_path = gs_def_stitched_img_save_path;

    g_sys_settings_blk.hv_params.valid = true;
    g_sys_settings_blk.hv_params.tube_volt_kV
        = ui->tubeVoltKvSpinBox->value() * g_sys_settings_blk.hv_params.ui_to_sw_volt_factor;
    g_sys_settings_blk.hv_params.tube_current_mA
        = ui->tubeCurrentSpinBox->value() * g_sys_settings_blk.hv_params.ui_to_sw_current_factor;
    g_sys_settings_blk.hv_params.expo_dura_ms
        = ui->expoDuraSpinBox->value() * g_sys_settings_blk.hv_params.ui_to_sw_dura_factor;

    g_sys_settings_blk.ini_disp_img_line_cnt = ui->iniDisplayLineCntSpinBox->value();
    g_sys_settings_blk.merg_disp_img_line_cnt = ui->mergeDispLineCntSpinBox->value();

    g_sys_settings_blk.cali_mode_now = ui->caliModecheckBox->isChecked();
    g_sys_settings_blk.cali_bg_line_cnt = ui->caliBgSpinBox->value();
    g_sys_settings_blk.cali_stre_factor_line_cnt = ui->caliStreFactorSpinBox->value();

    g_sys_settings_blk.recvd_line_number_limit = ui->recvdLineLimitSpinBox->value();

    ret = check_expo_and_scan_time(succ_silent);

    return ret;
}

bool SysSettingsWidget::check_expo_and_scan_time(bool succ_silent)
{
    QString ret_str;
    bool ret;

    if((int)(g_sys_settings_blk.hv_params.expo_dura_ms / 1000)
            > g_sys_settings_blk.max_scan_dura_sec)
    {
        g_sys_settings_blk.hv_params.valid = false;

        ret_str = QString(g_str_syssettings_error) + ":\n";
        ret_str += QString("%1:%2\n%3%4\n%5:%6\n")
                .arg(g_str_expo_dura).arg(g_sys_settings_blk.hv_params.expo_dura_ms/1000)
                .arg(g_str_cannt, g_str_exceed)
                .arg(g_str_scan_dura_time).arg(g_sys_settings_blk.max_scan_dura_sec);
        ret_str += g_str_plz_check;
        QMessageBox::critical(this, "Error!", ret_str);
        ret = false;
    }
    else
    {
        ret_str = g_str_set_succeeds;
        if(!succ_silent) QMessageBox::information(this, "Ok!", ret_str);
        ret = true;
    }
    return ret;
}

bool SysSettingsWidget::rmt_dbg_enabled()
{
    return ui->rmtDbgEnableRBtn->isChecked();
}

void SysSettingsWidget::on_rmtDbgEnableRBtn_toggled(bool checked)
{
    emit rmt_dbg_enabled_sig(checked);
}

quint16 SysSettingsWidget::rmt_dbg_local_port()
{
    return (quint16)ui->rmtDbgLocalPortSpinBox->value();
}

void SysSettingsWidget::on_caliModecheckBox_toggled(bool checked)
{
    g_sys_settings_blk.cali_mode_now = checked;
    g_sys_settings_blk.cali_bg_line_cnt = ui->caliBgSpinBox->value();
    g_sys_settings_blk.cali_stre_factor_line_cnt = ui->caliStreFactorSpinBox->value();
}


void SysSettingsWidget::on_caliBgSpinBox_valueChanged(int arg1)
{
    g_sys_settings_blk.cali_bg_line_cnt = arg1;
}


void SysSettingsWidget::on_caliStreFactorSpinBox_valueChanged(int arg1)
{
    g_sys_settings_blk.cali_stre_factor_line_cnt = arg1;
}

void SysSettingsWidget::on_motorSpeedPBtn_clicked()
{
    emit motor_speed_set_sig(ui->motorSpeedSpinBox->value());
}


void SysSettingsWidget::on_pbSleepPBtn_clicked()
{
    emit pb_slp_wkp_sig(false);
}


void SysSettingsWidget::on_pbWakeupPBtn_clicked()
{
    emit pb_slp_wkp_sig(true);
}

