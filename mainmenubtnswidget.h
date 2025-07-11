#ifndef MAINMENUBTNSWIDGET_H
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

private:
    Ui::MainmenuBtnsWidget *ui;
};

#endif // MAINMENUBTNSWIDGET_H
