#include "scanwidget.h"
#include "ui_scanwidget.h"

ScanWidget::ScanWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ScanWidget)
{
    ui->setupUi(this);
}

ScanWidget::~ScanWidget()
{
    delete ui;
}
