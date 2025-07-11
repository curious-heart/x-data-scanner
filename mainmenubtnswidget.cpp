#include "mainmenubtnswidget.h"
#include "ui_mainmenubtnswidget.h"

MainmenuBtnsWidget::MainmenuBtnsWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainmenuBtnsWidget)
{
    ui->setupUi(this);

    connect(ui->exitPBtn, &QPushButton::clicked, QCoreApplication::instance(),
            &QCoreApplication::quit, Qt::QueuedConnection);
}

MainmenuBtnsWidget::~MainmenuBtnsWidget()
{
    delete ui;
}
