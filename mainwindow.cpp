#include "mainwindow.h"
#include "ui_mainwindow.h"

const char* g_str_row_int = "行发送间隔时间";
const char* g_str_unit_ms = "ms";

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_stacked_widget = new QStackedWidget(this);
    ui->mainPartVBoxLayot->addWidget(m_stacked_widget);

    m_self_chk_widget = new SelfCheckWidget(this);
    m_stacked_widget->addWidget(m_self_chk_widget);

    m_login_widget = new LoginWidget(this);
    m_login_widget->hide();
    m_scan_widget = new ScanWidget(this);
    m_scan_widget->hide();

    m_mainmenubtns_widget = new MainmenuBtnsWidget(this);
    ui->buttonsHBoxLayout->addWidget(m_mainmenubtns_widget);

    start_self_chk();
}

void MainWindow::start_self_chk()
{
    bool self_chk_ret = m_self_chk_widget->self_check(true);
    if(self_chk_ret)
    {
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}
