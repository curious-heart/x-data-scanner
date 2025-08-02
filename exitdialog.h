#ifndef EXITDIALOG_H
#define EXITDIALOG_H

#include <QDialog>

namespace Ui {
class ExitDialog;
}

class ExitDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ExitDialog(QWidget *parent = nullptr);
    ~ExitDialog();

    enum ResultCode
    {
        ResultExitApp,
        ResultShutdownOS,
        ResultCancel,
    };

private slots:
    void on_exitAppPBtn_clicked();

    void on_shutDownOSPBtn_clicked();

    void on_cancelPbtn_clicked();

private:
    Ui::ExitDialog *ui;
};

#endif // EXITDIALOG_H
