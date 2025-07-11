#ifndef SELFCHECKWIDGET_H
#define SELFCHECKWIDGET_H

#include <QWidget>
#include <QVector>
#include <QTimer>

namespace Ui {
class SelfCheckWidget;
}

class SelfCheckWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SelfCheckWidget(QWidget *parent = nullptr);
    ~SelfCheckWidget();

private:
    Ui::SelfCheckWidget *ui;

    bool pwr_st_check(QString &title_str);
    bool x_ray_source_st_check(QString &title_str);
    bool detector_st_check(QString &title_str);
    bool storage_st_check(QString &title_str);

    using CheckHandler = bool (SelfCheckWidget::*)(QString &);
    QVector<CheckHandler> m_check_hdlrs =
    {
        &SelfCheckWidget::pwr_st_check,
        &SelfCheckWidget::x_ray_source_st_check,
        &SelfCheckWidget::detector_st_check,
        &SelfCheckWidget::storage_st_check,
    };

    void go_to_next_check_item();
    QTimer m_simu_timer;

public slots:
    bool self_check(bool start = false);

signals:
    void check_next_item_sig(bool start = false);
    void self_check_finished_sig(bool result);
};

#endif // SELFCHECKWIDGET_H
