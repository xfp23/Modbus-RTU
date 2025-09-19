# Modbus RTU 从机库

一个轻量级的Modbus RTU从机实现库，支持基本的读写操作。

## 功能特性

- 支持Modbus RTU协议
- 实现从机（Slave）功能
- 支持的功能码：
  - `0x01` - 读线圈 (Read Coils)
  - `0x03` - 读保持寄存器 (Read Holding Registers)  
  - `0x06` - 写单个寄存器 (Write Single Register)
  - `0x16` - 寄存器按位写 (Mask Write Register)
- 自动CRC校验
- 灵活的寄存器映射机制
- 内存管理安全

## 目录结构

```
Modbus-RTU/
├── include/
│   ├── RtuSlave.h          # 主头文件
│   └── RtuSlave_types.h    # 类型定义
├── src/
│   └── RtuSlave.c          # 实现文件
├── example/
│   └── example.c           # 使用示例
└── readme.md               # 本文档
```

## API 接口

### 初始化函数

```c
RTU_Sta_t RTUSlave_Init(RTU_Slavehandle_t *handle, RtuSlave_Conf_t *conf);
```

初始化Modbus RTU从机实例。

**参数：**
- `handle`: 从机句柄指针
- `conf`: 配置结构体指针

**返回值：**
- `RTU_OK`: 初始化成功
- `RTU_ERR`: 初始化失败

### 反初始化函数

```c
void RTU_Deinit(RTU_Slavehandle_t *handle);
```

释放从机实例占用的内存。

### 数据处理函数

```c
RTU_Sta_t RTUSlave_TimerHandler(RTU_Slavehandle_t handle, uint8_t *frame, size_t size);
```

处理接收到的Modbus RTU数据帧。

**参数：**
- `handle`: 从机句柄
- `frame`: 接收到的数据帧
- `size`: 数据帧长度

**返回值：**
- `RTU_OK`: 处理成功
- `RTU_READ`: 执行了读操作
- `RTU_WRITE`: 执行了写操作
- `RTU_ERR`: 处理失败

## 数据结构

### 配置结构体

```c
typedef struct
{
    uint8_t id;              // 设备ID
    uint8_t buf_size;        // 数据帧缓冲区大小
    RTU_TransmitFn transmit; // 发送函数指针
    RTU_RegisterTable_t coils;       // 线圈寄存器表
    RTU_RegisterTable_t holdingRegs; // 保持寄存器表
    RTU_RegisterTable_t writeRegs;   // 写寄存器表
} RtuSlave_Conf_t;
```

### 寄存器映射

```c
typedef struct
{
    uint16_t addr;  // 寄存器地址
    void *data;     // 数据指针
} RTU_RegisterMap_t;
```

## 使用示例

### 基本使用流程

1. 定义寄存器映射表
2. 实现发送函数
3. 配置从机参数
4. 初始化从机
5. 在接收数据时调用处理函数
6. 程序结束时反初始化

### 完整示例

```c
#include "RtuSlave.h"
#include <stdio.h>

// 定义寄存器数据
uint8_t coil_data[10] = {0};
uint16_t holding_reg_data[5] = {100, 200, 300, 400, 500};
uint16_t write_reg_data[3] = {0};

// 发送函数实现
int transmit_data(uint8_t *data, size_t size)
{
    printf("发送数据: ");
    for(size_t i = 0; i < size; i++) {
        printf("%02X ", data[i]);
    }
    printf("\n");
    return 0;
}

int main()
{
    RTU_Slavehandle_t slave_handle = NULL;
    
    // 定义寄存器映射表
    RTU_RegisterMap_t coil_map[] = {
        {0x0000, &coil_data[0]},
        {0x0001, &coil_data[1]},
        {0x0002, &coil_data[2]},
        // ... 更多线圈
    };
    
    RTU_RegisterMap_t holding_map[] = {
        {0x0000, &holding_reg_data[0]},
        {0x0001, &holding_reg_data[1]},
        {0x0002, &holding_reg_data[2]},
        // ... 更多保持寄存器
    };
    
    RTU_RegisterMap_t write_map[] = {
        {0x0000, &write_reg_data[0]},
        {0x0001, &write_reg_data[1]},
        {0x0002, &write_reg_data[2]},
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
    if(RTUSlave_Init(&slave_handle, &config) != RTU_OK) {
        printf("从机初始化失败\n");
        return -1;
    }
    
    printf("Modbus RTU从机初始化成功\n");
    
    // 模拟接收数据帧 (读保持寄存器 0x0000-0x0002)
    uint8_t frame[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x03, 0x05, 0xCB};
    
    // 处理接收到的数据
    RTU_Sta_t result = RTUSlave_TimerHandler(slave_handle, frame, sizeof(frame));
    
    switch(result) {
        case RTU_OK:
            printf("数据处理成功\n");
            break;
        case RTU_READ:
            printf("执行了读操作\n");
            break;
        case RTU_WRITE:
            printf("执行了写操作\n");
            break;
        case RTU_ERR:
            printf("数据处理失败\n");
            break;
    }
    
    // 反初始化
    RTU_Deinit(&slave_handle);
    
    return 0;
}
```

## 编译说明

### 依赖项

- C99标准编译器
- 标准C库 (stdlib.h, string.h, stdint.h)

### 编译命令

```bash
gcc -o example example.c src/RtuSlave.c -Iinclude
```

## 注意事项

1. **寄存器地址连续性**: 寄存器地址必须连续，不能有间隔
2. **内存管理**: 库会自动管理内部内存，用户只需调用`RTU_Deinit`释放资源
3. **线程安全**: 当前实现不是线程安全的，多线程环境下需要额外的同步机制
4. **缓冲区大小**: 确保配置的缓冲区大小足够处理最大的Modbus帧
5. **发送函数**: 用户必须实现发送函数，库会调用此函数发送响应数据

## 协议支持

### 支持的功能码

| 功能码 | 名称 | 描述 |
|--------|------|------|
| 0x01 | Read Coils | 读线圈状态 |
| 0x03 | Read Holding Registers | 读保持寄存器 |
| 0x06 | Write Single Register | 写单个寄存器 |
| 0x16 | Mask Write Register | 寄存器按位写 |

### 数据格式

- **设备ID**: 1字节
- **功能码**: 1字节  
- **数据**: 变长
- **CRC**: 2字节 (低字节在前)

## 许可证

请查看LICENSE文件了解详细的许可证信息。

## 版本历史

- v0.1 - 初始版本，支持基本的Modbus RTU从机功能
