#include <QDateTime>
#include <QFileDialog>

#include "camerawidget.h"
#include "ui_camerawidget.h"

CameraWidget::CameraWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::CameraWidget)
    , camera(nullptr)
    , viewfinder(nullptr)
    , imageCapture(nullptr)
{
    ui->setupUi(this);

    // 初始化相机
    camera = new QCamera(this);
    viewfinder = new QCameraViewfinder(this);
    viewfinder->setMinimumSize(200, 300);
    ui->viewfinderContainer->layout()->addWidget(viewfinder);

    camera->setViewfinder(viewfinder);

    // 初始化图像捕获
    imageCapture = new QCameraImageCapture(camera, this);
    connect(imageCapture, &QCameraImageCapture::imageCaptured,
            this, &CameraWidget::onImageCaptured);

    camera->start();
}

CameraWidget::~CameraWidget()
{
    camera->stop();
    delete ui;
}

void CameraWidget::onImageCaptured(int id, const QImage &preview)
{
    Q_UNUSED(id);
    lastCapturedImage = preview;
    ui->labelPreview->setPixmap(QPixmap::fromImage(preview).scaled(
        ui->labelPreview->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void CameraWidget::on_btnCapture_clicked()
{
    imageCapture->capture(); // 捕获图像
}

void CameraWidget::on_btnSave_clicked()
{
    if (lastCapturedImage.isNull()) {
        qWarning() << "没有可保存的图片";
        return;
    }
    QString filePath = QFileDialog::getSaveFileName(this,
        tr("保存图片"),
        QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".jpg",
        tr("JPEG (*.jpg);;PNG (*.png)"));
    if (!filePath.isEmpty()) {
        lastCapturedImage.save(filePath);
    }
}

