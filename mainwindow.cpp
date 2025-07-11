#include "mainwindow.h"
#include "ui_mainwindow.h"

const char* g_str_row_int = "行发送间隔时间";
const char* g_str_unit_ms = "ms";

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_pushButton_clicked()
{
    QApplication::quit();
}

