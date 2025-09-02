#include <QDirIterator>
#include "imageprocessorwidget.h"
#include "ui_imageprocessorwidget.h"

#include "common_tools/common_tool_func.h"
#include "logger/logger.h"
#include "img_proc_common.h"
#include "syssettings.h"

typedef struct
{
    img_proc_filter_mode_e_t mode;
    const char* str;
}img_proc_filter_combobox_item_s_t;

static const img_proc_filter_combobox_item_s_t img_proc_filter_combox_list[] =
{
    {IMG_PROC_FILTER_ALL, "所有"},
    {IMG_PROC_FILTER_DT_RANGE, "按时间范围"},
};

ImageProcessorWidget::ImageProcessorWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ImageProcessorWidget)
{
    ui->setupUi(this);

    qRegisterMetaType<img_proc_filter_mode_e_t>("img_proc_filter_mode_e_t");

    for(int i = 0; i < ARRAY_COUNT(img_proc_filter_combox_list); ++i)
    {
        ui->imgFilterComboBox->addItem(img_proc_filter_combox_list[i].str,
                                       img_proc_filter_combox_list[i].mode);
    }
    ui->imgFilterComboBox->setCurrentIndex(IMG_PROC_FILTER_ALL);

    ui->startDTEdit->setDateTime(QDateTime::currentDateTime());
    ui->stopDTEdit->setDateTime(QDateTime::currentDateTime());

    QList<QRegularExpression> pat_list;
    get_saved_img_name_or_pat(nullptr, nullptr, &pat_list);
    m_filter_fn_reg = pat_list[IMG_TYPE_PNG];

    /*setup widges for thumnail display*/
    m_thumbnail_scroll_area = new QScrollArea(this);
    m_thumbnail_container_wgt = new QWidget;
    m_thumbnail_layout = new QGridLayout(m_thumbnail_container_wgt);
    m_thumbnail_scroll_area->setWidget(m_thumbnail_container_wgt);
    m_thumbnail_scroll_area->setWidgetResizable(true);

    ui->imgViewStackedWgt->addWidget(m_thumbnail_scroll_area);

    /*--------------------*/
    refresh_ctrls_display();
}

ImageProcessorWidget::~ImageProcessorWidget()
{
    delete ui;
}

void ImageProcessorWidget::refresh_ctrls_display()
{
    bool dt_ctrl_enabled = (ui->imgFilterComboBox->currentData() != IMG_PROC_FILTER_ALL);
    ui->startDTEdit->setEnabled(dt_ctrl_enabled);
    ui->stopDTEdit->setEnabled(dt_ctrl_enabled);
}

void ImageProcessorWidget::on_imgFilterComboBox_currentIndexChanged(int /*index*/)
{
    refresh_ctrls_display();
}


void ImageProcessorWidget::on_imgFilterConfPBtn_clicked()
{
    ui->imgViewStackedWgt->setCurrentWidget(m_thumbnail_scroll_area);

    QDateTime start_dt = ui->startDTEdit->dateTime();
    QDateTime stop_dt  = ui->stopDTEdit->dateTime();

    // 1) 清空原有缩略图
    QLayoutItem *child;
    while ((child = m_thumbnail_layout->takeAt(0)) != nullptr) {
        if (child->widget()) {
            child->widget()->deleteLater();
        }
        delete child;
    }

    // 2) 遍历路径下所有符合正则的文件
    QDirIterator it(g_sys_settings_blk.img_save_path,
                    QStringList() << QString("*") + g_str_img_png_type,   // 先按后缀过滤
                    QDir::Files,
                    QDirIterator::Subdirectories);

    QList<QFileInfo> fileList;
    while (it.hasNext()) {
        QString filePath = it.next();
        QFileInfo fi(filePath);

        // 用正则过滤文件名
        if (!m_filter_fn_reg.match(fi.fileName()).hasMatch())
            continue;

        // 分支 1: 显示所有
        if (ui->imgFilterComboBox->currentData() == IMG_PROC_FILTER_ALL) {
            fileList.append(fi);
        }
        // 分支 2: 时间范围过滤
        else {
            QDateTime created_dt = fi.created();   // 或 fi.lastModified()

            if (created_dt >= start_dt && created_dt <= stop_dt) {
                fileList.append(fi);
            }
        }
    }

    // 3) 把文件列表显示为缩略图
    int row = 0, col = 0, maxCol = 5;
    for (const QFileInfo &fi : fileList) {
        QPixmap pixmap(fi.filePath());
        QLabel *thumbLabel = new QLabel;
        thumbLabel->setPixmap(pixmap.scaled(100, 100, Qt::KeepAspectRatio, Qt::FastTransformation));
        thumbLabel->setProperty("filePath", fi.filePath());
        thumbLabel->setAlignment(Qt::AlignCenter);

        // 开启鼠标事件
        thumbLabel->setAttribute(Qt::WA_Hover);
        thumbLabel->installEventFilter(this);

        m_thumbnail_layout->addWidget(thumbLabel, row, col++);
        if (col >= maxCol) {
            col = 0;
            row++;
        }
    }

    m_thumbnail_container_wgt->adjustSize();
}

bool ImageProcessorWidget::eventFilter(QObject *obj, QEvent *event)
{
    QLabel *thumbLabel = qobject_cast<QLabel*>(obj);
    if (!thumbLabel) return QWidget::eventFilter(obj, event);

    if (event->type() == QEvent::MouseButtonDblClick) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            QString filePath = thumbLabel->property("filePath").toString();
            // TODO: 打开大图显示
            qDebug() << "Double clicked:" << filePath;
            return true;
        }
    }
    else if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

        if (mouseEvent->button() == Qt::LeftButton) {
            QString filePath = thumbLabel->property("filePath").toString();

            if (QApplication::keyboardModifiers() & Qt::ControlModifier) {
                // Ctrl + 左键：多选
                if (m_selectedFiles.contains(filePath)) {
                    m_selectedFiles.removeAll(filePath);
                    thumbLabel->setStyleSheet(""); // 取消高亮
                } else {
                    m_selectedFiles.append(filePath);
                    thumbLabel->setStyleSheet("border: 2px solid red;");
                }
            } else {
                // 单选
                m_selectedFiles.clear();
                m_selectedFiles.append(filePath);

                // 先清空所有缩略图的高亮
                for (QObject *child : m_thumbnail_container_wgt->children()) {
                    QLabel *lbl = qobject_cast<QLabel*>(child);
                    if (lbl) lbl->setStyleSheet("");
                }
                thumbLabel->setStyleSheet("border: 2px solid red;");
            }

            qDebug() << "Selected files:" << m_selectedFiles;
            return true;
        }
    }

    return QWidget::eventFilter(obj, event);
}
