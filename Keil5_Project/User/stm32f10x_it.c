/**
 ******************************************************************************
 * @file    Project/STM32F10x_StdPeriph_Template/stm32f10x_it.c
 * @author  MCD Application Team
 * @version V3.5.0
 * @date    08-April-2011
 * @brief   Main Interrupt Service Routines.
 *          This file provides template for all exceptions handler and
 *          peripherals interrupt service routine.
 ******************************************************************************
 * @attention
 *
 * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
 * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
 * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
 * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
 * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
 * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
 *
 * <h2><center>&copy; COPYRIGHT 2011 STMicroelectronics</center></h2>
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x_it.h"
#include "stdbool.h"

/** @addtogroup STM32F10x_StdPeriph_Template
 * @{
 */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M3 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
 * @brief  This function handles NMI exception.
 * @param  None
 * @retval None
 */
void NMI_Handler(void) {}

/**
 * @brief  This function handles Hard Fault exception.
 * @param  None
 * @retval None
 */
// void HardFault_Handler(void)
// {
//     /* Go to infinite loop when Hard Fault exception occurs */
//     while (1) {
//     }
// }
#if 1
__asm void HardFault_Handler(void)
{
    extern HardFault_Handler_C;

    TST LR, #4    // if(lr[2]==1)
    ITE EQ 
    MRSEQ R0, MSP // lr[2] == 0, 代表使用的是msp，将其写入r0, 相当于给HardFault_Handler_C传参
    MRSNE R0, PSP // lr[2] == 1, 将psp写入r0
    B HardFault_Handler_C // 无条件跳转到HardFault_Handler_C
}
#endif

typedef struct {
    uint32_t R0;
    uint32_t R1;
    uint32_t R2;
    uint32_t R3;
    uint32_t R12;
    uint32_t LR;
    uint32_t PC;
    uint32_t xPSR;
} StackBuffer;

void HardFault_Handler_C(StackBuffer *debugBuffer)
{
    // 打印进入Handler之前硬件压栈的寄存器
    printf("HardFault_Handler is running\r\n");
#if 0
    printf("R0:0X%08x\r\n", debugBuffer->R0);
    printf("R1:0X%08x\r\n", debugBuffer->R1);
    printf("R2:0X%08x\r\n", debugBuffer->R2);
    printf("R3:0X%08x\r\n", debugBuffer->R3);
    printf("R12:0X%08x\r\n", debugBuffer->R12);
    printf("LR:0X%08x\r\n", debugBuffer->LR);
    printf("PC:0X%08x\r\n", debugBuffer->PC);
    printf("xPSR:0X%08x\r\n", debugBuffer->xPSR);

    // 打印函数调用过程中压栈的内容
    uint32_t *funcStackPointer = (uint32_t *)((uint32_t)debugBuffer + sizeof(StackBuffer));
    printf("funcStack:\r\n");

    for (int i = 0; i < 1024; i++) {
        // 判断是否代码段地址
        bool isCodeAddress = ((*funcStackPointer & 0xFFFF0000) == 0x08000000);

        uint32_t *previousAddress = (uint32_t *)(*funcStackPointer - 4 - 1);
        // 判断上一条指令是否BL指令
        // bool ThePreviousIsBL = ((*previousAddress & 0xf800f000) == 0xf800f000);

        // 打印出函数跳转地址
        if (isCodeAddress) {
            printf("reg val: 0x%08x\r\n", *funcStackPointer);
        }

        funcStackPointer++;
    }
#endif


    // 最后必须while循环,否则会回到触发fault的地方,再次触发fault
    while (1);
}

/**
 * @brief  This function handles Memory Manage exception.
 * @param  None
 * @retval None
 */
void MemManage_Handler(void)
{
    /* Go to infinite loop when Memory Manage exception occurs */
    while (1) {
    }
}

/**
 * @brief  This function handles Bus Fault exception.
 * @param  None
 * @retval None
 */
void BusFault_Handler(void)
{
    /* Go to infinite loop when Bus Fault exception occurs */
    while (1) {
    }
}

/**
 * @brief  This function handles Usage Fault exception.
 * @param  None
 * @retval None
 */
void UsageFault_Handler(void)
{
    /* Go to infinite loop when Usage Fault exception occurs */
    while (1) {
    }
}

/**
 * @brief  This function handles SVCall exception.
 * @param  None
 * @retval None
 */
void SVC_Handler(void)
{
    extern void vPortSVCHandler();
    vPortSVCHandler();
}

/**
 * @brief  This function handles Debug Monitor exception.
 * @param  None
 * @retval None
 */
void DebugMon_Handler(void) {}

/**
 * @brief  This function handles PendSVC exception.
 * @param  None
 * @retval None
 */
void PendSV_Handler(void)
{
    extern void xPortPendSVHandler();
    xPortPendSVHandler();
}

/**
 * @brief  This function handles SysTick Handler.
 * @param  None
 * @retval None
 */
void SysTick_Handler(void)
{
    extern void xPortSysTickHandler();
    xPortSysTickHandler();
}

/******************************************************************************/
/*                 STM32F10x Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f10x_xx.s).                                            */
/******************************************************************************/

/**
 * @brief  This function handles PPP interrupt request.
 * @param  None
 * @retval None
 */
/*void PPP_IRQHandler(void)
{
}*/

/**
 * @}
 */

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
