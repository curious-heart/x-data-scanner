#ifndef SYSSETTINGS_H
#define SYSSETTINGS_H

#include <QString>

typedef struct
{
    QString img_save_path;
    int conn_data_src_timeout_sec;
    int max_scan_dura_sec;
    int max_recvd_line_number;
}sys_settings_block_s_t;

extern sys_settings_block_s_t g_sys_settings_blk;

#endif // SYSSETTINGS_H
