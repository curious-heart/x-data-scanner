#include "literal_strings/literal_strings.h"
#include "modbus_regs.h"
#include "scanwidget.h"
#include "sysconfigs/sysconfigs.h"
#include "syssettings.h"

#undef HV_OP_ITEM
#define HV_OP_ITEM(op) #op
static const char* gs_hv_op_name_list[] =
{
    HV_OP_LIST
};

#define GET_HV_OP_NAME_STR(op) \
    (((HV_OP_NULL <= (op)) && ((op) <= HV_OP_STOP_EXPO)) ?\
        gs_hv_op_name_list[(op)] :  g_str_unknown_hv_op)

static const int gs_mb_reg_read_cnt = 8;

bool ScanWidget::hv_construct_mb_du(hv_op_enum_t op, QModbusDataUnit &mb_du)
{
    QVector<quint16> mb_reg_vals;
    quint16 tube_volt = (quint16)(g_sys_settings_blk.hv_params.tube_volt_kV),
            tube_current = (quint16)(g_sys_settings_blk.hv_params.sw_to_dev_current_factor
                                         * g_sys_settings_blk.hv_params.tube_current_mA),
            expo_dura = (quint16)(g_sys_settings_blk.hv_params.sw_to_dev_dura_factor
                                         * g_sys_settings_blk.hv_params.expo_dura_ms);
    QString log_str;
    LOG_LEVEL log_lvl = LOG_INFO;
    bool ret = true;

    quint16 mb_triple_w_start_reg_no = (quint16)g_sys_configs_block.mb_triple_w_start_reg;

    if(HV_EXPO_S_AND_E_MODE_SET_TRIPLE == g_sys_configs_block.hv_expo_s_and_e_mode)
    {
        switch(op)
        {
            case HV_OP_STOP_EXPO:
                tube_volt = tube_current = expo_dura = 0;
            case HV_OP_START_EXPO:
            case HV_OP_SET_EXPO_TRIPLE: //normal this may not be used here.
                mb_reg_vals.append(tube_volt);
                mb_reg_vals.append(tube_current);
                mb_reg_vals.append(expo_dura);

                mb_du.setStartAddress(mb_triple_w_start_reg_no);
                mb_du.setValues(mb_reg_vals);

                log_str = QString("%1：%2； %3：%4； %5：%6\n")
                          .arg(g_str_tube_volt, QString::number(tube_volt),
                              g_str_tube_current, QString::number(tube_current),
                              g_str_expo_dura, QString::number(expo_dura));
                break;


            case HV_OP_READ_REGS:
                mb_du.setStartAddress(State);
                mb_du.setValueCount(gs_mb_reg_read_cnt);
                break;

            default:
                ret = false;
                log_lvl = LOG_ERROR;
                log_str = QString("%1 %2").arg(g_str_unknown_hv_op, QString::number(op));
                break;
        }
    }
    else //HV_EXPO_S_AND_E_MODE_SET_OP_REG
    {
        switch(op)
        {
            case HV_OP_SET_EXPO_TRIPLE:
                mb_reg_vals.append(tube_volt);
                mb_reg_vals.append(tube_current);
                mb_reg_vals.append(expo_dura);

                mb_du.setStartAddress(mb_triple_w_start_reg_no);
                mb_du.setValues(mb_reg_vals);

                log_str = QString("%1：%2； %3：%4； %5：%6\n")
                          .arg(g_str_tube_volt, QString::number(tube_volt),
                              g_str_tube_current, QString::number(tube_current),
                              g_str_expo_dura, QString::number(expo_dura));
                break;

            case HV_OP_START_EXPO:
                mb_reg_vals.append(START_EXPO_DATA);
                mb_du.setStartAddress(ExposureStart);
                mb_du.setValues(mb_reg_vals);
                break;

            case HV_OP_READ_REGS:
                mb_du.setStartAddress(State);
                mb_du.setValueCount(gs_mb_reg_read_cnt);
                break;

            case HV_OP_STOP_EXPO:
                mb_reg_vals.append(STOP_EXPO_DATA);
                mb_du.setStartAddress(ExposureStart);
                mb_du.setValues(mb_reg_vals);
                break;

            default:
                ret = false;
                log_lvl = LOG_ERROR;
                log_str = QString("%1 %2").arg(g_str_unknown_hv_op, QString::number(op));
                break;
        }
    }

    if(!log_str.isEmpty())
    {
        DIY_LOG(log_lvl, log_str);
    }
    return ret;
}

bool ScanWidget::hv_send_op_cmd(hv_op_enum_t op)
{
    QModbusDataUnit mb_du(QModbusDataUnit::HoldingRegisters);
    QModbusReply *mb_reply;
    QString log_str;
    LOG_LEVEL log_lvl = LOG_INFO;
    bool ret = true;

    m_hv_curr_op = op;

    if(!hv_construct_mb_du(op, mb_du)) return false;

    switch(op)
    {
        case HV_OP_SET_EXPO_TRIPLE:
        case HV_OP_START_EXPO:
        case HV_OP_STOP_EXPO:
            mb_reply = m_hv_device->sendWriteRequest(mb_du,
                                         g_sys_configs_block.x_ray_mb_conn_params.srvr_address);
            log_str = QString("hv_op: %1").arg(gs_hv_op_name_list[op]);
            break;

        case HV_OP_READ_REGS:
            mb_reply = m_hv_device->sendReadRequest(mb_du,
                                         g_sys_configs_block.x_ray_mb_conn_params.srvr_address);
            log_str = QString("hv_op: %1").arg(gs_hv_op_name_list[op]);
            break;

        default:
            ret = false;
            log_lvl = LOG_ERROR;
            log_str = QString("%1 %2").arg(g_str_unknown_hv_op, QString::number(op));
            break;
    }

    DIY_LOG(log_lvl, log_str);

    if(ret)
    {
        mb_rw_reply_received(op, mb_reply, &ScanWidget::mb_op_finished_sig_handler, true, false);
    }

    return ret;
}

void ScanWidget::mb_rw_reply_received(hv_op_enum_t op, QModbusReply* mb_reply,
                                    void (ScanWidget::*finished_sig_handler)(),
                                    bool sync, bool err_notify)
{
    QString err_str;

    if(!mb_reply)
    {
        DIY_LOG(LOG_ERROR, g_str_mb_op_null_reply);
        return;
    }

    if(!sync || mb_reply->isFinished())
    {//sync and finished; or, async, including finished and error

        QModbusDevice::Error err = mb_reply->error();
        if(QModbusDevice::NoError == err)
        {
            if(HV_OP_READ_REGS == op)
            {
                QModbusDataUnit rb_du = mb_reply->result();
                if(!rb_du.isValid())
                {
                    emit hv_op_finish_sig(false, g_str_mb_read_regs_invalid);
                    DIY_LOG(LOG_ERROR, g_str_mb_read_regs_invalid);
                }
                else
                {
                    err_str = "mb regs read: ";

                    m_regs_read_result.clear();
                    int st_addr = rb_du.startAddress(), idx = 0, cnt = rb_du.valueCount();
                    hv_mb_reg_e_t reg_no;
                    quint16 reg_v;
                    for(; idx < cnt; ++idx)
                    {
                        reg_no = hv_mb_reg_e_t(st_addr + idx);
                        reg_v = rb_du.value(idx);
                        m_regs_read_result.insert(reg_no, reg_v);

                        err_str += QString("%1->%2; ").arg((int)reg_no).arg(reg_v);
                    }
                    emit mb_regs_read_ret_sig(m_regs_read_result);

                    emit hv_op_finish_sig(true);
                    DIY_LOG(LOG_INFO, err_str);
                }
            }
            else
            {
                emit hv_op_finish_sig(true);
            }
        }
        else
        {
            err_str = mb_reply->errorString();
            err_str += err_notify ? " (from error sig hadnler)" : " (from finish sig hadnler)";
            emit hv_op_finish_sig(false, err_str);

            DIY_LOG(LOG_ERROR, err_str);
        }
    }
    else
    {//sync op, and not finished.
        if(finished_sig_handler)
        {
            connect(mb_reply, &QModbusReply::finished,
                        this, finished_sig_handler, Qt::QueuedConnection);
        }

        connect(mb_reply, &QModbusReply::errorOccurred,
                    this, &ScanWidget::mb_rw_error_sig_handler, Qt::QueuedConnection);
    }
    return;
}

void ScanWidget::mb_op_finished_sig_handler()
{
    QModbusReply * mb_reply = qobject_cast<QModbusReply *>(sender());
    DIY_LOG(LOG_INFO, QString("mb_op_finished_sig_handler: ") + GET_HV_OP_NAME_STR(m_hv_curr_op));
    mb_rw_reply_received(m_hv_curr_op, mb_reply, nullptr, false, false);
    if(mb_reply)
    {
        mb_reply->deleteLater();
    }
}

void ScanWidget::mb_rw_error_sig_handler(QModbusDevice::Error error)
{
    QModbusReply * mb_reply = qobject_cast<QModbusReply *>(sender());
    QString err_str = mb_reply ? mb_reply->errorString() : "";
    DIY_LOG(LOG_INFO, QString("HV op %1 mb_rw_error_sig_handler: %2 ").
            arg(GET_HV_OP_NAME_STR(m_hv_curr_op)).arg(error) + err_str);
    mb_rw_reply_received(m_hv_curr_op, mb_reply, nullptr, false, true);
    if(mb_reply)
    {
        mb_reply->deleteLater();
    }
}
