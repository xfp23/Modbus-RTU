#include <stdlib.h>  // calloc, free
#include "modbus_rtu_types.h"

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
        return RTU_ERR;

    /* copy basic fields */
    obj->id = conf->id;
    obj->transmit = conf->transmit;
    obj->receive = conf->receive;

    /* check required function pointers */
    if (obj->transmit == NULL || obj->receive == NULL)
    {
        free(obj);
        return RTU_ERR;
    }

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
        /* 没有任何表被初始化，认为是错误（按你的要求） */
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

    /* free allocated lists (first real node is head.next) */
    if (obj->coils.next)
        rtufree_register_list(obj->coils.next);
    if (obj->holdingRegs.next)
        rtufree_register_list(obj->holdingRegs.next);
    if (obj->writeRegs.next)
        rtufree_register_list(obj->writeRegs.next);

    /* clear sentinel just for safety (not strictly needed) */
    obj->coils.next = NULL;
    obj->holdingRegs.next = NULL;
    obj->writeRegs.next = NULL;

    free(obj);
    *handle = NULL;
}
