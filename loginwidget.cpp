#include "loginwidget.h"
#include "ui_loginwidget.h"

static const char* gs_str_usr_or_pwd_error = "用户名或密码错误";

LoginWidget::LoginWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LoginWidget)
{
    ui->setupUi(this);

    ui->pwdLEdit->setEchoMode(QLineEdit::Password);

    connect(ui->pwdLEdit, &QLineEdit::returnPressed, this, &LoginWidget::on_loginPBtn_clicked);
    connect(ui->usrNameLEdit, &QLineEdit::returnPressed, this, &LoginWidget::on_loginPBtn_clicked);

    connect(ui->exitPBtn, &QPushButton::clicked, QCoreApplication::instance(),
            &QCoreApplication::quit, Qt::QueuedConnection);
}

LoginWidget::~LoginWidget()
{
    delete ui;
}

void LoginWidget::on_loginPBtn_clicked()
{
    bool passed = check_pwd(ui->usrNameLEdit->text(), ui->pwdLEdit->text());
    if(passed)
    {
        ui->pwdChkRetLbl->setText("");

        emit login_chk_passed_sig();
    }
    else
    {
        ui->pwdChkRetLbl->setText(gs_str_usr_or_pwd_error);
        ui->pwdChkRetLbl->setStyleSheet("color: red;");
    }
}

bool LoginWidget::check_pwd(QString usr_name_str, QString pwd_str)
{
    if(pwd_str == "right") return true;
    else return false;
}
