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

#include "modbus_rtu_types.h"

#ifdef __cplusplus
extern "C"
{
#endif


RTU_Sta_t RTU_Init(RTU_handle_t *handle,RTU_Conf_t *conf);

#ifdef __cplusplus
}
#endif

#endif