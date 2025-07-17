#include "literal_strings/literal_strings.h"
#include "syssettingswidget.h"
#include "ui_syssettingswidget.h"

#include "syssettings.h"
#include "sysconfigs/sysconfigs.h"

static int g_def_conn_data_src_timeout_sec = 3;

sys_settings_block_s_t g_sys_settings_blk;

SysSettingsWidget::SysSettingsWidget(UiConfigRecorder * cfg_recorder, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SysSettingsWidget), m_cfg_recorder(cfg_recorder)
{
    ui->setupUi(this);

    m_rec_ui_cfg_fin.clear(); m_rec_ui_cfg_fout.clear();

    ui->maxScanDuraSpinBox->setMaximum(g_sys_configs_block.allowed_max_scan_dura_sec);
    ui->maxScanDuraSpinBox->setValue(g_sys_configs_block.allowed_max_scan_dura_sec);
    ui->connDataSrcTimeoutSpinBox->setValue(g_def_conn_data_src_timeout_sec);

    g_sys_settings_blk.max_scan_dura_sec = ui->maxScanDuraSpinBox->value();
    g_sys_settings_blk.conn_data_src_timeout_sec = ui->connDataSrcTimeoutSpinBox->value();
    g_sys_settings_blk.img_save_path = ".";

    setup_hv_params_convert_factors();
    set_hv_params_limit_on_ui();
}

SysSettingsWidget::~SysSettingsWidget()
{
    delete ui;
}

void SysSettingsWidget::on_pushButton_clicked()
{
    g_sys_settings_blk.max_scan_dura_sec = ui->maxScanDuraSpinBox->value();

    g_sys_settings_blk.conn_data_src_timeout_sec = ui->connDataSrcTimeoutSpinBox->value();

    rec_ui_settings();
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

    //config: current is mA
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
            ui->tubeCurrentSpinBox->setMaximum(g_sys_configs_block.tube_current_ma_max);
            ui->tubeCurrentSpinBox->setDecimals(2);
            break;

    }

    //config: dura is s
    switch(g_sys_configs_block.ui_mb_dura_unit)
    {
        case MB_DURA_UNIT_MS:
            ui->expoDuraSpinBox->setMinimum(g_sys_configs_block.dura_ms_min * 1000);
            ui->expoDuraSpinBox->setSingleStep(1);
            ui->expoDuraSpinBox->setDecimals(0);
            break;

        case MB_DURA_UNIT_SEC:
            ui->expoDuraSpinBox->setMinimum(g_sys_configs_block.dura_ms_min * 1);
            ui->expoDuraSpinBox->setSingleStep(1);
            ui->expoDuraSpinBox->setDecimals(0);
            break;

        default: //MB_DURA_UNIT_MIN
            ui->expoDuraSpinBox->setMinimum(g_sys_configs_block.dura_ms_min / 60);
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
}
