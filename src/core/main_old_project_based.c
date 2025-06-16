/**
 * ================================================================
 * 憨云 DTU - 基于旧项目的完整硬件测试程序
 * ================================================================
 * 文件: main_old_project_based.c
 * 功能: 完全基于旧项目配置的硬件测试程序
 * 硬件: PC8(LED), PA6(蜂鸣器PWM0_CH3), PC14(SCL), PA12(SDA)
 * 作者: 憨云 DTU 开发团队
 * 日期: 2025-03-28
 * ================================================================
 */

#include "../../inc/nano100b_types.h"
#include "../../inc/nano100b_reg.h"

// ================================================================
// 基于旧项目的寄存器定义
// ================================================================

// 系统寄存器解锁/锁定 (基于旧项目)
#define SYS_REGLCTL_BASE 0x50000100UL
#define SYS_UnlockReg() do { \
    REG32(SYS_REGLCTL_BASE) = 0x59; \
    REG32(SYS_REGLCTL_BASE) = 0x16; \
    REG32(SYS_REGLCTL_BASE) = 0x88; \
} while(0)

#define SYS_LockReg() do { \
    REG32(SYS_REGLCTL_BASE) = 0x00; \
} while(0)

// 时钟控制寄存器 (基于旧项目)
#define CLK_PWRCTL_HIRC_EN (1UL << 2)
#define CLK_CLKSTATUS_HIRC_STB (1UL << 2)
#define CLK_AHBCLK_GPIO_EN (1UL << 2)
#define CLK_APBCLK_PWM0_CH23_EN (1UL << 20)

// GPIO模式定义 (基于旧项目)
#define GPIO_PMD_INPUT 0x0UL
#define GPIO_PMD_OUTPUT 0x1UL
#define GPIO_PMD_OPEN_DRAIN 0x2UL
#define GPIO_PMD_QUASI 0x3UL

// PWM寄存器偏移 (基于旧项目)
#define PWM_PCR_OFFSET 0x00
#define PWM_CNR3_OFFSET 0x18
#define PWM_CMR3_OFFSET 0x1C
#define PWM_PPR_OFFSET 0x04
#define PWM_CSR_OFFSET 0x08

// 引脚复用功能寄存器 (基于旧项目)
#define SYS_PA_L_MFP_BASE 0x50000030UL
#define SYS_PA_L_MFP_PA6_MFP_PWM0_CH3 (0x3UL << 24)

// ================================================================
// 基于旧项目的函数实现
// ================================================================

/**
 * @brief 简单延时函数
 * @param ms 延时毫秒数
 */
static void DelaySecond_1(uint32_t ms)
{
    // 基于12MHz时钟的软件延时
    volatile uint32_t count = ms * 3000;
    while (count--)
    {
        __asm volatile("nop");
    }
}

/**
 * @brief GPIO设置模式 (基于旧项目的GPIO_SetMode)
 * @param port GPIO端口基地址
 * @param pin 引脚位掩码
 * @param mode GPIO模式
 */
static void GPIO_SetMode(uint32_t port, uint32_t pin, uint32_t mode)
{
    uint32_t pin_num = 0;
    uint32_t temp_pin = pin;
    
    // 找到引脚号
    while (temp_pin > 1)
    {
        temp_pin >>= 1;
        pin_num++;
    }
    
    // 设置PMD寄存器
    uint32_t pmd_offset = pin_num * 2;
    REG32(port + GPIO_PMD_OFFSET) &= ~(0x3UL << pmd_offset);
    REG32(port + GPIO_PMD_OFFSET) |= (mode << pmd_offset);
}

/**
 * @brief 系统时钟初始化 (基于旧项目)
 */static void SystemCoreClockUpdate(void)
{
    SYS_UnlockReg();

    // 开启 HIRC
    CLK_ENABLE_HIRC();
    CLK_WAIT_HIRC_READY();

    // 设置 HCLK 时钟源为 HIRC
    REG32(CLK_BASE + CLK_CLKSEL0_OFFSET) &= ~(0x7UL << 0);
    REG32(CLK_BASE + CLK_CLKSEL0_OFFSET) |= CLK_CLKSEL0_HCLK_S_HIRC;

    // 开启外设时钟（GPIO、PWM）
    CLK_ENABLE_GPIO();
    CLK_ENABLE_PWM0();

    SYS_LockReg();
}


/**
 * @brief LED初始化 (基于旧项目的LEDInital)
 */
static void LEDInital(void)
{
    GPIO_SetMode(GPIOC_BASE, (1UL << 8), GPIO_PMD_OUTPUT);
}

/**
 * @brief LED开启 (基于旧项目的LEDOn)
 */
static void LEDOn(void)
{
    REG32(GPIOC_BASE + GPIO_DOUT_OFFSET) |= (1UL << 8);  // PC8=1
}

/**
 * @brief LED关闭 (基于旧项目的LEDOff)
 */
static void LEDOff(void)
{
    REG32(GPIOC_BASE + GPIO_DOUT_OFFSET) &= ~(1UL << 8); // PC8=0
}

/**
 * @brief PWM初始化 (基于旧项目的PWM_init)
 */
static void PWM_init(void)
{
    SYS_UnlockReg();
    
    // 1. 使能PWM0_CH23模块时钟
    REG32(CLK_BASE + CLK_APBCLK_OFFSET) |= CLK_APBCLK_PWM0_CH23_EN;
    
    // 2. 设置PWM0时钟源为HCLK
    REG32(CLK_BASE + CLK_CLKSEL1_OFFSET) &= ~(0x3UL << 28);
    REG32(CLK_BASE + CLK_CLKSEL1_OFFSET) |= (0x0UL << 28); // HCLK
    
    // 3. 配置PA6为PWM0_CH3功能
    REG32(SYS_PA_L_MFP_BASE) &= ~(0xFUL << 24); // 清除PA6的MFP设置
    REG32(SYS_PA_L_MFP_BASE) |= SYS_PA_L_MFP_PA6_MFP_PWM0_CH3; // 设置PA6为PWM0_CH3
    
    // 4. 配置PWM0通道3 (2700Hz, 30%占空比)
    uint32_t pwm_base = PWM0_BASE;
    
    // 设置预分频器
    REG32(pwm_base + PWM_PPR_OFFSET) = 0; // 预分频为1
    
    // 设置时钟分频器
    REG32(pwm_base + PWM_CSR_OFFSET) = 0; // 分频为1
    
    // 设置周期值 (12MHz / 2700Hz ≈ 4444)
    REG32(pwm_base + PWM_CNR3_OFFSET) = 4444;
    
    // 设置占空比 (30% = 4444 * 0.3 ≈ 1333)
    REG32(pwm_base + PWM_CMR3_OFFSET) = 1333;
    
    // 5. 使能PWM输出
    REG32(pwm_base + PWM_PCR_OFFSET) |= (1UL << 11); // 使能CH3输出
    
    SYS_LockReg();
}

/**
 * @brief 蜂鸣器开启 (基于旧项目的BellOn)
 */
static void BellOn(void)
{
    PWM_init();
    // 启动PWM0通道3
    REG32(PWM0_BASE + PWM_PCR_OFFSET) |= (1UL << 3);
}

/**
 * @brief 蜂鸣器关闭 (基于旧项目的BellOff)
 */
static void BellOff(void)
{
    // 停止PWM0通道3
    REG32(PWM0_BASE + PWM_PCR_OFFSET) &= ~(1UL << 3);
}

/**
 * @brief 蜂鸣器控制 (基于旧项目的BellBell)
 * @param Ontime 开启时间 (10ms单位)
 * @param OffTime 关闭时间 (10ms单位)
 * @param Times 重复次数
 */
static void BellBell(uint8_t Ontime, uint8_t OffTime, uint8_t Times)
{
    uint8_t i;
    for (i = 0; i < Times; i++)
    {
        BellOn();
        DelaySecond_1(Ontime * 10);
        BellOff();
        DelaySecond_1(OffTime * 10);
    }
}

/**
 * @brief 主循环初始化 (基于旧项目的Main_loop_Initial)
 */
static void Main_loop_Initial(void)
{
    // 1. 系统时钟初始化
    SystemCoreClockUpdate();
    
    // 2. LED初始化
    LEDInital();
    LEDOff();
    
    // 3. 蜂鸣器初始化
    PWM_init();
    BellOff();
    
    // 4. 延时
    DelaySecond_1(1000);
    
    // 5. 启动测试序列 (基于旧项目)
    BellBell(10, 10, 2);  // 第一组测试
    BellBell(20, 20, 2);  // 第二组测试
}

/**
 * @brief 主程序入口
 */
int main(void)
{
    // 1. 主循环初始化
    Main_loop_Initial();
    
    // 2. 主循环
    uint32_t loop_counter = 0;
    
    while (1)
    {
        // 每1024次循环执行一次完整测试序列
        if ((loop_counter & 0x3FF) == 0)
        {
            BellBell(10, 10, 2);  // 短测试
            DelaySecond_1(500);
            BellBell(20, 20, 2);  // 长测试
        }
        
        // 心跳指示：LED慢闪 (每256次循环)
        if ((loop_counter & 0xFF) == 0)
        {
            LEDOn();
            DelaySecond_1(50);
            LEDOff();
        }
        
        loop_counter++;
        DelaySecond_1(10);
        
        // 防止计数器溢出
        if (loop_counter >= 0xFFFFF000)
        {
            loop_counter = 0;
        }
    }
    
    return 0;
}
