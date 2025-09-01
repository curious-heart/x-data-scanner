#include "common_tools/common_tool_func.h"

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

    int ret = exit_dialog.exec();
    switch (ret)
    {
        case ExitDialog::ResultExitApp:
            QCoreApplication::exit(APP_EXIT_NORMAL);
            break;

        case ExitDialog::ResultShutdownOS:
            emit send_pb_power_off_sig();
            QCoreApplication::exit(APP_EXIT_APP_POWER_OFF);
            break;

        case ExitDialog::ResultCancel:
        default:
            // 什么也不做
            break;
    }
}


void MainmenuBtnsWidget::on_imgManagePBtn_clicked()
{
    emit go_to_img_proc_widget_sig();
}

