#ifndef IMG_PROC_COMMON_H
#define IMG_PROC_COMMON_H

#include <QStringList>
#include <QRegularExpression>

typedef enum
{
    IMG_TYPE_PNG = 0,
    IMG_TYPE_8BIT_PNG,
    IMG_TYPE_RAW,

    IMG_TYPE_MAX
}saved_img_type_e_t;

typedef enum
{
    IMG_NAME_PNG_TO_8BIT,
    IMG_NAME_8BIT_TO_PNG,

    IMG_NAME_PNG_TO_RAW,
    IMG_NAME_RAW_TO_PNG,

    IMG_NAME_8BIT_TO_RAW,
    IMG_NAME_RAW_TO_8BIT,

}img_name_convert_op_e_t;

extern const char* g_str_img_raw_type;
extern const char* g_str_img_png_type;
extern const char* g_str_8bit_img_apx;

void get_saved_img_name_or_pat(QString *parent_dir, QStringList * fn_list,
                               QList<QRegularExpression> * fn_pat_list);
QString img_name_convert(img_name_convert_op_e_t op, QString now_fpn);

#endif // IMG_PROC_COMMON_H
