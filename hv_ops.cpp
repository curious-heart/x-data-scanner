#include "modbus_regs.h"
#include "scanwidget.h"

#undef HV_OP_ITEM
#define HV_OP_ITEM(op) #op
static const char* gs_hv_op_name_list[] =
{
    HV_OP_LIST
};

void ScanWidget::hv_construct_mb_du(hv_op_enum_t op, QModbusDataUnit &mb_du)
{
    /*
    QVector<quint16> mb_reg_vals;

    switch(op)
    {
        case HV_OP_SET_EXPO_TRIPLE:
            mb_reg_vals.append((quint16)(hv_curr_expo_param_triple.cube_volt_kv));
            mb_reg_vals.append((quint16)(hv_HV_params->expo_param_block.sw_to_mb_current_factor
                                         * hv_curr_expo_param_triple.cube_current_ma));
            mb_reg_vals.append((quint16)(hv_HV_params->expo_param_block.sw_to_mb_dura_factor
                                         * hv_curr_expo_param_triple.dura_ms));

            mb_du.setStartAddress(VoltSet);
            mb_du.setValues(mb_reg_vals);
            break;

        case HV_OP_START_EXPO:
            mb_reg_vals.append(get_start_expo_cmd_from_HV_method());
            mb_du.setStartAddress(ExposureStart);
            mb_du.setValues(mb_reg_vals);
            emit begin_exposure_sig(true);
            break;

        case HV_OP_READ_REGS:
            mb_du.setStartAddress(HSV);
            mb_du.setValueCount(MAX_HV_NORMAL_MB_REG_NUM);
            break;

        case HV_OP_READ_DISTANCE:
            mb_du.setStartAddress(EXT_MB_REG_DISTANCE);
            mb_du.setValueCount(1);
            break;

        default:
            DIY_LOG(LOG_ERROR, QString("%1 %2").arg(gs_str_unexpected_HVer_op,
                                                    QString::number(op)));
            emit begin_exposure_sig(false);
            return;
    }
    */
}
