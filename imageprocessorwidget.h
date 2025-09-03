#ifndef IMAGEPROCESSORWIDGET_H
#define IMAGEPROCESSORWIDGET_H

#include <QWidget>
#include <QRegularExpression>
#include <QScrollArea>
#include <QGridLayout>
#include <QButtonGroup>

#include "imageviewerwidget.h"

namespace Ui {
class ImageProcessorWidget;
}

typedef enum
{
    IMG_PROC_FILTER_ALL = 0,
    IMG_PROC_FILTER_DT_RANGE,
}img_proc_filter_mode_e_t;
Q_DECLARE_METATYPE(img_proc_filter_mode_e_t)

class _img_fns_list;
typedef class _img_fns_list
{
public:
    QString fn_png, fn_png8bit, fn_raw;
    int width, height;

public:
    _img_fns_list(QString png = "", QString png8bit = "", QString raw = "", int width = 0, int height = 0);
    bool operator==(const _img_fns_list &other);

}img_fns_list_s_t;

class ImageProcessorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ImageProcessorWidget(QWidget *parent = nullptr);
    ~ImageProcessorWidget();

private slots:
    void on_imgFilterComboBox_currentIndexChanged(int index);

    void on_imgFilterConfPBtn_clicked();

    void on_translateRBtn_toggled(bool checked);

    void on_bri_contr_RBtn_toggled(bool checked);

private:
    Ui::ImageProcessorWidget *ui;

    QRegularExpression m_filter_fn_reg;

    QButtonGroup * m_sort_rbtn_grp = nullptr, * m_op_rbtn_grp = nullptr;

    QScrollArea * m_thumbnail_scroll_area = nullptr;
    QWidget * m_thumbnail_container_wgt = nullptr;
    QGridLayout * m_thumbnail_layout = nullptr;

    QList<img_fns_list_s_t> m_selectedFiles;// 保存用户选中的文件

    QWidget * m_img_view_container_wgt = nullptr;
    QHBoxLayout * m_img_view_hbox_layout = nullptr;
    QWidget *m_img_with_info_wgt = nullptr, *m_img_with_info_wgt_2 = nullptr;
    QVBoxLayout * m_img_with_info_vbox_layout = nullptr, * m_img_with_info_vbox_layout_2 = nullptr;
    ImageViewerWidget * m_img_viewr = nullptr, *m_img_viewr_2 = nullptr;
    QLabel *m_img_info_lbl = nullptr, *m_img_info_lbl_2 = nullptr;

    void refresh_ctrls_display();
    bool eventFilter(QObject *obj, QEvent *event);

    void go_display_one_big_img();
    void go_display_parallel_imgs();
};

#endif // IMAGEPROCESSORWIDGET_H
