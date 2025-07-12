#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "common_tools/common_tool_func.h"
#include "literal_strings/literal_strings.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    /*this widget should be newed first since it loads sys settings.*/
    m_syssettings_widget = new SysSettingsWidget(this);
    m_syssettings_widget->hide();

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

    connect(m_self_chk_widget, &SelfCheckWidget::self_check_finished_sig,
            this, &MainWindow::self_check_finished_sig_hdlr, Qt::QueuedConnection);
    connect(m_login_widget, &LoginWidget::login_chk_passed_sig,
            this, &MainWindow::login_chk_passed_sig_hdlr, Qt::QueuedConnection);

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

void MainWindow::self_check_finished_sig_hdlr(bool result)
{
    if(result)
    {
        if(m_stacked_widget->indexOf(m_login_widget) < 0)
        {
            m_stacked_widget->addWidget(m_login_widget);
        }
        m_stacked_widget->setCurrentWidget(m_login_widget);
    }
}

void MainWindow::login_chk_passed_sig_hdlr()
{
    if(m_stacked_widget->indexOf(m_scan_widget) < 0)
    {
        m_stacked_widget->addWidget(m_scan_widget);
    }
    m_stacked_widget->setCurrentWidget(m_scan_widget);
}

QString hv_work_st_str(quint16 st_reg_val)
{
    static bool ls_first = true;
    typedef struct
    {
        quint16 val; QString str;
    }st_val_to_str_s_t;
    static const st_val_to_str_s_t ls_st_val_to_str_arr[] =
    {
        {0x11, "空闲"},
        {0x22, "散热"},
        {0xE1, "input1状态反馈异常"},
        {0xE2, "电流反馈异常"},
        {0xE3, "电压反馈异常"},
        {0xE4, "多个异常同时发生"},
    };
    static QMap<quint16, QString> ls_st_val_to_str_map;

    if(ls_first)
    {
        for(int i = 0; i < ARRAY_COUNT(ls_st_val_to_str_arr); ++i)
        {
            ls_st_val_to_str_map.insert(ls_st_val_to_str_arr[i].val,
                                        ls_st_val_to_str_arr[i].str);
        }
        ls_first = false;
    }

    QString st_str;
    if(ls_st_val_to_str_map.contains(st_reg_val))
    {
        st_str = ls_st_val_to_str_map[st_reg_val];
    }
    else
    {
        st_str = QString(g_str_unkonw_st) + ":0x" + QString::number(st_reg_val, 16).toUpper();
    }
    return st_str;
}
