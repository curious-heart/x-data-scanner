#ifndef SYSCONFIGS_H
#define SYSCONFIGS_H

#include <QString>
#include "common_tools/common_tool_func.h"
#include "modbus_regs.h"

//__ARGS__ should be empty or as form "=5"
#define ENUM_NAME_DEF(e, ...) e __VA_ARGS__,
#define MB_TUBE_CURRENT_UNIT_E \
    ENUM_NAME_DEF(MB_TUBE_CURRENT_UNIT_UA)\
    ENUM_NAME_DEF(MB_TUBE_CURRENT_UNIT_MA)\
    ENUM_NAME_DEF(MB_TUBE_CURRENT_UNIT_A)
typedef enum
{
    MB_TUBE_CURRENT_UNIT_E
}mb_tube_current_unit_e_t;

#define MB_DURA_UNIT_E \
    ENUM_NAME_DEF(MB_DURA_UNIT_MS) \
    ENUM_NAME_DEF(MB_DURA_UNIT_SEC) \
    ENUM_NAME_DEF(MB_DURA_UNIT_MIN)
typedef enum
{
    MB_DURA_UNIT_E
}mb_dura_unit_e_t;

#define UI_DISP_TUBE_OR_OILBOX_E \
    ENUM_NAME_DEF(UI_DISP_TUBE_NO) \
    ENUM_NAME_DEF(UI_DISP_OILBOX_NO)
typedef enum
{
    UI_DISP_TUBE_OR_OILBOX_E
}ui_disp_tube_or_oilbox_str_e_t;

#define MB_TRIPLE_W_START_REG_E \
    ENUM_NAME_DEF(MB_TRIPLE_W_START_REG_CUR, =1) \
    ENUM_NAME_DEF(MB_TRIPLE_W_START_REG_DR, =ExposureStart)
typedef enum
{
    MB_TRIPLE_W_START_REG_E
}mb_triple_w_start_reg_e_t;

#define HV_EXPO_S_AND_E_MODE_E \
    ENUM_NAME_DEF(HV_EXPO_S_AND_E_MODE_SET_TRIPLE) \
    ENUM_NAME_DEF(HV_EXPO_S_AND_E_MODE_SET_OP_REG)
typedef enum
{
    HV_EXPO_S_AND_E_MODE_E
}hv_expo_s_and_e_mode_e_t;

typedef struct
{
    QString com_port_s;
    int boudrate, databits, parity, stopbits;
}serial_port_params_struct_t;

typedef struct
{
    serial_port_params_struct_t serial_params;
    int resp_wait_time_ms;
    int srvr_address;
    int mb_retry_times;
}modbus_conn_parameters_struct_t;

typedef struct
{
    int log_level;

    double cool_dura_factor;
    int extra_cool_time_ms;
    int expo_prepare_time_ms, consec_rw_wait_ms;

    int tube_volt_kv_min;
    int tube_volt_kv_max;
    double tube_current_ma_min;
    double tube_current_ma_max;
    double dura_ms_min;
    double dura_ms_max;

    double coil_current_a_min, coil_current_a_max;

    int mb_reconnect_wait_ms, mb_err_retry_wait_ms;
    int test_time_stat_grain_sec;
    int mb_one_cmd_round_time_ms;

    mb_tube_current_unit_e_t mb_tube_current_intf_unit, ui_current_unit;
    mb_dura_unit_e_t mb_dura_intf_unit, ui_mb_dura_unit;
    double sw_to_dev_extra_factor_volt, sw_to_dev_extra_factor_current, sw_to_dev_extra_factor_dura;

    int test_proc_monitor_period_ms;

    int distance_group_disp, sw_ver_disp, hw_ver_disp, hv_ctrl_board_no_disp;
    ui_disp_tube_or_oilbox_str_e_t tube_or_oilbox_no_disp;
    int test_params_settings_disp, pause_test_disp;

    int max_pt_number, all_bytes_per_pt, pkt_idx_byte_cnt;

    int expo_to_coll_max_allowed_delay_ms, expo_to_coll_min_allowed_delay_ms;
    int pb_monitor_period_ms, pb_self_chk_to_ms;
    bool pb_monitor_log;

    int scrn_w, scrn_h;

    int scan_dura_allowed_min_sec, scan_dura_allowed_max_sec;
    int conn_data_src_tmo_allowed_min_sec, conn_data_src_tmo_allowed_max_sec;
    bool limit_recvd_line_number;

    QString data_src_ip;
    int data_src_port;

    serial_port_params_struct_t pb_sport_params;
    modbus_conn_parameters_struct_t x_ray_mb_conn_params;
    bool sniffer_hv_sport;

    bool enable_self_check;
    bool skip_pwr_self_chk, skip_x_src_self_chk, skip_detector_self_chk, skip_storage_self_chk;
    bool enable_pb_monitor, enable_hv_monitor, enable_hv_auto_reconn;

    bool disable_monitor_during_scan;

    gray_pixel_data_type def_scan_bg_value;
    double def_scan_stre_factor_value;

    mb_triple_w_start_reg_e_t mb_triple_w_start_reg;
    hv_expo_s_and_e_mode_e_t hv_expo_s_and_e_mode;

    bool scan_without_x;
    int hv_monitor_period_ms;
}sys_configs_struct_t;

extern sys_configs_struct_t g_sys_configs_block;

bool fill_sys_configs(QString *);

#endif // SYSCONFIGS_H
