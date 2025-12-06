// XV7001BB SPI驱动与寄存器读写
#include "xv7001.h"

// 片选控制
static inline void CS_Low(void) { HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET); }  // 片选拉低
static inline void CS_High(void) { HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET); }   // 片选拉高

static SPI_HandleTypeDef hspi2; // SPI2句柄（保持静态）
int16_t g_TempRaw = 0;          // 温度裸数据（12位）
float   g_TempC = 0.0f;         // 温度摄氏度

void xv7001_init(void)
{ // 初始化SPI2与PB12片选
    GPIO_InitTypeDef GPIO_InitStruct;            // GPIO配置结构体

    __HAL_RCC_GPIOB_CLK_ENABLE();                // 使能GPIOB时钟
    __HAL_RCC_SPI2_CLK_ENABLE();                 // 使能SPI2时钟

    // 配置PB12为GPIO输出，作为片选CS
    GPIO_InitStruct.Pin = GPIO_PIN_12;           // PB12
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;  // 推挽输出
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;// 高速
    GPIO_InitStruct.Pull = GPIO_NOPULL;          // 无上下拉
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);      // 初始化PB12
    CS_High();                                   // 默认拉高，不选中设备

    // 配置SPI2引脚：PB13=SCK(AF_PP), PB14=MISO(Input), PB15=MOSI(AF_PP)
    GPIO_InitStruct.Pin = GPIO_PIN_13 | GPIO_PIN_15; // SCK与MOSI
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;      // 复用推挽输出
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;// 高速
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);      // 初始化PB13/PB15

    GPIO_InitStruct.Pin = GPIO_PIN_14;           // MISO
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;      // 输入模式
    GPIO_InitStruct.Pull = GPIO_NOPULL;          // 无上下拉（如需可用上拉）
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);      // 初始化PB14

    // SPI2参数配置
    hspi2.Instance = SPI2;                       // 选择SPI2
    hspi2.Init.Mode = SPI_MODE_MASTER;           // 主模式
    hspi2.Init.Direction = SPI_DIRECTION_2LINES; // 全双工
    hspi2.Init.DataSize = SPI_DATASIZE_8BIT;     // 8位数据
    hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;   // CPOL=0
    hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;       // CPHA=0（第1边沿采样）
    hspi2.Init.NSS = SPI_NSS_SOFT;               // 软件管理NSS（使用PB12手动片选）
    hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8; // 分频8：64MHz/8=8MHz ≤10MHz
    hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;      // 高位先行
    hspi2.Init.TIMode = SPI_TIMODE_DISABLE;      // 关闭TI模式
    hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE; // 关闭CRC
    hspi2.Init.CRCPolynomial = 7;                // 默认值
    if (HAL_SPI_Init(&hspi2) != HAL_OK)          // 初始化SPI2
    { // 初始化失败处理
        while (1) { }                            // 卡死等待
    }


}
HAL_StatusTypeDef xv7001_read_temperature(void)
{
	// 读取温度（12位模式）：两个字节，组装为12位二补码并转换为摄氏度
	uint8_t buf[2] = { 0, 0 }; // 接收缓冲
	HAL_StatusTypeDef st; // 状态
	uint8_t addr = 0x80 | 0x08; // 读地址：MSB=1，寄存器0x08
	uint8_t dummy = 0xFF; // 占位发送

	CS_Low(); // 片选拉低
	st = HAL_SPI_Transmit(&hspi2, &addr, 1, 10); // 发送地址
	if (st == HAL_OK)
		st = HAL_SPI_TransmitReceive(&hspi2, &dummy, &buf[0], 1, 10); // 读第1字节
	if (st == HAL_OK)
		st = HAL_SPI_TransmitReceive(&hspi2, &dummy, &buf[1], 1, 10); // 读第2字节
	CS_High(); // 片选拉高

	if (st != HAL_OK)                          // 读取失败直接返回
		return st;

	int16_t raw12 = ((int16_t)buf[0] << 4) | (buf[1] >> 4); // 组装12位数据
	if (raw12 & 0x800) raw12 |= 0xF000; // 符号扩展到16位
	g_TempRaw = raw12; // 保存裸数据
	g_TempC = 25.0f + ((float)(raw12 - 400)) / 16.0f; // 转换为摄氏度
	return HAL_OK;                              // 成功
}
HAL_StatusTypeDef xv7001_reg_write(uint8_t reg, uint8_t data)
{ // 写寄存器：首字节MSB=0表示写入，后续发送数据
    uint8_t addr = (0u << 7) | (reg & 0x7Fu);    // 写命令（MSB=0），A[6:0]=地址
    HAL_StatusTypeDef st;                        // 返回状态

    CS_Low();                                    // 片选拉低，开始事务
    st = HAL_SPI_Transmit(&hspi2, &addr, 1, 10); // 发送地址
    if (st == HAL_OK)
        st = HAL_SPI_Transmit(&hspi2, &data, 1, 10); // 发送数据
    CS_High();                                   // 片选拉高，结束事务
    return st;                                   // 返回状态
}

HAL_StatusTypeDef xv7001_reg_read(uint8_t reg, uint8_t *data)
{ // 读寄存器：首字节MSB=1表示读取，第二字节接收数据
    uint8_t addr = (1u << 7) | (reg & 0x7Fu);    // 读命令（MSB=1）
    HAL_StatusTypeDef st;                        // 返回状态
    uint8_t dummy = 0xFF;                        // 占位发送

    CS_Low();                                    // 片选拉低，开始事务
    st = HAL_SPI_Transmit(&hspi2, &addr, 1, 10); // 发送地址
    if (st == HAL_OK)
        st = HAL_SPI_TransmitReceive(&hspi2, &dummy, data, 1, 10); // 发送占位并接收数据
    CS_High();                                   // 片选拉高，结束事务
    return st;                                   // 返回状态
}
