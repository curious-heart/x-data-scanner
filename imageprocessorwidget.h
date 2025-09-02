#ifndef IMAGEPROCESSORWIDGET_H
#define IMAGEPROCESSORWIDGET_H

#include <QWidget>
#include <QRegularExpression>
#include <QScrollArea>
#include <QGridLayout>

namespace Ui {
class ImageProcessorWidget;
}

typedef enum
{
    IMG_PROC_FILTER_ALL = 0,
    IMG_PROC_FILTER_DT_RANGE,
}img_proc_filter_mode_e_t;
Q_DECLARE_METATYPE(img_proc_filter_mode_e_t)

class ImageProcessorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ImageProcessorWidget(QWidget *parent = nullptr);
    ~ImageProcessorWidget();

private slots:
    void on_imgFilterComboBox_currentIndexChanged(int index);

    void on_imgFilterConfPBtn_clicked();

private:
    Ui::ImageProcessorWidget *ui;

    QRegularExpression m_filter_fn_reg;

    QScrollArea * m_thumbnail_scroll_area = nullptr;
    QWidget * m_thumbnail_container_wgt = nullptr;
    QGridLayout * m_thumbnail_layout = nullptr;

    QStringList m_selectedFiles;   // 保存用户选中的文件

    void refresh_ctrls_display();
    bool eventFilter(QObject *obj, QEvent *event);
};

#endif // IMAGEPROCESSORWIDGET_H
