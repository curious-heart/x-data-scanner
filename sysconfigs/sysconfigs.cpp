﻿#include <QSettings>
#include <QSerialPort>

#include "logger/logger.h"
#include "sysconfigs/sysconfigs.h"
#include "literal_strings/literal_strings.h"

#undef ENUM_NAME_DEF
#define ENUM_NAME_DEF(e) <<e

static const char* gs_sysconfigs_file_fpn = "configs/configs.ini";

static const char* gs_ini_grp_sys_cfgs = "sys_cfgs";
static const char* gs_ini_key_log_level = "log_level";

static const char* gs_ini_grp_expo_ctrl = "expo_ctrl";
static const char* gs_ini_key_cool_dura_factor = "cool_dura_factor";
static const char* gs_ini_key_extra_cool_time_ms = "extra_cool_time_ms";
static const char* gs_ini_key_expo_prepare_time_ms = "expo_prepare_tims_ms";
static const char* gs_ini_key_mb_consec_rw_wait_ms = "mb_consec_rw_wait_ms";
static const char* gs_ini_key_mb_err_retry_wait_ms = "mb_err_retry_wait_ms";
static const char* gs_ini_key_mb_one_cmd_round_time_ms = "mb_one_cmd_round_time_ms";

static const char* gs_ini_key_cube_volt_kv_min = "cube_volt_kv_min";
static const char* gs_ini_key_cube_volt_kv_max = "cube_volt_kv_max";
static const char* gs_ini_key_cube_current_ma_min = "cube_current_ma_min";
static const char* gs_ini_key_cube_current_ma_max = "cube_current_ma_max";
static const char* gs_ini_key_dura_sec_min = "dura_sec_min";
static const char* gs_ini_key_dura_sec_max = "dura_sec_max";
static const char* gs_ini_key_mb_reconnect_wait_sep_ms = "mb_reconnect_wait_sep_ms";
static const char* gs_ini_key_test_time_stat_grain_sec = "test_time_stat_grain_sec";

static const char* gs_ini_key_coil_current_a_min = "coil_current_a_min";
static const char* gs_ini_key_coil_current_a_max = "coil_current_a_max";

static const char* gs_ini_key_mb_cube_current_intf_unit = "mb_cube_current_intf_unit";
static const char* gs_ini_key_ui_current_unit = "ui_current_unit";

static const char* gs_ini_key_mb_dura_intf_unit = "mb_dura_intf_unit";
static const char* gs_ini_key_hidden_ui_mb_dura_unit = "hidden_ui_mb_dura_unit";

static const char* gs_ini_key_test_proc_monitor_period_ms = "test_proc_monitor_period_ms";
static const char* gs_ini_key_mb_srv_addr = "mb_srv_addr";
static const char* gs_ini_key_mb_resp_wait_time_ms = "mb_resp_wait_time_ms";

static const char* gs_ini_grp_ui_disp_cfg = "ui_disp_cfg";
static const char* gs_ini_key_distance_group_disp = "distance_group_disp";
static const char* gs_ini_key_sw_ver_disp = "sw_ver_disp";
static const char* gs_ini_key_hw_ver_disp = "hw_ver_disp";
static const char* gs_ini_key_hv_ctrl_board_no_disp = "hv_ctrl_board_no_disp";
static const char* gs_ini_key_tube_or_oilbox_no_disp = "tube_or_oilbox_no_disp";
static const char* gs_ini_key_test_params_settings_disp = "test_params_settings_disp";
static const char* gs_ini_key_pause_test_disp = "pause_test_disp";

static const char* gs_ini_grp_sc_data_cfg = "sc_data_cfg";
static const char* gs_ini_key_data_src_ip = "data_src_ip";
static const char* gs_ini_key_data_src_port = "data_src_port";
static const char* gs_ini_key_max_pt_number = "max_pt_number";
static const char* gs_ini_key_all_bytes_per_pt = "all_bytes_per_pt";
static const char* gs_ini_key_pkt_idx_byte_cnt = "pkt_idx_byte_cnt";
static const char* gs_ini_key_expo_to_coll_delay_def_ms = "expo_to_coll_delay_def_ms";
static const char* gs_ini_key_expo_to_coll_delay_max_ms = "expo_to_coll_delay_max_ms";
static const char* gs_ini_key_scrn_w = "scrn_w";
static const char* gs_ini_key_scrn_h = "scrn_h";
static const char* gs_ini_key_allowed_max_scan_dura_sec = "allowed_max_scan_dura_sec";
static const char* gs_ini_key_limit_recvd_line_number = "limit_recvd_line_number";
static const char* gs_ini_key_enable_self_check = "enable_self_check";
static const char* gs_ini_key_disable_monitor_during_scan = "disable_monitor_during_scan";
static const char* gs_ini_key_def_scan_bg_value = "def_scan_bg_value";
static const char* gs_ini_key_def_scan_stre_factor_value = "def_scan_stre_factor_value";

static const char* gs_ini_grp_pb_set_and_monitor_cfg = "pb_set_and_monitor_cfg";
static const char* gs_ini_key_pb_monitor_period_ms = "pb_monitor_period_ms";
static const char* gs_ini_key_pb_monitor_log = "pb_monitor_log";

static const char* gs_ini_key_sport_name = "sport_name";
static const char* gs_ini_key_sport_baudrate = "sport_baudrate";
static const char* gs_ini_key_sport_databits = "sport_databits";
static const char* gs_ini_key_sport_parity = "sport_parity";
static const char* gs_ini_key_sport_stopbits = "sport_stopbits";

sys_configs_struct_t g_sys_configs_block;

static const int gs_def_log_level = LOG_ERROR;

static const int gs_def_cube_volt_kv_min = 40;
static const int gs_def_cube_volt_kv_max = 90;
static const float gs_def_cube_current_ma_min = 0.5;
static const float gs_def_cube_current_ma_max = 5;
static const float gs_def_dura_sec_min = 0.5;
static const float gs_def_dura_sec_max = (float)1.4;

static const float gs_def_coil_current_a_min = 0;
static const float gs_def_coil_current_a_max = 2;

static const float gs_def_cool_dura_factor = 30;
static const int gs_def_extra_cool_time_ms = 2000;
static const int gs_def_expo_prepare_time_ms = 3500;
static const int gs_def_mb_consec_rw_wait_ms = 500;
static const int gs_def_mb_err_retry_wait_ms = 1000;
static const int gs_def_mb_reconnect_wait_sep_ms = 1000;
static const int gs_def_test_time_stat_grain_sec = 3;
static const int gs_def_mb_one_cmd_round_time_ms = 150;

static const int gs_def_test_proc_monitor_period_ms = 1000;
static const int gs_def_key_mb_srv_addr = 1;
static const int gs_def_key_mb_resp_wait_time_ms = 3000;

static const int gs_def_distance_group_disp = 1;
static const int gs_def_sw_ver_disp = 1;
static const int gs_def_hw_ver_disp = 1;
static const int gs_def_hv_ctrl_board_no_disp = 1;
static const ui_disp_tube_or_oilbox_str_e_t gs_def_tube_or_oilbox_no_disp = UI_DISP_OILBOX_NO;
static const int gs_def_test_params_settings_disp = 0;
static const int gs_def_pause_test_disp = 0;

static const char* gs_str_cfg_param_limit_error = "参数门限配置错误";
static const char* gs_str_param_in_cfg_file_error = "配置文件参数错误";
static const char* gs_str_plz_check = "请检查！";
static const char* gs_str_mb_intf_unit = "接口单位";
static const char* gs_str_mb_intf_unit_error = "接口单位配置错误";
static const char* gs_str_ui_unit_error = "ui显示单位配置错误";
static const char* gs_str_should_be_one_val_of = "应为如下值之一：";
static const char* gs_str_actual_val = "实际值";
static const char* gs_str_hidden_ui_mb_dura_unit = "GUI隐藏的曝光时间单位选项";
static const char* gs_str_cannot_be_the_same = "不能相等";

static const mb_cube_current_unit_e_t gs_def_mb_cube_current_intf_unit = MB_CUBE_CURRENT_UNIT_UA;
static const mb_cube_current_unit_e_t gs_def_ui_current_unit = MB_CUBE_CURRENT_UNIT_UA;

static const mb_dura_unit_e_t gs_def_mb_dura_intf_unit = MB_DURA_UNIT_MS;
static const mb_dura_unit_e_t gs_def_hidden_ui_mb_dura_unit = MB_DURA_UNIT_MIN;

static const char* gs_def_data_src_ip = "192.168.1.123";
static const int gs_def_data_src_port = 8020;
static const int gs_def_max_pt_number = 200, gs_def_all_bytes_per_pt = 3,
                 gs_def_pkt_idx_byte_cnt = 2;
static const int gs_def_expo_to_coll_delay_def_ms = 500,
                 gs_def_expo_to_coll_delay_max_ms = 1000,
                 gs_def_pb_monitor_period_ms = 3000;
static const int gs_def_scrn_w = 600;
static const int gs_def_scrn_h = 800;
static const int gs_allowed_max_scan_dura_sec = 30;
static const int gs_def_allowed_max_scan_dura_sec = 30; //def val should not exceed above max val
static const int gs_def_limit_recvd_line_number = 0;
static const int gs_def_enable_self_check = 1;
static const int gs_def_disable_monitor_during_scan = 1;
static const gray_pixel_data_type gs_def_def_scan_bg_value = 10;
static const double gs_def_def_scan_stre_factor_value = 2;

static const int gs_def_pb_monitor_log = false;

static const char* gs_def_sport_name_com1 = "COM1";
static const char* gs_def_sport_name_com2 = "COM2";
static const int gs_def_sport_baudrate = 115200;
static const int gs_def_sport_databits = 8;
static const int gs_def_sport_parity = (int)(QSerialPort::NoParity);
static const int gs_def_sport_stopbits = 1;

static RangeChecker<int> gs_cfg_file_log_level_ranger((int)LOG_DEBUG, (int)LOG_ERROR, "",
                     EDGE_INCLUDED, EDGE_INCLUDED);

static RangeChecker<int> gs_cfg_file_value_ge0_int_ranger(0, 0, "",
                           EDGE_INCLUDED, EDGE_INFINITE);

static RangeChecker<float> gs_cfg_file_value_ge0_float_ranger(0, 0, "",
                       EDGE_INCLUDED, EDGE_INFINITE);

static RangeChecker<int> gs_cfg_file_value_gt0_int_ranger(0, 0, "",
                       EDGE_EXCLUDED, EDGE_INFINITE);

static RangeChecker<int> gs_cfg_file_value_01_int_ranger(0, 1, "",
                       EDGE_INCLUDED, EDGE_INCLUDED);

static RangeChecker<float> gs_cfg_file_value_gt0_float_ranger(0, 0, "",
                       EDGE_EXCLUDED, EDGE_INFINITE);

static RangeChecker<int> gs_cfg_file_allow_scan_dura_sec_ranger(0, gs_allowed_max_scan_dura_sec,
                        "", EDGE_EXCLUDED, EDGE_INCLUDED);

RangeChecker<int> g_ip_port_ranger(0, 65535, "", EDGE_INCLUDED, EDGE_INCLUDED);
RangeChecker<int> g_12bitpx_value_ranger(0, g_12bitpx_max_v, "", EDGE_INCLUDED, EDGE_INCLUDED);
RangeChecker<double> g_12bitpx_stre_factor_ranger(1, g_12bitpx_max_v, "",
                       EDGE_INCLUDED, EDGE_INCLUDED);


/*the __VA_ARGS__ should be empty or a type converter like (cust_type).*/
#define GET_INF_CFG_NUMBER_VAL(settings, key, type_func, var, def, factor, checker, ...)\
{\
    (var) = __VA_ARGS__((factor) * ((settings).value((key), (def)).type_func()));\
    if((checker) && !((checker)->range_check((var))))\
    {\
        (var) = (def);\
    }\
}

#define BEGIN_INT_RANGE_CHECK(low, up, low_inc, up_inc)\
{\
    RangeChecker<int> int_range_checker((low), (up), "", low_inc, up_inc);
#define END_INT_RANGE_CHECK \
}

bool fill_sys_configs(QString * ret_str_ptr)
{
    bool ret = true, cfg_ret;
    QString ret_str;
    QSettings settings(gs_sysconfigs_file_fpn, QSettings::IniFormat);

    /*--------------------*/
    settings.beginGroup(gs_ini_grp_sys_cfgs);
    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_log_level, toInt,
                           g_sys_configs_block.log_level, gs_def_log_level,
                           1, &gs_cfg_file_log_level_ranger);
    settings.endGroup();

    /*--------------------*/
    settings.beginGroup(gs_ini_grp_expo_ctrl);

    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_cube_volt_kv_min, toInt,
                           g_sys_configs_block.cube_volt_kv_min, gs_def_cube_volt_kv_min,
                           1, (RangeChecker<int>*)0);

    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_cube_volt_kv_max, toInt,
                           g_sys_configs_block.cube_volt_kv_max, gs_def_cube_volt_kv_max,
                           1, (RangeChecker<int>*)0);

    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_cube_current_ma_min, toFloat,
                           g_sys_configs_block.cube_current_ma_min, gs_def_cube_current_ma_min,
                           1, (RangeChecker<float>*)0);

    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_cube_current_ma_max, toFloat,
                           g_sys_configs_block.cube_current_ma_max, gs_def_cube_current_ma_max,
                           1, (RangeChecker<float>*)0);

    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_dura_sec_min, toFloat,
                           g_sys_configs_block.dura_ms_min, gs_def_dura_sec_min,
                           1000, (RangeChecker<float>*)0);

    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_dura_sec_max, toFloat,
                           g_sys_configs_block.dura_ms_max, gs_def_dura_sec_max,
                           1000, (RangeChecker<float>*)0);


    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_coil_current_a_min, toFloat,
                           g_sys_configs_block.coil_current_a_min, gs_def_coil_current_a_min,
                           1, (RangeChecker<float>*)0);

    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_coil_current_a_max, toFloat,
                           g_sys_configs_block.coil_current_a_max, gs_def_coil_current_a_max,
                           1, (RangeChecker<float>*)0);


    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_cool_dura_factor, toFloat,
                           g_sys_configs_block.cool_dura_factor, gs_def_cool_dura_factor,
                           1, &gs_cfg_file_value_ge0_float_ranger);

    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_extra_cool_time_ms, toInt,
                           g_sys_configs_block.extra_cool_time_ms, gs_def_extra_cool_time_ms,
                           1, &gs_cfg_file_value_ge0_int_ranger);

    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_expo_prepare_time_ms, toInt,
                           g_sys_configs_block.expo_prepare_time_ms, gs_def_expo_prepare_time_ms,
                           1, &gs_cfg_file_value_ge0_int_ranger);

    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_mb_consec_rw_wait_ms, toInt,
                           g_sys_configs_block.consec_rw_wait_ms, gs_def_mb_consec_rw_wait_ms,
                           1, &gs_cfg_file_value_gt0_int_ranger);

    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_mb_reconnect_wait_sep_ms, toInt,
                           g_sys_configs_block.mb_reconnect_wait_ms, gs_def_mb_reconnect_wait_sep_ms,
                           1, &gs_cfg_file_value_ge0_int_ranger);

    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_mb_err_retry_wait_ms, toInt,
                       g_sys_configs_block.mb_err_retry_wait_ms, gs_def_mb_err_retry_wait_ms,
                           1, &gs_cfg_file_value_ge0_int_ranger);

    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_test_time_stat_grain_sec, toInt,
                   g_sys_configs_block.test_time_stat_grain_sec, gs_def_test_time_stat_grain_sec,
                           1, &gs_cfg_file_value_ge0_int_ranger);

    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_mb_one_cmd_round_time_ms, toInt,
                   g_sys_configs_block.mb_one_cmd_round_time_ms, gs_def_mb_one_cmd_round_time_ms,
                           1, &gs_cfg_file_value_ge0_int_ranger);

    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_mb_cube_current_intf_unit, toInt,
                           g_sys_configs_block.mb_cube_current_intf_unit,
                           gs_def_mb_cube_current_intf_unit,
                           1, (RangeChecker<int>*)0, (mb_cube_current_unit_e_t));

    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_ui_current_unit, toInt,
                           g_sys_configs_block.ui_current_unit,
                           gs_def_ui_current_unit,
                           1, (RangeChecker<int>*)0, (mb_cube_current_unit_e_t));

    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_mb_dura_intf_unit, toInt,
                           g_sys_configs_block.mb_dura_intf_unit,
                           gs_def_mb_dura_intf_unit,
                           1, (RangeChecker<int>*)0, (mb_dura_unit_e_t));

    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_hidden_ui_mb_dura_unit, toInt,
                           g_sys_configs_block.hidden_ui_mb_dura_unit,
                           gs_def_hidden_ui_mb_dura_unit,
                           1, (RangeChecker<int>*)0, (mb_dura_unit_e_t));


    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_test_proc_monitor_period_ms, toInt,
                   g_sys_configs_block.test_proc_monitor_period_ms, gs_def_test_proc_monitor_period_ms,
                           1, &gs_cfg_file_value_gt0_int_ranger);

    g_sys_configs_block.x_ray_mb_conn_params.serial_params.com_port_s
            = settings.value(gs_ini_key_sport_name, gs_def_sport_name_com1).toString();
    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_sport_baudrate, toInt,
                   g_sys_configs_block.x_ray_mb_conn_params.serial_params.boudrate, gs_def_sport_baudrate,
                           1, &gs_cfg_file_value_gt0_int_ranger);
    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_sport_databits, toInt,
                   g_sys_configs_block.x_ray_mb_conn_params.serial_params.databits, gs_def_sport_databits,
                           1, &gs_cfg_file_value_gt0_int_ranger);
    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_sport_parity, toInt,
                   g_sys_configs_block.x_ray_mb_conn_params.serial_params.parity, gs_def_sport_parity,
                           1, &gs_cfg_file_value_gt0_int_ranger);
    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_sport_stopbits, toInt,
                   g_sys_configs_block.x_ray_mb_conn_params.serial_params.stopbits, gs_def_sport_stopbits,
                           1, &gs_cfg_file_value_gt0_int_ranger);
    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_mb_srv_addr, toInt,
                   g_sys_configs_block.x_ray_mb_conn_params.srvr_address, gs_def_key_mb_srv_addr,
                           1, &gs_cfg_file_value_ge0_int_ranger);
    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_mb_resp_wait_time_ms, toInt,
                   g_sys_configs_block.x_ray_mb_conn_params.resp_wait_time_ms, gs_def_key_mb_resp_wait_time_ms,
                           1, &gs_cfg_file_value_gt0_int_ranger);

    settings.endGroup();

    /*check the validation of config parameters.*/
#define CHECK_LIMIT_RANGE(l_name, min_l, max_l, checker, unit_str) \
    cfg_ret = true; \
    if(((checker) && (!((checker)->range_check(min_l)) || !((checker)->range_check(max_l)))) \
        || ((min_l) > (max_l)))\
    {\
        cfg_ret = false;\
        ret_str += QString((l_name)) + \
                   " [" + QString::number((min_l)) + ", " + QString::number((max_l)) + "] " +\
                   (unit_str) + "\n";\
    }\
    ret = ret && cfg_ret;

    CHECK_LIMIT_RANGE(g_str_cube_volt,
                g_sys_configs_block.cube_volt_kv_min, g_sys_configs_block.cube_volt_kv_max,
                &gs_cfg_file_value_gt0_int_ranger, g_str_volt_unit_kv)

    CHECK_LIMIT_RANGE(g_str_cube_current,
                g_sys_configs_block.cube_current_ma_min, g_sys_configs_block.cube_current_ma_max,
                &gs_cfg_file_value_gt0_float_ranger, g_str_current_unit_ma)

    CHECK_LIMIT_RANGE(g_str_expo_dura,
                (g_sys_configs_block.dura_ms_min/1000), (g_sys_configs_block.dura_ms_max/1000),
                &gs_cfg_file_value_gt0_float_ranger, g_str_dura_unit_s)

    CHECK_LIMIT_RANGE(g_str_coil_current,
                g_sys_configs_block.coil_current_a_min, g_sys_configs_block.coil_current_a_max,
                &gs_cfg_file_value_ge0_float_ranger, g_str_current_unit_a)

    if(!ret)  ret_str += QString(gs_str_cfg_param_limit_error) + "," + gs_str_plz_check + "\n" + ret_str;
#undef CHECK_LIMIT_RANGE

#define CHECK_ENUM(title_str, e_v, e_set, str_func) \
    cfg_ret = true; ret_str += (ret_str.isEmpty() ? "" : "\n");\
    if(!e_set.contains(e_v))\
    {\
        cfg_ret = false;\
        ret_str += QString(title_str) + gs_str_should_be_one_val_of + "\n{";\
        auto it = e_set.constBegin();\
        while(it != e_set.constEnd()) {ret_str += str_func(*it) + ", "; ++it;}\
        ret_str.chop(2);\
        ret_str += "}\n";\
        ret_str += QString(gs_str_actual_val) + str_func(e_v) + "\n";\
    }\
    ret = ret && cfg_ret;

    QSet<mb_cube_current_unit_e_t> cube_current_unit_set;
    cube_current_unit_set MB_CUBE_CURRENT_UNIT_E;
    CHECK_ENUM((QString(g_str_current) + gs_str_mb_intf_unit_error + "," + gs_str_plz_check),
               g_sys_configs_block.mb_cube_current_intf_unit,
               cube_current_unit_set, QString::number)
    CHECK_ENUM((QString(g_str_current) + gs_str_ui_unit_error + "," + gs_str_plz_check),
               g_sys_configs_block.ui_current_unit,
               cube_current_unit_set, QString::number)

    QSet<mb_dura_unit_e_t> dura_unit_set;
    dura_unit_set MB_DURA_UNIT_E;
    CHECK_ENUM((QString(g_str_expo_dura) + gs_str_mb_intf_unit_error + "," + gs_str_plz_check),
               g_sys_configs_block.mb_dura_intf_unit,
               dura_unit_set, QString::number)
    CHECK_ENUM((QString(gs_str_hidden_ui_mb_dura_unit) + gs_str_mb_intf_unit_error + "," + gs_str_plz_check),
               g_sys_configs_block.hidden_ui_mb_dura_unit,
               dura_unit_set, QString::number)

    if(ret)
    {
        if(g_sys_configs_block.hidden_ui_mb_dura_unit == g_sys_configs_block.mb_dura_intf_unit)
        {
            ret = false;
            ret_str += QString(g_str_expo_dura) + gs_str_mb_intf_unit
                    + " " + g_str_and + " "
                    + gs_str_hidden_ui_mb_dura_unit + " " + gs_str_cannot_be_the_same;
        }
    }

    /*--------------------*/
    settings.beginGroup(gs_ini_grp_ui_disp_cfg);
    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_distance_group_disp, toInt,
                           g_sys_configs_block.distance_group_disp, gs_def_distance_group_disp,
                           1, &gs_cfg_file_value_01_int_ranger);
    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_sw_ver_disp, toInt,
                           g_sys_configs_block.sw_ver_disp, gs_def_sw_ver_disp,
                           1, &gs_cfg_file_value_01_int_ranger);
    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_hw_ver_disp, toInt,
                           g_sys_configs_block.hw_ver_disp, gs_def_hw_ver_disp,
                           1, &gs_cfg_file_value_01_int_ranger);
    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_hv_ctrl_board_no_disp, toInt,
                           g_sys_configs_block.hv_ctrl_board_no_disp, gs_def_hv_ctrl_board_no_disp,
                           1, &gs_cfg_file_value_01_int_ranger);

    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_tube_or_oilbox_no_disp, toInt,
                           g_sys_configs_block.tube_or_oilbox_no_disp,
                           gs_def_tube_or_oilbox_no_disp,
                           1, (RangeChecker<int>*)0, (ui_disp_tube_or_oilbox_str_e_t));
    QSet<ui_disp_tube_or_oilbox_str_e_t> tube_or_oilbox_no_disp_set;
    tube_or_oilbox_no_disp_set UI_DISP_TUBE_OR_OILBOX_E;
    CHECK_ENUM(QString(gs_ini_key_tube_or_oilbox_no_disp),
               g_sys_configs_block.tube_or_oilbox_no_disp, tube_or_oilbox_no_disp_set,
               QString::number)

    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_test_params_settings_disp, toInt,
                           g_sys_configs_block.test_params_settings_disp, gs_def_test_params_settings_disp,
                           1, &gs_cfg_file_value_01_int_ranger);
    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_pause_test_disp, toInt,
                           g_sys_configs_block.pause_test_disp, gs_def_pause_test_disp,
                           1, &gs_cfg_file_value_01_int_ranger);

    settings.endGroup();

    /*--------------------*/
    settings.beginGroup(gs_ini_grp_sc_data_cfg);

    g_sys_configs_block.data_src_ip
            = settings.value(gs_ini_key_data_src_ip, QString(gs_def_data_src_ip)).toString();
    //check if addr is valid
    if(!ip_addr_valid(g_sys_configs_block.data_src_ip))
    {
        ret = false;
        ret_str += QString(gs_str_param_in_cfg_file_error) + "," + gs_str_plz_check + "\n"
                    + gs_ini_key_data_src_ip + "=" + g_sys_configs_block.data_src_ip;
    }

    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_data_src_port, toInt,
                           g_sys_configs_block.data_src_port, gs_def_data_src_port,
                           1, (RangeChecker<int>*)0);
    //check if port is valid
    if(!g_ip_port_ranger.range_check(g_sys_configs_block.data_src_port))
    {
        ret = false;
        ret_str += QString(gs_str_param_in_cfg_file_error) + "," + gs_str_plz_check + "\n"
                    + gs_ini_key_data_src_port + "="
                    + QString::number(g_sys_configs_block.data_src_port);
    }

    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_max_pt_number, toInt,
                           g_sys_configs_block.max_pt_number, gs_def_max_pt_number,
                           1, &gs_cfg_file_value_gt0_int_ranger);

    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_all_bytes_per_pt, toInt,
                           g_sys_configs_block.all_bytes_per_pt, gs_def_all_bytes_per_pt,
                           1, &gs_cfg_file_value_gt0_int_ranger);

    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_pkt_idx_byte_cnt, toInt,
                           g_sys_configs_block.pkt_idx_byte_cnt, gs_def_pkt_idx_byte_cnt,
                           1, &gs_cfg_file_value_gt0_int_ranger);

    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_expo_to_coll_delay_def_ms, toInt,
                           g_sys_configs_block.expo_to_coll_delay_def_ms, gs_def_expo_to_coll_delay_def_ms,
                           1, &gs_cfg_file_value_gt0_int_ranger);

    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_expo_to_coll_delay_max_ms, toInt,
                           g_sys_configs_block.expo_to_coll_delay_max_ms, gs_def_expo_to_coll_delay_max_ms,
                           1, &gs_cfg_file_value_gt0_int_ranger);

    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_scrn_w, toInt,
                           g_sys_configs_block.scrn_w, gs_def_scrn_w,
                           1, &gs_cfg_file_value_gt0_int_ranger);
    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_scrn_h, toInt,
                           g_sys_configs_block.scrn_h, gs_def_scrn_h,
                           1, &gs_cfg_file_value_gt0_int_ranger);
    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_allowed_max_scan_dura_sec, toInt,
                           g_sys_configs_block.allowed_max_scan_dura_sec, gs_def_allowed_max_scan_dura_sec,
                           1, &gs_cfg_file_allow_scan_dura_sec_ranger);
    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_limit_recvd_line_number, toInt,
                           g_sys_configs_block.limit_recvd_line_number, gs_def_limit_recvd_line_number,
                           1, (RangeChecker<int>*)0, (bool));
    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_enable_self_check, toInt,
                           g_sys_configs_block.enable_self_check, gs_def_enable_self_check,
                           1, (RangeChecker<int>*)0, (bool));
    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_disable_monitor_during_scan, toInt,
                           g_sys_configs_block.disable_monitor_during_scan, gs_def_disable_monitor_during_scan,
                           1, (RangeChecker<int>*)0, (bool));
    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_def_scan_bg_value, toInt,
                           g_sys_configs_block.def_scan_bg_value, gs_def_def_scan_bg_value,
                           1, &g_12bitpx_value_ranger, (gray_pixel_data_type));
    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_def_scan_stre_factor_value, toDouble,
                           g_sys_configs_block.def_scan_stre_factor_value, gs_def_def_scan_stre_factor_value,
                           1, &g_12bitpx_stre_factor_ranger);
    settings.endGroup();

    if(g_sys_configs_block.expo_to_coll_delay_def_ms >
            g_sys_configs_block.expo_to_coll_delay_max_ms)
    {
        ret = false;
        ret_str += QString(gs_str_param_in_cfg_file_error) + ": "
                 + gs_ini_key_expo_to_coll_delay_def_ms + " > "
                 + gs_ini_key_expo_to_coll_delay_max_ms;
    }
    /*--------------------*/
    settings.beginGroup(gs_ini_grp_pb_set_and_monitor_cfg);
    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_pb_monitor_period_ms, toInt,
                           g_sys_configs_block.pb_monitor_period_ms, gs_def_pb_monitor_period_ms,
                           1, &gs_cfg_file_value_gt0_int_ranger);
    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_pb_monitor_log, toInt,
                           g_sys_configs_block.pb_monitor_log, gs_def_pb_monitor_log,
                           1, &gs_cfg_file_value_gt0_int_ranger);

    g_sys_configs_block.pb_sport_params.com_port_s
            = settings.value(gs_ini_key_sport_name, gs_def_sport_name_com2).toString();
    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_sport_baudrate, toInt,
                   g_sys_configs_block.pb_sport_params.boudrate, gs_def_sport_baudrate,
                           1, &gs_cfg_file_value_gt0_int_ranger);
    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_sport_databits, toInt,
                   g_sys_configs_block.pb_sport_params.databits, gs_def_sport_databits,
                           1, &gs_cfg_file_value_gt0_int_ranger);
    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_sport_parity, toInt,
                   g_sys_configs_block.pb_sport_params.parity, gs_def_sport_parity,
                           1, &gs_cfg_file_value_gt0_int_ranger);
    GET_INF_CFG_NUMBER_VAL(settings, gs_ini_key_sport_stopbits, toInt,
                   g_sys_configs_block.pb_sport_params.stopbits, gs_def_sport_stopbits,
                           1, &gs_cfg_file_value_gt0_int_ranger);
    settings.endGroup();
    /*--------------------*/

    if(ret_str_ptr) *ret_str_ptr = ret_str;
    return ret;

#undef CHECK_ENMU
}
