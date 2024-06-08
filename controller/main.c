//*****************************************************************************
//
// Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com/ 
// 
// 
//  Redistribution and use in source and binary forms, with or without 
//  modification, are permitted provided that the following conditions 
//  are met:
//
//    Redistributions of source code must retain the above copyright 
//    notice, this list of conditions and the following disclaimer.
//
//    Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the 
//    documentation and/or other materials provided with the   
//    distribution.
//
//    Neither the name of Texas Instruments Incorporated nor the names of
//    its contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
//  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
//  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
//  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
//  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
//  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
//  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
//  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//*****************************************************************************

//*****************************************************************************
//
// Application Name     - Controller
// Authors:             - Ethan Chan, Anusheel Nand
// Application Overview - This application transmits acceleration data
//                        over UARTA1, allowing for a CC3200 to be used
//                        as a game controller.
//
//*****************************************************************************

//*****************************************************************************
//
//! \addtogroup i2c_demo
//! @{
//
//*****************************************************************************

// Standard includes
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h> //needed for int8_t -> 2's comp BMA222 output
#include <math.h>

// Driverlib includes
#include "hw_types.h"
#include "hw_ints.h"
#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "rom.h"
#include "rom_map.h"
#include "interrupt.h"
#include "prcm.h"
#include "utils.h"
#include "uart.h"

// Common interface includes
#include "uart_if.h"
#include "i2c_if.h"
#include "pinmux.h"
#include "spi.h"

// OLED Includes
#include "Adafruit_GFX.h"
#include "glcdfont.h"
#include "oled_test.h"
#include "Adafruit_SSD1351.h"


//*****************************************************************************
//                      MACRO DEFINITIONS
//*****************************************************************************
#define APPLICATION_VERSION     "1.4.0"
#define SPI_IF_BIT_RATE  100000
#define FOREVER                 1
#define FAILURE                 -1
#define SUCCESS                 0
#define RETERR_IF_TRUE(condition) {if(condition) return FAILURE;}
#define RET_IF_ERR(Func)          {int iRetVal = (Func); \
                                   if (SUCCESS != iRetVal) \
                                     return  iRetVal;}

//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************
#if defined(ccs)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif
//*****************************************************************************
//                 GLOBAL VARIABLES -- End
//*****************************************************************************

volatile int score_p1, score_p2;

volatile float accel_x, accel_y;
volatile float p1_accel_y;
volatile float p2_accel_y;

volatile float paddle1_x = 6;
volatile float paddle1_y = 64;
volatile float paddle2_x = 122;
volatile float paddle2_y = 64;
volatile float ball_x = 64;
volatile float ball_y = 64;

volatile unsigned int paddle_width = 3;
volatile unsigned int paddle_height = 14;
volatile char accel = 0;

//****************************************************************************
//                      LOCAL FUNCTION DEFINITIONS                          
//****************************************************************************

//**************************************************************************************
//
//! Utilizes "readreg" mechanism to read x, y, z accel. values from BMA222 accelerometer
//!
//! \param  char is the axis that has its acceleration value returned by the function
//!
//! \return 8-bit signed acceleration information for specified axis
//!
//***************************************************************************************
int8_t
PollBMA222(char axis)
{
    unsigned char ucDevAddr, ucRegOffset, ucRdLen;
    unsigned char aucRdDataBuf[256];
    //
    // Get the device address
    //
    ucDevAddr = 0x18;
    //
    // Mapping the "axis" parameter to the respective register offsets
    //
    switch(axis)
    {
        case 'x': ucRegOffset = 0x3; break;
        case 'y': ucRegOffset = 0x5; break;
        case 'z': ucRegOffset = 0x7; break;
        default: return -1;
    }


    //
    // Get the length of data to be read (1 byte)
    //
    ucRdLen = 1;

    //
    // Write the register address to be read from.
    // Stop bit implicitly assumed to be 0.
    //
    RET_IF_ERR(I2C_IF_Write(ucDevAddr,&ucRegOffset,1,0));

    //
    // Read the specified length of data
    //
    RET_IF_ERR(I2C_IF_Read(ucDevAddr, &aucRdDataBuf[0], ucRdLen));

    //
    // Return 8-bit signed acceleration information for specified axis
    //
    int8_t accel = aucRdDataBuf[0];
    return accel;
}


//*****************************************************************************
//
//! Board Initialization & Configuration
//!
//! \param  None
//!
//! \return None
//
//*****************************************************************************
static void
BoardInit(void)
{
/* In case of TI-RTOS vector table is initialize by OS itself */
#ifndef USE_TIRTOS
    //
    // Set vector table base
    //
#if defined(ccs)
    MAP_IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);
#endif
#if defined(ewarm)
    MAP_IntVTableBaseSet((unsigned long)&__vector_table);
#endif
#endif
    //
    // Enable Processor
    //
    MAP_IntMasterEnable();
    MAP_IntEnable(FAULT_SYSTICK);

    PRCMCC3200MCUInit();
}

//*****************************************************************************
//
//! Main function handling sliding ball simulation
//!
//! \param  None
//!
//! \return None
//! 
//*****************************************************************************
void main()
{
    //
    // Initialize board configurations
    //
    BoardInit();
    
    //
    // Configure the pinmux settings for I2C communication with BMA222 and SPI communication with OLED
    //
    PinMuxConfig();
    

    // Configure and enable UART1 for communication with other CC3200
    // 115200 BAUD Rate, 8 data bits, two stop bits, even parity
    UARTConfigSetExpClk(UARTA1_BASE, PRCMPeripheralClockGet(PRCM_UARTA1), UART_BAUD_RATE, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_TWO |
            UART_CONFIG_PAR_EVEN));

    // To support smaller message transmissions, set UART interrupt level to lowest possible
    UARTFIFOLevelSet(UARTA1_BASE, UART_FIFO_TX1_8, UART_FIFO_RX1_8);

    //
    // Initiate I2C communication
    //
    I2C_IF_Open(I2C_MASTER_MODE_FST);

    while(FOREVER)
    {
        accel = PollBMA222('y');
        UARTCharPutNonBlocking(UARTA1_BASE, accel);
    }
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @
//
//*****************************************************************************


