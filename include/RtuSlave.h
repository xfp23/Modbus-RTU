/**
 * @file modbus_rtu.h
 * @author https://github.com/xfp23
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

#define RTU_GETMAPSIZE(x) sizeof(x) / sizeof(RTU_RegisterMap_t) 

/**
 * @brief 初始化RTU从机
 * 
 * @param handle 句柄
 * @param conf 配置
 * @return RTU_Sta_t 
 */
extern RTU_Sta_t RTUSlave_Init(RTUSlave_handle_t *handle,RtuSlave_Conf_t *conf);

/**
 * @brief 卸载RTU从机
 * 
 * @param handle 句柄
 */
extern void RTU_Deinit(RTUSlave_handle_t *handle);

/**
 * @brief 从机主调函数
 * 
 * @param handle 句柄
 * @param frame 接收到的主机数据帧
 * @param size 数据帧大小 单位 : 字节
 * @return RTU_Sta_t 
 */
extern RTU_Sta_t RTUSlave_TimerHandler(RTUSlave_handle_t handle, uint8_t *frame, size_t size);

/**
 * @brief 修改从机id
 * 
 * @param handle 
 * @param id 
 * @return RTU_Sta_t 
 */
extern RTU_Sta_t RTUSlave_Modifyid(RTUSlave_handle_t handle,uint8_t id);

#ifdef __cplusplus
}
#endif

#endif