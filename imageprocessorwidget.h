#ifndef IMAGEPROCESSORWIDGET_H
#define IMAGEPROCESSORWIDGET_H

#include <QWidget>

namespace Ui {
class ImageProcessorWidget;
}

class ImageProcessorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ImageProcessorWidget(QWidget *parent = nullptr);
    ~ImageProcessorWidget();

private:
    Ui::ImageProcessorWidget *ui;
};

#endif // IMAGEPROCESSORWIDGET_H
