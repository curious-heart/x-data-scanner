#include "imageprocessorwidget.h"
#include "ui_imageprocessorwidget.h"

ImageProcessorWidget::ImageProcessorWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ImageProcessorWidget)
{
    ui->setupUi(this);
}

ImageProcessorWidget::~ImageProcessorWidget()
{
    delete ui;
}
