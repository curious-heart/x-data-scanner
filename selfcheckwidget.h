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

    typedef enum
    {
        SELF_CHECK_PWR = 0,
        SELF_CHECK_XRAY,
        SELF_CHECK_DETECTOR,
        SELF_CHECK_STORAGE,
        SELF_CHECK_CNT,
    }self_check_type_e_t;
    Q_ENUM(self_check_type_e_t)

    typedef enum
    {
        SELF_CHECKING,
        SELF_CHECK_PASS,
        SELF_CHECK_FAIL,
    }self_check_stage_e_t;
    Q_ENUM(self_check_stage_e_t)

    typedef struct
    {
      self_check_type_e_t self_check_type;
      const char* type_str;
    }self_check_type_str_map_s_t;

private:
    Ui::SelfCheckWidget *ui;

public slots:
    void self_check_item_ret_sig_hdlr(SelfCheckWidget::self_check_type_e_t chk_type,
                                      SelfCheckWidget::self_check_stage_e_t st);
};

#endif // SELFCHECKWIDGET_H
