demo EE2024
=====================
This project contains an example using several of the peripherals
on the LPCXpresso base Board.

With the joystick it is possible to draw on the OLED display
With the Rotary witch it is possible to control the 7-segment display
With the trimpot it is possible to control the RGB LED
Moving the board (accelerometer) will control a running pattern on the LEDs
Click SW3 button to start playing a melody on the Speaker

The project makes use of code from the following library projects:
- CMSISv1p30_LPC17xx : for CMSIS 1.30 files relevant to LPC17xx
- MCU_Lib        	 : for LPC17xx peripheral driver files
- EaBaseBoard_Lib    : for Embedded Artists LPCXpresso Base Board peripheral drivers

These library projects must exist in the same workspace in order
for the project to successfully build.
=====================
Implementing a Car Automation and Telemetry System (CATS) in an electric car 
The CATS will send data periodically to a server known as Recorder for Automated Telemetry (RAT)

The base board is a larger prototype version of CATS, with several output devices to help in the debugging during development
The XBee RF module is assumed to be a low powered wireless communication device that sends collected data to RAT

The main purpose of the CATS is to use sensors for data capture and data transmission
The reason behind the use of the three main sensors are in the table below: 
> Accelerometer: Mounted in the front of the car to detect collisions and deploy airbag to minimise injuries to passengers 
> Light Sensor: Simulate a radar, where the intensity increases as the obstacle is closer 
> Temperature Sensor: Measures the temperature of the battery in the electric car to reduce the risk of explosion 

CATS has three modes of operation namely Stationary mode, Forward mode and Reverse mode
1. Stationary mode is the default mode when CATS is initiated. This mode has all sensors idle and is not reading or transmitting any data 
2. Forward mode is entered with one press of Switch Button 3. This mode monitors the temperature of the battery and detects collisions to trigger any emergency response
3. Reverse mode is entered with two presses of Switch Button 3. This mode measures distance to obstacles based on ambient light intensity and indicates how close nearby objects are  
