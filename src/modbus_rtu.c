#include "stdlib.h" // calloc, free
#include "modbus_rtu_types.h"
#include "string.h"

static uint16_t CRC16(const uint8_t *buf, size_t len)
{
    uint16_t crc = 0xFFFF; // 初始值
    for (uint16_t pos = 0; pos < len; pos++)
    {
        crc ^= (uint16_t)buf[pos]; // 异或当前字节
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
    return crc; // 注意：低字节在前，高字节在后
}

/* helper: free a list of RTU_Register_t nodes (head is first real node) */
static void rtufree_register_list(RTU_Register_t *node)
{
    while (node)
    {
        RTU_Register_t *next = node->next;
        free(node);
        node = next;
    }
}

/* helper: build linked list from map array.
 * head is pointer to sentinel RTU_Register_t stored in RTU_Obj.
 * On success head->next points to first allocated node (or NULL if count==0).
 * Returns 0 on success, -1 on allocation failure.
 */
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
            // allocation failed: free already created nodes
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
            head->next = node; // first real node

        prev = node;
    }

    return 0;
}

/* RTU_Init - allocate RTU_Obj and initialize lists from conf arrays (use calloc)
 * Caller passes pointer to RTU_handle_t (i.e. RTU_Obj **)
 */
RTU_Sta_t RTU_Init(RTU_handle_t *handle, RTU_Conf_t *conf)
{
    if (handle == NULL || conf == NULL)
        return RTU_ERR;

    if (*handle != NULL) // already initialized
        return RTU_ERR;

    RTU_Obj *obj = (RTU_Obj *)calloc(1, sizeof(RTU_Obj));
    if (obj == NULL)
    {
        free(obj->buf);
        return RTU_ERR;
    }


    /* copy basic fields */
    obj->id = conf->id;
    obj->buf_size = conf->buf_size;
    obj->transmit = conf->transmit;
    obj->receive = conf->receive;

    /* check required function pointers */
    if (obj->transmit == NULL || obj->receive == NULL)
    {
        free(obj);
        return RTU_ERR;
    }

    obj->buf = (uint8_t*)calloc(1,sizeof(uint8_t) * obj->buf_size);
    int init_count = 0;
    int ret = 0;

    /* build coils list */
    ret = rtubuild_register_list(&obj->coils, conf->coils.map, conf->coils.count);
    if (ret < 0)
    {
        free(obj);
        return RTU_ERR;
    }
    if (conf->coils.map != NULL && conf->coils.count > 0)
        init_count++;

    /* build holding registers list */
    ret = rtubuild_register_list(&obj->holdingRegs, conf->holdingRegs.map, conf->holdingRegs.count);
    if (ret < 0)
    {
        rtufree_register_list(obj->coils.next);
        free(obj);
        return RTU_ERR;
    }
    if (conf->holdingRegs.map != NULL && conf->holdingRegs.count > 0)
        init_count++;

    /* build write registers list */
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
        /* 没有任何表被初始化，认为是错误 */
        free(obj);
        return RTU_ERR;
    }

    /* 成功：把分配好的对象返回给调用者 */
    *handle = obj;
    return RTU_OK;
}

void RTU_Deinit(RTU_handle_t *handle)
{
    if (handle == NULL || *handle == NULL)
        return;

    RTU_Obj *obj = *handle;

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

/* helper: find node by address in a linked list whose sentinel is `head` */
static RTU_Register_t *rtu_find_node(RTU_Register_t *head, uint16_t addr)
{
    if (head == NULL) return NULL;
    RTU_Register_t *cur = head->next; // first real node
    while (cur)
    {
        if (cur->address == addr) return cur;
        cur = cur->next;
    }
    return NULL;
}

RTU_Sta_t RTU_TimerHandler(RTU_handle_t handle, uint8_t *frame, size_t size)
{
    if (handle == NULL || frame == NULL) return RTU_ERR;

    /* minimal request length for Read/Write single is 8 bytes (addr..crc) */
    if (size < 8) return RTU_ERR;

    /* check device id */
    if (frame[0] != handle->id) return RTU_ERR;

    /* check CRC: Modbus CRC is low byte first, high byte second in frame */
    uint16_t recv_crc = (uint16_t)frame[size - 2] | ((uint16_t)frame[size - 1] << 8);
    if (recv_crc != CRC16(frame, size - 2)) return RTU_ERR;

    uint8_t func = frame[1];
    uint16_t regAddr = ((uint16_t)frame[2] << 8) | frame[3];
    uint16_t reqNum  = ((uint16_t)frame[4] << 8) | frame[5];

    /* response buffer (动态申请，长度以 handle->buf_size 为上限) */
    // if (handle->buf_size == 0 || handle->buf_size > 1024) return RTU_ERR; // sanity limit
    //  uint8_t *resp = (uint8_t *)calloc(1, handle->buf_size);

    // if (resp == NULL) return RTU_ERR;

    size_t resp_len = 0;

    switch (func)
    {
    case RTU_FUNC_READ_COILS:
    {
        /* validate quantity */
        if (reqNum == 0 || reqNum > 2000) { /*free(resp);*/ return RTU_ERR; }

        size_t byte_count = (reqNum + 7) / 8;
        size_t needed = 1 + 1 + 1 + byte_count + 2; // id+func+bytecount+data+crc
        if (needed > handle->buf_size) { /*free(resp);*/ return RTU_ERR; }

        handle->buf[0] = handle->id;
        handle->buf[1] = RTU_FUNC_READ_COILS;
        handle->buf[2] = (uint8_t)byte_count;

        /* find starting node */
        RTU_Register_t *node = rtu_find_node(&handle->coils, regAddr);
        if (node == NULL) { /*free(resp);*/ return RTU_ERR; }

        /* pack bits (LSB of first data byte is coil at lowest address) */
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
            size_t bit_index  = i & 0x07;
            handle->buf[byte_index] |= (uint8_t)(bit << bit_index);

            node = node->next;
        }

        /* append CRC */
        size_t crc_pos = 3 + byte_count;
        uint16_t crc = CRC16(handle->buf, crc_pos);
        handle->buf[crc_pos + 0] = (uint8_t)(crc & 0x00FF);       // CRC low
        handle->buf[crc_pos + 1] = (uint8_t)((crc >> 8) & 0x00FF); // CRC high

        resp_len = crc_pos + 2;
        handle->transmit(handle->buf, resp_len);
        break;
    }

    case RTU_FUNC_READ_HOLD_REGS:
    {
        /* validate quantity */
        if (reqNum == 0 || reqNum > 125) {/* free(resp);*/ return RTU_ERR; } // Modbus limit 125 regs

        size_t byte_count = reqNum * 2;
        size_t needed = 1 + 1 + 1 + byte_count + 2;
        if (needed > handle->buf_size) { /*free(resp)*/; return RTU_ERR; }

        handle->buf[0] = handle->id;
        handle->buf[1] = RTU_FUNC_READ_HOLD_REGS;
        handle->buf[2] = (uint8_t)byte_count;

        RTU_Register_t *node = rtu_find_node(&handle->holdingRegs, regAddr);
        if (node == NULL) { /*free(resp);*/ return RTU_ERR; }

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
        break;
    }

    case RTU_FUNC_WRITE_SINGLE_REG:
    {
        /* request format already checked above (size>=8) */
        RTU_Register_t *node = rtu_find_node(&handle->writeRegs, regAddr);
        if (node == NULL) { /*free(resp)*/; return RTU_ERR; }

        uint16_t value = ((uint16_t)frame[4] << 8) | frame[5];
        if (node->value)
        {
            *((uint16_t *)node->value) = value;
        }

        /* echo request back (modbus behavior) - request already contains CRC, so send same bytes */
        handle->transmit(frame, size);
        resp_len = size;
        break;
    }

    case RTU_FUNC_MASK_WRITE_REG:
    {
        /* For now: simple echo back (implement full mask-write parsing later if needed) */
        /* Proper 0x16 requires parsing AND/OR masks and applying to register. */
        RTU_Register_t *node = rtu_find_node(&handle->writeRegs, regAddr);
        if (node == NULL) { /*free(resp)*/; return RTU_ERR; }

        /* echo request */
        handle->transmit(frame, size);
        resp_len = size;
        break;
    }

    default:
        //free(resp);
        return RTU_ERR;
    }

    //free(resp);
    memset(handle->buf, 0, handle->buf_size);
    return RTU_OK;
}
