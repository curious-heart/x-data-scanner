#include "syssettingswidget.h"
#include "ui_syssettingswidget.h"

#include "syssettings.h"
#include "sysconfigs/sysconfigs.h"

static int g_def_conn_data_src_timeout_sec = 3;

sys_settings_block_s_t g_sys_settings_blk;

SysSettingsWidget::SysSettingsWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SysSettingsWidget)
{
    ui->setupUi(this);

    ui->maxScanDuraSpinBox->setMaximum(g_sys_configs_block.allowed_max_scan_dura_sec);
    ui->maxScanDuraSpinBox->setValue(g_sys_configs_block.allowed_max_scan_dura_sec);
    ui->connDataSrcTimeoutSpinBox->setValue(g_def_conn_data_src_timeout_sec);

    g_sys_settings_blk.max_scan_dura_sec = ui->maxScanDuraSpinBox->value();
    g_sys_settings_blk.conn_data_src_timeout_sec = ui->connDataSrcTimeoutSpinBox->value();
    g_sys_settings_blk.img_save_path = ".";
}

SysSettingsWidget::~SysSettingsWidget()
{
    delete ui;
}

void SysSettingsWidget::on_pushButton_clicked()
{
    g_sys_settings_blk.max_scan_dura_sec = ui->maxScanDuraSpinBox->value();

    g_sys_settings_blk.conn_data_src_timeout_sec = ui->connDataSrcTimeoutSpinBox->value();
}

