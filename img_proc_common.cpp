#include "common_tools/common_tool_func.h"
#include "img_proc_common.h"

const char* g_str_img_raw_type = ".raw";
const char* g_str_img_png_type = ".png";
const char* g_str_8bit_img_apx = "-8bit";

void get_saved_img_name_or_pat(QString *parent_dir, QStringList * fn_list,
                               QList<QRegularExpression> * fn_pat_list)
{
    QString curr_date_str = common_tool_get_curr_date_str();
    if(parent_dir) *parent_dir = curr_date_str;

    QString curr_dt_str = common_tool_get_curr_dt_str();
    if(fn_list)
    {
        //the order MUST accord with saved_img_type_e_t
        fn_list->append(curr_dt_str + g_str_img_png_type);
        fn_list->append(curr_dt_str + g_str_8bit_img_apx + g_str_img_png_type);
        fn_list->append(curr_dt_str + g_str_img_raw_type);
    }

    /* Note: the re MUST be according to strings format in fn_list */
    if(fn_pat_list)
    {
        QString pat_prefix = QString("^\\d{14}");

        fn_pat_list->append(QRegularExpression(pat_prefix
                             + QRegularExpression::escape(g_str_img_png_type) + "$"));
        fn_pat_list->append(QRegularExpression(pat_prefix
         + QRegularExpression::escape(QString(g_str_8bit_img_apx) + g_str_img_png_type) + "$"));
        fn_pat_list->append(QRegularExpression(pat_prefix
                             + QRegularExpression::escape(g_str_img_raw_type) + "$"));
    }
}

QString img_name_convert(img_name_convert_op_e_t op, QString now_fpn)
{
    // Note: the rx should accord with name format in get_saved_img_name_or_pat
    QRegularExpression png_rx(QRegularExpression::escape(g_str_img_png_type)
                                  + "$", QRegularExpression::CaseInsensitiveOption);
    QRegularExpression png8bit_rx(QRegularExpression::escape(QString(g_str_8bit_img_apx) + g_str_img_png_type)
                                  + "$", QRegularExpression::CaseInsensitiveOption);
    QRegularExpression raw_rx(QRegularExpression::escape(g_str_img_raw_type)
                                  + "$", QRegularExpression::CaseInsensitiveOption);

    QString new_fpn = now_fpn;
    switch(op)
    {
        case IMG_NAME_PNG_TO_8BIT:
            new_fpn.replace(png_rx, g_str_8bit_img_apx);
            new_fpn += g_str_img_png_type;
            break;

        case IMG_NAME_8BIT_TO_PNG:
            new_fpn.replace(png8bit_rx, g_str_img_png_type);
            break;

        case IMG_NAME_PNG_TO_RAW:
            new_fpn.replace(png_rx, g_str_img_raw_type);
            break;

        case IMG_NAME_RAW_TO_PNG:
            new_fpn.replace(raw_rx, g_str_img_png_type);
            break;

        case IMG_NAME_8BIT_TO_RAW:
            new_fpn.replace(png8bit_rx, g_str_img_raw_type);
            break;

        case IMG_NAME_RAW_TO_8BIT:
            new_fpn.replace(raw_rx, g_str_8bit_img_apx);
            new_fpn += g_str_img_png_type;
            break;

        default:
            break;
    }
    return new_fpn;
}
