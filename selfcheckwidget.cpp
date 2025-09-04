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

void SelfCheckWidget::self_check_item_ret_sig_hdlr(self_check_type_e_t chk_type, self_check_stage_e_t st)
{
    size_t idx;
    QString title;
    QLabel * self_chk_lbl = nullptr;
    static QLabel * lbl_arr[SELF_CHECK_CNT] = {nullptr};
    for(idx = 0; idx < ARRAY_COUNT(gs_self_check_type_str_map); ++idx)
    {
        if(gs_self_check_type_str_map[idx].self_check_type == chk_type)
        {
            title = gs_self_check_type_str_map[idx].type_str;
            if(nullptr == lbl_arr[idx])
            {
                self_chk_lbl = new QLabel(this);
                lbl_arr[idx] = self_chk_lbl;
                ui->selfChkVBoxLayout->addWidget(self_chk_lbl);
            }
            else
            {
                self_chk_lbl = lbl_arr[idx];
            }
            break;
        }
    }
    if(idx >= ARRAY_COUNT(gs_self_check_type_str_map))
    {
        DIY_LOG(LOG_WARN, QString("unknown self_check_type: %1").arg((int)chk_type));
        return;
    }

    QString ret_str;
    if(SELF_CHECK_PASS == st) ret_str = g_str_normal;
    else if(SELF_CHECK_FAIL == st) ret_str = g_str_abnormal;
    else ret_str = g_str_checking;
    QString disp_str = title + ":" + ret_str;
    self_chk_lbl->setAlignment(Qt::AlignHCenter);
    self_chk_lbl->setText(disp_str);
}
