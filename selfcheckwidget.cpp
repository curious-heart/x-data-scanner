#include <QThread>
#include <QLabel>
#include <QLayout>

#include "logger/logger.h"
#include "selfcheckwidget.h"
#include "ui_selfcheckwidget.h"
#include "literal_strings/literal_strings.h"

SelfCheckWidget::SelfCheckWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SelfCheckWidget)
{
    ui->setupUi(this);

    connect(this, &SelfCheckWidget::check_next_item_sig,
            this, &SelfCheckWidget::self_check, Qt::QueuedConnection);

    m_simu_timer.setSingleShot(true);
    connect(&m_simu_timer, &QTimer::timeout, this,
            [=]()
            {emit check_next_item_sig();});
}

SelfCheckWidget::~SelfCheckWidget()
{
    delete ui;
}

bool SelfCheckWidget::self_check(bool start)
{
    static int hdlr_idx = 0;
    static bool ret = true;
    if(start)
    {
        hdlr_idx = 0;
        ret = true;
    }
    else if(hdlr_idx >= m_check_hdlrs.count())
    {
        DIY_LOG(LOG_INFO, QString("self_check call: hdlr_idx %1 exceeds count %2, end.")
                .arg(hdlr_idx).arg(m_check_hdlrs.count()));

        hdlr_idx = 0;
        bool final_ret = ret;
        ret = true;

        emit self_check_finished_sig(final_ret);
        return final_ret;
    }

    QString title;
    bool sub_ret = (this->*m_check_hdlrs[hdlr_idx])(title);
    ret = ret && sub_ret;

    QString sub_ret_str = sub_ret ? g_str_normal : g_str_abnormal;
    QString disp_str = title + ":" + sub_ret_str;
    QLabel * self_chk_lbl = new QLabel(this);
    self_chk_lbl->setAlignment(Qt::AlignHCenter);
    self_chk_lbl->setText(disp_str);
    ui->selfChkVBoxLayout->addWidget(self_chk_lbl);

    ++hdlr_idx;

    return ret;
}

void SelfCheckWidget::go_to_next_check_item()
{
    m_simu_timer.start(500);
}

bool SelfCheckWidget::pwr_st_check(QString &title_str)
{
    title_str = g_str_pwr_st;

    go_to_next_check_item();
    return true;
}

bool SelfCheckWidget::x_ray_source_st_check(QString &title_str)
{
    title_str = g_str_x_rar_source_st;

    go_to_next_check_item();
    return true;
}

bool SelfCheckWidget::detector_st_check(QString &title_str)
{//fpga. ethernet.
    title_str = g_str_detector_st;

    go_to_next_check_item();
    return true;
}

bool SelfCheckWidget::storage_st_check(QString &title_str)
{
    title_str = g_str_storage_st;

    go_to_next_check_item();
    return true;
}
