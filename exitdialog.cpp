#include "common_tools/common_tool_func.h"
#include "exitdialog.h"
#include "ui_exitdialog.h"

ExitDialog::ExitDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ExitDialog)
{
    ui->setupUi(this);
}

ExitDialog::~ExitDialog()
{
    delete ui;
}

void ExitDialog::on_exitAppPBtn_clicked()
{
    QCoreApplication::exit(APP_EXIT_NORMAL);
}


void ExitDialog::on_shutDownOSPBtn_clicked()
{
    QCoreApplication::exit(APP_EXIT_APP_POWER_OFF);
}


void ExitDialog::on_cancelPbtn_clicked()
{
    reject();
}

