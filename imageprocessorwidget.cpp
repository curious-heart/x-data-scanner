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

bool _img_fns_list::operator ==(const img_fns_list_s_t &other)
{
    return fn_png == other.fn_png;
}
_img_fns_list::_img_fns_list(QString png, QString png8bit, QString raw, int width, int height)
{
    fn_png = png; fn_png8bit = png8bit; fn_raw = raw;
    this->width = width;
    this->height = height;
}

static const img_proc_filter_combobox_item_s_t img_proc_filter_combox_list[] =
{
    {IMG_PROC_FILTER_ALL, "所有"},
    {IMG_PROC_FILTER_DT_RANGE, "按时间范围"},
};

static const char* gs_thumbnail_prop_png_fp_name = "file_png";
static const char* gs_thumbnail_prop_png8bit_fp_name = "file_png8bit_path";
static const char* gs_thumbnail_prop_raw_fp_name = "file_raw_path";
static const char* gs_thumbnail_prop_base_f_name = "file_base_name";
static const char* gs_thumbnail_prop_width_name = "img_width";
static const char* gs_thumbnail_prop_height_name = "img_height";

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
    m_filter_fn_reg = pat_list[IMG_TYPE_8BIT_PNG];

    /*--------------------*/
    m_sort_rbtn_grp = new QButtonGroup(this);
    m_sort_rbtn_grp->addButton(ui->newFirstRBtn);
    m_sort_rbtn_grp->addButton(ui->oldFirstRBtn);

    /*--------------------*/
     m_op_rbtn_grp= new QButtonGroup(this);
    m_op_rbtn_grp->addButton(ui->translateRBtn);
    m_op_rbtn_grp->addButton(ui->markRBtn);
    m_op_rbtn_grp->addButton(ui->bri_contr_RBtn);
    m_op_rbtn_grp->addButton(ui->delMarkRBtn);

    /*setup widges for thumnail display*/
    m_thumbnail_scroll_area = new QScrollArea(this);
    m_thumbnail_container_wgt = new QWidget;
    m_thumbnail_layout = new QGridLayout(m_thumbnail_container_wgt);
    m_thumbnail_scroll_area->setWidget(m_thumbnail_container_wgt);
    m_thumbnail_scroll_area->setWidgetResizable(true);

    /*setup widges for image view and process*/
    m_img_view_container_wgt = new QWidget(this);
    m_img_view_hbox_layout = new QHBoxLayout(m_img_view_container_wgt);
    m_img_view_container_wgt->setLayout(m_img_view_hbox_layout);

    m_img_with_info_wgt = new QWidget(m_img_view_container_wgt);
    m_img_with_info_wgt_2 = new QWidget(m_img_view_container_wgt);
    m_img_view_hbox_layout->addWidget(m_img_with_info_wgt);
    m_img_view_hbox_layout->addWidget(m_img_with_info_wgt_2);

    m_img_with_info_vbox_layout = new QVBoxLayout(m_img_with_info_wgt);
    m_img_info_lbl = new QLabel(m_img_with_info_wgt);
    m_img_viewr = new ImageViewerWidget(m_img_info_lbl, m_img_with_info_wgt);
    m_img_with_info_vbox_layout->addWidget(m_img_viewr);
    m_img_with_info_vbox_layout->addWidget(m_img_info_lbl);

    m_img_with_info_vbox_layout_2 = new QVBoxLayout(m_img_with_info_wgt_2);
    m_img_info_lbl_2 = new QLabel(m_img_with_info_wgt_2);
    m_img_viewr_2 = new ImageViewerWidget(m_img_info_lbl_2, m_img_with_info_wgt_2);
    m_img_with_info_vbox_layout_2->addWidget(m_img_viewr_2);
    m_img_with_info_vbox_layout_2->addWidget(m_img_info_lbl_2);

    QFontMetrics fm(m_img_info_lbl->font());
    int lbl_h = fm.height();
    m_img_info_lbl->setFixedHeight(lbl_h);
    m_img_info_lbl_2->setFixedHeight(lbl_h);
    /*--------------------*/
    ui->imgViewStackedWgt->addWidget(m_thumbnail_scroll_area);
    ui->imgViewStackedWgt->addWidget(m_img_view_container_wgt);

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
    // the img display: use 8bit png. refer to m_filter_fn_reg value.
    int row = 0, col = 0, maxCol = 5;
    for (const QFileInfo &fi : fileList) {
        QString fp_8bit = fi.filePath();
        QString fp_png = img_name_convert(IMG_NAME_8BIT_TO_PNG, fp_8bit),
                fp_raw = img_name_convert(IMG_NAME_8BIT_TO_RAW, fp_8bit),
                fp_base = QFileInfo(fp_png).baseName();
        if((fp_png == fp_8bit) || !QFileInfo::exists(fp_png))
        {
            DIY_LOG(LOG_WARN, QString("%1 exist, but png file %2 does not exist.").arg(fp_8bit, fp_png));
            continue;
        }
        if((fp_raw == fp_8bit) || !QFileInfo::exists(fp_raw))
        {
            DIY_LOG(LOG_WARN, QString("%1 exist, but png file %2 does not exist.").arg(fp_8bit, fp_raw));
            continue;
        }

        QPixmap pixmap(fp_8bit);
        int ori_img_width = pixmap.width(), ori_img_height = pixmap.height();
        QLabel *thumbLabel = new QLabel;
        thumbLabel->setPixmap(pixmap.scaled(100, 100, Qt::KeepAspectRatio, Qt::FastTransformation));
        thumbLabel->setAlignment(Qt::AlignCenter);
        thumbLabel->setProperty(gs_thumbnail_prop_png8bit_fp_name, fp_8bit);
        thumbLabel->setProperty(gs_thumbnail_prop_png_fp_name, fp_png);
        thumbLabel->setProperty(gs_thumbnail_prop_raw_fp_name, fp_raw);
        thumbLabel->setProperty(gs_thumbnail_prop_base_f_name, fp_base);
        thumbLabel->setProperty(gs_thumbnail_prop_width_name, ori_img_width);
        thumbLabel->setProperty(gs_thumbnail_prop_height_name, ori_img_height);
        thumbLabel->setAttribute(Qt::WA_Hover);
        thumbLabel->installEventFilter(this);

        QLabel *nameLabel = new QLabel(fp_base);
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

    QString fn_png = thumbLabel->property(gs_thumbnail_prop_png_fp_name).toString();
    QString fn_png8bit = thumbLabel->property(gs_thumbnail_prop_png8bit_fp_name).toString();
    QString fn_raw = thumbLabel->property(gs_thumbnail_prop_raw_fp_name).toString();
    int ori_img_width = thumbLabel->property(gs_thumbnail_prop_width_name).toInt();
    int ori_img_height = thumbLabel->property(gs_thumbnail_prop_height_name).toInt();
    img_fns_list_s_t fns = img_fns_list_s_t(fn_png, fn_png8bit, fn_raw, ori_img_width, ori_img_height);

    // 1) 双击查看大图
    if (event->type() == QEvent::MouseButtonDblClick) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            m_selectedFiles.clear();
            m_selectedFiles.append(fns);
            go_display_one_big_img();
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
            if (m_selectedFiles.contains(fns)) {
                // 取消选中
                m_selectedFiles.removeAll(fns);
                cellWidget->setStyleSheet("");
            } else {
                // 选中
                m_selectedFiles.append(fns);
                static const char* ls_thumb_wgt_name = "thumbCellWidget";
                cellWidget->setObjectName(ls_thumb_wgt_name);
                cellWidget->setStyleSheet(QString("#") + ls_thumb_wgt_name+ " {border: 2px solid red; }");
            }

            return true;
        }
    }

    return QWidget::eventFilter(obj, event);
}

void ImageProcessorWidget::go_display_one_big_img()
{
    m_img_with_info_wgt->setVisible(true);
    m_img_with_info_wgt_2->setVisible(false);

    ui->imgViewStackedWgt->setCurrentWidget(m_img_view_container_wgt);
    m_img_viewr->loadImage(m_selectedFiles[0].fn_raw, m_selectedFiles[0].width, m_selectedFiles[0].height);
}

void ImageProcessorWidget::go_display_parallel_imgs()
{
    m_img_with_info_wgt->setVisible(true);
    m_img_with_info_wgt_2->setVisible(true);

    ui->imgViewStackedWgt->setCurrentWidget(m_img_view_container_wgt);
}

void ImageProcessorWidget::on_translateRBtn_toggled(bool checked)
{
    if(m_img_viewr) m_img_viewr->translate(checked);
    if(m_img_viewr_2) m_img_viewr_2->translate(checked);
}


void ImageProcessorWidget::on_bri_contr_RBtn_toggled(bool checked)
{
    if(m_img_viewr) m_img_viewr->bright_contrast(checked);
    if(m_img_viewr_2) m_img_viewr_2->bright_contrast(checked);
}

