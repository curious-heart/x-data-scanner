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

extern const char* g_str_img_raw_type;
extern const char* g_str_img_png_type;
extern const char* g_str_8bit_img_apx;

void get_saved_img_name_or_pat(QString *parent_dir, QStringList * fn_list,
                               QList<QRegularExpression> * fn_pat_list);

#endif // IMG_PROC_COMMON_H
