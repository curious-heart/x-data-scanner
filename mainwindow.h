#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QTimer>

#include <selfcheckwidget.h>
#include <loginwidget.h>
#include <scanwidget.h>
#include <syssettingswidget.h>
#include <mainmenubtnswidget.h>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    SelfCheckWidget * m_self_chk_widget;

private:
    Ui::MainWindow *ui;
    QStackedWidget *m_stacked_widget;

    LoginWidget * m_login_widget;
    ScanWidget * m_scan_widget;
    SysSettingsWidget * m_syssettings_widget;

    MainmenuBtnsWidget * m_mainmenubtns_widget;

    void start_self_chk();

public slots:
    void self_check_finished_sig_hdlr(bool result);
    void login_chk_passed_sig_hdlr();
};
#endif // MAINWINDOW_H
