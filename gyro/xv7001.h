#pragma once
#include <stm32f1xx_hal.h>

// XV7001 寄存器与SPI接口
// 使用SPI2与PB12作为片选

#ifdef __cplusplus
extern "C" {
#endif

void xv7001_init(void);                                   // 初始化SPI与GPIO片选
HAL_StatusTypeDef xv7001_reg_write(uint8_t reg, uint8_t data); // 写寄存器
HAL_StatusTypeDef xv7001_reg_read(uint8_t reg, uint8_t *data); // 读寄存器
HAL_StatusTypeDef xv7001_read_temperature(void);               // 读取温度（默认12bit），更新全局变量

extern int16_t g_TempRaw;                                      // 温度裸数据（12位二补码，已符号扩展）
extern float   g_TempC;                                        // 温度（摄氏度）

#ifdef __cplusplus
}
#endif
