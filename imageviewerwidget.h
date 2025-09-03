#ifndef IMAGEVIEWERWIDGET_H
#define IMAGEVIEWERWIDGET_H

#include <QLabel>
#include <QImage>
#include <QPixmap>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPainter>
#include <QTransform>

class ImageViewerWidget : public QLabel
{
    Q_OBJECT
public:
    explicit ImageViewerWidget(QLabel * info_lbl = nullptr, QWidget *parent = nullptr);

    // 加载图像
    bool loadImage(const QString &filePath, int width, int height);

    // 图像操作
    void resetImage();          // 恢复原图
    void translate(bool op = true);
    void bright_contrast(bool op = true);
    void zoomIn();
    void zoomOut();
    void setScale(double factor);
    void rotateLeft90();
    void rotateRight90();
    void flipHorizontal();
    void flipVertical();

    void clear_op_flags();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    QLabel * m_info_lbl = nullptr;
    QImage m_originalImage;   // 原始图像
    QImage m_processedImage;  // 处理后的图像
    double m_scaleFactor;     // 缩放
    QPoint m_offset;          // 平移
    QPoint m_lastMousePos;    // 拖拽记录
    double m_rotationAngle;   // 旋转角度
    bool m_flipH;
    bool m_flipV;
    bool m_translate, m_WW_adjust, m_WL_adjust;
    bool m_mark, m_del_mark;
    quint16 m_windowWidth, m_windowLevel, m_ori_WW, m_ori_WL;

    void reset_op_params();

    void updateProcessedImage(); // 根据旋转/翻转/亮度/对比度更新 m_processedImage
    QImage applyBrightnessContrast(QImage &img);
    QImage applyWindowWidth(const QImage &img, quint16 WW, quint16 WL);
};

#endif // IMAGEVIEWERWIDGET_H
