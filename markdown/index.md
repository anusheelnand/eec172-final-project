---
title: 'PvPong'
author: '**Anusheel Nand and Ethan Chan** (website template by Ryan Tsang)'
date: '*EEC172 SP24*'

toc-title: 'Table of Contents'
abstract-title: '<h2>Description</h2>'
abstract: 'Our product is a handheld 2-player recreation of the classic Pong game. A main CC3200 is responsible for controlling an Adafruit SSD1351 OLED display according to the game logic characteristic of Pong. The main CC3200’s accelerometer values are also used to control player 1’s paddle. Another CC3200 is used to control player 2’s paddle. This is facilitated by sending the second CC3200’s accelerometer data over a UART connection to the main CC3200 responsible for game logic. AWS IoT Core is used to keep track of the global high score for this implementation of Pong. As long as the main CC3200 has an internet connection, the two players will be able to compete for a common high score even if the CC3200s are power cycled.
<br/><br/>
Our source code can be found 
<!-- replace this link -->
<a href="https://github.com/ucd-eec172/project-website-example">
  here</a>.

<h2>Video Demo</h2>
<div style="text-align:center;margin:auto;max-width:560px">
  <div style="padding-bottom:56.25%;position:relative;height:0;">
    <iframe style="left:0;top:0;width:100%;height:100%;position:absolute;" width="560" height="315" src="https://www.youtube.com/embed/YGBqSsvvzqU?si=l45ZtsbULXn7ugIY" title="YouTube video player" frameborder="0" allow="accelerometer; clipboard-write; encrypted-media; gyroscope; picture-in-picture; web-share" referrerpolicy="strict-origin-when-cross-origin" allowfullscreen></iframe>
  </div>
</div>
'
---

<!-- EDIT METADATA ABOVE FOR CONTENTS TO APPEAR ABOVE THE TABLE OF CONTENTS -->
<!-- ALL CONTENT THAT FOLLWOWS WILL APPEAR IN AND AFTER THE TABLE OF CONTENTS -->

# Design

## Functional Specification

<div style="display:flex;flex-wrap:wrap;justify-content:space-evenly;">
  <div style="display:flex; justify-content:center; align-items:center; width:100%; height:100%">
    <div class="fig">
      <img src="./media/fsm.png" style="width:auto;height:4in" />
      <span class="caption">Figure 1: Game States</span>
    </div>
  </div>
  <div style="display:inline-block;vertical-align:top;flex:1 0 300px;">
	When the main CC3200 is powered on, it first connects to an AWS endpoint which will be used for the game’s global high score tracking. Upon successfully connecting to the endpoint, the CC3200 will then retrieve the current global high score from its device shadow on AWS and display a start menu on the Adafruit OLED to indicate the start menu state. The game will remain in the start menu state until the main CC3200’s SW2 switch is pressed.
	Once the SW2 switch is pressed, the game then transitions to the play game state. During this state, a timer is incremented. When the game timer reaches 950, the game will then transition to the game over state and display a game over screen on the OLED.
	The game will stay in the game over state until the SW3 switch is pressed. Once that switch is pressed, the game will retrieve the current global high score from AWS again and loop back to the start menu state.
  </div>
    <div style="display:flex; justify-content:center; align-items:center; width:100%; height:100%">
    <div class="fig">
      <img src="./media/p2_paddle.png" style="width:auto;height:2in" />
      <span class="caption">Figure 2: P2 Paddle</span>
    </div>
  </div>
   <div style="display:inline-block;vertical-align:top;flex:1 0 300px;">
	The figure above depicts the control flow of the second player’s paddle. The CC3200 used as the second controller first polls y-axis data from its accelerometer via I2C. A variable named accel is then set to that y-axis data and is then sent to the UART port UARTA1_BASE to be transmitted to the main CC3200.
  </div>
    <div style="display:flex; justify-content:center; align-items:center; width:100%; height:100%">
    <div class="fig">
      <img src="./media/screen_up.png" style="width:auto;height:2in" />
      <span class="caption">Figure 3: Game/Screen updating</span>
    </div>
  </div>
     <div style="display:inline-block;vertical-align:top;flex:1 0 300px;">
	Figure 3 provides a high-level diagram of the game’s updating logic in the play game state. The game program first increments its timer. The game program then updates the positions of the ball and paddles. The ball’s position is updated based on if it meets certain conditions such as being within the OLED display’s boundaries. Updating the paddles’ positions involves the program reading in data polled from the main CC3200’s accelerometer and accelerometer data transmitted from the Player 2 controller via UART. After the positions of the ball and paddles are updated, they are then redrawn on the OLED display. The game program then checks if its timer has reached 950. If it has, the game program will then update the OLED display to show the game over screen. Otherwise, the game program continues running and loops back to the increment timer step.
  </div>
</div>

## System Architecture

<div style="display:flex;flex-wrap:wrap;justify-content:space-evenly;">
  <div style="display:flex; justify-content:center; align-items:center; width:100%; height:100%">
    <div class="fig">
      <img src="./media/system_arch.png" style="width:auto;height:4in" />
      <span class="caption">Figure 4: High-level system architecture of Pong</span>
    </div>
  </div>
  <div style="display:inline-block;vertical-align:top;flex:1 0 400px;">
	The main component of the product is a main CC3200 that is programmed with the Pong game’s logic and controller code. In order to double as a paddle controller, the main CC3200 additionally communicates with its on-board BMA222 accelerometer via I2C.
	The main CC3200 is connected to another CC3200 board which serves as a second paddle controller. Like the main CC3200, the second CC3200 board communicates with its on-board accelerometer via I2C. In order to send accelerometer data to the main CC3200 to update the second paddle’s position, the second CC3200 makes use of UART interrupts.
	The main CC3200 is additionally connected to multiple peripherals to provide several game functions. The main CC3200 is connected to two GPIO inputs, switches SW2 and SW3, which are used to start or reset the game. The main board is additionally connected to a GPIO output, a 5-volt buzzer, for game audio. In order to display the game’s graphics, the main board also communicates with an Adafruit OLED via SPI.
	To implement global high score tracking, the main CC3200 is also connected to an AWS endpoint. In order to retrieve or update the global high score stored in its device shadow, the main board makes use of HTTP POST methods.
  </div>
</div>

# Implementation

## BMA222 Accelerometer Reading

Controlling the paddles in Pong is the most integral part of the game. The BMA222 accelerometers on two TI CC3200 were used to implement a reliable, interactive paddle control system, creating an arguably better playing experience than the original Pong controllers. 

A function called PollBMA222 was carried over from the sliding ball implementation in Lab 2. This function reads registers containing the BMA222’s x, y,  and z acceleration values and returns the desired axis’ 8-bit value using a switch statement. Communication with the BMA222 is done over I2C, specifically using functions I2C_IF_Write to write the desired register address and I2C_IF_Read to read the produced data. For the paddle implementations, only the y-axis of the accelerometer was needed.

## CC3200 Controller over UART

A two-player implementation of Pong requires data from two accelerometers. Thus, a second TI CC3200 is used as a “controller” with a direct connection to the “main” CC3200. The controller CC3200 runs a separate program that constantly polls the accelerometer’s y-axis register and sends the acceleration value over UART. This is done using a forever loop that calls PollBMA222 and uses the returned accelerometer value as the argument for the UARTCharPut function. UARTCharPut is found in the TI SDK’s “uart.h” file, allowing for simple UART data transmission. This data transmission specifically occurs over UARTA1.

Pins used to receive and transmit over UARTA1 were configured using  TI SysConfig. UARTA1 was further configured using the UARTConfigSetExpClk function from uart.h. To minimize player 2’s disadvantage due to UART, flags for two stop bits and even parity were set. These flags minimize the chance of transmitting corrupted data, allowing for a better gaming experience for both players. 

The “main” CC3200 was then set up with UART receive interrupts to allow for instant acceleration updates for player 2’s paddle.  Functions UARTIntRegister and UARTIntEnable from uart.h were used to configure these interrupts. Additionally, UARTFIFOLevelSet was used to trigger interrupts at the minimum possible FIFO level for the least amount of latency. 

The UART receive interrupt handler simply uses UARTCharGet from uart.h to set the player 2 paddle’s acceleration value to the accelerometer value from the controller CC3200.
## Pong Game Logic

Pong’s game logic was effectively translated into C code. First, SPI was initialized to control the Adafruit OLED display with SSD1351 driver. The graphics libraries Adafruit_GFX.h and oled_test.h were carried over from Lab 2, along with the sliding ball implementation. The required pins, MOSI, SPI CLK,  Reset, DC, and Chip Select were initialized using TI SysConfig. The ball is drawn simply using the fillCircle function from Adafruit_GFX.h. The paddles are similarly drawn using fillRect with a height of 14 pixels and a width of 3 pixels in our implementation.

The game is divided into three states: waiting, playing, and game-over. When the main CC3200 is turned on, the game begins in the “waiting” state. In this state, a “PONG” title card is printed and the game prompts the user to press SW2 on the CC3200. When a SW2 press is sensed using GPIOPinRead, the game transitions to the “playing” state. The ball and the two paddles are drawn and the game begins. 

At the start of the game, the ball is initialized with a random velocity, angle, and direction using a serveBall function. This function utilizes rand from stdlib.h to generate a random velocity, angle, and direction. The ball’s direction is randomized by selecting 0 or 1 with 0 indicating a positive x-direction and 1 indicating a negative x-direction. In general, quantities were randomly generated using the following equation:
$random Quantity = rand()  \%\ (largest Possible Value - smallest Possible Value + 1) + smallest Possible Value$. For velocity, the possible range of values was 20 to 30. For angle, the possible range of values was -70 to 70.

In order to prevent the ball from initially having a straight trajectory, the randomly generated angle was automatically set to 25 degrees if it was generated as 0 degrees.

After generating a random velocity; angle; and direction, the x-component and y-component of the random velocity are calculated as follows:
```
accel_x = (int)(ball_speed * (cos(ball_angle * (3.14 / 180))));
accel_y = (int)(ball_speed * (sin(ball_angle * (3.14 / 180))));
```
Figure 5 depicts the trigonometric derivations that those calculations were based on:
  <div style="display:flex; justify-content:center; align-items:center; width:100%; height:100%">
    <div class="fig">
      <img src="./media/fig5.png" style="width:auto;height:4in" />
      <span class="caption">Figure 5: Diagram of x- and y-components of velocity</span>
    </div>
  </div>
  
After the ball is served, the updateBall function keeps the ball movement valid. If the ball’s x and y position is still within the 128x128 pixel OLED, the ball is constantly redrawn according to its current speed and direction. This redrawing mechanism allows the ball to seemingly fly across the screen. When the ball reaches the top or bottom of the display, its y-axis acceleration is inverted to produce the bouncing behavior. 

When the ball reaches the left or right edges of the screen, it can bounce off a paddle or fly off the screen. Much like the top and bottom edge bouncing, “if'' statements were used to create a region of x and y position values that cause the ball to bounce. The ball bounces off the left paddle when its x-position is less than the x-position of the left paddle incremented by 3 to account for the paddle width. The ball’s y-position must also be less than the paddle’s y-position + paddle height, and greater than the paddle’s y-position - 1. The same mechanism applies to the right side of the display, but the x-position of the ball must be greater than paddle2’s x-position - 3. If the ball is not in either of these regions when at the edge of the display, it is erased and the ball is served again. The player on the opposite side of the screen has their score variable incremented. An active buzzer is also turned on to emulate the noise of a ball hitting a paddle. GPIOPinWrite from gpio.h is used to trigger the “base” pin of a 2N2222A transistor, connecting the active buzzer to the 3.3 volts supplied to the “collector” pin. This 3.3 volts comes from the same source as the 3.3 volt input for the OLED, the VCC pin of the CC3200. 

Lastly, score tracking was implemented on the OLED. Player 1 and player 2 score variables are continually updated throughout the game. drawChar from Adafruit_GFX.h is used to print each player’s score on their side of the display. Since drawChar draws according to an ASCII argument, it cannot individually handle double-digit numbers. Thus, two drawChar functions are used every score update. The first drawChar prints the relevant score modulo 10, representing the ones digit. The second drawChar prints the relevant score divided by 10, representing the tens digit. These scores are initially set to 0, with blue numbers corresponding to player 1 and red numbers corresponding to player 2. The function printincScore was created to simplify displaying an incremented score value. printincScore accepts color arguments “BLUE'' and “RED'' to choose which score to increment on the display. In either case, the current score is printed in black to erase it from the display. Next, the corresponding score integer variable is incremented. Finally, the two drawChar mechanism is used to display the updated double-digit score value. 

After the game is played for a minute, the game state transitions to “game over.” This incentivizes players to play as aggressively as possible against each and gain the maximum amount of points in a short period of time. To further incentivize point gain, the global high score for the game is tracked using AWS IoT. 
## Global High Score using AWS IoT Core

In the “game over” state, this Pong implementation prints the highest score ever achieved when playing the game. This mechanism initializes when the main CC3200 first turns on. The libraries simplelink.h and network_utils.h were specifically used to establish a connection with an AWS IoT device shadow. 

An AWS account was created in the us-east-1 region, allowing for access to the AWS IoT Core service. The IoT Core console was used to create a “Device Thing” to represent the main CC3200 in the IoT cloud. A “Device Shadow” was created to track the state of the CC3200, including our global high score. The device shadow consists of a JSON file, where the global high score is a “key” containing the score value. SSL certificates were also generated in the “Security” tab to ensure that the main CC3200 has a secure  connection to the device shadow endpoint. 

The connection method is directly carried over from Lab 4, where the functions set_time, connectToAccessPoint, and tls_connect are used. Set_time relies on user-defined macros to set the current time in the CC3200. connectToAccessPoint establishes a connection with the Wi-Fi SSID defined in common.h. Tls_connect securely connects to the AWS endpoint defined in the SERVER_NAME macro. This secure SSL connection requires the main TI CC3200 to be flashed with the root certificate, private certificate, and root certificate generated earlier.

When the CC3200 successfully establishes a secure connection to the AWS endpoint, the game state transitions to the “waiting” state to display the title card. While waiting for SW2 to be pressed, the function http_get is called to “get” the current CC3200’s IoT shadow state. Http_get loads a send buffer with the standard HTTP get header information corresponding to the IoT shadow state endpoint. This buffer is sent to the endpoint using sl_Send from socket.h, and the resulting device shadow JSON is placed in a string variable using sl_Recv. Traversing the JSON string manually for the score value is unwieldy, requiring the use of an external library. cJSON is a C library specifically for traversing JSONs stored in strings [1]. The function cJSON_Parse was used with the receive buffer to convert the string into a cJSON tree [1]. The function cJSON_GetObjectItemCaseSensitive was used on this cJSON tree to retrieve the “state” key of the shadow, the “desired” key, and finally the “score” key. A “high_score” variable was set to the value of the score key, allowing for the game to keep track of the highest score ever achieved even if the main CC3200 is turned off.

When the game state finally reaches “game over,” the scores of player 1 and player 2 are compared to the global high_score variable. If the current players’ scores are less than the global high score, the global score is simply printed to the OLED. If one of the players manages to beat the global high score, then the device shadow is updated with this new score using http_post. Http_post is implemented much like http_get, except with an updated device shadow JSON sent along with the POST header information. The updated device shadow JSON is loaded as a string using sprintf, where the score key’s value is the decimal format specifier ‘%d.’ sprintf is additionally called with the sendScore variable to define the value of the decimal format specifier. sendScore is a global variable updated with the highest player score value when the game reaches the “game over” state. This mechanism thus creates a cycle of http_post functions updating the device shadow when the game ends and http_get functions retrieving device shadow information when the game first begins. The two players can try again for the high score by pressing the reset button, SW3. The reset button transitions the game state back to the initial state, where http_get is used to pull the global high score and the title screen is displayed.
# Challenges

## Pong Game Mechanics

Pong seems like a straightforward game to implement, but there are plenty of opportunities to create game-breaking conditions. Through much testing, bugs like bouncing the ball backwards or getting stuck in a loop of bouncing were removed.
## Interrupts

Interrupts seem like the best way to implement key game features like keeping track of the time (SysTick) or switching states due to button presses (GPIO Interrupts). However, the addition of these interrupts lead to puzzling glitches in the Pong game logic. Adding SysTick interrupts resulted in “ghost” balls and paddles appearing on the display. GPIO Interrupts would inadvertently break execution of key features like http_get in the waiting state and http_post in the game-over state. Thus, interrupts were used only for the essential function of receiving acceleration data from the controller CC3200 over UART.
# Future Work

A more advanced integration with AWS is one of the most significant things to be done in the future. The AWS Amplify service allows for the creation of a relatively simple web application that integrates directly with AWS IoT Core [2]. This could allow for the creation of a “leaderboard” web page that interfaces with the main CC3200’s device shadow. The web page would display real-time changes in high score, along with the ability to add time-stamps.
# Finalized BOM

<table cellspacing="0" cellpadding="0" dir="ltr" border="1" style="" data-sheets-root="1">
  <thead>
    <tr style="height:21px;">
      <th>Manufacturer</th>
      <th>Product</th>
      <th>Price</th>
    </tr>
  </thead><colgroup><col width="100"><col width="100"><col width="120"></colgroup>
  <tbody>
    <tr style="height:21px;">
      <td style="overflow:hidden;padding:2px 3px 2px 3px;vertical-align:bottom;font-family:Cambria;font-size:11pt;font-weight:normal;wrap-strategy:4;white-space:normal;word-wrap:break-word;" data-sheets-value="{&quot;1&quot;:2,&quot;2&quot;:&quot;Texas Instruments&quot;}">Texas Instruments</td>
      <td style="overflow:hidden;padding:2px 3px 2px 3px;vertical-align:bottom;font-family:Cambria;font-size:11pt;font-weight:normal;wrap-strategy:4;white-space:normal;word-wrap:break-word;" data-sheets-value="{&quot;1&quot;:2,&quot;2&quot;:&quot;CC3200&quot;}">CC3200</td>
      <td style="overflow:hidden;padding:2px 3px 2px 3px;vertical-align:bottom;font-family:Cambria;font-size:11pt;font-weight:normal;wrap-strategy:4;white-space:normal;word-wrap:break-word;" data-sheets-value="{&quot;1&quot;:2,&quot;2&quot;:&quot;2x $66.00\n(Mouser/Digikey)&quot;}">2x $66.00<br>(Mouser/Digikey)</td>
    </tr>
    <tr style="height:21px;">
      <td style="overflow:hidden;padding:2px 3px 2px 3px;vertical-align:bottom;font-family:Cambria;font-size:11pt;font-weight:normal;wrap-strategy:4;white-space:normal;word-wrap:break-word;" data-sheets-value="{&quot;1&quot;:2,&quot;2&quot;:&quot;Adafruit&quot;}">Adafruit</td>
      <td style="overflow:hidden;padding:2px 3px 2px 3px;vertical-align:bottom;font-family:Cambria;font-size:11pt;font-weight:normal;wrap-strategy:4;white-space:normal;word-wrap:break-word;" data-sheets-value="{&quot;1&quot;:2,&quot;2&quot;:&quot;16-bit OLED\nSSD1351&quot;}">16-bit OLED<br>SSD1351</td>
      <td style="overflow:hidden;padding:2px 3px 2px 3px;vertical-align:bottom;font-family:Cambria;font-size:11pt;font-weight:normal;wrap-strategy:4;white-space:normal;word-wrap:break-word;" data-sheets-value="{&quot;1&quot;:2,&quot;2&quot;:&quot;$39.95\n(Adafruit)&quot;}">$39.95<br>(Adafruit)</td>
    </tr>
    <tr style="height:21px;">
      <td style="overflow:hidden;padding:2px 3px 2px 3px;vertical-align:bottom;font-family:Cambria;font-size:11pt;font-weight:normal;wrap-strategy:4;white-space:normal;word-wrap:break-word;" data-sheets-value="{&quot;1&quot;:2,&quot;2&quot;:&quot;Adafruit&quot;}">Adafruit</td>
      <td style="overflow:hidden;padding:2px 3px 2px 3px;vertical-align:bottom;font-family:Cambria;font-size:11pt;font-weight:normal;wrap-strategy:4;white-space:normal;word-wrap:break-word;" data-sheets-value="{&quot;1&quot;:2,&quot;2&quot;:&quot;Buzzer 5V&quot;}">Buzzer 5V</td>
      <td style="overflow:hidden;padding:2px 3px 2px 3px;vertical-align:bottom;font-family:Cambria;font-size:11pt;font-weight:normal;wrap-strategy:4;white-space:normal;word-wrap:break-word;" data-sheets-value="{&quot;1&quot;:2,&quot;2&quot;:&quot;$0.95\n(Adafruit)&quot;}">$0.95<br>(Adafruit)</td>
    </tr>
    <tr style="height:21px;">
      <td style="overflow:hidden;padding:2px 3px 2px 3px;vertical-align:bottom;font-family:Cambria;font-size:11pt;font-weight:normal;wrap-strategy:4;white-space:normal;word-wrap:break-word;" data-sheets-value="{&quot;1&quot;:2,&quot;2&quot;:&quot;EDGELEC&quot;}">EDGELEC</td>
      <td style="overflow:hidden;padding:2px 3px 2px 3px;vertical-align:bottom;font-family:Cambria;font-size:11pt;font-weight:normal;wrap-strategy:4;white-space:normal;word-wrap:break-word;" data-sheets-value="{&quot;1&quot;:2,&quot;2&quot;:&quot;50cm Jumper Wires&quot;}">50cm Jumper Wires</td>
      <td style="overflow:hidden;padding:2px 3px 2px 3px;vertical-align:bottom;font-family:Cambria;font-size:11pt;font-weight:normal;wrap-strategy:4;white-space:normal;word-wrap:break-word;" data-sheets-value="{&quot;1&quot;:2,&quot;2&quot;:&quot;$6\n(eBay)&quot;}">$6<br>(eBay)</td>
    </tr>
    <tr style="height:21px;">
      <td style="overflow:hidden;padding:2px 3px 2px 3px;vertical-align:bottom;font-family:Cambria;font-size:11pt;font-weight:normal;wrap-strategy:4;white-space:normal;word-wrap:break-word;" data-sheets-value="{&quot;1&quot;:2,&quot;2&quot;:&quot;Amazon Web Services&quot;}">Amazon Web Services</td>
      <td style="overflow:hidden;padding:2px 3px 2px 3px;vertical-align:bottom;font-family:Cambria;font-size:11pt;font-weight:normal;wrap-strategy:4;white-space:normal;word-wrap:break-word;" data-sheets-value="{&quot;1&quot;:2,&quot;2&quot;:&quot;IoT Core&quot;}">IoT Core</td>
      <td style="overflow:hidden;padding:2px 3px 2px 3px;vertical-align:bottom;font-family:Cambria;font-size:11pt;font-weight:normal;wrap-strategy:4;white-space:normal;word-wrap:break-word;" data-sheets-value="{&quot;1&quot;:2,&quot;2&quot;:&quot;Free Tier (for 12 months)&quot;}">Free Tier (for 12 months)</td>
    </tr>
  </tbody>
</table>

 
# References

[1] D. Gamble, “cJSON: Ultralightweight JSON parser.” Github. [github.com/DaveGamble/cJSON](http://github.com/DaveGamble/cJSON) (accessed May 2024).

[2] Amazon Web Services, “AWS End-to-End IoT Amplify Application” Github. [github.com/aws-samples/aws-end-to-end-iot-amplify-demo](http://github.com/aws-samples/aws-end-to-end-iot-amplify-demo) (accessed May 2024).