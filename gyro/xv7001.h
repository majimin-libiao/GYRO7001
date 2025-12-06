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

HAL_StatusTypeDef xv7001_read_angular_rate24(void);            // 读取角速度（24位二补码）
extern int32_t g_GyroRaw24;                                    // 角速度裸数据（24位二补码，已符号扩展到32位）
extern int32_t g_GyroZeroOffset;                               // 启动零偏（平均100次得到）
extern int32_t g_GyroRawCalibrated;                            // 零偏校准后的角速度裸数据
extern float   g_AngleDeg;                                      // 对时间积分得到的角度（度）
void xv7001_set_gyro_scale(float scale);                        // 设置角速度单位换算系数（原始数值→度/秒）
float xv7001_get_gyro_dps(void);                                // 获取当前校准后角速度（度/秒）

#ifdef __cplusplus
}
#endif
