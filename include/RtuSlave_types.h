/**
 * @file modbus_rtu_types.h
 * @brief Modbus RTU type definitions (limited feature set)
 * @version 0.1
 * @date 2025-09-18
 */

#ifndef MODBUS_RTU_TYPES_H
#define MODBUS_RTU_TYPES_H

#include "stdint.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum
{
    RTU_OK,
    RTU_ERR,
}RTU_Sta_t;

/* ---------------- 功能码定义 ---------------- */
typedef enum
{
    RTU_FUNC_READ_COILS = 0x01,       // 读线圈 (Read Coils)
    RTU_FUNC_READ_HOLD_REGS = 0x03,   // 读保持寄存器 (Read Holding Registers)
    RTU_FUNC_WRITE_SINGLE_REG = 0x06, // 写单个寄存器 (Write Single Register)
    RTU_FUNC_MASK_WRITE_REG = 0x16    // 寄存器按位写 (Mask Write Register)
} RTU_FunctionCode_t;

typedef struct RTU_Register
{
    uint16_t address;          // 寄存器地址
    void *value;               // 寄存器数据指针
    struct RTU_Register *next; // 下一个寄存器
} RTU_Register_t;

typedef struct
{
uint16_t addr;
void *data;
}RTU_RegisterMap_t; // 寄存器表，用户需要定义这样的数组来映射寄存器



typedef struct
{
    RTU_RegisterMap_t *map; // 链表头
    size_t count;         // 表内寄存器数量
} RTU_RegisterTable_t;

typedef int (*RTU_TransmitFn)(uint8_t *data, size_t size);
// typedef int (*RTU_ReceiveFn)(uint8_t *data, size_t size);

typedef struct
{
    uint8_t id;              // 设备id
    uint8_t buf_size;      // 数据帧大小
    RTU_TransmitFn transmit;
    RTU_RegisterTable_t coils;       // 线圈表 (功能码 0x01) 用户必须传进来的是数组
    RTU_RegisterTable_t holdingRegs; // 保持寄存器表 (功能码 0x03) 用户必须传进来的是数组
    RTU_RegisterTable_t writeRegs;   // 写寄存器表 (功能码 0x06 / 0x16) 用户必须传进来的是数组
} RTU_Conf_t;

typedef struct
{
    uint8_t id;              // 设备id
    uint8_t *buf;
    uint16_t buf_size;
    RTU_TransmitFn transmit; // 下行发送接口

    RTU_Register_t coils;       // 线圈表 (功能码 0x01)
    RTU_Register_t holdingRegs; // 保持寄存器表 (功能码 0x03)
    RTU_Register_t writeRegs;   // 写寄存器表 (功能码 0x06 / 0x16)

} RTU_SlaveObj;

typedef RTU_SlaveObj *RTU_Slavehandle_t;



#ifdef __cplusplus
}
#endif

#endif /* MODBUS_RTU_TYPES_H */
