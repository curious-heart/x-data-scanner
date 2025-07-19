#include <QMessageBox>

#include "literal_strings/literal_strings.h"
#include "syssettingswidget.h"
#include "ui_syssettingswidget.h"

#include "syssettings.h"
#include "sysconfigs/sysconfigs.h"

sys_settings_block_s_t g_sys_settings_blk;

SysSettingsWidget::SysSettingsWidget(UiConfigRecorder * cfg_recorder, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SysSettingsWidget), m_cfg_recorder(cfg_recorder)
{
    ui->setupUi(this);

    m_rec_ui_cfg_fin.clear(); m_rec_ui_cfg_fout.clear();

    setup_scan_params_limit_on_ui();

    setup_hv_params_convert_factors();
    set_hv_params_limit_on_ui();
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
    g_sys_settings_blk.img_save_path = ".";

    g_sys_settings_blk.hv_params.valid = true;
    g_sys_settings_blk.hv_params.tube_volt_kV = ui->tubeVoltKvSpinBox->value();
    g_sys_settings_blk.hv_params.tube_current_mA
        = ui->tubeVoltKvSpinBox->value() * g_sys_settings_blk.hv_params.ui_to_sw_current_factor;
    g_sys_settings_blk.hv_params.expo_dura_ms
        = ui->expoDuraSpinBox->value() * g_sys_settings_blk.hv_params.ui_to_sw_dura_factor;

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
