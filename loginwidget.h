#ifndef LOGINWIDGET_H
#define LOGINWIDGET_H

#include <QWidget>

namespace Ui {
class LoginWidget;
}

class LoginWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LoginWidget(QWidget *parent = nullptr);
    ~LoginWidget();

private slots:
    void on_loginPBtn_clicked();

private:
    Ui::LoginWidget *ui;

    bool check_pwd(QString usr_name_str, QString pwd_str);

signals:
    void login_chk_passed_sig();
};

#endif // LOGINWIDGET_H
