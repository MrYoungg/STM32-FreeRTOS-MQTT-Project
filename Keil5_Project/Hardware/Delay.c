#include <Delay.h>
#define portNVIC_SHPR3_REG                 (*((volatile uint32_t *)0xe000ed20))
#define portNVIC_SYSTICK_PRI               (((uint32_t)configKERNEL_INTERRUPT_PRIORITY) << 24UL)
#define portNVIC_PENDSV_PRI                (((uint32_t)configKERNEL_INTERRUPT_PRIORITY) << 16UL)
#define portNVIC_SYSTICK_CTRL_REG          (*((volatile uint32_t *)0xe000e010))
#define portNVIC_SYSTICK_CURRENT_VALUE_REG (*((volatile uint32_t *)0xe000e018))
#define portNVIC_SYSTICK_LOAD_REG          (*((volatile uint32_t *)0xe000e014))
#define configSYSTICK_CLOCK_HZ             (configCPU_CLOCK_HZ)
#define portNVIC_SYSTICK_CLK_BIT           (1UL << 2UL)
#define portNVIC_SYSTICK_CLK_BIT_CONFIG    (portNVIC_SYSTICK_CLK_BIT)
#define portNVIC_SYSTICK_INT_BIT           (1UL << 1UL)
#define portNVIC_SYSTICK_ENABLE_BIT        (1UL << 0UL)

void Systick_Init(void)
{
    /* Make PendSV and SysTick the lowest priority interrupts. */
    portNVIC_SHPR3_REG |= portNVIC_PENDSV_PRI;

    portNVIC_SHPR3_REG |= portNVIC_SYSTICK_PRI;

    /* Start the timer that generates the tick ISR.  Interrupts are disabled
     * here already. */
    /* Stop and clear the SysTick. */
    portNVIC_SYSTICK_CTRL_REG = 0UL;
    portNVIC_SYSTICK_CURRENT_VALUE_REG = 0UL;

    /* Configure SysTick to interrupt at the requested rate. */
    portNVIC_SYSTICK_LOAD_REG = (configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ) - 1UL;
    portNVIC_SYSTICK_CTRL_REG =
        (portNVIC_SYSTICK_CLK_BIT_CONFIG | portNVIC_SYSTICK_INT_BIT | portNVIC_SYSTICK_ENABLE_BIT);
}

static uint32_t g_fac_us = 8; // 每us有8个ticks

/**
 * @brief  微秒级延时
 * @param  nus 延时时长，范围：0~233015
 * @retval 无
 */
void Delay_us(uint32_t xus)
{
    uint32_t ticks;
    uint32_t told, tnow, tcnt = 0;
    uint32_t reload = SysTick->LOAD;
    // printf("reload=%d\r\n", reload);
    ticks = xus * g_fac_us;
    // printf("ticks=%d\r\n", ticks);
    told = SysTick->VAL;
    // printf("told=%d\r\n", told);
    while (1) {
        tnow = SysTick->VAL;
        // printf("tnow=%d\r\n", tnow);
        if (tnow != told) {
            if (tnow < told) {
                tcnt += told - tnow;
            }
            else {
                tcnt += reload - (tnow - told);
            }
            told = tnow;
            // printf("tcnt=%d\r\n", tcnt);
            if (tcnt >= ticks) {
                // printf("break\r\n");
                break;
            }
        }
    }
}

/**
 * @brief  毫秒级延时
 * @param  xms 延时时长，范围：0~4294967295
 * @retval 无
 */
void Delay_ms(uint32_t xms)
{
    while (xms--) {
        // printf("xms=%d\r\n", xms);
        Delay_us(1000);
    }
}

/**
 * @brief  秒级延时
 * @param  xs 延时时长，范围：0~4294967295
 * @retval 无
 */
void Delay_s(uint32_t xs)
{
    while (xs--) {
        Delay_ms(1000);
    }
}
