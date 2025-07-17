#ifndef LITERAL_STRINGS_H
#define LITERAL_STRINGS_H

#define STR_DELCARE_STMT(var_name, lit_str) \
    extern const char* var_name;

#define LIT_STRINGS_LIST \
STR_DELCARE_STMT(g_str_tube_volt, "管电压") \
STR_DELCARE_STMT(g_str_tube_current, "管电流") \
STR_DELCARE_STMT(g_str_current, "电流") \
STR_DELCARE_STMT(g_str_expo_dura, "曝光时间") \
STR_DELCARE_STMT(g_str_coil_current, "灯丝电流") \
STR_DELCARE_STMT(g_str_volt_unit_kv, "kV") \
STR_DELCARE_STMT(g_str_current_unit_a, "A") \
STR_DELCARE_STMT(g_str_current_unit_ma, "mA") \
STR_DELCARE_STMT(g_str_current_unit_ua, "uA") \
STR_DELCARE_STMT(g_str_dura_unit_min, "min") \
STR_DELCARE_STMT(g_str_dura_unit_s, "s") \
STR_DELCARE_STMT(g_str_dura_unit_ms, "ms") \
STR_DELCARE_STMT(g_str_or, "或") \
STR_DELCARE_STMT(g_str_and, "与") \
STR_DELCARE_STMT(g_str_connected, "连接") \
STR_DELCARE_STMT(g_str_disconnected, "断开") \
STR_DELCARE_STMT(g_str_unkonw_st, "未知状态") \
STR_DELCARE_STMT(g_str_create_file, "创建文件") \
STR_DELCARE_STMT(g_str_fail, "失败") \
STR_DELCARE_STMT(g_str_create_folder, "创建文件夹") \
STR_DELCARE_STMT(g_str_row_int, "行发送间隔时间") \
STR_DELCARE_STMT(g_str_unit_ms, "ms") \
STR_DELCARE_STMT(g_str_abnormal, "异常") \
STR_DELCARE_STMT(g_str_normal, "正常") \
STR_DELCARE_STMT(g_str_pwr_st, "电源状态") \
STR_DELCARE_STMT(g_str_x_rar_source_st, "射线源状态") \
STR_DELCARE_STMT(g_str_detector_st, "探测器状态") \
STR_DELCARE_STMT(g_str_storage_st, "数据存储状态") \
STR_DELCARE_STMT(g_str_invalid_ip_addr, "无效的ip地址") \
STR_DELCARE_STMT(g_str_plz_conn_dev_firstly, "请先连接设备") \
STR_DELCARE_STMT(g_str_charging, "正在充电") \
STR_DELCARE_STMT(gs_str_sport_open_succeed, "串口打开成功") \
STR_DELCARE_STMT(gs_str_sport_open_fail, "串口打开失败") \
STR_DELCARE_STMT(g_str_modbus_exceptional_error, "异常的modbus错误") \
STR_DELCARE_STMT(g_str_modbus_unkonwn_state ,"未知的modbus连接状态")

LIT_STRINGS_LIST

#endif // LITERAL_STRINGS_H
