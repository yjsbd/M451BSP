/********************************************************************************
 * @file     main.c
 * @version  V3.0
 * $Revision: 2 $
 * $Date: 15/09/02 10:03a $
 * @brief    Demonstrate how to use PDMA scatter gather mode and trigger DAC by PWM.
 * @note
 * @copyright SPDX-License-Identifier: Apache-2.0
 *
 * @copyright Copyright (C) 2014~2015 Nuvoton Technology Corp. All rights reserved.
 ********************************************************************************/
#include "stdio.h"
#include "M451Series.h"

#define PLLCTL_SETTING      CLK_PLLCTL_72MHz_HXT
#define PLL_CLOCK           72000000

/*---------------------------------------------------------------------------------------------------------*/
/* Define global variables and constants                                                                   */
/*---------------------------------------------------------------------------------------------------------*/
/* The positive duty table of Sine wave */
const uint16_t sinePos[] = {2047, 2251, 2453, 2651, 2844, 3028, 3202, 3365, 3515, 3650, 3769, 3871, 3954,
                         4019, 4064, 4088, 4095, 4076, 4040, 3984, 3908, 3813, 3701, 3573, 3429, 3272,
                         3102, 2921, 2732, 2536, 2335, 2132
                        };
/* The negative duty table of Sine wave */
const uint16_t sineNeg[] = {1927, 1724, 1523, 1328, 1141,  962,  794, 639,  497,  371,  262,  171,   99,
                         45,   12,    0,    7,   35,   84,  151, 238,  343,  465,  602,  754,  919, 1095,
                         1281, 1475, 1674, 1876
                        };

static uint32_t index = 0;
const uint32_t array_Pos_size = sizeof(sinePos) / sizeof(uint16_t);
const uint32_t array_Neg_size = sizeof(sineNeg) / sizeof(uint16_t);

typedef struct dma_desc_t
{
    uint32_t ctl;
    uint32_t src;
    uint32_t dest;
    uint32_t offset;
} DMA_DESC_T;

DMA_DESC_T DMA_DESC[2];
uint32_t gu32DMAConfig = 0;
uint32_t gu32DMAConfigPos = 0;
uint32_t gu32DMAConfigNeg = 0;

/*---------------------------------------------------------------------------------------------------------*/
/* Define functions prototype                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
int32_t main(void);
void DAC_FunctionTest(void);


void SYS_Init(void)
{
	uint32_t u32TimeOutCnt;


    /*---------------------------------------------------------------------------------------------------------*/
    /* Init System Clock                                                                                       */
    /*---------------------------------------------------------------------------------------------------------*/

    /* Enable HIRC clock (Internal RC 22.1184MHz) */
    CLK->PWRCTL |= CLK_PWRCTL_HIRCEN_Msk;

    /* Waiting for HIRC clock ready */
    u32TimeOutCnt = __HIRC;
	while(!(CLK->STATUS & CLK_STATUS_HIRCSTB_Msk))
		if(--u32TimeOutCnt == 0) break;

    /* Select HCLK clock source as HIRC and HCLK clock divider as 1 */
    CLK->CLKSEL0 &= ~CLK_CLKSEL0_HCLKSEL_Msk;
    CLK->CLKSEL0 |= CLK_CLKSEL0_HCLKSEL_HIRC;
    CLK->CLKDIV0 &= ~CLK_CLKDIV0_HCLKDIV_Msk;
    CLK->CLKDIV0 |= CLK_CLKDIV0_HCLK(1);

    /* Set PLL to Power-down mode and PLLSTB bit in CLK_STATUS register will be cleared by hardware.*/
    CLK->PLLCTL |= CLK_PLLCTL_PD_Msk;

    /* Enable HXT clock (external XTAL 12MHz) */
    CLK->PWRCTL |= CLK_PWRCTL_HXTEN_Msk;

    /* Waiting for HXT clock ready */
    u32TimeOutCnt = __HIRC;
	while(!(CLK->STATUS & CLK_STATUS_HXTSTB_Msk))
		if(--u32TimeOutCnt == 0) break;

    /* Set core clock as PLL_CLOCK from PLL */
    CLK->PLLCTL = PLLCTL_SETTING;
    u32TimeOutCnt = __HIRC;
	while(!(CLK->STATUS & CLK_STATUS_PLLSTB_Msk))
		if(--u32TimeOutCnt == 0) break;
    CLK->CLKSEL0 &= (~CLK_CLKSEL0_HCLKSEL_Msk);
    CLK->CLKSEL0 |= CLK_CLKSEL0_HCLKSEL_PLL;

    /* Update system core clock */
    PllClock        = PLL_CLOCK;            // PLL
    SystemCoreClock = PLL_CLOCK / 1;        // HCLK
    CyclesPerUs     = PLL_CLOCK / 1000000;  // For CLK_SysTickDelay()

    /* Enable UART module clock */
    CLK->APBCLK0 |= CLK_APBCLK0_UART0CKEN_Msk;

    /* Select UART module clock source as HXT and UART module clock divider as 1 */
    CLK->CLKSEL1 &= ~CLK_CLKSEL1_UARTSEL_Msk;
    CLK->CLKSEL1 |= CLK_CLKSEL1_UARTSEL_HXT;
    CLK->CLKDIV0 &= ~CLK_CLKDIV0_UARTDIV_Msk;
    CLK->CLKDIV0 |= CLK_CLKDIV0_UART(1);

    /* Enable PWM0 module clock */
    CLK->APBCLK1 |= CLK_APBCLK1_PWM0CKEN_Msk;

    /* Select PWM0 module clock source as PLL */
    CLK->CLKSEL2 &= ~CLK_CLKSEL2_PWM0SEL_Msk;
    CLK->CLKSEL2 |= CLK_CLKSEL2_PWM0SEL_PLL;

    /* Enable DAC module clock */
    CLK->APBCLK1 |= CLK_APBCLK1_DACCKEN_Msk;

    /* Enable PDMA module clock */
    CLK->AHBCLK |= CLK_AHBCLK_PDMACKEN_Msk;

    /*---------------------------------------------------------------------------------------------------------*/
    /* Init I/O Multi-function                                                                                 */
    /*---------------------------------------------------------------------------------------------------------*/

    /* Set PD multi-function pins for UART0 RXD and TXD */
    SYS->GPD_MFPL &= ~(SYS_GPD_MFPL_PD0MFP_Msk | SYS_GPD_MFPL_PD1MFP_Msk);
    SYS->GPD_MFPL |= (SYS_GPD_MFPL_PD0MFP_UART0_RXD | SYS_GPD_MFPL_PD1MFP_UART0_TXD);

    /* Set PB multi-function pins for DAC voltage output */
    SYS->GPB_MFPL &= ~SYS_GPB_MFPL_PB0MFP_Msk;
    SYS->GPB_MFPL |= SYS_GPB_MFPL_PB0MFP_DAC;

    /* Set PC multi-function pins for PWM0 Channel 0 */
    SYS->GPC_MFPL = (SYS->GPC_MFPL & (~SYS_GPC_MFPL_PC0MFP_Msk));
    SYS->GPC_MFPL |= SYS_GPC_MFPL_PC0MFP_PWM0_CH0;

}

void UART0_Init()
{
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init UART                                                                                               */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Reset UART IP */
    SYS->IPRST1 |=  SYS_IPRST1_UART0RST_Msk;
    SYS->IPRST1 &= ~SYS_IPRST1_UART0RST_Msk;

    /* Configure UART0 and set UART0 baud rate */
    UART0->BAUD = UART_BAUD_MODE2 | UART_BAUD_MODE2_DIVIDER(__HXT, 115200);
    UART0->LINE = UART_WORD_LEN_8 | UART_PARITY_NONE | UART_STOP_BIT_1;
}

void PWM0_Init()
{
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init PWM0                                                                                               */
    /*---------------------------------------------------------------------------------------------------------*/

    /* Set PWM0 clock prescaler */
    PWM_SET_PRESCALER(PWM0, 0, 0);

    /* Set up counter type */
    PWM0->CTL1 &= ~PWM_CTL1_CNTTYPEn_Msk;

    /* Set PWM0 timer duty */
    PWM_SET_CMR(PWM0, 0, 360);

    /* Set PWM0 timer period */
    PWM_SET_CNR(PWM0, 0, 720);

    /* PWM period point trigger DAC enable */
    PWM0->DACTRGEN = 0x1 << PWM_DACTRGEN_PTEn_Pos;

    /* Set waveform generation */
    PWM0->WGCTL0 = 0xAAA;//PWM zero point and period point output High.
    PWM0->WGCTL1 = 0x555;//PWM compare down point and  compare up point output Low.

    /* Enable output of all PWM0 channels */
    PWM0->POEN |= PWM_POEN_POENn_Msk;
}

/*---------------------------------------------------------------------------------------------------------*/
/* DAC function test                                                                                       */
/*---------------------------------------------------------------------------------------------------------*/
void DAC_FunctionTest(void)
{
    static uint32_t u32TableIndex = 0;

    printf("\n");
    printf("+----------------------------------------------------------------------+\n");
    printf("|        DAC PWM trigger with PDMA scatter gather mode test            |\n");
    printf("+----------------------------------------------------------------------+\n");
    printf("\n");
    printf("This sample code use PDMA scatter gather mode transfer sine wave table to DAC(PB.0) output.\n");

    /* Reset DAC module */
    SYS->IPRST2 |= SYS_IPRST2_DACRST_Msk ;
    SYS->IPRST2 &= ~SYS_IPRST2_DACRST_Msk ;

    /* CH0 source request from DAC */
    PDMA->REQSEL0_3 = 0x08; //CH0_SEl=DAC

    gu32DMAConfigPos = ((array_Pos_size - 1) << PDMA_DSCT_CTL_TXCNT_Pos) | PDMA_WIDTH_16 | PDMA_SAR_INC | PDMA_DAR_FIX | PDMA_REQ_SINGLE | PDMA_OP_SCATTER;
    gu32DMAConfigNeg = ((array_Neg_size - 1) << PDMA_DSCT_CTL_TXCNT_Pos) | PDMA_WIDTH_16 | PDMA_SAR_INC | PDMA_DAR_FIX | PDMA_REQ_SINGLE | PDMA_OP_SCATTER;

    DMA_DESC[0].ctl = gu32DMAConfigPos;
    DMA_DESC[0].src = (uint32_t)&sinePos[index];
    DMA_DESC[0].dest = (uint32_t)&DAC->DAT;
    DMA_DESC[0].offset = (uint32_t)&DMA_DESC[1] - (PDMA->SCATBA);

    DMA_DESC[1].ctl = gu32DMAConfigNeg;
    DMA_DESC[1].src = (uint32_t)&sineNeg[index];
    DMA_DESC[1].dest = (uint32_t)&DAC->DAT;
    DMA_DESC[1].offset = (uint32_t)&DMA_DESC[0] - (PDMA->SCATBA);

    /* Enable PDMA channel 0 */
    PDMA->CHCTL=0x1;
    PDMA->DSCT[0].CTL = PDMA_OP_SCATTER;
    PDMA->DSCT[0].NEXT = (uint32_t)&DMA_DESC[0] - (PDMA->SCATBA);


    /* Set the PWM0 trigger,enable DAC even trigger mode and enable D/A converter */
    DAC->CTL = DAC_PWM0_TRIGGER | DAC_CTL_TRGEN_Msk | DAC_CTL_DMAEN_Msk | DAC_CTL_DACEN_Msk;

    /* When DAC controller APB clock speed is 72MHz and DAC conversion settling time is 8us,
       the selected SETTLET value must be greater than 0x241.  */
    DAC->TCTL = 0x250;

    /* Set DAC 12-bit holding data */
    DAC->DAT = 2047/*sine[index]*/;

    /* Clear the DAC conversion complete finish flag for safe */
    DAC->STATUS = DAC_STATUS_FINISH_Msk;

    /* Enable the DAC interrupt.  */
    DAC->CTL |= DAC_CTL_DACIEN_Msk;
    NVIC_EnableIRQ(DAC_IRQn);

    printf("\nHit any key to quit!\n");

    /* Enable PWM0 chann0 to start D/A conversion */
    PWM0->CNTEN |= 0x1 << PWM_CNTEN_CNTENn_Pos;

    /* initial gu32DMAConfig value as gu32DMAConfigPos */
    gu32DMAConfig = gu32DMAConfigPos;

    while(1)
    {
        if (PDMA->TDSTS == 0x1)
        {
            /* Re-Set transfer count and basic operation mode */
            DMA_DESC[u32TableIndex].ctl = gu32DMAConfig;
            u32TableIndex ^= 1;
            if (u32TableIndex == 0)
            {
                gu32DMAConfig = gu32DMAConfigPos;
            }
            else
            {
                gu32DMAConfig = gu32DMAConfigNeg;
            }
            /* Clear CH0 transfer done flag */
            PDMA->TDSTS = 0x01;
        }

        if((DEBUG_PORT->FIFOSTS & UART_FIFOSTS_RXEMPTY_Msk) != 0)
            continue;
        else
        {
            /* PWM0 counter and clock prescaler stop running. */
            PWM0->CNTEN &= ~PWM_CNTEN_CNTENn_Msk;
            break;
        }
    }

    return;
}

/*---------------------------------------------------------------------------------------------------------*/
/* DAC interrupt handler                                                                                   */
/*---------------------------------------------------------------------------------------------------------*/
void DAC_IRQHandler(void)
{

    if(DAC->STATUS & DAC_STATUS_FINISH_Msk)
    {
        DAC->STATUS = DAC_STATUS_FINISH_Msk;
    }
    return;
}

/*---------------------------------------------------------------------------------------------------------*/
/*  Main Function                                                                                          */
/*---------------------------------------------------------------------------------------------------------*/
int32_t main(void)
{

    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Init System, IP clock and multi-function I/O */
    SYS_Init();

    /* Lock protected registers */
    SYS_LockReg();

    /* Init UART0 for printf */
    UART0_Init();

    /* Init PWM0 for DAC */
    PWM0_Init();

    /*---------------------------------------------------------------------------------------------------------*/
    /* SAMPLE CODE                                                                                             */
    /*---------------------------------------------------------------------------------------------------------*/

    printf("\nSystem clock rate: %d Hz", SystemCoreClock);

    /* DAC function test */
    DAC_FunctionTest();

    /* Disable External Interrupt */
    NVIC_DisableIRQ(DAC_IRQn);

    /* Reset PDMA module */
    SYS->IPRST0 |= SYS_IPRST0_PDMARST_Msk ;
    SYS->IPRST0 &= ~SYS_IPRST0_PDMARST_Msk ;

    /* Reset DAC module */
    SYS->IPRST2 |= SYS_IPRST2_DACRST_Msk ;
    SYS->IPRST2 &= ~SYS_IPRST2_DACRST_Msk ;

    /* Reset PWM0 module */
    SYS->IPRST2 |= SYS_IPRST2_PWM0RST_Msk ;
    SYS->IPRST2 &= ~SYS_IPRST2_PWM0RST_Msk ;

    /* Disable PDMA module clock */
    CLK->AHBCLK &= ~CLK_AHBCLK_PDMACKEN_Msk;

    /* Disable PWM0 IP clock */
    CLK->APBCLK1 &= ~CLK_APBCLK1_PWM0CKEN_Msk;

    /* Disable DAC IP clock */
    CLK->APBCLK1 &= ~CLK_APBCLK1_DACCKEN_Msk;

    printf("Stop DAC output and exit DAC sample code\n");

    while(1);

}
