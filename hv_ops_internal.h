#ifndef HV_OPS_INTERNAL_H
#define HV_OPS_INTERNAL_H

#define HV_OP_ITEM(op) op
#define HV_OP_LIST \
    HV_OP_ITEM(HV_OP_NULL),\
    HV_OP_ITEM(HV_OP_SET_EXPO_TRIPLE),\
    HV_OP_ITEM(HV_OP_START_EXPO),\
    HV_OP_ITEM(HV_OP_READ_REGS),

typedef enum
{
    HV_OP_LIST
}hv_op_enum_t;

#endif // HV_OPS_INTERNAL_H
