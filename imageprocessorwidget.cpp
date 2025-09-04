#include <QMessageBox>
#include <QDirIterator>
#include "imageprocessorwidget.h"
#include "ui_imageprocessorwidget.h"

#include "common_tools/common_tool_func.h"
#include "logger/logger.h"
#include "img_proc_common.h"
#include "syssettings.h"
#include "literal_strings/literal_strings.h"

typedef struct
{
    img_proc_filter_mode_e_t mode;
    const char* str;
}img_proc_filter_combobox_item_s_t;

bool img_fns_list_s_t::operator ==(const img_fns_list_s_t &other) const
{
    return fn_png == other.fn_png;
}
img_fns_list_s_t::img_fns_list_s_t(QString png, QString png8bit, QString raw,
                             int width, int height,  QWidget *cw)
{
    fn_png = png; fn_png8bit = png8bit; fn_raw = raw;
    this->width = width;
    this->height = height;

    cellWidget = cw;
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

static const char* gs_thumbnail_wgt_name = "thumbCellWidget";

ImageProcessorWidget::ImageProcessorWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ImageProcessorWidget)
{
    ui->setupUi(this);

    qRegisterMetaType<img_proc_filter_mode_e_t>("img_proc_filter_mode_e_t");

    for(size_t i = 0; i < ARRAY_COUNT(img_proc_filter_combox_list); ++i)
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

    /*--------------------*/
    m_stitch_dirc_rbtn_grp = new QButtonGroup(this);
    m_stitch_dirc_rbtn_grp->addButton(ui->stitchHoriRBtn);
    m_stitch_dirc_rbtn_grp->addButton(ui->stitchVertRBtn);
    ui->stitchHoriRBtn->setChecked(true);
    ui->stitchChkBox->setTristate(true);

    /*--------------------*/
    ui->compareChkBox->setTristate(true);

    /*--------------------*/
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
    m_img_viewr = new ImageViewerWidget(m_img_info_lbl, this, m_img_with_info_wgt);
    m_img_with_info_vbox_layout->addWidget(m_img_viewr);
    m_img_with_info_vbox_layout->addWidget(m_img_info_lbl);

    m_img_with_info_vbox_layout_2 = new QVBoxLayout(m_img_with_info_wgt_2);
    m_img_info_lbl_2 = new QLabel(m_img_with_info_wgt_2);
    m_img_viewr_2 = new ImageViewerWidget(m_img_info_lbl_2, this, m_img_with_info_wgt_2);
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
    m_thumnail_style = QString("#") + gs_thumbnail_wgt_name + " {border: 2px solid red; }";
    /*--------------------*/
    uncheck_op_rbtns();
    refresh_ctrls_state();
}

ImageProcessorWidget::~ImageProcessorWidget()
{
    delete ui;
}

void ImageProcessorWidget::refresh_ctrls_state()
{
    bool dt_ctrl_enabled = (ui->imgFilterComboBox->currentData() != IMG_PROC_FILTER_ALL);
    ui->startDTEdit->setEnabled(dt_ctrl_enabled);
    ui->stopDTEdit->setEnabled(dt_ctrl_enabled);
}

void ImageProcessorWidget::on_imgFilterComboBox_currentIndexChanged(int /*index*/)
{
    refresh_ctrls_state();
}

void ImageProcessorWidget::on_imgFilterConfPBtn_clicked()
{
    ui->imgViewStackedWgt->setCurrentWidget(m_thumbnail_scroll_area);
    ui->returnToThumbListPBtn->setEnabled(false);

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
    QStringList img_dirs =
    {
        g_sys_settings_blk.img_save_path,
        g_sys_settings_blk.stitched_img_save_path
    };
    QList<QFileInfo> fileList;
    for(const QString &img_dir : img_dirs)
    {
        QDirIterator it(img_dir, QStringList() << QString("*%1").arg(g_str_img_png_type),
                         QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext())
        {
            QString filePath = it.next();
            QFileInfo fi(filePath);

            // 用正则过滤文件名
            if (!m_filter_fn_reg.match(fi.fileName()).hasMatch())
                continue;

            // 分支 1: 显示所有
            if (ui->imgFilterComboBox->currentData() == IMG_PROC_FILTER_ALL)
            {
                fileList.append(fi);
            }
            // 分支 2: 时间范围过滤
            else {
                QDateTime created_dt = fi.lastModified();

                if (created_dt >= start_dt && created_dt <= stop_dt) {
                    fileList.append(fi);
                }
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
    img_fns_list_s_t fns = img_fns_list_s_t(fn_png, fn_png8bit, fn_raw,
                                            ori_img_width, ori_img_height, cellWidget);

    // 1) 双击查看大图
    if (event->type() == QEvent::MouseButtonDblClick) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            m_selectedFiles.clear();
            m_selectedFiles.append(fns);
            go_display_one_big_img(QImage());
            return true;
        }
    }
    // 2) 左键点击选择（单选 / Ctrl 多选）
    else if (event->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton)
        {

            bool multi_sel = can_multi_sel_thumbnail();

            if (!multi_sel) {
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
                cellWidget->setObjectName(gs_thumbnail_wgt_name);
                cellWidget->setStyleSheet(m_thumnail_style);
            }

            return true;
        }
    }

    return QWidget::eventFilter(obj, event);
}

void ImageProcessorWidget::uncheck_op_rbtns(bool clear_sel_files)
{
    m_op_rbtn_grp->setExclusive(false);
        ui->translateRBtn->setChecked(false);
        ui->markRBtn->setChecked(false);
        ui->delMarkRBtn->setChecked(false);
        ui->bri_contr_RBtn->setChecked(false);
    m_op_rbtn_grp->setExclusive(true);

    set_compare_op_chkbox_st(Qt::Unchecked);
    if(clear_sel_files) clear_all_selected_files();
}

void ImageProcessorWidget::go_display_one_big_img(const QImage &img)
{
    ui->returnToThumbListPBtn->setEnabled(true);

    m_img_with_info_wgt->setVisible(true);
    m_img_with_info_wgt_2->setVisible(false);

    ui->imgViewStackedWgt->setCurrentWidget(m_img_view_container_wgt);
    if(img.isNull())
    {
        if(m_selectedFiles.size() <= 0)
        {
            DIY_LOG(LOG_ERROR, "no image file to display.");
            return;
        }
        m_img_viewr->loadImage(m_selectedFiles[0].fn_raw,
                               m_selectedFiles[0].width, m_selectedFiles[0].height);
    }
    else
    {
        m_img_viewr->loadImage(img);
    }
}

void ImageProcessorWidget::go_display_parallel_imgs()
{
    ui->returnToThumbListPBtn->setEnabled(true);

    m_img_with_info_wgt->setVisible(true);
    m_img_with_info_wgt_2->setVisible(true);

    ui->imgViewStackedWgt->setCurrentWidget(m_img_view_container_wgt);
    m_img_viewr->loadImage(m_selectedFiles[0].fn_raw, m_selectedFiles[0].width, m_selectedFiles[0].height);
    m_img_viewr_2->loadImage(m_selectedFiles[1].fn_raw, m_selectedFiles[1].width, m_selectedFiles[1].height);
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

void ImageProcessorWidget::on_restoreImgPBtn_clicked()
{
    if(m_img_viewr) m_img_viewr->resetImage();
    if(m_img_viewr_2) m_img_viewr_2->resetImage();
}

void ImageProcessorWidget::on_leftRotatePBtn_clicked()
{
    if(m_img_viewr) m_img_viewr->rotateLeft90();
    if(m_img_viewr_2) m_img_viewr_2->rotateLeft90();
}

void ImageProcessorWidget::on_rightRotatePBtn_clicked()
{
    if(m_img_viewr) m_img_viewr->rotateRight90();
    if(m_img_viewr_2) m_img_viewr_2->rotateRight90();
}

void ImageProcessorWidget::on_horiFlipPBtn_clicked()
{
    if(m_img_viewr) m_img_viewr->flipHorizontal();
    if(m_img_viewr_2) m_img_viewr_2->flipHorizontal();
}


void ImageProcessorWidget::on_verFlipPbtn_clicked()
{
    if(m_img_viewr) m_img_viewr->flipVertical();
    if(m_img_viewr_2) m_img_viewr_2->flipVertical();
}

void ImageProcessorWidget::on_clearOpflagsPBtn_clicked()
{
    if(m_img_viewr) m_img_viewr->clear_op_flags();
    if(m_img_viewr_2) m_img_viewr_2->clear_op_flags();

    uncheck_op_rbtns();
}

void ImageProcessorWidget::on_enlargePBtn_clicked()
{
    if(m_img_viewr) m_img_viewr->zoomIn();
    if(m_img_viewr_2) m_img_viewr_2->zoomIn();
}

void ImageProcessorWidget::on_shrinkPBtn_clicked()
{
    if(m_img_viewr) m_img_viewr->zoomOut();
    if(m_img_viewr_2) m_img_viewr_2->zoomOut();
}

void ImageProcessorWidget::clear_all_selected_files()
{
    for(const img_fns_list_s_t &sel : std::as_const(m_selectedFiles))
    {
        if(sel.cellWidget)
        {
            sel.cellWidget->setStyleSheet("");
        }
    }
    m_selectedFiles.clear();
}

void ImageProcessorWidget::on_compareChkBox_clicked()
{
    if(curr_proc_st() == IMG_PROC_IMG_VIEW)
    {
        DIY_LOG(LOG_WARN, "In img view state, compare op has no meaning.");
        return;
    }
    Qt::CheckState cur_state = get_compare_op_chkbox_last_st();

    if(m_selectedFiles.size() >= 2)
    {
        if(Qt::Checked == cur_state)
        {
            clear_all_selected_files();
            set_compare_op_chkbox_st(Qt::Unchecked);
        }
        else
        {
            go_display_parallel_imgs();
            set_compare_op_chkbox_st(Qt::Checked);
        }
    }
    else
    {
        QMessageBox::information(this, "", g_str_plz_sel_two_files);
        set_compare_op_chkbox_st(Qt::PartiallyChecked);
    }
}

void ImageProcessorWidget::on_stitchChkBox_clicked()
{
    if(curr_proc_st() == IMG_PROC_IMG_VIEW)
    {
        DIY_LOG(LOG_WARN, "In img view state, compare op has no meaning.");
        return;
    }

    Qt::CheckState cur_state = get_stitch_op_chkbox_last_st();

    if(m_selectedFiles.size() >= 2)
    {
        if(Qt::Checked == cur_state)
        {
            clear_all_selected_files();
            set_stitch_op_chkbox_st(Qt::Unchecked);
        }
        else
        {
            display_stitched_image();
            set_stitch_op_chkbox_st(Qt::Checked);
        }
    }
    else
    {
        QMessageBox::information(this, "", g_str_plz_sel_at_least_two_files);
        set_stitch_op_chkbox_st(Qt::PartiallyChecked);
    }
}

img_processor_st_e_t ImageProcessorWidget::curr_proc_st()
{
    if(ui->imgViewStackedWgt->currentWidget() == m_thumbnail_scroll_area)
    {
        return IMG_PROC_THUMBNAIL_LIST;
    }
    else
    {
        return IMG_PROC_IMG_VIEW;
    }
}

bool ImageProcessorWidget::can_multi_sel_thumbnail()
{
    bool ctrlPressed = QApplication::keyboardModifiers() & Qt::ControlModifier;
    return ctrlPressed
           || (ui->compareChkBox->checkState() == Qt::PartiallyChecked)
           || (ui->stitchChkBox->checkState() == Qt::PartiallyChecked);
}

void ImageProcessorWidget::on_returnToThumbListPBtn_clicked()
{
    ui->imgViewStackedWgt->setCurrentWidget(m_thumbnail_scroll_area);
    ui->returnToThumbListPBtn->setEnabled(false);

    uncheck_op_rbtns(false);
}

Qt::CheckState ImageProcessorWidget::get_compare_op_chkbox_last_st()
{
    return m_compare_op_chkbox_last_st;
}

void ImageProcessorWidget::set_compare_op_chkbox_st(Qt::CheckState new_st)
{
    m_compare_op_chkbox_last_st = ui->compareChkBox->checkState();
    ui->compareChkBox->setCheckState(new_st);
}

Qt::CheckState ImageProcessorWidget::get_stitch_op_chkbox_last_st()
{
    return m_stitch_op_chkbox_last_st;
}

void ImageProcessorWidget::set_stitch_op_chkbox_st(Qt::CheckState new_st)
{
    m_stitch_op_chkbox_last_st = ui->stitchChkBox->checkState();
    ui->stitchChkBox->setCheckState(new_st);
}

void ImageProcessorWidget::get_latest_op_flags(bool &tr, bool &mark, bool &del_mark,
                                               bool &br_con)
{
    tr = ui->translateRBtn->isChecked();
    mark = ui->markRBtn->isChecked();
    del_mark = ui->delMarkRBtn->isChecked();
    br_con = ui->bri_contr_RBtn->isChecked();
}

QImage ImageProcessorWidget::stitch_images()
{
    if(m_selectedFiles.size() < 2)
    {
        QMessageBox::information(this, "", g_str_plz_sel_at_least_two_files);
        return QImage();
    }

    bool hori_stitch = ui->stitchHoriRBtn->isChecked();
    const int bpp = 2; //bytes per pix

    QImage stitched_img;
    if(hori_stitch)
    {
        int height = m_selectedFiles[0].height;
        int width = m_selectedFiles[0].width;
        for(int i = 1; i < m_selectedFiles.size(); ++i)
        {
            if(m_selectedFiles[i].height != height)
            {
                QMessageBox::critical(this, "", g_str_img_height_should_be_identical);
                return QImage();
            }
            width += m_selectedFiles[i].width;
        }
        QImage hori_stitched_img(width, height, QImage::Format_Grayscale16);
        int bpl_of_tgt = hori_stitched_img.bytesPerLine();
        uchar* dst_bits = hori_stitched_img.bits();
        for(int x_pos = 0, i = 0; i < m_selectedFiles.size(); ++i)
        {
            QImage curr_img = read_gray_raw_img(m_selectedFiles[i].fn_raw, m_selectedFiles[i].width,
                                                m_selectedFiles[i].height, QImage::Format_Grayscale16);
            if(curr_img.isNull())
            {
                DIY_LOG(LOG_ERROR, QString("load raw data file %1 error.")
                                            .arg(m_selectedFiles[i].fn_raw));
                return QImage();
            }
            int actual_blp_of_src = curr_img.width() * bpp;
            for(int y = 0; y < height; ++y)
            {
                memcpy(dst_bits + y * bpl_of_tgt + x_pos * bpp, curr_img.scanLine(y),
                       actual_blp_of_src);
            }
            x_pos += curr_img.width();
        }
        stitched_img = hori_stitched_img;
    }
    else
    {
        int height = m_selectedFiles[0].height;
        int width = m_selectedFiles[0].width;
        for(int i = 1; i < m_selectedFiles.size(); ++i)
        {
            if(m_selectedFiles[i].width != width)
            {
                QMessageBox::critical(this, "", g_str_img_width_should_be_identical);
                return QImage();
            }
            height += m_selectedFiles[i].height;
        }
        QImage vert_stitched_img(width, height, QImage::Format_Grayscale16);
        int bpl_of_tgt = vert_stitched_img.bytesPerLine();
        uchar* dst_bits = vert_stitched_img.bits();
        for(int y_pos = 0, i = 0; i < m_selectedFiles.size(); ++i)
        {
            QImage curr_img = read_gray_raw_img(m_selectedFiles[i].fn_raw, m_selectedFiles[i].width,
                                                m_selectedFiles[i].height, QImage::Format_Grayscale16);
            if(curr_img.isNull())
            {
                DIY_LOG(LOG_ERROR, QString("load raw data file %1 error.")
                                            .arg(m_selectedFiles[i].fn_raw));
                return QImage();
            }
            int actual_blp_of_src = curr_img.width() * bpp;
            for(int y = 0; y < curr_img.height(); ++y)
            {
                memcpy(dst_bits + (y + y_pos) * bpl_of_tgt, curr_img.scanLine(y),
                       actual_blp_of_src);
            }
            y_pos += curr_img.height();
        }
        stitched_img = vert_stitched_img;
    }

    return stitched_img;
}

void ImageProcessorWidget::display_stitched_image()
{
    QImage stitched_img = stitch_images();

    if(stitched_img.isNull()) return;

    save_stitched_image(stitched_img);
    go_display_one_big_img(stitched_img);
}

void ImageProcessorWidget::save_stitched_image(QImage &img)
{
    QString parent_dir;
    QStringList fn_list;

    get_saved_img_name_or_pat(&parent_dir, &fn_list, nullptr);
    QString curr_path = g_sys_settings_blk.stitched_img_save_path + "/" + parent_dir;
    if(!chk_mk_pth_and_warn(curr_path, this))
    {
        return;
    }

    img.save(curr_path + "/" + fn_list[IMG_TYPE_PNG]);

    QImage img8bit = convertGrayscale16To8(img);
    img8bit.save(curr_path + "/" + fn_list[IMG_TYPE_8BIT_PNG]);

    write_gray_raw_img(curr_path + "/" + fn_list[IMG_TYPE_RAW], img);
}
