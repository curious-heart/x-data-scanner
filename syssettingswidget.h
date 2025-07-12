#ifndef SYSSETTINGSWIDGET_H
#define SYSSETTINGSWIDGET_H

#include <QWidget>

namespace Ui {
class SysSettingsWidget;
}

class SysSettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SysSettingsWidget(QWidget *parent = nullptr);
    ~SysSettingsWidget();

private slots:
    void on_pushButton_clicked();

private:
    Ui::SysSettingsWidget *ui;
};

#endif // SYSSETTINGSWIDGET_H
