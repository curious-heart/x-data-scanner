#include "mainmenubtnswidget.h"
#include "ui_mainmenubtnswidget.h"
#include "exitdialog.h"

MainmenuBtnsWidget::MainmenuBtnsWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainmenuBtnsWidget)
{
    ui->setupUi(this);
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

void MainmenuBtnsWidget::on_photoPBtn_clicked()
{
    emit go_to_camera_widget_sig();
}


void MainmenuBtnsWidget::on_exitPBtn_clicked()
{
    ExitDialog exit_dialog;

    exit_dialog.exec();
}

