#include <QDateTime>
#include <QFileDialog>
#include <QElapsedTimer>

#include "camerawidget.h"
#include "ui_camerawidget.h"

#include "common_tools/common_tool_func.h"
#include "logger/logger.h"
#include "syssettings.h"

CameraWidget::CameraWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::CameraWidget)
    , camera(nullptr)
    , viewfinder(nullptr)
    , imageCapture(nullptr)
{
    ui->setupUi(this);

    m_dir_ready = mkpth_if_not_exists(g_sys_settings_blk.cam_photo_save_path);
    if(!m_dir_ready)
    {
        m_init_err_str += QString("create director %1 error!")
                              .arg(g_sys_settings_blk.cam_photo_save_path);

        DIY_LOG(LOG_ERROR, m_init_err_str);
        return;
    }

    // 初始化相机
    camera = new QCamera(this);
    viewfinder = new QCameraViewfinder(this);
    viewfinder->setMinimumSize(200, 300);
    ui->viewfinderContainer->layout()->addWidget(viewfinder);

    camera->setViewfinder(viewfinder);

    // 初始化图像捕获
    imageCapture = new QCameraImageCapture(camera, this);
    imageCapture->setCaptureDestination(QCameraImageCapture::CaptureToBuffer);
    connect(imageCapture, &QCameraImageCapture::imageCaptured,
            this, &CameraWidget::onImageCaptured);

    camera->start();

    m_init_ok = true;
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
        ui->labelPreview->size(), Qt::KeepAspectRatio, Qt::FastTransformation));

    if(m_dir_ready)
    {
        QDateTime curDateTime = QDateTime::currentDateTime();
        QString dtstr = curDateTime.toString("yyyyMMdd-hhmmsszzz");
        QString photo_fpn = g_sys_settings_blk.cam_photo_save_path + "/" + dtstr + ".jpg";
        preview.save(photo_fpn);
    }
}

void CameraWidget::on_btnCapture_clicked()
{
    imageCapture->capture(); // 捕获图像
}
