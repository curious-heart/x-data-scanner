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

