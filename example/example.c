/**
 * @file example.c
 * @brief Modbus RTU从机库使用示例
 * @version 0.1
 * @date 2025-01-27
 * 
 * 本示例展示了如何使用Modbus RTU从机库：
 * 1. 定义寄存器映射
 * 2. 实现发送函数
 * 3. 初始化和配置从机
 * 4. 处理接收到的数据帧
 * 5. 清理资源
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/RtuSlave.h"

// 定义寄存器数据存储
static uint8_t coil_data[10] = {0};           // 线圈数据 (10个线圈)
static uint16_t holding_reg_data[5] = {100, 200, 300, 400, 500};  // 保持寄存器数据
static uint16_t write_reg_data[3] = {0};      // 可写寄存器数据

// 发送函数实现 - 在实际应用中，这里应该通过串口或其他通信方式发送数据
int transmit_data(uint8_t *data, size_t size)
{
    printf("发送响应数据 (%zu字节): ", size);
    for(size_t i = 0; i < size; i++) {
        printf("%02X ", data[i]);
    }
    printf("\n");
    
    // 在实际应用中，这里应该调用串口发送函数
    // 例如: uart_send(data, size);
    
    return 0;
}

// 打印寄存器状态
void print_register_status(void)
{
    printf("\n=== 当前寄存器状态 ===\n");
    
    printf("线圈状态: ");
    for(int i = 0; i < 10; i++) {
        printf("%d ", coil_data[i]);
    }
    printf("\n");
    
    printf("保持寄存器: ");
    for(int i = 0; i < 5; i++) {
        printf("%d ", holding_reg_data[i]);
    }
    printf("\n");
    
    printf("可写寄存器: ");
    for(int i = 0; i < 3; i++) {
        printf("%d ", write_reg_data[i]);
    }
    printf("\n");
}

// 模拟接收数据帧并处理
void simulate_modbus_request(RTU_Slavehandle_t handle, const char* description, uint8_t *frame, size_t size)
{
    printf("\n--- %s ---\n", description);
    printf("接收数据帧 (%zu字节): ", size);
    for(size_t i = 0; i < size; i++) {
        printf("%02X ", frame[i]);
    }
    printf("\n");
    
    RTU_Sta_t result = RTUSlave_TimerHandler(handle, frame, size);
    
    switch(result) {
        case RTU_OK:
            printf("处理结果: 成功\n");
            break;
        case RTU_READ:
            printf("处理结果: 执行了读操作\n");
            break;
        case RTU_WRITE:
            printf("处理结果: 执行了写操作\n");
            break;
        case RTU_ERR:
            printf("处理结果: 处理失败\n");
            break;
        default:
            printf("处理结果: 未知状态\n");
            break;
    }
}

int main()
{
    printf("Modbus RTU从机库使用示例\n");
    printf("========================\n");
    
    RTU_Slavehandle_t slave_handle = NULL;
    
    // 定义线圈寄存器映射表 (地址0x0000-0x0009)
    RTU_RegisterMap_t coil_map[] = {
        {0x0000, &coil_data[0]},
        {0x0001, &coil_data[1]},
        {0x0002, &coil_data[2]},
        {0x0003, &coil_data[3]},
        {0x0004, &coil_data[4]},
        {0x0005, &coil_data[5]},
        {0x0006, &coil_data[6]},
        {0x0007, &coil_data[7]},
        {0x0008, &coil_data[8]},
        {0x0009, &coil_data[9]}
    };
    
    // 定义保持寄存器映射表 (地址0x0000-0x0004)
    RTU_RegisterMap_t holding_map[] = {
        {0x0000, &holding_reg_data[0]},
        {0x0001, &holding_reg_data[1]},
        {0x0002, &holding_reg_data[2]},
        {0x0003, &holding_reg_data[3]},
        {0x0004, &holding_reg_data[4]}
    };
    
    // 定义可写寄存器映射表 (地址0x0000-0x0002)
    RTU_RegisterMap_t write_map[] = {
        {0x0000, &write_reg_data[0]},
        {0x0001, &write_reg_data[1]},
        {0x0002, &write_reg_data[2]}
    };
    
    // 配置从机参数
    RtuSlave_Conf_t config = {
        .id = 0x01,                    // 设备ID
        .buf_size = 256,               // 缓冲区大小
        .transmit = transmit_data,     // 发送函数
        .coils = {
            .map = coil_map,
            .count = sizeof(coil_map) / sizeof(coil_map[0])
        },
        .holdingRegs = {
            .map = holding_map,
            .count = sizeof(holding_map) / sizeof(holding_map[0])
        },
        .writeRegs = {
            .map = write_map,
            .count = sizeof(write_map) / sizeof(write_map[0])
        }
    };
    
    // 初始化从机
    printf("正在初始化Modbus RTU从机...\n");
    if(RTUSlave_Init(&slave_handle, &config) != RTU_OK) {
        printf("错误: 从机初始化失败\n");
        return -1;
    }
    
    printf("从机初始化成功 (设备ID: 0x%02X)\n", config.id);
    
    // 显示初始寄存器状态
    print_register_status();
    
    // 模拟各种Modbus请求
    
    // 1. 读保持寄存器 (地址0x0000-0x0002)
    uint8_t read_holding_frame[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x03, 0x05, 0xCB};
    simulate_modbus_request(slave_handle, "读保持寄存器 (0x0000-0x0002)", 
                           read_holding_frame, sizeof(read_holding_frame));
    
    // 2. 读线圈 (地址0x0000-0x0007)
    uint8_t read_coils_frame[] = {0x01, 0x01, 0x00, 0x00, 0x00, 0x08, 0x3D, 0xCA};
    simulate_modbus_request(slave_handle, "读线圈 (0x0000-0x0007)", 
                           read_coils_frame, sizeof(read_coils_frame));
    
    // 3. 写单个寄存器 (地址0x0000, 值0x1234)
    uint8_t write_single_frame[] = {0x01, 0x06, 0x00, 0x00, 0x12, 0x34, 0x89, 0x78};
    simulate_modbus_request(slave_handle, "写单个寄存器 (0x0000 = 0x1234)", 
                           write_single_frame, sizeof(write_single_frame));
    
    // 4. 再次读保持寄存器查看变化
    simulate_modbus_request(slave_handle, "再次读保持寄存器验证写入", 
                           read_holding_frame, sizeof(read_holding_frame));
    
    // 5. 测试错误情况 - 读不存在的寄存器
    uint8_t invalid_frame[] = {0x01, 0x03, 0x00, 0x10, 0x00, 0x01, 0x84, 0x0F};
    simulate_modbus_request(slave_handle, "读不存在的寄存器 (错误测试)", 
                           invalid_frame, sizeof(invalid_frame));
    
    // 6. 测试错误情况 - 错误的设备ID
    uint8_t wrong_id_frame[] = {0x02, 0x03, 0x00, 0x00, 0x00, 0x01, 0x85, 0xF9};
    simulate_modbus_request(slave_handle, "错误的设备ID (错误测试)", 
                           wrong_id_frame, sizeof(wrong_id_frame));
    
    // 显示最终寄存器状态
    print_register_status();
    
    // 反初始化
    printf("\n正在清理资源...\n");
    RTU_Deinit(&slave_handle);
    printf("资源清理完成\n");
    
    printf("\n示例程序执行完成\n");
    return 0;
}

/*
编译命令:
gcc -o example example.c ../src/RtuSlave.c -I../include

运行示例:
./example

预期输出:
- 从机初始化成功
- 各种Modbus请求的处理结果
- 寄存器状态变化
- 错误情况的处理

注意事项:
1. 在实际应用中，需要实现真实的串口发送函数
2. 需要根据实际硬件平台调整缓冲区大小
3. 寄存器地址必须连续，不能有间隔
4. 确保CRC校验正确
*/
