#ifndef CAMERAWIDGET_H
#define CAMERAWIDGET_H

#include <QWidget>
#include <QCamera>
#include <QCameraViewfinder>
#include <QCameraImageCapture>
#include <QImage>

namespace Ui {
class CameraWidget;
}

class CameraWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CameraWidget(QWidget *parent = nullptr);
    ~CameraWidget();

    bool m_init_ok = false;
    QString m_init_err_str;

private slots:
    void onImageCaptured(int id, const QImage &preview);

    void on_btnCapture_clicked();

private:
    Ui::CameraWidget *ui;

    bool m_dir_ready = false;
    QCamera *camera;
    QCameraViewfinder *viewfinder;
    QCameraImageCapture *imageCapture;
    QImage lastCapturedImage;
};

#endif // CAMERAWIDGET_H
