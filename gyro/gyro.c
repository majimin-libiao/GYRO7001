// 主程序：FreeRTOS 线程创建示例，使用自定义中文注释规则

/* Includes ------------------------------------------------------------------*/
#include <stm32f1xx_hal.h>
#include <../CMSIS_RTOS/cmsis_os.h>

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
osThreadId LEDThread1Handle, LEDThread2Handle;

/* Private function prototypes -----------------------------------------------*/
static void LED_Thread1(void const *argument);
static void LED_Thread2(void const *argument);

/* Private functions ---------------------------------------------------------*/

int main(void)
{ // 主程序入口
    HAL_Init();                       // 初始化HAL库
	
    __GPIOB_CLK_ENABLE();             // 使能GPIOB时钟
    GPIO_InitTypeDef GPIO_InitStructure; // GPIO初始化结构体

    GPIO_InitStructure.Pin = GPIO_PIN_9 | GPIO_PIN_9; // 配置PB9引脚（示例中两次或运算，按实际需求调整）

    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;    // 推挽输出模式
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;  // 高速
    GPIO_InitStructure.Pull = GPIO_NOPULL;            // 无上拉下拉
    HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);        // 初始化GPIOB

    osThreadDef(LED1, LED_Thread1, osPriorityNormal, 0, configMINIMAL_STACK_SIZE); // 线程1定义
  
    osThreadDef(LED2, LED_Thread2, osPriorityNormal, 0, configMINIMAL_STACK_SIZE); // 线程2定义
  
    LEDThread1Handle = osThreadCreate(osThread(LED1), NULL); // 启动线程1
  
    LEDThread2Handle = osThreadCreate(osThread(LED2), NULL); // 启动线程2
  
    osKernelStart();                   // 启动调度器

    for (;;)                            // 调度器接管后不应执行到此
        ;
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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
