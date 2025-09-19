/**
 * @file modbus_rtu.h
 * @author ...
 * @brief 
 * @version 0.1
 * @date 2025-09-18
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#ifndef MODBUS_RTU_H
#define MODBUS_RTU_H

#include "RTUSlave_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define RTU_GETBIT(x, byte)   ((byte >> x) & 0x01)

#define RTU_GETBYTEH(byte)    ((byte >> 8) & 0xff) 
#define RTU_GETBYTEL(byte)    ((byte) & 0xff)

#define RTU_MERGEBYTE(H,L)    ((((uint16_t)(H) & 0xff) << 8) | ((L) & 0xff))

/**
 * @brief 
 * 
 * @param handle 
 * @param conf 
 * @return RTU_Sta_t 
 */
extern RTU_Sta_t RTUSlave_Init(RTU_Slavehandle_t *handle,RTU_Conf_t *conf);

/**
 * @brief 
 * 
 * @param handle 
 */
extern void RTU_Deinit(RTU_Slavehandle_t *handle);

/**
 * @brief 
 * 
 * @param handle 
 * @param frame 
 * @param size 
 * @return RTU_Sta_t 
 */
extern RTU_Sta_t RTUSlave_TimerHandler(RTU_Slavehandle_t handle, uint8_t *frame, size_t size);

#ifdef __cplusplus
}
#endif

#endif