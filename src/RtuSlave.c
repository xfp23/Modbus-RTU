#include "stdlib.h"
#include "string.h"
#include "RtuSlave.h"

static uint16_t CRC16(const uint8_t *buf, size_t len)
{
    uint16_t crc = 0xFFFF;
    for (uint16_t pos = 0; pos < len; pos++)
    {
        crc ^= (uint16_t)buf[pos];
        for (int i = 0; i < 8; i++)
        {
            if (crc & 0x0001)
            {
                crc >>= 1;
                crc ^= 0xA001;
            }
            else
            {
                crc >>= 1;
            }
        }
    }
    return crc;
}

static void rtufree_register_list(RTU_Register_t *node)
{
    while (node)
    {
        RTU_Register_t *next = node->next;
        free(node);
        node = next;
    }
}

static int rtubuild_register_list(RTU_Register_t *head, RTU_RegisterMap_t *map, size_t count)
{
    head->next = NULL;

    if (map == NULL || count == 0)
        return 0; // nothing to build, not an error

    RTU_Register_t *prev = NULL;
    for (size_t i = 0; i < count; ++i)
    {
        RTU_Register_t *node = (RTU_Register_t *)calloc(1, sizeof(RTU_Register_t));
        if (node == NULL)
        {
            if (head->next)
                rtufree_register_list(head->next);
            head->next = NULL;
            return -1;
        }

        node->address = map[i].addr;
        node->value = map[i].data;
        node->next = NULL;

        if (prev)
            prev->next = node;
        else
            head->next = node;

        prev = node;
    }

    return 0;
}

RTU_Sta_t RTUSlave_Init(RTU_Slavehandle_t *handle, RtuSlave_Conf_t *conf)
{
    if (handle == NULL || conf == NULL)
        return RTU_ERR;

    if (*handle != NULL)
        return RTU_ERR;

    RTU_SlaveObj *obj = (RTU_SlaveObj *)calloc(1, sizeof(RTU_SlaveObj));
    if (obj == NULL)
    {
        free(obj->buf);
        return RTU_ERR;
    }

    obj->id = conf->id;
    obj->buf_size = conf->buf_size;
    obj->transmit = conf->transmit;

    obj->buf = (uint8_t *)calloc(1, sizeof(uint8_t) * obj->buf_size);
    int init_count = 0;
    int ret = 0;

    ret = rtubuild_register_list(&obj->coils, conf->coils.map, conf->coils.count);
    if (ret < 0)
    {
        free(obj);
        return RTU_ERR;
    }
    if (conf->coils.map != NULL && conf->coils.count > 0)
        init_count++;

    ret = rtubuild_register_list(&obj->holdingRegs, conf->holdingRegs.map, conf->holdingRegs.count);
    if (ret < 0)
    {
        rtufree_register_list(obj->coils.next);
        free(obj);
        return RTU_ERR;
    }
    if (conf->holdingRegs.map != NULL && conf->holdingRegs.count > 0)
        init_count++;

    ret = rtubuild_register_list(&obj->writeRegs, conf->writeRegs.map, conf->writeRegs.count);
    if (ret < 0)
    {
        rtufree_register_list(obj->coils.next);
        rtufree_register_list(obj->holdingRegs.next);
        free(obj);
        return RTU_ERR;
    }
    if (conf->writeRegs.map != NULL && conf->writeRegs.count > 0)
        init_count++;

    if (init_count == 0)
    {
        free(obj);
        return RTU_ERR;
    }

    *handle = obj;
    return RTU_OK;
}

void RTU_Deinit(RTU_Slavehandle_t *handle)
{
    if (handle == NULL || *handle == NULL)
        return;

    RTU_SlaveObj *obj = *handle;

    if (obj->coils.next)
        rtufree_register_list(obj->coils.next);
    if (obj->holdingRegs.next)
        rtufree_register_list(obj->holdingRegs.next);
    if (obj->writeRegs.next)
        rtufree_register_list(obj->writeRegs.next);
    if (obj->buf)
        free(obj->buf);

    obj->coils.next = NULL;
    obj->holdingRegs.next = NULL;
    obj->writeRegs.next = NULL;

    free(obj);
    *handle = NULL;
}

static RTU_Register_t *rtu_find_node(RTU_Register_t *head, uint16_t addr)
{
    if (head == NULL)
        return NULL;
    RTU_Register_t *cur = head->next;
    while (cur)
    {
        if (cur->address == addr)
            return cur;
        cur = cur->next;
    }
    return NULL;
}

RTU_Sta_t RTUSlave_TimerHandler(RTU_Slavehandle_t handle, uint8_t *frame, size_t size)
{
    if (handle == NULL || frame == NULL)
        return RTU_ERR;

    if (size < 8)
        return RTU_ERR;

    if (frame[0] != handle->id)
        return RTU_ERR;

    uint16_t recv_crc = (uint16_t)frame[size - 2] | ((uint16_t)frame[size - 1] << 8);
    if (recv_crc != CRC16(frame, size - 2))
        return RTU_ERR;

    uint8_t func = frame[1];
    uint16_t regAddr = ((uint16_t)frame[2] << 8) | frame[3];
    uint16_t reqNum = ((uint16_t)frame[4] << 8) | frame[5];
    RTU_Sta_t ret = RTU_ERR;

    size_t resp_len = 0;

    switch (func)
    {
    case RTU_FUNC_READ_COILS:
    {
        /* validate quantity */
        if (reqNum == 0 || reqNum > 2000)
        {
            return RTU_ERR;
        }

        size_t byte_count = (reqNum + 7) / 8;
        size_t needed = 1 + 1 + 1 + byte_count + 2; // id+func+bytecount+data+crc
        if (needed > handle->buf_size)
        {
            return RTU_ERR;
        }

        handle->buf[0] = handle->id;
        handle->buf[1] = RTU_FUNC_READ_COILS;
        handle->buf[2] = (uint8_t)byte_count;

        /* find starting node */
        RTU_Register_t *node = rtu_find_node(&handle->coils, regAddr);
        if (node == NULL)
        {
            return RTU_ERR;
        }

        for (uint16_t i = 0; i < reqNum; ++i)
        {
            /* expected address of this coil */
            uint16_t expect_addr = regAddr + i;
            if (node == NULL || node->address != expect_addr)
            {
                /* address gap -> protocol error (Illegal Data Address) */
                // free(resp);
                return RTU_ERR;
            }

            uint8_t bit = 0;
            if (node->value)
            {
                /* interpret value as uint8_t or bool: non-zero => 1 */
                bit = ((*((uint8_t *)node->value)) != 0) ? 1u : 0u;
            }

            size_t byte_index = 3 + (i >> 3);
            size_t bit_index = i & 0x07;
            handle->buf[byte_index] |= (uint8_t)(bit << bit_index);

            node = node->next;
        }

        /* append CRC */
        size_t crc_pos = 3 + byte_count;
        uint16_t crc = CRC16(handle->buf, crc_pos);
        handle->buf[crc_pos + 0] = (uint8_t)(crc & 0x00FF);        // CRC low
        handle->buf[crc_pos + 1] = (uint8_t)((crc >> 8) & 0x00FF); // CRC high

        resp_len = crc_pos + 2;
        handle->transmit(handle->buf, resp_len);

        ret = RTU_READ;
        break;
    }

    case RTU_FUNC_READ_HOLD_REGS:
    {
        /* validate quantity */
        if (reqNum == 0 || reqNum > 125)
        { /* free(resp);*/
            return RTU_ERR;
        } // Modbus limit 125 regs

        size_t byte_count = reqNum * 2;
        size_t needed = 1 + 1 + 1 + byte_count + 2;
        if (needed > handle->buf_size)
        { /*free(resp)*/
            ;
            return RTU_ERR;
        }

        handle->buf[0] = handle->id;
        handle->buf[1] = RTU_FUNC_READ_HOLD_REGS;
        handle->buf[2] = (uint8_t)byte_count;

        RTU_Register_t *node = rtu_find_node(&handle->holdingRegs, regAddr);
        if (node == NULL)
        {
            return RTU_ERR;
        }

        for (uint16_t i = 0; i < reqNum; ++i)
        {
            uint16_t expect_addr = regAddr + i;
            if (node == NULL || node->address != expect_addr)
            {
                // free(resp);
                return RTU_ERR;
            }

            uint16_t val = 0;
            if (node->value)
            {
                val = *((uint16_t *)node->value);
            }

            size_t off = 3 + i * 2;
            handle->buf[off + 0] = (uint8_t)((val >> 8) & 0xFF); // High byte first
            handle->buf[off + 1] = (uint8_t)(val & 0xFF);        // Low byte

            node = node->next;
        }

        size_t crc_pos = 3 + byte_count;
        uint16_t crc = CRC16(handle->buf, crc_pos);
        handle->buf[crc_pos + 0] = (uint8_t)(crc & 0x00FF);
        handle->buf[crc_pos + 1] = (uint8_t)((crc >> 8) & 0x00FF);

        resp_len = crc_pos + 2;
        handle->transmit(handle->buf, resp_len);
        ret = RTU_READ;
        break;
    }

    case RTU_FUNC_WRITE_SINGLE_REG:
    {
        RTU_Register_t *node = rtu_find_node(&handle->writeRegs, regAddr);
        if (node == NULL)
        {
            ;
            return RTU_ERR;
        }

        uint16_t value = ((uint16_t)frame[4] << 8) | frame[5];
        if (node->value)
        {
            *((uint16_t *)node->value) = value;
        }

        handle->transmit(frame, size);
        resp_len = size;
        ret = RTU_WRITE;
        break;
    }

    case RTU_FUNC_MASK_WRITE_REG:
    {

        RTU_Register_t *node = rtu_find_node(&handle->writeRegs, regAddr);
        if (node == NULL)
        {
            return RTU_ERR;
        }

        handle->transmit(frame, size);
        resp_len = size;
        ret = RTU_WRITE;
        break;
    }

    default:
        return RTU_ERR;
    }

    memset(handle->buf, 0, handle->buf_size);
    return RTU_OK;
}
