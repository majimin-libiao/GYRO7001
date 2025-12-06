// 主程序：FreeRTOS 线程创建示例，使用自定义中文注释规则

/* Includes ------------------------------------------------------------------*/
#include <stm32f1xx_hal.h>
#include <../CMSIS_RTOS/cmsis_os.h>
#include "xv7001.h"                    // XV7001驱动接口

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
osThreadId LEDThread1Handle, LEDThread2Handle;
osThreadId TempThreadHandle;                    // 温度读取任务句柄

/* Private function prototypes -----------------------------------------------*/
static void LED_Thread1(void const *argument);
static void LED_Thread2(void const *argument);
static void SystemClock_Config(void);           // 时钟配置：外部8MHz，系统时钟64MHz
static void Temp_Thread(void const *argument);  // 温度读取任务

/* Private functions ---------------------------------------------------------*/

int main(void)
{ // 主程序入口
    HAL_Init();                       // 初始化HAL库
    SystemClock_Config();             // 配置系统时钟为64MHz（HSE=8MHz，PLL×8）
	
    __GPIOB_CLK_ENABLE();             // 使能GPIOB时钟
    GPIO_InitTypeDef GPIO_InitStructure; // GPIO初始化结构体

    GPIO_InitStructure.Pin = GPIO_PIN_9 | GPIO_PIN_9; // 配置PB9引脚（示例中两次或运算，按实际需求调整）

    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;    // 推挽输出模式
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;  // 高速
    GPIO_InitStructure.Pull = GPIO_NOPULL;            // 无上拉下拉
    HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);        // 初始化GPIOB

    xv7001_init();                    // 初始化XV7001：SPI与片选

    osThreadDef(LED1, LED_Thread1, osPriorityNormal, 0, configMINIMAL_STACK_SIZE); // 线程1定义
  
    osThreadDef(LED2, LED_Thread2, osPriorityNormal, 0, configMINIMAL_STACK_SIZE); // 线程2定义
    osThreadDef(TEMP, Temp_Thread, osPriorityNormal, 0, configMINIMAL_STACK_SIZE); // 温度任务定义
  
    LEDThread1Handle = osThreadCreate(osThread(LED1), NULL); // 启动线程1
  
    LEDThread2Handle = osThreadCreate(osThread(LED2), NULL); // 启动线程2
    TempThreadHandle = osThreadCreate(osThread(TEMP), NULL); // 启动温度读取任务
  
    osKernelStart();                   // 启动调度器

    for (;;)                            // 调度器接管后不应执行到此
        ;
}

static void Temp_Thread(void const *argument)
{ // 温度读取任务：周期调用xv7001_read_temperature更新全局变量
    (void) argument;                   // 未使用的参数
    
    // 上电后需要等待≥1ms再进行SPI通信，确保设备就绪
    osDelay(2);                        // 简单等待2ms
    
    // 初始零偏校准：1秒内采集100次角速度原始值（10ms间隔），计算平均作为零偏
    int64_t sum = 0;                   // 累计和
    for (int i = 0; i < 100; ++i)      // 采样100次
    { // 累积角速度原始值
        xv7001_read_angular_rate24();  // 读取角速度原始值
        sum += g_GyroRaw24;            // 累加
        osDelay(10);                   // 10ms间隔
    }
    g_GyroZeroOffset = (int32_t)(sum / 100); // 计算平均作为零偏
    
    for (;;)                           // 无限循环
    { // 周期读取温度与角速度
        xv7001_read_temperature();     // 读取温度，更新g_TempRaw与g_TempC
        xv7001_read_angular_rate24();  // 读取角速度并更新校准值
        
        // 角速度积分：将校准后的原始值转换为度/秒并积分得到角度
        const float dt = 0.1f;         // 时间步长（100ms，对应任务周期）
        const float deadband = 0.05f;  // 死区（度/秒），抑制微小噪声积分漂移
        float rate_dps = xv7001_get_gyro_dps(); // 校准后角速度（度/秒），已按比例换算
        
        // 仅当角速度超出死区时才积分，避免静止时噪声累积
        if (rate_dps > deadband || rate_dps < -deadband)
        { // 超出死区才积分
            g_AngleDeg += rate_dps * dt; // 积分更新角度
            // 角度限制到[-180, 180]
            if (g_AngleDeg > 180.0f)       g_AngleDeg -= 360.0f; // 超上限折返
            else if (g_AngleDeg < -180.0f) g_AngleDeg += 360.0f; // 超下限折返
        }
        // 静止时（死区内）不积分，角度保持不变
        
        osDelay(100);                    // 100ms周期
    }
}

void SysTick_Handler(void)
{ // 系统滴答中断：更新HAL时基并通知OS
    HAL_IncTick();                     // 递增HAL时间基准
    osSystickHandler();                // 调用OS系统滴答处理
}

static void LED_Thread1(void const *argument)
{ // 任务1：以50ms周期闪烁LED，独立运行
    (void) argument;                   // 未使用的参数
    
    for (;;)                           // 无限循环
    { // 50ms闪灯
        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_9);    // 翻转LED状态
        osDelay(50);                               // 延时50毫秒
    }
}

static void LED_Thread2(void const *argument)
{ // 任务2：10ms间隔休眠，独立运行
    uint32_t count;                        // 计数变量（保留，当前未使用）
    (void) argument;                       // 未使用的参数
    
    for (;;)                               // 无限循环
    { // 10ms休眠
        osDelay(10);                        // 延时10毫秒
    }
}

#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t* file, uint32_t line)
{ // 断言失败处理：打印文件与行号或进入无限循环
    // 用户可以添加自己的实现来报告文件名和行号
    // 例如: printf("错误参数: 文件 %s 第 %d 行\r\n", file, line)
    
    while (1)                            // 保持在此，便于调试定位
    { // 无限循环
    }
}
#endif

static void SystemClock_Config(void)
{ // 系统时钟配置：外部振荡器8MHz，经PLL×8得到64MHz
    RCC_OscInitTypeDef RCC_OscInitStruct = {0}; // 振荡器配置结构体
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0}; // 时钟配置结构体

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;     // 使用外部高速晶振
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;                        // 打开HSE
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;         // 不分频（F1支持）
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;                        // 保持HSI开启以防回退
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;                    // 开启PLL
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;            // PLL源选择HSE
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL8;                    // PLL倍频×8（8MHz→64MHz）
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)            // 应用振荡器配置
    { // 配置失败处理
        while (1) { }                                               // 卡死等待，便于调试
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2; // 配置各总线时钟
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;        // 系统时钟源选择PLL
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;               // AHB不分频，HCLK=64MHz
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;                // APB1分频2，最大36MHz，这里32MHz
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;                // APB2不分频，64MHz
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) // 设置Flash等待周期2
    { // 配置失败处理
        while (1) { }                                               // 卡死等待
    }
}

// SPI初始化与寄存器读写已迁移至xv7001.c，保持主程序简洁

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
