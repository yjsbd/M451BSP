
/******************************************************************************
 * @file     main.c
 * @version  V0.10
 * $Revision: 8 $
 * $Date: 15/09/02 10:03a $
 * @brief    Demonstrate how ACMP works with internal band-gap voltage.
 * @note
 * @copyright SPDX-License-Identifier: Apache-2.0
 * @copyright Copyright (C) 2014~2015 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
#include <stdio.h>
#include "M451Series.h"


/* Function prototype declaration */
void SYS_Init(void);
void UART_Init(void);

int32_t main(void)
{
    /* Unlock protected registers */
    SYS_UnlockReg();
    /* Init System, IP clock and multi-function I/O. */
    SYS_Init();
    /* Lock protected registers */
    SYS_LockReg();

    /* Init UART for print message */
    UART_Init();

    printf("\n\n");
    printf("+---------------------------------------+\n");
    printf("|         M451 ACMP Sample Code         |\n");
    printf("+---------------------------------------+\n");

    printf("\nThis sample code demonstrates ACMP0 function. Using ACMP0_P0 (PB7) as ACMP0\n");
    printf("positive input and using internal band-gap voltage as the negative input.\n");
    printf("The compare result reflects on ACMP0_O (PD6).\n");

    printf("When the voltage of the positive input is greater than the voltage of the negative input,\n");
    printf("the analog comparator outputs logical one; otherwise, it outputs logical zero.\n");
    printf("This sample code will show the expression of the comparator's inputs and a sequence ");
    printf("number when detecting a transition of analog comparator's output.\n");
    printf("Press any key to start ...");
    getchar();
    printf("\n");

    /* Select band-gap voltage as the source of ACMP negative input */
    ACMP_SET_NEG_SRC(ACMP01, 0, ACMP_CTL_NEGSEL_VBG);
    /* Enable ACMP0 */
    ACMP_ENABLE(ACMP01, 0);
    /* Enable interrupt */
    ACMP_ENABLE_INT(ACMP01, 0);

    /* Enable ACMP01 interrupt */
    NVIC_EnableIRQ(ACMP01_IRQn);

    while(1);

}

void ACMP01_IRQHandler(void)
{
    static uint32_t u32Cnt = 0;

    /* Clear ACMP 0 interrupt flag */
    ACMP_CLR_INT_FLAG(ACMP01, 0);
    /* Check Comparator 0 Output Status */
    if(ACMP_GET_OUTPUT(ACMP01, 0))
        printf("ACMP0_P voltage > Band-gap voltage (%d)\n", u32Cnt);
    else
        printf("ACMP0_P voltage <= Band-gap voltage (%d)\n", u32Cnt);

    u32Cnt++;
}


void SYS_Init(void)
{
	uint32_t u32TimeOutCnt;

    /*---------------------------------------------------------------------------------------------------------*/
    /* Init System Clock                                                                                       */
    /*---------------------------------------------------------------------------------------------------------*/

    /* Enable external 12MHz XTAL */
    CLK->PWRCTL |= CLK_PWRCTL_HXTEN_Msk;

    /* Waiting for clock ready */
    u32TimeOutCnt = __HIRC;
	while(!(CLK->STATUS & CLK_STATUS_HXTSTB_Msk))
		if(--u32TimeOutCnt == 0) break;

    /* Select HXT as the clock source of UART */
    CLK->CLKSEL1 &= (~CLK_CLKSEL1_UARTSEL_Msk);
    CLK->CLKSEL1 |= CLK_CLKSEL1_UARTSEL_HXT;

    /* Enable UART and ACMP clock */
    CLK->APBCLK0 = CLK_APBCLK0_UART0CKEN_Msk | CLK_APBCLK0_ACMP01CKEN_Msk;

    /* Update System Core Clock */
    /* User can use SystemCoreClockUpdate() to calculate SystemCoreClock and CyclesPerUs automatically. */
    SystemCoreClockUpdate();

    /*---------------------------------------------------------------------------------------------------------*/
    /* Init I/O Multi-function                                                                                 */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Set PB7 multi-function pin for ACMP0 positive input pin */
    SYS->GPB_MFPL &= ~SYS_GPB_MFPL_PB7MFP_Msk;
    SYS->GPB_MFPL |= SYS_GPB_MFPL_PB7MFP_ACMP0_P0;

    /* Set PD6 multi-function pin for ACMP0 output pin */
    SYS->GPD_MFPL &= ~SYS_GPD_MFPL_PD6MFP_Msk;
    SYS->GPD_MFPL |= SYS_GPD_MFPL_PD6MFP_ACMP0_O;

    /* Set PD multi-function pins for UART0 RXD and TXD */
    SYS->GPD_MFPL &= ~(SYS_GPD_MFPL_PD0MFP_Msk | SYS_GPD_MFPL_PD1MFP_Msk);
    SYS->GPD_MFPL |= (SYS_GPD_MFPL_PD0MFP_UART0_RXD | SYS_GPD_MFPL_PD1MFP_UART0_TXD);

    /* Disable digital input path of analog pin ACMP0_P to prevent leakage */
    PB->DINOFF |= (1 << GPIO_DINOFF_DINOFF7_Pos);

}

void UART_Init(void)
{
    /* Word length is 8 bits; 1 stop bit; no parity bit. */
    UART0->LINE = UART_WORD_LEN_8 | UART_PARITY_NONE | UART_STOP_BIT_1;
    /* UART peripheral clock rate 12MHz; UART bit rate 115200 bps. */
    UART0->BAUD = UART_BAUD_MODE2 | UART_BAUD_MODE2_DIVIDER(__HXT, 115200);
}

/*** (C) COPYRIGHT 2014~2015 Nuvoton Technology Corp. ***/
