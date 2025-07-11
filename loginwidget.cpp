#include "loginwidget.h"
#include "ui_loginwidget.h"

LoginWidget::LoginWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LoginWidget)
{
    ui->setupUi(this);

    connect(ui->exitPBtn, &QPushButton::clicked, QCoreApplication::instance(),
            &QCoreApplication::quit, Qt::QueuedConnection);
}

LoginWidget::~LoginWidget()
{
    delete ui;
}
