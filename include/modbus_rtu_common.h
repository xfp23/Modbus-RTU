#ifndef MODBUS_RTU_COMMON_H
#define MODBUS_RTU_COMMON_H

#ifdef __cplusplus
extern "C"
{
#endif

#define RTU_CHECKPTR(ptr) do \
{ \
   if(ptr == NULL) \
   return RTU_ERR; \
} while (0)

#define RTU_GETBIT(x, byte) ((byte >> x) & 0x01)

#ifdef __cplusplus
}
#endif

#endif