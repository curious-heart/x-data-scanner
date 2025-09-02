#include "imageviewerwidget.h"

#include "common_tools/common_tool_func.h"
#include "logger/logger.h"

static const int gs_min_px_val = 0, gs_max_px_val = 65535;

ImageViewerWidget::ImageViewerWidget(QWidget *parent)
    : QLabel(parent),
      m_scaleFactor(1.0),
      m_rotationAngle(0),
      m_flipH(false),
      m_flipV(false),
      m_brightness(0),
      m_contrast(0), m_translate(false), m_WW_adjust(false), m_WL_adjust(false)
{
    setBackgroundRole(QPalette::Base);
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    setScaledContents(false);
    setMouseTracking(true);
}

// 加载图像
// filePath indicates raw file
bool ImageViewerWidget::loadImage(const QString &filePath, int width, int height)
{
    QFile raw_data_file(filePath);

    if(!raw_data_file.open(QIODevice::ReadOnly))
    {
        DIY_LOG(LOG_ERROR, QString("open file %1 error: %2.").arg(filePath, raw_data_file.errorString()));
        return false;
    }

    QImage img(width, height, QImage::Format::Format_Grayscale16);
    qsizetype img_size = img.sizeInBytes();
    qint64 read_bytes;
    read_bytes = raw_data_file.read((char*)img.bits(), img_size);
    if(read_bytes != img_size)
    {
        DIY_LOG(LOG_ERROR, QString("load image faile: img_size is %1, read_bytes is %2")
                               .arg(img_size).arg(read_bytes));

        raw_data_file.close();
        return false;
    }
    raw_data_file.close();

    m_originalImage = img;

    count_WW_WL(m_originalImage, m_ori_WW, m_ori_WL);
    resetImage();
    return true;
}

// 恢复原图
void ImageViewerWidget::resetImage()
{
    m_processedImage = m_originalImage;
    m_scaleFactor = 1.0;
    m_offset = QPoint(0,0);
    m_rotationAngle = 0;
    m_flipH = false;
    m_flipV = false;
    m_brightness = 0;
    m_contrast = 0;

    m_windowWidth = m_ori_WW;
    m_windowLevel = m_ori_WL;
    update();
}

// 缩放
void ImageViewerWidget::zoomIn()  { setScale(m_scaleFactor * 1.1); }
void ImageViewerWidget::zoomOut() { setScale(m_scaleFactor / 1.1); }
void ImageViewerWidget::setScale(double factor)
{
    m_scaleFactor = factor;
    update();
}

// 旋转
void ImageViewerWidget::rotateLeft90()  { m_rotationAngle -= 90; update(); }
void ImageViewerWidget::rotateRight90() { m_rotationAngle += 90; update(); }

// 翻转
void ImageViewerWidget::flipHorizontal() { m_flipH = !m_flipH; update(); }
void ImageViewerWidget::flipVertical()   { m_flipV = !m_flipV; update(); }

// 亮度/对比度
void ImageViewerWidget::setBrightness(double value) { m_brightness = value; update(); }
void ImageViewerWidget::setContrast(double value)   { m_contrast = value; update(); }

// 鼠标拖拽
void ImageViewerWidget::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton)
    {
        m_lastMousePos = event->pos();
    }

    QLabel::mousePressEvent(event);
}
void ImageViewerWidget::mouseMoveEvent(QMouseEvent *event)
{
    if(event->buttons() & Qt::LeftButton){
        QPoint delta = event->pos() - m_lastMousePos;
        m_lastMousePos = event->pos();

        if(m_translate) m_offset += delta;
        else if(m_WW_adjust)
        {
            if(delta.x() >= 0) m_windowWidth += delta.x();
            else m_windowWidth -= (quint16)(-1 * delta.x());

            if(m_windowWidth < gs_min_px_val) m_windowWidth = gs_min_px_val;
            if(m_windowWidth > gs_max_px_val) m_windowWidth = gs_max_px_val;
        }
        else if(m_WL_adjust)
        {
            if(delta.y() >= 0) m_windowLevel -= delta.y();
            else m_windowLevel += (quint16)(-1 * delta.y());

            if(m_windowLevel < gs_min_px_val) m_windowLevel = gs_min_px_val;
            if(m_windowLevel > gs_max_px_val) m_windowLevel = gs_max_px_val;
        }

        update();
    }
}

// 鼠标滚轮缩放
void ImageViewerWidget::wheelEvent(QWheelEvent *event)
{
    if(event->angleDelta().y() > 0) zoomIn();
    else zoomOut();
}

// paintEvent
void ImageViewerWidget::paintEvent(QPaintEvent *event)
{
    if (m_originalImage.isNull()) return QLabel::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform,true);

    QImage img = m_originalImage;

    QImage::Format img_format = img.format();
    DIY_LOG(LOG_INFO, QString("img format is %1").arg(img_format));

    // 翻转
    if(m_flipH) img = img.mirrored(true,false);
    if(m_flipV) img = img.mirrored(false,true);

    // 旋转
    QTransform transform;
    transform.rotate(m_rotationAngle);
    img = img.transformed(transform);

    // 亮度/对比度
    img = applyBrightnessContrast(img);

    QImage display_img_8bit = convertGrayscale16To8(img);
    // 缩放
    QSize targetSize = img.size() * m_scaleFactor;
    QPixmap pix = QPixmap::fromImage(display_img_8bit.scaled(targetSize,
                                     Qt::KeepAspectRatio, Qt::SmoothTransformation));

    // 平移绘制
    QPoint center = QPoint(width()/2, height()/2);
    QPoint drawPos = center - QPoint(pix.width()/2, pix.height()/2) + m_offset;
    painter.drawPixmap(drawPos, pix);
}

// 简单亮度/对比度调整
QImage ImageViewerWidget::applyBrightnessContrast(QImage &img)
{
    return applyWindowWidth(img, m_windowWidth, m_windowLevel);
}

QImage ImageViewerWidget::applyWindowWidth(const QImage &img, quint16 WW, quint16 WL)
{
    QImage result(img.size(), QImage::Format_Grayscale16);
    int w = img.width();
    int h = img.height();

    double low = WL - WW / 2.0;
    double high = WL + WW / 2.0;

    if(low == high)
    {
        DIY_LOG(LOG_INFO, "img low == high, just return its own.");
        return img;
    }

    for (int y = 0; y < h; ++y) {
        const quint16 *srcLine = reinterpret_cast<const quint16 *>(img.constScanLine(y));
        quint16 *dstLine = reinterpret_cast<quint16 *>(result.scanLine(y));

        for (int x = 0; x < w; ++x) {
            double v = srcLine[x];
            double norm = (v - low) / (high - low);
            if (norm < 0.0) norm = 0.0;
            if (norm > 1.0) norm = 1.0;
            dstLine[x] = static_cast<quint16>(norm * gs_max_px_val);
        }
    }
    return result;
}
