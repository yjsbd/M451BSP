/**************************************************************************//**
 * @file     main.c
 * @version  V1.00
 * $Revision: 2 $
 * $Date: 15/09/02 10:03a $
 * @brief    Display an image on TFT LCD panel via SPI interface and control LED.
 *
 * @note
 * @copyright SPDX-License-Identifier: Apache-2.0
 *
 * @copyright Copyright (C) 2014~2015 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#include <stdio.h>
#include "M451Series.h"

#define ILI9341_RESET   PB15
#define ILI9341_DC      PB11
#define ILI9341_LED     PB5

#define LED1            PC3
#define LED2            PC2
#define LED3            PB1
#define LED4            PC7

#define LED_R           PC9
#define LED_G           PC10
#define LED_B           PC11


extern uint8_t u8Image[153600];
extern uint8_t Font8x16[];

#define White           0xFFFF
#define Black           0x0000
#define Blue            0x001F
#define Blue2           0x051F
#define Red             0xF800
#define Magenta         0xF81F
#define Green           0x07E0
#define Cyan            0x7FFF
#define Yellow          0xFFE0

uint8_t LCD_ReadReg(uint8_t u8Comm)
{
    SPI_ClearRxFIFO(SPI2);

    ILI9341_DC = 0;

    SPI_WRITE_TX(SPI2, u8Comm);
    SPI_WRITE_TX(SPI2, 0x00);

    // wait tx finish
    while(SPI_IS_BUSY(SPI2));

    SPI_READ_RX(SPI2);

    return (SPI_READ_RX(SPI2));
}

void LCD_WriteCommand(uint8_t u8Comm)
{
    ILI9341_DC = 0;

    SPI_WRITE_TX(SPI2, u8Comm);

    // wait tx finish
    while(SPI_IS_BUSY(SPI2));
}

void LCD_WriteData(uint8_t u8Data)
{
    ILI9341_DC = 1;

    SPI_WRITE_TX(SPI2, u8Data);

    // wait tx finish
    while(SPI_IS_BUSY(SPI2));
}

void ILI9341_LCD_DisplayOnScreen(void)
{
    uint32_t i, j;
    uint16_t *u16ImgPtr = (uint16_t *) &u8Image[0];

    LCD_WriteCommand(0x2c);

    ILI9341_DC = 1;

    /* Need to disable and then can change data width to 16-bit */
    SPI_DISABLE(SPI2);
    SPI_SET_DATA_WIDTH(SPI2, 16);
    SPI_ENABLE(SPI2);

    /* Load image */
    for(i = 0; i < 320; i++)
    {
        for(j = 0; j < 240; j++)
        {
            while(SPI_GET_TX_FIFO_FULL_FLAG(SPI2));
            SPI_WRITE_TX(SPI2, *(u16ImgPtr++));
        }
    }

    // wait tx finish
    while(SPI_IS_BUSY(SPI2));

    /* Change data width back to 8-bit */
    SPI_DISABLE(SPI2);
    SPI_SET_DATA_WIDTH(SPI2, 8);
    SPI_ENABLE(SPI2);

    LCD_WriteCommand(0x29);
}

void ILI9341_LCD_SetAddress(uint32_t x1, uint32_t x2, uint32_t y1, uint32_t y2)
{
    if(x1 >= 240)
        x1 = 239;
    if(x2 >= 240)
        x2 = 239;
    if(y1 >= 320)
        y1 = 319;
    if(y2 >= 320)
        y2 = 319;

    LCD_WriteCommand(0x2a);
    LCD_WriteData(x1 >> 8);
    LCD_WriteData(x1);
    LCD_WriteData(x2 >> 8);
    LCD_WriteData(x2);

    LCD_WriteCommand(0x2b);
    LCD_WriteData(y1 >> 8);
    LCD_WriteData(y1);
    LCD_WriteData(y2 >> 8);
    LCD_WriteData(y2);
}

void ILI9341_LCD_PutChar8x16(uint16_t x, uint16_t y, uint8_t c, uint32_t fColor, uint32_t bColor)
{
    uint32_t i, j;
    for(i = 0; i < 16; i++)
    {
        uint8_t m = Font8x16[c * 16 + i];
        ILI9341_LCD_SetAddress(x + i, x + i, y, y + 7);
        LCD_WriteCommand(0x2c);

        for(j = 0; j < 8; j++)
        {
            if((m & 0x01) == 0x01)
            {
                LCD_WriteData(fColor >> 8);
                LCD_WriteData(fColor);
            }
            else
            {
                LCD_WriteData(bColor >> 8);
                LCD_WriteData(bColor);
            }
            m >>= 1;
        }
    }
}

void ILI9341_LCD_PutString(uint16_t x, uint16_t y, char *s, uint32_t fColor, uint32_t bColor)
{
    uint8_t l = 0;
    while(*s)
    {
        if(*s < 0x80)
        {
            ILI9341_LCD_PutChar8x16(x, 312 - y - l * 8, *s, fColor, bColor);
            s++;
            l++;
        }
    }
}
void ILI9341_LCD_Init(void)
{
    /* Configure DC/RESET/LED pins */
    ILI9341_DC = 0;
    ILI9341_RESET = 0;
    ILI9341_LED = 0;

    GPIO_SetMode(PB, BIT5, GPIO_MODE_OUTPUT);
    GPIO_SetMode(PB, BIT11, GPIO_MODE_OUTPUT);
    GPIO_SetMode(PB, BIT15, GPIO_MODE_OUTPUT);

    /* Configure LCD */
    ILI9341_DC = 1;

    ILI9341_RESET = 0;
    TIMER_Delay(TIMER0, 20000);

    ILI9341_RESET = 1;
    TIMER_Delay(TIMER0, 40000);

    LCD_WriteCommand(0xCB);
    LCD_WriteData(0x39);
    LCD_WriteData(0x2C);
    LCD_WriteData(0x00);
    LCD_WriteData(0x34);
    LCD_WriteData(0x02);

    LCD_WriteCommand(0xCF);
    LCD_WriteData(0x00);
    LCD_WriteData(0xC1);
    LCD_WriteData(0x30);

    LCD_WriteCommand(0xE8);
    LCD_WriteData(0x85);
    LCD_WriteData(0x00);
    LCD_WriteData(0x78);

    LCD_WriteCommand(0xEA);
    LCD_WriteData(0x00);
    LCD_WriteData(0x00);

    LCD_WriteCommand(0xED);
    LCD_WriteData(0x64);
    LCD_WriteData(0x03);
    LCD_WriteData(0x12);
    LCD_WriteData(0x81);

    LCD_WriteCommand(0xF7);
    LCD_WriteData(0x20);

    LCD_WriteCommand(0xC0);
    LCD_WriteData(0x23);

    LCD_WriteCommand(0xC1);
    LCD_WriteData(0x10);

    LCD_WriteCommand(0xC5);
    LCD_WriteData(0x3e);
    LCD_WriteData(0x28);

    LCD_WriteCommand(0xC7);
    LCD_WriteData(0x86);

    LCD_WriteCommand(0x36);
    LCD_WriteData(0x48);

    LCD_WriteCommand(0x3A);
    LCD_WriteData(0x55);

    LCD_WriteCommand(0xB1);
    LCD_WriteData(0x00);
    LCD_WriteData(0x18);

    LCD_WriteCommand(0xB6);
    LCD_WriteData(0x08);
    LCD_WriteData(0x82);
    LCD_WriteData(0x27);

    LCD_WriteCommand(0xF2);
    LCD_WriteData(0x00);

    LCD_WriteCommand(0x26);
    LCD_WriteData(0x01);

    LCD_WriteCommand(0xE0);
    LCD_WriteData(0x0F);
    LCD_WriteData(0x31);
    LCD_WriteData(0x2B);
    LCD_WriteData(0x0C);
    LCD_WriteData(0x0E);
    LCD_WriteData(0x08);
    LCD_WriteData(0x4E);
    LCD_WriteData(0xF1);
    LCD_WriteData(0x37);
    LCD_WriteData(0x07);
    LCD_WriteData(0x10);
    LCD_WriteData(0x03);
    LCD_WriteData(0x0E);
    LCD_WriteData(0x09);
    LCD_WriteData(0x00);

    LCD_WriteCommand(0xE1);
    LCD_WriteData(0x00);
    LCD_WriteData(0x0E);
    LCD_WriteData(0x14);
    LCD_WriteData(0x03);
    LCD_WriteData(0x11);
    LCD_WriteData(0x07);
    LCD_WriteData(0x31);
    LCD_WriteData(0xC1);
    LCD_WriteData(0x48);
    LCD_WriteData(0x08);
    LCD_WriteData(0x0F);
    LCD_WriteData(0x0C);
    LCD_WriteData(0x31);
    LCD_WriteData(0x36);
    LCD_WriteData(0x0F);

    LCD_WriteCommand(0x11);
    TIMER_Delay(TIMER0, 60000);

    LCD_WriteCommand(0x29);    //Display on

    ILI9341_LED = 1;
}

void LED_Init(void)
{
    GPIO_SetMode(PB, BIT1, GPIO_MODE_OUTPUT);
    GPIO_SetMode(PC, BIT2, GPIO_MODE_OUTPUT);
    GPIO_SetMode(PC, BIT3, GPIO_MODE_OUTPUT);
    GPIO_SetMode(PC, BIT7, GPIO_MODE_OUTPUT);
    GPIO_SetMode(PC, BIT9, GPIO_MODE_OUTPUT);
    GPIO_SetMode(PC, BIT10, GPIO_MODE_OUTPUT);
    GPIO_SetMode(PC, BIT11, GPIO_MODE_OUTPUT);
}

void LED_Light(uint32_t u32LED)
{
    LED1 = u32LED;
    u32LED = u32LED >> 1;
    LED2 = u32LED;
    u32LED = u32LED >> 1;
    LED3 = u32LED;
    u32LED = u32LED >> 1;
    LED4 = u32LED;
    u32LED = u32LED >> 1;
    LED_R = u32LED;
    u32LED = u32LED >> 1;
    LED_G = u32LED;
    u32LED = u32LED >> 1;
    LED_B = u32LED;
    u32LED = u32LED >> 1;
}


void SYS_Init(void)
{
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init System Clock                                                                                       */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Enable External XTAL (4~24 MHz) */
    CLK_EnableXtalRC(CLK_PWRCTL_HXTEN_Msk);

    /* Waiting for 12MHz clock ready */
    CLK_WaitClockReady(CLK_STATUS_HXTSTB_Msk);

    /* Switch HCLK clock source to HXT */
    CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_HXT, CLK_CLKDIV0_HCLK(1));

    /* Set PLL to power down mode and PLL_STB bit in CLKSTATUS register will be cleared by hardware.*/
    CLK->PLLCTL |= CLK_PLLCTL_PD_Msk;

    /* Set PLL frequency */
    CLK->PLLCTL = CLK_PLLCTL_72MHz_HXT;

    /* Waiting for clock ready */
    CLK_WaitClockReady(CLK_STATUS_PLLSTB_Msk);

    /* Switch HCLK clock source to PLL */
    CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_PLL, CLK_CLKDIV0_HCLK(1));

    /* Enable IP clock */
    CLK_EnableModuleClock(UART0_MODULE);
    CLK_EnableModuleClock(TMR0_MODULE);
    CLK_EnableModuleClock(SPI2_MODULE);

    /* Select IP clock source */
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UARTSEL_HXT, CLK_CLKDIV0_UART(1));
    CLK_SetModuleClock(TMR0_MODULE, CLK_CLKSEL1_TMR0SEL_HXT, 0);
    CLK_SetModuleClock(SPI2_MODULE, CLK_CLKSEL2_SPI2SEL_PLL, 0);

    /* Update System Core Clock */
    /* User can use SystemCoreClockUpdate() to calculate SystemCoreClock. */
    SystemCoreClockUpdate();

    /*---------------------------------------------------------------------------------------------------------*/
    /* Init I/O Multi-function                                                                                 */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Set PD multi-function pins for UART0 RXD(PD.6) and TXD(PD.1) */
    SYS->GPD_MFPL &= ~(SYS_GPD_MFPL_PD6MFP_Msk | SYS_GPD_MFPL_PD1MFP_Msk);
    SYS->GPD_MFPL |= (SYS_GPD_MFPL_PD6MFP_UART0_RXD | SYS_GPD_MFPL_PD1MFP_UART0_TXD);

    /* SPI2: GPD12=SS, GPD15=CLK, GPD14=MISO, GPD13=MOSI */
    SYS->GPD_MFPH &= ~(SYS_GPD_MFPH_PD12MFP_Msk | SYS_GPD_MFPH_PD13MFP_Msk | SYS_GPD_MFPH_PD14MFP_Msk | SYS_GPD_MFPH_PD15MFP_Msk);
    SYS->GPD_MFPH |= (SYS_GPD_MFPH_PD12MFP_SPI2_SS | SYS_GPD_MFPH_PD13MFP_SPI2_MOSI | SYS_GPD_MFPH_PD14MFP_SPI2_MISO | SYS_GPD_MFPH_PD15MFP_SPI2_CLK);

    /* Lock protected registers */
    SYS_LockReg();
}



int main(void)
{
    uint32_t u32temp = 0;
    /* Init System, IP clock and multi-function I/O */
    SYS_Init();
    LED_Init();

    /* Init UART to 115200-8n1 for print message */
    UART_Open(UART0, 115200);

    /* Configure SPI3 as a master, MSB first, 8-bit transaction, SPI Mode-0 timing, clock is 4MHz */
    SPI_Open(SPI2, SPI_MASTER, SPI_MODE_0, 8, 4000000);

    /* Configure SPI1 as a low level active device. */
    SPI_EnableAutoSS(SPI2, SPI_SS, SPI_SS_ACTIVE_LOW);

    /* Start SPI */
    SPI_ENABLE(SPI2);

    /* Init LCD */
    ILI9341_LCD_Init();

    /* Setup display address */
    ILI9341_LCD_SetAddress(0, 239, 0, 319);

    /* Show the image on the screen */
    ILI9341_LCD_DisplayOnScreen();

    while(1)
    {
        CLK_SysTickDelay(200000);
        LED_Light(u32temp++);
        if(PA8 == 0)
            ILI9341_LCD_PutString(50, 60, "Hello World!", Red, Yellow);
        if(PD2 == 0)
            ILI9341_LCD_PutString(50, 60, "I Love You!", Red, Yellow);
    }
}
