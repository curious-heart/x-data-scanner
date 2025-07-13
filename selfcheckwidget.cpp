#include <QThread>
#include <QLabel>
#include <QLayout>

#include "common_tools/common_tool_func.h"
#include "logger/logger.h"
#include "selfcheckwidget.h"
#include "ui_selfcheckwidget.h"
#include "literal_strings/literal_strings.h"

static const SelfCheckWidget::self_check_type_str_map_s_t gs_self_check_type_str_map[] =
{
    {SelfCheckWidget::SELF_CHECK_PWR, g_str_pwr_st},
    {SelfCheckWidget::SELF_CHECK_XRAY, g_str_x_rar_source_st},
    {SelfCheckWidget::SELF_CHECK_DETECTOR, g_str_detector_st},
    {SelfCheckWidget::SELF_CHECK_STORAGE, g_str_storage_st},
};

SelfCheckWidget::SelfCheckWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SelfCheckWidget)
{
    ui->setupUi(this);

}

SelfCheckWidget::~SelfCheckWidget()
{
    delete ui;
}

void SelfCheckWidget::self_check_item_ret_sig_hdlr(self_check_type_e_t chk_type, bool ret)
{
    int idx;
    QString title;
    for(idx = 0; idx < ARRAY_COUNT(gs_self_check_type_str_map); ++idx)
    {
        if(gs_self_check_type_str_map[idx].self_check_type == chk_type)
        {
            title = gs_self_check_type_str_map[idx].type_str;
            break;
        }
    }
    if(idx >= ARRAY_COUNT(gs_self_check_type_str_map))
    {
        DIY_LOG(LOG_WARN, QString("unknown self_check_type: %1").arg((int)chk_type));
        return;
    }

    QString ret_str = ret ? g_str_normal : g_str_abnormal;
    QString disp_str = title + ":" + ret_str;
    QLabel * self_chk_lbl = new QLabel(this);
    self_chk_lbl->setAlignment(Qt::AlignHCenter);
    self_chk_lbl->setText(disp_str);
    ui->selfChkVBoxLayout->addWidget(self_chk_lbl);
}
