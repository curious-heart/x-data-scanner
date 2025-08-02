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
    done(ResultExitApp);
}

void ExitDialog::on_shutDownOSPBtn_clicked()
{
    done(ResultShutdownOS);
}

void ExitDialog::on_cancelPbtn_clicked()
{
    done(ResultCancel);
}

