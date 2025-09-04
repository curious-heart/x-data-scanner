#include <cmath>
#include "imageviewerwidget.h"
#include "imageprocessorwidget.h"

#include "common_tools/common_tool_func.h"
#include "logger/logger.h"

static const int gs_min_px_val = 0, gs_max_px_val = 65535;

ImageViewerWidget::ImageViewerWidget(QLabel *info_lbl, ImageProcessorWidget * op_ctrls,
                                     QWidget *parent)
    : QLabel(parent), m_op_ctrls(op_ctrls), m_info_lbl(info_lbl)
{
    reset_op_params();

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

    QImage img = read_gray_raw_img(filePath, width, height, QImage::Format::Format_Grayscale16);
    if(img.isNull())
    {
        DIY_LOG(LOG_ERROR, QString("load image from file %1 error.").arg(filePath));
        return false;
    }

    return loadImage(img);
}

bool ImageViewerWidget::loadImage(const QImage &img)
{
    m_originalImage = img;

    count_WW_WL(m_originalImage, m_ori_WW, m_ori_WL);
    resetImage();
    return true;

}

void ImageViewerWidget::clear_op_flags()
{
    m_flipH = false;
    m_flipV = false;
    m_translate = false;
    m_WW_adjust = m_WL_adjust = false;
    m_mark = m_del_mark = false;
}

void ImageViewerWidget::reset_op_params()
{
    m_scaleFactor = 1.0;
    m_offset = QPoint(0,0);
    m_rotationAngle = 0;

    clear_op_flags();
}

void ImageViewerWidget::refresh_op_flags()
{
    if(m_op_ctrls)
    {
        m_op_ctrls->get_latest_op_flags(m_translate, m_mark, m_del_mark, m_WW_adjust);
        m_WL_adjust = m_WW_adjust;
    }
}

// 恢复原图
void ImageViewerWidget::resetImage()
{
    m_processedImage = m_originalImage;

    reset_op_params();

    m_windowWidth = m_ori_WW;
    m_windowLevel = m_ori_WL;
    update();
}

void ImageViewerWidget::translate(bool op)
{
    m_translate = op;
}

void ImageViewerWidget::bright_contrast(bool op)
{
    m_WW_adjust = m_WL_adjust = op;
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

// 鼠标拖拽
void ImageViewerWidget::mousePressEvent(QMouseEvent *event)
{
    refresh_op_flags();
    if(event->button() == Qt::LeftButton)
    {
        m_lastMousePos = event->pos();
    }

    QLabel::mousePressEvent(event);
}
void ImageViewerWidget::mouseMoveEvent(QMouseEvent *event)
{
    refresh_op_flags();
    bool need_update = false;
    if(event->buttons() & Qt::LeftButton)
    {
        QPoint delta = event->pos() - m_lastMousePos;
        m_lastMousePos = event->pos();

        if(m_translate) m_offset += delta;
        if(m_WW_adjust)
        {
            if(delta.x() >= 0) m_windowWidth += delta.x();
            else m_windowWidth -= (quint16)(-1 * delta.x());

            if(m_windowWidth < gs_min_px_val) m_windowWidth = gs_min_px_val;
            if(m_windowWidth > gs_max_px_val) m_windowWidth = gs_max_px_val;
        }
        if(m_WL_adjust)
        {
            if(delta.y() >= 0) m_windowLevel -= delta.y();
            else m_windowLevel += (quint16)(-1 * delta.y());

            if(m_windowLevel < gs_min_px_val) m_windowLevel = gs_min_px_val;
            if(m_windowLevel > gs_max_px_val) m_windowLevel = gs_max_px_val;
        }

        need_update = true;
    }

    m_mouse_in_img = get_mouse_pos_px_val(event->pos(), m_curr_mouse_pos_x, m_curr_mouse_pos_y,
                                          m_curr_mouse_pos_val);
    if(m_mouse_in_img) need_update = true;

    if(need_update) update();
}

// 鼠标滚轮缩放
void ImageViewerWidget::wheelEvent(QWheelEvent *event)
{
    refresh_op_flags();
    if(event->angleDelta().y() > 0) zoomIn();
    else zoomOut();
}

// paintEvent
void ImageViewerWidget::paintEvent(QPaintEvent *event)
{
    if (m_originalImage.isNull()) return QLabel::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform,true);

    painter.save();
    // 图像显示中心（Widget 中心 + 用户偏移）
    QPointF center(width() / 2.0 + m_offset.x(),
                   height() / 2.0 + m_offset.y());

    QTransform transform;
    transform.translate(center.x(), center.y());      // 移到显示中心
    transform.scale(m_scaleFactor, m_scaleFactor);
    transform.rotate(m_rotationAngle);                // 旋转
    if (m_flipH) transform.scale(-1, 1);     // 水平翻转
    if (m_flipV) transform.scale(1, -1);     // 垂直翻转
    transform.translate(-1 * m_originalImage.width() / 2.0, -1 * m_originalImage.height() / 2.0); // 图像左上角对齐

    painter.setTransform(transform);
    QImage display_img_8bit = convertGrayscale16To8(m_originalImage);
    painter.drawImage(QPoint(0, 0), display_img_8bit);

    painter.restore();

    m_imageToWidget = transform;
    m_widgetToImage = transform.inverted();

    /*--------------------*/
    if(m_info_lbl)
    {
        QString info_str = QString("width: %1, height: %2;").arg(m_originalImage.width())
                                                           .arg(m_originalImage.height());
        info_str += QString(" WW: %1, WL: %2").arg(m_windowWidth).arg(m_windowLevel);
        if(m_mouse_in_img)
        {
            info_str += QString("; x:%1, y:%2, val:%3").arg(m_curr_mouse_pos_x)
                                            .arg(m_curr_mouse_pos_y).arg(m_curr_mouse_pos_y);
        }
        m_info_lbl->setText(info_str);
    }
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

bool ImageViewerWidget::get_mouse_pos_px_val(const QPoint &mouse_pos,
                                             int &pos_x, int &pos_y, quint16 &pos_val)
{
    if (m_originalImage.isNull()) return false;

    QPointF imgPt = m_widgetToImage.map(mouse_pos);
    int x = static_cast<int>(std::floor(imgPt.x()));
    int y = static_cast<int>(std::floor(imgPt.y()));

    if (x < 0 || y < 0 || x >= m_originalImage.width() || y >= m_originalImage.height())
    {
        return false;
    }

    pos_x = x;
    pos_y = y;

    if (m_originalImage.format() == QImage::Format_Grayscale16)
    {
        const quint16 *line = reinterpret_cast<const quint16*>(m_originalImage.constScanLine(y));
        pos_val = line[x];
    } else {
        pos_val = qGray(m_originalImage.pixel(x, y));
    }
    return true;
}
