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
// Application Name     - Final: Pong Game
// Authors:             - Ethan Chan, Anusheel Nand
// Application Overview - This application recreates the traditional Pong game.
//                        It uses the accelerometers of two CC3200s to move
//                        the paddles. Another CC3200's accelerometer is interfaced
//                        over UART using the "controller" project.
//                        A global high score is tracked using AWS IoT services.
//
//*****************************************************************************


//*****************************************************************************
//
//! \addtogroup ssl
//! @{
//
//*****************************************************************************

#include <stdio.h>

// Simplelink includes
#include "simplelink.h"

// Standard includes
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

//Driverlib includes
#include "hw_types.h"
#include "hw_ints.h"
#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "hw_nvic.h"
#include "hw_apps_rcm.h"
#include "rom.h"
#include "rom_map.h"
#include "interrupt.h"
#include "prcm.h"
#include "utils.h"
#include "uart.h"


//Common interface includes
#include "pinmux.h"
#include "gpio_if.h"
#include "common.h"
#include "uart_if.h"
#include "uart.h"
#include "i2c_if.h"
#include "spi.h"
#include "gpio.h"
#include "systick.h"

// Custom includes
#include "utils/network_utils.h"
#include "cJSON.h" // Parsing shadow JSON

// OLED Includes
#include "Adafruit_GFX.h"
#include "glcdfont.h"
#include "oled_test.h"
#include "Adafruit_SSD1351.h"


//*****************************************************************************
//                      MACRO DEFINITIONS
//*****************************************************************************
// Systick

// the cc3200's fixed clock frequency of 80 MHz
#define SYSCLKFREQ 80000000ULL

// macro to convert ticks to microseconds
#define TICKS_TO_US(ticks) \
    ((((ticks) / SYSCLKFREQ) * 1000000ULL) + \
    ((((ticks) % SYSCLKFREQ) * 1000000ULL) / SYSCLKFREQ))\

// macro to convert microseconds to ticks
#define US_TO_TICKS(us) ((SYSCLKFREQ / 1000000ULL) * (us))

// systick reload value set to 40ms period
// (PERIOD_SEC) * (SYSCLKFREQ) = PERIOD_TICKS
#define SYSTICK_RELOAD_VAL 80000000UL

// track systick counter periods elapsed
// if it is not 0, we know the transmission ended
volatile int systick_cnt = 0;


#define SPI_IF_BIT_RATE  100000
#define FOREVER                 1
#define FAILURE                 -1
#define SUCCESS                 0
#define RETERR_IF_TRUE(condition) {if(condition) return FAILURE;}
#define RET_IF_ERR(Func)          {int iRetVal = (Func); \
                                   if (SUCCESS != iRetVal) \
                                     return  iRetVal;}

// States for Pong
#define WAIT 0
#define PLAY 1
#define GAMEOVER 2


// HTTP connection setup

#define DATE                4    /* Current Date */
#define MONTH               6     /* Month 1-12 */
#define YEAR                2024  /* Current year */
#define HOUR                3   /* Time - hours */
#define MINUTE              24    /* Time - minutes */
#define SECOND              0     /* Time - seconds */


#define APPLICATION_NAME      "SSL"
#define APPLICATION_VERSION   "SQ24"
#define SERVER_NAME           "a3g5uhwn3rmn02-ats.iot.us-east-1.amazonaws.com"
#define GOOGLE_DST_PORT       8443


#define POSTHEADER "POST /things/Anusheel_CC3200Board/shadow HTTP/1.1\r\n"
#define GETHEADER "GET /things/Anusheel_CC3200Board/shadow HTTP/1.1\r\n"
#define HOSTHEADER "Host: a3g5uhwn3rmn02-ats.iot.us-east-1.amazonaws.com\r\n"
#define CHEADER "Connection: Keep-Alive\r\n"
#define CTHEADER "Content-Type: application/json; charset=utf-8\r\n"
#define CLHEADER1 "Content-Length: "
#define CLHEADER2 "\r\n\r\n"

volatile char DATA1[200];

//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************

#if defined(ccs) || defined(gcc)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif

volatile long lRetVal = -1;
// Score tracking
volatile int score_p1, score_p2;

// Track accelerometer values
volatile float accel_x, accel_y;
volatile float p1_accel_y;
volatile int8_t p2_accel_y;

// Positions of paddles and Pong ball
volatile float paddle1_x = 6;
volatile float paddle1_y = 64;
volatile float paddle2_x = 122;
volatile float paddle2_y = 64;
volatile float ball_x = 64;
volatile float ball_y = 64;

// Dimensions of paddles
volatile unsigned int paddle_width = 3;
volatile unsigned int paddle_height = 14;

// Required for variable ball speed
volatile int max_ball_speed = 30;
volatile int min_ball_speed = 20;
volatile int ball_speed;
volatile int ball_direction;
volatile int ball_angle;

volatile unsigned long SW3_intcount = 0;

// Track if http_post called in a state
volatile int sent = 0;
// Track game state
volatile int state = 0;
// Track if title/game over written to OLED
volatile int wr = 0;

volatile unsigned int count = 0;
// Track global and local high scores
volatile unsigned int high_score = 0;
volatile unsigned int sendScore = 0;

// Buffer to store device shadow JSON from AWS
volatile char scoreRecv[1460];

// Pins
typedef struct PinSetting {
    unsigned long port;
    unsigned int pin;
} PinSetting;

// GPIO Switch Sensing
static PinSetting sw2 = { .port = GPIOA2_BASE, .pin = 0x40};
static PinSetting sw3 = { .port = GPIOA1_BASE, .pin = 0x20};
// Active buzzer
static PinSetting buzzer = { .port = GPIOA0_BASE, .pin = 0x40};

//*****************************************************************************
//                 GLOBAL VARIABLES -- End: df
//*****************************************************************************

//****************************************************************************
//                      LOCAL FUNCTION PROTOTYPES
//****************************************************************************
static int set_time();
static void BoardInit(void);
static void printincScore(unsigned int col);

// Local functions

/*
 * SysTick Interrupt Handler
 *
 * Keep track of whether the systick counter wrapped
 */
static void SysTickHandler(void) {
    // increment every time the systick handler fires
    systick_cnt++;
}


/**
 * Initializes SysTick Module
 */
static void SysTickInit(void) {

    // configure the reset value for the systick countdown register
    MAP_SysTickPeriodSet(SYSTICK_RELOAD_VAL);

    // register interrupts on the systick module
    MAP_SysTickIntRegister(SysTickHandler);

    // enable interrupts on systick
    // (trigger SysTickHandler when countdown reaches 0)
    MAP_SysTickIntEnable();

    // enable the systick module itself
    MAP_SysTickEnable();
}

static inline void SysTickReset(void) {
    // any write to the ST_CURRENT register clears it
    // after clearing it automatically gets reset without
    // triggering exception logic
    // see reference manual section 3.2.1
    HWREG(NVIC_ST_CURRENT) = 1;

    // clear the global count variable
    systick_cnt = 0;
}


void
PrintBuffer(unsigned char *pucDataBuf, unsigned char ucLen)
{
    unsigned char ucBufIndx = 0;
    while(ucBufIndx < ucLen)
    {
        printf(" 0x%x, ", pucDataBuf[ucBufIndx]);
        ucBufIndx++;
        if((ucBufIndx % 8) == 0)
        {
            printf("\n\r");
        }
    }
    printf("\n\r");
}


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

// Redraw the Pong ball in a new position
void redrawCircle(int new_x, int new_y, unsigned int color)
{
    fillCircle(new_x, new_y, 2, color);
}

// Draw a paddle using fillRect()
void drawPaddle(int x, int y) {
    fillRect(x, y, paddle_width, paddle_height, WHITE);
}
// Erase a paddle
void erasePaddle(int x, int y) {
    fillRect(x, y, paddle_width, paddle_height, BLACK);
}

// Serve the ball in random direction with random speed
void serveBall()
{
    ball_x = 64;
    ball_y = 64;
    fillCircle(ball_x, ball_y, 2, WHITE);

    ball_speed = (rand() % (max_ball_speed - min_ball_speed + 1)) + min_ball_speed;

    // ball direction = 0 -> positive x-direction
    // ball direction = 1 -> negative x-direction
    ball_direction = rand() % (1 - 0 + 1) + 0;
    if (ball_direction == 1) {
       ball_speed = ball_speed * -1;
    }
    ball_angle = (rand() % (70 - (-70) + 1)) + (-70);
    if (ball_angle == 0) {
        ball_angle = 25;
    }
    accel_x = (int)(ball_speed * (cos(ball_angle * (3.14 / 180))));
    accel_y = (int)(ball_speed * (sin(ball_angle * (3.14 / 180))));
}

// Initialize/restart game
void resetGame()
{
    fillScreen(BLACK);
    srand(time(0));

    score_p1 = 0;
    score_p2 = 0;
    sent = 0;
    wr = 0;
    count = 0;


    // Serve the ball randomly
    serveBall();

    // Draw initial local scores on OLED
    drawChar(30, 10, '0', BLUE, BLACK, 2);
    drawChar(45, 10, '0', BLUE, BLACK, 2);
    drawChar(75, 10, '0', RED, BLACK, 2);
    drawChar(90, 10, '0', RED, BLACK, 2);
}

// Update position of ball every tick
void updateBall()
{
    // Turn off buzzer if turned on previously
    GPIOPinWrite(buzzer.port, buzzer.pin, 0x0);

    // Redraw the ball in a new x position if still in OLED bounds
    if((ball_x < 128 && ball_x > 2))
    {
        redrawCircle(ball_x,ball_y, BLACK);
        ball_x += (accel_x) / 10;
    }
    else // Player missed the ball -> Increase opponent score
    {
        redrawCircle(ball_x, ball_y, BLACK);
        if(accel_x < 0) // Use acceleration data to determine who missed
        {
            printincScore(RED);

        }
        else
        {
            printincScore(BLUE);
        }
        serveBall();
    }
    // Redraw ball in new y-position
    if((ball_y <= 126 && ball_y >= 2) )
    {
        redrawCircle(ball_x,ball_y, BLACK);
        ball_y += (accel_y) / 10;

        // Bounce off the top and bottom edges of the OLED
        if(ball_y >= 126)
        {
            ball_y = 126;
            accel_y = -accel_y;
        }
        if(ball_y <= 2)
        {
            ball_y = 2;
            accel_y = -accel_y;
        }
    }
    // Bounce if ball hits left paddle
    if((ball_x < 7) && ((ball_y < paddle1_y + 14) && (ball_y > paddle1_y - 1)))
    {
        accel_x = -accel_x;
        GPIOPinWrite(buzzer.port, buzzer.pin, 0x40); // Turn on buzzer to simulate hit sound
    }
    // Bounce if ball hits right paddle
    if((ball_x > 121) && ((ball_y < paddle2_y + 14) && (ball_y > paddle2_y - 1)))
    {
        accel_x = -accel_x;
        GPIOPinWrite(buzzer.port, buzzer.pin, 0x40); // Turn on buzzer to simulate hit sound
    }
    redrawCircle(ball_x, ball_y, WHITE);
}

// Move left paddle using data from on-board accelerometer
void updatePaddle1()
{
    p1_accel_y = PollBMA222('y');

    // Same logic as sliding ball lab, but in y-axis
    if(p1_accel_y >= 5 || p1_accel_y <= -5)
    {
        if((paddle1_y < 110 && paddle1_y > 5) || (paddle1_y == 110 && p1_accel_y <= -5) || (paddle1_y == 5 && p1_accel_y >= 5))
        {
            erasePaddle(paddle1_x, paddle1_y);
            if (p1_accel_y > 0) {
                paddle1_y += (p1_accel_y * p1_accel_y) / 90;
            } else {
                paddle1_y -= (p1_accel_y * p1_accel_y) / 90;
            }

            if(paddle1_y >= 110)
            {
                paddle1_y = 110;
            }
            if(paddle1_y <= 5)
            {
                paddle1_y = 5;
            }
        }
    }
    drawPaddle(paddle1_x, paddle1_y);
}
// Move right paddle using data from UART
// Acceleration data is received using UARTIntHandler
void updatePaddle2()
{
    if(p2_accel_y >= 5 || p2_accel_y <= -5)
    {
        if((paddle2_y < 112 && paddle2_y > 5) || (paddle2_y == 112 && p2_accel_y <= -5) || (paddle2_y == 5 && p2_accel_y >= 5))
        {
            erasePaddle(paddle2_x, paddle2_y);
            if (p2_accel_y > 0) {
                paddle2_y += (p2_accel_y * p2_accel_y) / 90;
            } else {
                paddle2_y -= (p2_accel_y * p2_accel_y) / 90;
            }

            if(paddle2_y >= 112)
            {
                paddle2_y = 112;
            }
            if(paddle2_y <= 5)
            {
                paddle2_y = 5;
            }
        }
    }
    drawPaddle(paddle2_x, paddle2_y);
}
// UART Rx Interrupt handler that updates acceleration of right paddle
static void UARTIntHandler()
{
    p2_accel_y = UARTCharGetNonBlocking(UARTA1_BASE);
}
// Print incremented player score after erasing old score
static void printincScore(unsigned int col)
{
    char player_score;
    switch(col)
    {
        case BLUE:
                // Erase old score
                drawChar(45, 10, (score_p1 % 10) + '0' , BLACK, BLACK, 2);
                drawChar(30, 10, (score_p1 / 10) + '0' , BLACK, BLACK, 2);
                // Increment score
                score_p1++;
                // Print new score
                drawChar(45, 10, (score_p1 % 10) + '0', BLUE, BLACK, 2);
                drawChar(30, 10, (score_p1 / 10) + '0', BLUE, BLACK, 2);
                drawChar(90, 10, (score_p2 % 10) + '0', RED, BLACK, 2);
                drawChar(75, 10, (score_p2 / 10) + '0', RED, BLACK, 2);
                break;
        case RED:
                // Erase old score
                drawChar(90, 10, (score_p2 % 10) + '0' , BLACK, BLACK, 2);
                drawChar(75, 10, (score_p2 / 10) + '0' , BLACK, BLACK, 2);
                // Increment score
                score_p2++;
                // Print new score
                drawChar(90, 10, (score_p2 % 10) + '0', RED, BLACK, 2);
                drawChar(75, 10, (score_p2 / 10) + '0', RED, BLACK, 2);
                drawChar(45, 10, (score_p1 % 10) + '0', BLUE, BLACK, 2);
                drawChar(30, 10, (score_p1 / 10) + '0', BLUE, BLACK, 2);
                break;
    }
}

// Modified http_get() from Lab 4
// Sends HTTP GET request to AWS
// Parses received JSON data for global high score
static int http_get(int iTLSSockID){
    char acSendBuff[512];
    char cCLLength[200];
    char acRecvbuff[1460];
    char* pcBufHeaders;
    int lRetVal = 0;

    pcBufHeaders = acSendBuff;
    strcpy(pcBufHeaders, GETHEADER);
    pcBufHeaders += strlen(GETHEADER);
    strcpy(pcBufHeaders, HOSTHEADER);
    pcBufHeaders += strlen(HOSTHEADER);
    strcpy(pcBufHeaders, CHEADER);
    pcBufHeaders += strlen(CHEADER);
    strcpy(pcBufHeaders, "\r\n\r\n");


    UART_PRINT(acSendBuff);


    //
    // Send the packet to AWS
    //
    lRetVal = sl_Send(iTLSSockID, acSendBuff, strlen(acSendBuff), 0);
    if(lRetVal < 0) {
        UART_PRINT("POST failed. Error Number: %i\n\r",lRetVal);
        sl_Close(iTLSSockID);
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
        return lRetVal;
    }
    //
    // Receive JSON data
    //
    lRetVal = sl_Recv(iTLSSockID, &acRecvbuff[0], sizeof(acRecvbuff), 0);
    if(lRetVal < 0) {
        UART_PRINT("Received failed. Error Number: %i\n\r",lRetVal);
        //sl_Close(iSSLSockID);
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
           return lRetVal;
    }
    else {
        acRecvbuff[lRetVal+1] = '\0';
        UART_PRINT(acRecvbuff);
        UART_PRINT("\n\r\n\r");

        // Parse the JSON string
        char *json_string = &acRecvbuff[189]; // Ignore HTTP header information
        if (json_string[0] != '{') { // Check if HTTP header changed from last time
            printf("JSON data is corrupted.\n");
            return -1;
        }

        // Shadow parsing done using cJSON: https://github.com/DaveGamble/cJSON
        // Populate cJSON tree
        cJSON *json = cJSON_Parse(json_string);
        // Get value of "state" from JSON
        cJSON *state = cJSON_GetObjectItemCaseSensitive(json, "state");
        // Get value of "desired" from JSON
        cJSON *desired = cJSON_GetObjectItemCaseSensitive(state, "desired");
        // Get value of "score" from JSON
        cJSON *score = cJSON_GetObjectItemCaseSensitive(desired, "score");
        
        // Initialize global high score
        // ->valueint did not work, hence use of atoi() and valuestring
        high_score = atoi(score->valuestring);
    }
    return 0;
}

// This function sends a HTTP POST request over the socket.
// It has been modified to send the local high score to AWS
static int http_post(int iTLSSockID){
    char acSendBuff[512];
    char acRecvbuff[1460];
    char cCLLength[200];
    char* pcBufHeaders;
    int lRetVal = 0;

    sprintf(DATA1, "{" \
            "\"state\": {\r\n"                                              \
                "\"desired\" : {\r\n"                                       \
                    "\"score\" :\""                                           \
                        "%d"                                     \
                        ""                  \
                        "\"\r\n"                                            \
                "}"                                                         \
            "}"                                                             \
        "}\r\n\r\n", sendScore);

    pcBufHeaders = acSendBuff;
    strcpy(pcBufHeaders, POSTHEADER);
    pcBufHeaders += strlen(POSTHEADER);
    strcpy(pcBufHeaders, HOSTHEADER);
    pcBufHeaders += strlen(HOSTHEADER);
    strcpy(pcBufHeaders, CHEADER);
    pcBufHeaders += strlen(CHEADER);
    strcpy(pcBufHeaders, "\r\n\r\n");

    int dataLength = strlen(DATA1);

    strcpy(pcBufHeaders, CTHEADER);
    pcBufHeaders += strlen(CTHEADER);
    strcpy(pcBufHeaders, CLHEADER1);

    pcBufHeaders += strlen(CLHEADER1);
    sprintf(cCLLength, "%d", dataLength);

    strcpy(pcBufHeaders, cCLLength);
    pcBufHeaders += strlen(cCLLength);
    strcpy(pcBufHeaders, CLHEADER2);
    pcBufHeaders += strlen(CLHEADER2);

    strcpy(pcBufHeaders, DATA1);
    pcBufHeaders += strlen(DATA1);

    int testDataLength = strlen(pcBufHeaders);

    UART_PRINT(acSendBuff);


    //
    // Send the packet to the server */
    //
    lRetVal = sl_Send(iTLSSockID, acSendBuff, strlen(acSendBuff), 0);
    if(lRetVal < 0) {
        UART_PRINT("POST failed. Error Number: %i\n\r",lRetVal);
        sl_Close(iTLSSockID);
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
        return lRetVal;
    }
    lRetVal = sl_Recv(iTLSSockID, &acRecvbuff[0], sizeof(acRecvbuff), 0);
    if(lRetVal < 0) {
        UART_PRINT("Received failed. Error Number: %i\n\r",lRetVal);
        //sl_Close(iSSLSockID);
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
           return lRetVal;
    }
    else {
        acRecvbuff[lRetVal+1] = '\0';
        UART_PRINT(acRecvbuff);
        UART_PRINT("\n\r\n\r");
    }

    return 0;
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
static void BoardInit(void) {
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
//! This function updates the date and time of CC3200.
//!
//! \param None
//!
//! \return
//!     0 for success, negative otherwise
//!
//*****************************************************************************

static int set_time() {
    long retVal;

    g_time.tm_day = DATE;
    g_time.tm_mon = MONTH;
    g_time.tm_year = YEAR;
    g_time.tm_sec = SECOND;
    g_time.tm_hour = HOUR;
    g_time.tm_min = MINUTE;

    retVal = sl_DevSet(SL_DEVICE_GENERAL_CONFIGURATION,
                          SL_DEVICE_GENERAL_CONFIGURATION_DATE_TIME,
                          sizeof(SlDateTime),(unsigned char *)(&g_time));

    ASSERT_ON_ERROR(retVal);
    return SUCCESS;
}

//*****************************************************************************
//
//! Main
//!
//! \param  none
//!
//! \return None
//!
//*****************************************************************************
void main() {

    unsigned long ulStatus;
    long lRetVal = -1;

    // Initialize board configuration
    BoardInit();
    PinMuxConfig();

    // Clear UART0 terminal
    InitTerm();
    ClearTerm();

    // Reset SPI
    GPIOPinWrite(GPIOA2_BASE, 0x2, 0x2); // Set CS to high initially
    MAP_SPIReset(GSPI_BASE);

    UART_PRINT("My terminal works!\n\r");

    // initialize global default app configuration
    g_app_config.host = SERVER_NAME;
    g_app_config.port = GOOGLE_DST_PORT;

    // Connect the CC3200 to the local access point
    lRetVal = connectToAccessPoint();

    // Set time so that encryption can be used
    lRetVal = set_time();
    if(lRetVal < 0) {
        UART_PRINT("Unable to set time in the device");
        LOOP_FOREVER();
    }

    // Connect to AWS endpoint with TLS encryption
    lRetVal = tls_connect();
    if(lRetVal < 0) {
        ERR_PRINT(lRetVal);
    }

    // Configure SPI interface with mode 0 (CPOL = 0, CPHA = 0)
    MAP_SPIConfigSetExpClk(GSPI_BASE,MAP_PRCMPeripheralClockGet(PRCM_GSPI),
                     SPI_IF_BIT_RATE,SPI_MODE_MASTER,SPI_SUB_MODE_0,
                     (SPI_SW_CTRL_CS |
                     SPI_4PIN_MODE |
                     SPI_TURBO_OFF |
                     SPI_CS_ACTIVEHIGH |
                     SPI_WL_8));

    // Register UART receive interrupt handler for Paddle 2
    UARTIntRegister(UARTA1_BASE, UARTIntHandler);
    UARTIntEnable(UARTA1_BASE, UART_INT_RX);

    // Configure and enable UART1 for communication with other CC3200
    // 115200 BAUD Rate, 8 data bits, two stop bits, even parity
    UARTConfigSetExpClk(UARTA1_BASE, PRCMPeripheralClockGet(PRCM_UARTA1), UART_BAUD_RATE, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_TWO |
            UART_CONFIG_PAR_EVEN));

    // FIFO level set to minimum for faster updates
    UARTFIFOLevelSet(UARTA1_BASE, UART_FIFO_TX1_8, UART_FIFO_RX1_8);

    // Enable SPI for OLED
    MAP_SPIEnable(GSPI_BASE);
    MAP_SPICSEnable(GSPI_BASE);

    // Initialize display
    Adafruit_Init();
    fillScreen(BLACK);
    wr = 0; // Nothing written to OLED yet

    // Initiate I2C communication with BMA222 accelerometer
    I2C_IF_Open(I2C_MASTER_MODE_FST);


    while(1)
    {
        switch(state)
        {
            case WAIT: // Print title screen until SW2 pressed
            {
                if(wr == 0)
                {
                    setCursor(18, 42);
                    Outstr("PONG", 4);
                    setCursor(10, 82);
                    Outstr("Play: SW2", 2);
                    wr = 1;
                    http_get(lRetVal);
                }
                // SW2 pressed
                if(GPIOPinRead(sw2.port, sw2.pin) == sw2.pin)
                {
                    resetGame();
                    state = PLAY;
                }
                break;
            }
            case PLAY: // Play Pong for a minute
            {
                count++;
                updateBall();
                updatePaddle1();
                updatePaddle2();
                if(count == 950)
                {
                    state = GAMEOVER;
                    GPIOPinWrite(buzzer.port, buzzer.pin, 0);
                    wr = 0;
                }
                break;
            }
            case GAMEOVER: // Game over screen, display global high score
            {
                if(wr == 0)
                {
                    fillScreen(BLACK);
                    setCursor(10, 22);
                    Outstr("High score: ", 2);
                    sendScore = score_p1;
                    if(score_p2 > score_p1)
                    {
                        sendScore = score_p2;
                    }
                }
                if(sent == 0 && (sendScore > high_score)) // Update new global high score
                {
                    drawChar(70, 64, (sendScore % 10) + '0', MAGENTA, BLACK, 2);
                    drawChar(55, 64, (sendScore / 10) + '0', MAGENTA, BLACK, 2);
                    setCursor(10, 82);
                    Outstr("Play: SW2", 2);
                    wr = 1;
                    sent = 1;
                    http_post(lRetVal);
                }
                else if(wr == 0) // or just print old high score
                {
                    drawChar(70, 64, (high_score % 10) + '0', WHITE, BLACK, 2);
                    drawChar(55, 64, (high_score / 10) + '0', WHITE, BLACK, 2);
                    setCursor(10, 86);
                    Outstr("Play: SW3", 2);
                    wr = 1;
                }
                if(GPIOPinRead(sw3.port, sw3.pin) == sw3.pin)
                {
                    fillScreen(BLACK);
                    state = WAIT;
                    wr = 0;
                }
                break;
            }
        }
    }
}
//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************


