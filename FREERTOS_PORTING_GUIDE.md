# FreeRTOS移植指南

## 1. 文件结构

```
User/
├── main.c                    # 原有main文件（保留）
├── main_freertos.c           # FreeRTOS版本main文件
├── freertos_port.h           # FreeRTOS端口层头文件
├── freertos_port.c           # FreeRTOS端口层实现
├── freertos_tasks.h          # FreeRTOS任务头文件
└── freertos_tasks.c          # FreeRTOS任务实现
```

## 2. 核心改动

### 2.1 OLED访问保护
- 使用互斥信号量保护OLED访问
- 所有OLED操作前需调用 `xOLEDTake()`
- OLED操作后需调用 `xOLEDGive()`

### 2.2 按键处理
- 创建独立按键任务 `vKeyTask`
- 按键事件通过队列 `xKeyQueue` 传递
- 各功能模块从队列读取按键事件

### 2.3 任务创建
```c
void vInitTasks(void)
{
    xKeyQueue = xQueueCreate(10, sizeof(uint8_t));
    xMenuQueue = xQueueCreate(5, sizeof(uint8_t));
    vOLEDMutexCreate();
    
    xTaskCreate(vKeyTask, "KeyTask", 128, NULL, 2, &xKeyTaskHandle);
    vTaskStartScheduler();
}
```

## 3. 使用方法

### 3.1 替换main.c
将 `main_freertos.c` 替换原 `main.c`

### 3.2 添加头文件
在 `menu.c` 开头添加：
```c
#include "freertos_port.h"
```

### 3.3 修改OLED操作
将原有OLED操作改为：
```c
if(xOLEDTake(pdMS_TO_TICKS(100)) == pdTRUE)
{
    // OLED操作
    OLED_Update();
    xOLEDGive();
}
```

### 3.4 修改按键检测
将 `KeyNum = Key_GetNum()` 改为：
```c
if(xQueueReceive(xKeyQueue, &key_num, pdMS_TO_TICKS(0)) == pdPASS)
{
    // 处理按键
}
```

## 4. 任务优先级

- 按键任务: 2 (高优先级)
- 时钟任务: 1 (中优先级)
- 其他任务: 1-3 (根据需要)

## 5. 注意事项

1. **不要在中断中使用OLED操作**
2. **确保所有任务都有延迟**，避免阻塞调度器
3. **队列大小根据实际需求调整**
4. **堆栈大小根据函数调用深度调整**

## 6. 示例：修改Stop_Watch函数

```c
int Stop_Watch(void)
{
    uint8_t stop_watch_flag = 1;
    uint8_t key_num;
    
    for(;;)
    {
        if(xQueueReceive(xMenuQueue, &stop_watch_flag, pdMS_TO_TICKS(100)) == pdPASS)
        {
            if(stop_watch_flag == 1) return 0;
            else if(stop_watch_flag == 2) stop_watch_start = 1;
            else if(stop_watch_flag == 3) stop_watch_start = 0;
            else if(stop_watch_flag == 4) { stop_watch_start = 0; hour = min = sec = 0; }
        }
        
        if(xQueueReceive(xKeyQueue, &key_num, pdMS_TO_TICKS(0)) == pdPASS)
        {
            if(key_num == 1) { stop_watch_flag--; if(stop_watch_flag <= 0) stop_watch_flag = 4; }
            else if(key_num == 2) { stop_watch_flag++; if(stop_watch_flag >= 5) stop_watch_flag = 1; }
            else if(key_num == 3) 
            {
                if(xOLEDTake(pdMS_TO_TICKS(100)) == pdTRUE)
                {
                    OLED_Clear();
                    OLED_Update();
                    xOLEDGive();
                }
                xQueueSend(xMenuQueue, &stop_watch_flag, pdMS_TO_TICKS(100));
            }
        }
        
        if(xOLEDTake(pdMS_TO_TICKS(100)) == pdTRUE)
        {
            Show_Stop_WatchUI();
            switch(stop_watch_flag)
            {
                case 1: OLED_ReverseArea(0, 0, 16, 16); break;
                case 2: OLED_ReverseArea(8, 40, 32, 16); break;
                case 3: OLED_ReverseArea(48, 40, 32, 16); break;
                case 4: OLED_ReverseArea(88, 40, 32, 16); break;
            }
            OLED_Update();
            xOLEDGive();
        }
        
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
```

## 7. 编译配置

确保在项目中添加以下FreeRTOS源文件：
- `tasks.c`
- `queue.c`
- `list.c`
- `event_groups.c`
- `timers.c`
- `port.c`
- `heap_4.c`
