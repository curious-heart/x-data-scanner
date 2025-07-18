#ifndef MODBUS_REGS_H
#define MODBUS_REGS_H

#include <QObject>
#include <QMap>

#define MB_REG_ENUM \
{\
    C(HSV = 0),                            /*软硬件版本*/\
    C(OTA = 1),                            /*OTA升级*/\
    C(BaudRate = 2),                       /*波特率*/\
    C(ServerAddress = 3),                  /*设备地址*/\
    C(State = 4),                          /*状态*/\
    C(VoltSet = 5),                        /*5管电压设置值*/\
    C(FilamentSet = 6),                    /*6 管电流设置值*/\
    C(ExposureTime = 7),                   /*曝光时间*/\
    C(Voltmeter = 8),                      /*管电压读出值*/\
    C(Ammeter = 9),                        /*管电流读出值*/\
    C(RangeIndicationStatus = 10),         /*范围指示状态*/\
    C(ExposureStatus = 11),                /*曝光状态*/\
    C(RangeIndicationStart = 12),          /*范围指示启动*/\
    C(ExposureStart = 13),                 /*曝光启动*/\
\
    C(MAX_HV_NORMAL_MB_REG_NUM),\
}
#undef C
#define C(a) a
typedef enum MB_REG_ENUM hv_mb_reg_e_t;

#define START_EXPO_DATA 2
#define STOP_EXPO_DATA 0

typedef QMap<hv_mb_reg_e_t, quint16> mb_reg_val_map_t;
Q_DECLARE_METATYPE(mb_reg_val_map_t)

#endif // MODBUS_REGS_H
