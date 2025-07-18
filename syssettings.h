#ifndef SYSSETTINGS_H
#define SYSSETTINGS_H

#include <QString>

typedef struct
{
    bool valid;
    int tube_volt_kV;
    double tube_current_mA, expo_dura_ms;

    double ui_to_sw_current_factor, ui_to_sw_dura_factor;
    double sw_to_dev_current_factor, sw_to_dev_dura_factor;
}hv_params_s_t;

typedef struct
{
    QString img_save_path;
    int conn_data_src_timeout_sec;
    int max_scan_dura_sec;
    int max_recvd_line_number;

    hv_params_s_t hv_params;
}sys_settings_block_s_t;

extern sys_settings_block_s_t g_sys_settings_blk;

#endif // SYSSETTINGS_H
