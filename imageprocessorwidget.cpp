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

    /*--------------------*/
    m_sort_rbtn_grp = new QButtonGroup(this);
    m_sort_rbtn_grp->addButton(ui->newFirstRBtn);
    m_sort_rbtn_grp->addButton(ui->oldFirstRBtn);

    /*setup widges for thumnail display*/
    m_thumbnail_scroll_area = new QScrollArea(this);
    m_thumbnail_container_wgt = new QWidget;
    m_thumbnail_layout = new QGridLayout(m_thumbnail_container_wgt);
    m_thumbnail_scroll_area->setWidget(m_thumbnail_container_wgt);
    m_thumbnail_scroll_area->setWidgetResizable(true);

    /*--------------------*/
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
            QDateTime created_dt = fi.lastModified();
            DIY_LOG(LOG_INFO, "time is");
        }
        // 分支 2: 时间范围过滤
        else {
            QDateTime created_dt = fi.lastModified();

            if (created_dt >= start_dt && created_dt <= stop_dt) {
                fileList.append(fi);
            }
        }
    }

    // 3) 按创建时间排序
    if (ui->newFirstRBtn->isChecked()) {
        std::sort(fileList.begin(), fileList.end(),
                  [](const QFileInfo &a, const QFileInfo &b){
                      return a.lastModified() > b.lastModified(); // 新文件在前
                  });
    } else {
        std::sort(fileList.begin(), fileList.end(),
                  [](const QFileInfo &a, const QFileInfo &b){
                      return a.lastModified() < b.lastModified(); // 老文件在前
                  });
    }

    // 4) 显示缩略图 + 文件名
    int row = 0, col = 0, maxCol = 5;
    for (const QFileInfo &fi : fileList) {
        QPixmap pixmap(fi.filePath());
        QLabel *thumbLabel = new QLabel;
        thumbLabel->setPixmap(pixmap.scaled(100, 100, Qt::KeepAspectRatio, Qt::FastTransformation));
        thumbLabel->setAlignment(Qt::AlignCenter);
        thumbLabel->setProperty("filePath", fi.filePath());
        thumbLabel->setAttribute(Qt::WA_Hover);
        thumbLabel->installEventFilter(this);

        QLabel *nameLabel = new QLabel(fi.fileName());
        nameLabel->setAlignment(Qt::AlignCenter);

        // 使用一个 QWidget + QVBoxLayout 包裹缩略图和文件名
        QWidget *cellWidget = new QWidget;
        QVBoxLayout *cellLayout = new QVBoxLayout(cellWidget);
        cellLayout->setContentsMargins(2,2,2,2);
        cellLayout->addWidget(thumbLabel);
        cellLayout->addWidget(nameLabel);

        m_thumbnail_layout->addWidget(cellWidget, row, col++);
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

    // 获取 cellWidget
    QWidget *cellWidget = thumbLabel->parentWidget();
    if (!cellWidget) return QWidget::eventFilter(obj, event);

    QString filePath = thumbLabel->property("filePath").toString();

    // 1) 双击查看大图
    if (event->type() == QEvent::MouseButtonDblClick) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            // TODO: 打开大图显示
            qDebug() << "Double clicked:" << filePath;
            return true;
        }
    }
    // 2) 左键点击选择（单选 / Ctrl 多选）
    else if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {

            bool ctrlPressed = QApplication::keyboardModifiers() & Qt::ControlModifier;

            if (!ctrlPressed) {
                // 单选：先清除所有高亮
                for (QObject *child : m_thumbnail_container_wgt->children()) {
                    QWidget *cw = qobject_cast<QWidget*>(child);
                    if (cw) {
                        cw->setStyleSheet("");
                    }
                }
                m_selectedFiles.clear();
            }

            // 切换当前 cell 高亮
            if (m_selectedFiles.contains(filePath)) {
                // 取消选中
                m_selectedFiles.removeAll(filePath);
                cellWidget->setStyleSheet("");
            } else {
                // 选中
                m_selectedFiles.append(filePath);
                cellWidget->setStyleSheet("border: 2px solid red; background-color: rgba(255,0,0,30);");
            }

            qDebug() << "Selected files:" << m_selectedFiles;
            return true;
        }
    }

    return QWidget::eventFilter(obj, event);
}
