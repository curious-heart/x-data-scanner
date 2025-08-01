﻿#ifndef MAINMENUBTNSWIDGET_H
#define MAINMENUBTNSWIDGET_H

#include <QWidget>

namespace Ui {
class MainmenuBtnsWidget;
}

class MainmenuBtnsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MainmenuBtnsWidget(QWidget *parent = nullptr);
    ~MainmenuBtnsWidget();

private slots:
    void on_settingsPBtn_clicked();

    void on_scanPBtn_clicked();

    void on_photoPBtn_clicked();

    void on_exitPBtn_clicked();

private:
    Ui::MainmenuBtnsWidget *ui;

signals:
    void go_to_scan_widget_sig();
    void go_to_syssettings_widget_sig();
    void go_to_camera_widget_sig();

    void send_pb_power_off_sig();
};

#endif // MAINMENUBTNSWIDGET_H
