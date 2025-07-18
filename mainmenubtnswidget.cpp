#include "mainmenubtnswidget.h"
#include "ui_mainmenubtnswidget.h"

MainmenuBtnsWidget::MainmenuBtnsWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainmenuBtnsWidget)
{
    ui->setupUi(this);

    connect(ui->exitPBtn, &QPushButton::clicked, QCoreApplication::instance(),
            &QCoreApplication::quit, Qt::QueuedConnection);
}

MainmenuBtnsWidget::~MainmenuBtnsWidget()
{
    delete ui;
}

void MainmenuBtnsWidget::on_settingsPBtn_clicked()
{
    emit go_to_syssettings_widget_sig();
}


void MainmenuBtnsWidget::on_scanPBtn_clicked()
{
    emit go_to_scan_widget_sig();
}

