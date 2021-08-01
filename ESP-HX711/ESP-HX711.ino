/*********************************************************************
This is an example sketch for our Monochrome Nokia 5110 LCD Displays

Pick one up today in the adafruit shop!
------> http://www.adafruit.com/products/338

These displays use SPI to communicate, 4 or 5 pins are required to
interface

Adafruit invests time and resources providing this open source code,
please support Adafruit and open-source hardware by purchasing
products from Adafruit!

Written by Limor Fried/Ladyada  for Adafruit Industries.
BSD license, check license.txt for more information
All text above, and the splash screen must be included in any redistribution

84x48 individual pixels

* Pin 1 (VDD) -> +3.3V (rightmost, when facing the display head-on) connected to Arduino digital pin 8.
* Pin 2 (SCLK)-> Arduino digital pin 3
* Pin 3 (SI)-> Arduino digital pin 4
* Pin 4 (D/C)-> Arduino digital pin 6
* Pin 5 (CS)-> Arduino digital pin 7
* Pin 6 (GND)-> Ground
* Pin 7 (Vout)-> 10uF capacitor -> Ground
* Pin 8 (RESet)-> Arduino digital pin 5
* 
*   3      SI   serial data input of LCD
*   2      SCLK serial clock line of LCD
*   4      D/C  (or sometimes named A0) command/data switch
*   8      /RES active low reset
*   9      Backlight (optional, not on display)
*   6      GND  Ground for printer port and VDD
*   1      VDD  +V (? mA) Chip power supply
*   5      /CS   active low chip select (connected to GND)
*   7      Vout output of display-internal dc/dc converter
* 
* 
Hardwired D8 for LED Backlight
Vsupply ---470 ohms---LED----C--E--GND
B
1k
|
D8

D9 - Hx711 data, D10 Hx711 clock
All switches - connect to ground when pressed
D2 - "Unit" switch - probably will use it as "Power" switch
D10 - "Down" switch
D11- "Set" switch
D12-"UP" switch.
3.3V ----PDR--- |^^^^^10k^^^^----GND
|
|
A5

https://github.com/bogde/HX711
How to calibrate your load cell
Call set_scale() with no parameter.
Call tare() with no parameter.
Place a known weight on the scale and call get_units(10).
Divide the result in step 3 to your known weight. You should get about the parameter you need to pass to set_scale().
Adjust the parameter in step 4 until you get an accurate reading.

Pin to interrupt map:
D0-D7 = PCINT 16-23 = PCIR2 = PD = PCIE2 = pcmsk2
D8-D13 = PCINT 0-5 = PCIR0 = PB = PCIE0 = pcmsk0
A0-A5 (D14-D19) = PCINT 8-13 = PCIR1 = PC = PCIE1 = pcmsk1


*********************************************************************/

#include "PinChangeInterrupt.h"
#include "HX711.h"
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include <avr/sleep.h>
#include <avr/power.h>
//#include <Fonts/FreeMonoBold24pt7b.h> //too big for nokia display even with text size 1
#include <Fonts/FreeSansBold12pt7b.h>
Adafruit_PCD8544 display = Adafruit_PCD8544(3, 4, 6, 7, 5); //Hardwired this way - don't change
int myVersion = 1;
const int LED_BACKLIGHT_PIN =  8; //Hardwired via a transistor in order to run on supply voltage rather than on regulated 3.3 V.
// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 9; //Hardwired
const int LOADCELL_SCK_PIN = 10; //Hardwired
const int LIGHT_LEVEL_PIN = A5;
const int SWITCH_UNIT_PIN = 2; // External interrupt
const int SWITCH_DOWN_PIN = 11; // Pin Change Interrupt Request 0 (pins D8 to D13) (PCINT0_vect)
const int SWITCH_SET_PIN = 12;  // Pin Change Interrupt Request 0 (pins D8 to D13) (PCINT0_vect)
const int SWITCH_UP_PIN = 13;   // Pin Change Interrupt Request 0 (pins D8 to D13) (PCINT0_vect)
int light_level =0;
int cutoffLightLevel = 512;
boolean unitoptionlbs = false; // false = Kg, true = lbs

unsigned long previousMillis = 0;     
long timeoutinterval = 150000; 

unsigned long DisplaypreviousMillis = 0;
const long DisplayRefreshRate= 100;

volatile boolean UNIT=false;
volatile boolean SET = false;
volatile boolean UP = false;
volatile boolean DOWN = false;

boolean isNavigatingMenu = false;

int contrast=65;

int menuitem = 1;
int page = 1;

HX711 scale;

void enterSleep(void)
{
//attachInterrupt(0, UNITpinInterrupt, LOW);
attachInterrupt (digitalPinToInterrupt (SWITCH_UNIT_PIN), UNITpinInterrupt, LOW);
delay(100);
set_sleep_mode(SLEEP_MODE_PWR_DOWN);   /* EDIT: could also use SLEEP_MODE_PWR_DOWN for lowest power consumption. */
scale.power_down();
sleep_enable();
digitalWrite(LED_BACKLIGHT_PIN, LOW);
display.clearDisplay();   //write 0 to shadow buffer
display.display();        // copy buffer to display memory
display.command( PCD8544_FUNCTIONSET | PCD8544_POWERDOWN);
UNIT=false;
/* Now enter sleep mode. */
sleep_mode();

/* The program will continue from here after the WDT timeout*/
sleep_disable(); /* First thing to do is disable sleep. */

/* Re-enable the peripherals. */
power_all_enable();
scale.power_up();
display.initDisplay();
previousMillis = millis();
Serial.println("awake now");
}


void UNITpinInterrupt(void)
{

static unsigned long last_interrupt_time = 0;
unsigned long interrupt_time = millis();
// If interrupts come faster than 200ms, assume it's a bounce and ignore
if (interrupt_time - last_interrupt_time > 200)
{
detachInterrupt (digitalPinToInterrupt (SWITCH_UNIT_PIN));
UNIT=true;

}
last_interrupt_time = interrupt_time;



}
void SETpinInterrupt(void)
{
static unsigned long last_interrupt_time = 0;
unsigned long interrupt_time = millis();
// If interrupts come faster than 200ms, assume it's a bounce and ignore
if (interrupt_time - last_interrupt_time > 200)
{

SET=true;
detachPinChangeInterrupt(digitalPinToPinChangeInterrupt(SWITCH_SET_PIN));

}
last_interrupt_time = interrupt_time;



}
void UPpinInterrupt(void)
{

static unsigned long last_interrupt_time = 0;
unsigned long interrupt_time = millis();
// If interrupts come faster than 200ms, assume it's a bounce and ignore
if (interrupt_time - last_interrupt_time > 200)
{

UP=true;
detachPinChangeInterrupt(digitalPinToPinChangeInterrupt(SWITCH_UP_PIN));

}
last_interrupt_time = interrupt_time;


}
void DOWNpinInterrupt(void)
{
static unsigned long last_interrupt_time = 0;
unsigned long interrupt_time = millis();
// If interrupts come faster than 200ms, assume it's a bounce and ignore
if (interrupt_time - last_interrupt_time > 200)
{

DOWN=true;
detachPinChangeInterrupt(digitalPinToPinChangeInterrupt(SWITCH_DOWN_PIN));

}
last_interrupt_time = interrupt_time;



}

void backlightcontrol(){
light_level = analogRead(LIGHT_LEVEL_PIN);
if (light_level<cutoffLightLevel){
digitalWrite(LED_BACKLIGHT_PIN, HIGH);}
else
{
digitalWrite(LED_BACKLIGHT_PIN, LOW);
}
}


void handleMenu(){



/*
Page 1 = Main menu,Menu Item 1 = Contrast, Page 2 = Contrast value
Menu Item 2 = Unit, Page 3 = Unit options
Menu Item 3 = Light cutoff level, Page 4 = Light Cutoff Value
Menu Item 4 = Timeout , Page 5 = timeout value
Menu Item 5 = Reset Defaults - will be on Page 6
Menu Item 6 = Exit Menu - will be on Page 6

*/
if ((UP || DOWN) == true){
previousMillis = millis(); // Reset the sleep timer
isNavigatingMenu = true; //Draw Menu
}

if ((UP) && (page == 1 )) {
UP = false;
attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(SWITCH_UP_PIN), UPpinInterrupt, FALLING);
menuitem--;
if (menuitem==0)
{
menuitem=6;
}
if (menuitem>3)
{
page=6;    
}      
}
else if ((UP) && (page == 6 )) {
UP = false;
attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(SWITCH_UP_PIN), UPpinInterrupt, FALLING);
menuitem--;
if (menuitem==0)
{
menuitem=6;
}
if (menuitem<4)
{
page=1;    
}      
}

else if ((UP) && (page == 2 )) {
UP = false;
attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(SWITCH_UP_PIN), UPpinInterrupt, FALLING);
contrast--;
setContrast();
}
else if ((UP) && (page == 3 )) {
UP = false;
attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(SWITCH_UP_PIN), UPpinInterrupt, FALLING);
unitoptionlbs =!unitoptionlbs;
}
else if ((UP) && (page == 4 )) {
UP = false;
attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(SWITCH_UP_PIN), UPpinInterrupt, FALLING);
cutoffLightLevel--;
}
else if ((UP) && (page == 5 )) {
UP = false;
attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(SWITCH_UP_PIN), UPpinInterrupt, FALLING);
timeoutinterval = timeoutinterval-1000;
}
if ((DOWN) && (page == 1)) {
DOWN = false;
attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(SWITCH_DOWN_PIN), DOWNpinInterrupt, FALLING);

menuitem++;
if (menuitem==7) 
{
menuitem=1;
}
if (menuitem>3)
{
page=6;    
}
}
else if ((DOWN) && (page == 6)) {
DOWN = false;
attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(SWITCH_DOWN_PIN), DOWNpinInterrupt, FALLING);

menuitem++;
if (menuitem==7) 
{
menuitem=1;
}
if (menuitem<4)
{
page=1;    
}
}
else if ((DOWN) && (page == 2 )) {
DOWN = false;
attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(SWITCH_DOWN_PIN), DOWNpinInterrupt, FALLING);
contrast++;
setContrast();
}
else if ((DOWN) && (page == 3 )) {
DOWN = false;
attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(SWITCH_DOWN_PIN), DOWNpinInterrupt, FALLING);
unitoptionlbs =!unitoptionlbs;
}
else if ((DOWN) && (page == 4 )) {
DOWN = false;
attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(SWITCH_DOWN_PIN), DOWNpinInterrupt, FALLING);
cutoffLightLevel++;
}
else if ((DOWN) && (page == 5 )) {
DOWN = false;
attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(SWITCH_DOWN_PIN), DOWNpinInterrupt, FALLING);
timeoutinterval = timeoutinterval+1000;
}
if (SET) {
SET = false;
attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(SWITCH_SET_PIN), SETpinInterrupt, FALLING);

if(isNavigatingMenu){


if (page == 1 && menuitem==2) 
{
page=3;
}
else if(page == 1 && menuitem ==3)
{
page=4;
}
else if (page == 1 && menuitem==1) 
{
page=2;
}
else if (page == 6 && menuitem==4) 
{
page=5;
}
else if (page == 5) 
{
page=6;
}
else if (menuitem==5) 
{
resetDefaults();
}
else if (menuitem==6) 
{
isNavigatingMenu = false;
display.clearDisplay();
display.display();
page=1;
menuitem =1;
return;
} 
else if ((page == 2)|(page == 3)|(page ==4)) 
{
page=1;
}

}
isNavigatingMenu = true;
previousMillis = millis(); // Reset the sleep timer
}


}


void drawMenu()
{
display.setFont();  
if (page==1) 
{  

display.setTextSize(1);
display.clearDisplay();
display.setTextColor(BLACK, WHITE);
display.setCursor(15, 0);
display.print("MAIN MENU");
display.drawFastHLine(0,10,83,BLACK);
display.setCursor(0, 15);

if (menuitem==1) 
{ 
display.setTextColor(WHITE, BLACK);
}
else 
{
display.setTextColor(BLACK, WHITE);
}
display.print(">Contrast");
display.setCursor(0, 25);

if (menuitem==2) 
{
display.setTextColor(WHITE, BLACK);
}
else 
{
display.setTextColor(BLACK, WHITE);
}    
display.print(">Unit ");



if (menuitem==3) 
{ 
display.setTextColor(WHITE, BLACK);
}
else 
{
display.setTextColor(BLACK, WHITE);
}  
display.setCursor(0, 35);
display.print(">Light Cutoff");
display.display();
}


else if (page==2) 
{

display.setTextSize(1);
display.clearDisplay();
display.setTextColor(BLACK, WHITE);
display.setCursor(15, 0);
display.print("CONTRAST");
display.drawFastHLine(0,10,83,BLACK);
display.setCursor(5, 15);
display.print("Value");
display.setTextSize(2);
display.setCursor(5, 25);
display.print(contrast);
display.setTextSize(2);
display.display();
}
else if (page==3) 
{

display.setTextSize(1);
display.clearDisplay();
display.setTextColor(BLACK, WHITE);
display.setCursor(15, 0);
display.print("UNIT");
display.drawFastHLine(0,10,83,BLACK);
display.setCursor(5, 15);
display.print("Value");
display.setTextSize(2);
display.setCursor(5, 25);
if (unitoptionlbs){
display.print("lbs");
}
else {
display.print("Kgs");
}
display.setTextSize(2);
display.display();
}
else if (page==4) 
{

display.setTextSize(1);
display.clearDisplay();
display.setTextColor(BLACK, WHITE);
display.setCursor(15, 0);
display.print("Light Cutoff");
display.drawFastHLine(0,10,83,BLACK);
display.setCursor(5, 15);
display.print("Value");
display.setTextSize(2);
display.setCursor(5, 25);
display.print(cutoffLightLevel);
display.setTextSize(2);
display.display();
}
else if (page==5) 
{

display.setTextSize(1);
display.clearDisplay();
display.setTextColor(BLACK, WHITE);
display.setCursor(15, 0);
display.print("Timeout");
display.drawFastHLine(0,10,83,BLACK);
display.setCursor(5, 15);
display.print("m Secs");
display.setTextSize(2);
display.setCursor(5, 25);
display.print(timeoutinterval);
display.setTextSize(2);
display.display();
}

else if (page==6) 
{    
display.setTextSize(1);
display.clearDisplay();
display.setTextColor(BLACK, WHITE);
display.setCursor(15, 0);
display.print("MAIN MENU");
display.drawFastHLine(0,10,83,BLACK);
display.setCursor(0, 15);

if (menuitem==4) 
{ 
display.setTextColor(WHITE, BLACK);
}
else 
{
display.setTextColor(BLACK, WHITE);
}
display.print(">TimeOut");
display.setCursor(0, 25);

if (menuitem==5) 
{
display.setTextColor(WHITE, BLACK);
}
else 
{
display.setTextColor(BLACK, WHITE);
}    
display.print(">Reset ");



if (menuitem==6) 
{ 
display.setTextColor(WHITE, BLACK);
}
else 
{
display.setTextColor(BLACK, WHITE);
}  
display.setCursor(0, 35);
display.print(">Exit");
display.display();
}
}

void resetDefaults()
{
delayMicroseconds(1000000); // Let the user remove button pressing finger to get correct tare
cutoffLightLevel = 512;
contrast = 65;
timeoutinterval = 150000;
delayMicroseconds(4000000);
scale.tare(); //Re-zero the scale;
} 
void setContrast()
{
display.setContrast(contrast);
display.display();
}
void setup()   {
Serial.begin(9600);
scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
//scale.set_scale(211.8*10.7);                      // this value is obtained by calibrating the scale with known weights; see above and the https://github.com/bogde/HX711 README for details
scale.set_scale(226.626); 
scale.tare(); 

pinMode(LED_BACKLIGHT_PIN, OUTPUT);
pinMode(SWITCH_UNIT_PIN, INPUT_PULLUP);
pinMode(SWITCH_SET_PIN, INPUT_PULLUP);
pinMode(SWITCH_UP_PIN, INPUT_PULLUP);
pinMode(SWITCH_DOWN_PIN, INPUT_PULLUP);

attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(SWITCH_UP_PIN), UPpinInterrupt, FALLING);
attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(SWITCH_SET_PIN), SETpinInterrupt, FALLING);
attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(SWITCH_DOWN_PIN), DOWNpinInterrupt, FALLING);


display.begin();
display.clearDisplay();
display.setContrast(contrast);
display.setTextSize(2);
display.setTextColor(BLACK);
display.setCursor(0,0);
display.print("Ver =");
display.println(myVersion);
display.println("POST");
display.display();

Serial.println("All good in setup");

}


void loop() {
/*   
if (scale.is_ready()) {

display.clearDisplay();
//long reading = scale.read();
long reading = scale.get_units(10);
//Serial.print("HX711 reading: ");
//Serial.println(reading);
//for (int i=0; i<mynum; i++) {
//display.setFont(); to reset back to default font
display.setFont(&FreeSansBold12pt7b);
display.setTextSize(2);
display.setTextColor(BLACK);
display.setCursor(0,32);
display.print(reading);
display.display();

//display.clearDisplay();
//}
} else {
display.setCursor(0,32);

//display.print(light_level);
display.display();

}
delay(5000);



*/

unsigned long currentMillis = millis();

if (currentMillis - previousMillis >= timeoutinterval) {
previousMillis = currentMillis;
isNavigatingMenu=false;
Serial.println("Entering Sleep");
enterSleep();
}
else{
backlightcontrol();
handleMenu();
if(isNavigatingMenu){

drawMenu();
}
else{
if (scale.is_ready()) {

display.clearDisplay();
float reading = scale.get_units(10); //With scale.set_scale(226.626) - this gives accurate up to 1 deca gram (10 grams). 500 gram water bottle values are 50.07 to 50.32
float readinginGs = reading*10;
//float readinginKGs = round(readinginGs/1000); 
float readinginLBs = readinginGs/453.592;// 4545
//Serial.print("reading in lbs= ");
//Serial.println(readinginLBs);
if (unitoptionlbs){
int intermediatevalueholder = (int) (round(readinginLBs*10)); //155.83 becomes  1558 
int leftofdecimal = intermediatevalueholder/10; //1558 becomes 155
// Serial.println(leftofdecimal);
int rightofdecimal = (intermediatevalueholder)-(leftofdecimal*10);
// Serial.println(rightofdecimal);
display.setFont(&FreeSansBold12pt7b);
display.setTextSize(2);
display.setTextColor(BLACK);
display.setCursor(0,32);
display.print(leftofdecimal);
display.setFont();
display.setTextSize(1);
display.setCursor(48,39);
display.print(".");
display.print(rightofdecimal);
display.setCursor(6,39);
display.print("lbs");
}
else {
int intermediatevalueholder = (int) (round(readinginGs/100)); //70992.87 becomes  710
int leftofdecimal = intermediatevalueholder/10; //71
//Serial.println(leftofdecimal);
int rightofdecimal = (intermediatevalueholder)-(leftofdecimal*10);
//Serial.println(rightofdecimal);
display.setFont(&FreeSansBold12pt7b);
display.setTextSize(2);
display.setTextColor(BLACK);
display.setCursor(0,32);
display.print(leftofdecimal);
display.setFont();
display.setTextSize(1);
display.setCursor(48,39);
display.print(".");
display.print(rightofdecimal);
display.setCursor(6,39);
display.print("Kgs");
}
display.display();
}

}







}
}





#include <ESP8266WiFi.h>
#include <WebSocketsServer.h>
#include <ESP8266WebServer.h>
#include <string.h>
#include <ArduinoOTA.h>

#ifndef APSSID
#define APSSID "myRover1"
#define APPSK  "revoRym123"
#endif

#define ROOMBA_READ_TIMEOUT 200;
/* Set these to your desired credentials. */
const char *ssid = APSSID;
const char *password = APPSK;
String mystring;
ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(8000);

static const char PROGMEM MANIFEST_JSON[] = R"rawliteral(
{
"name": "myROVER",
"short_name": "myrover",
"description": "A simple rover controller app.",
"display": "standalone",
"scope": "/",
"orientation":  "landscape"
}
)rawliteral";

static const char PROGMEM CHECK_SENSORS[] = R"rawliteral(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, shrink-to-fit=no">
</head>
<body>
</body>
</html>
)rawliteral";
static const char PROGMEM INDEX_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=0">
<link rel="manifest" href="manifest.json">
<style>
/*        disable scrolling    */
html {
margin: 0;
padding: 0;
overflow: hidden;
}

body {
position: absolute;
width: 100%;
height: 100%;
overflow: auto;
margin: 0px;
background: black;
}

* {
margin: 0;
padding: 0;
box-sizing: border-box;
}

/*
input
{
width:0px;
height:0px;


}
input:checked + .slider-knob
{
background: green;
-webkit-transform: translateY(-26px);
-ms-transform: translateY(-26px);
transform: translateY(-26px);
}
*/
.sliderbuttons {
position: absolute;
bottom: 40%;
width: 100%;
height: 20%;
}

.slidercolumn {
width: 100%;
height: 100%;
position: relative;
}

.servosliderbuttons {
position: absolute;
bottom: 0%;
width: 100%;
height: 20%;
}

.fuelgaugecolumn {
width: 49%;
height: 100%;
}

.individual-status-lights {
width: 100%;
box-sizing: border-box;
height: 33.3%;
}

/*     top 10%           */
.notification-bar {
width: 100%;
display: flex;
position: fixed;
height: 10%;
background: white;
top: 0%;
}

.selectables {
width: 20%;
}

.console {
width: 20%;
left: 0px;
}

.numericals {
border: solid;
border-radius: 20%;
width: 10%;
}

/*                bottom 90%*/
.half {
width: 100%;
display: flex;
position: fixed;
height: 45%;
}

/*                position top half-10% away from top*/
#top-half {
top: 10%;

}
#sensors-top-half {
top: 10%;
position: fixed;
visibility: hidden;
width:100%;
z-index: 1;
overflow: scroll;
background-color: black;

}
/*                position bottom half -at bottom*/
.bottom {
right: 0;
bottom: 0;
}

/*                divide 5 even width columns*/
.twenty {
width: 20%;
}

.ten {
width: 10%;
}

.fifty {
width: 50%;
position: relative;
}

.sixty {
width: 60%;
}

.eighty {
width: 80%;
}

.horizontalslider {
position: relative;
height: 50%;
}

/*                if height is not sufficient - make room for steering*/
@media only screen and (max-height: 444px) {
.horizontalslider {
position: relative;
height: 10%;
}
}

.slider-knob {
height: 33.3%;
top: 33.3%;
position: relative;
background: green;
transition: .4s;
border-radius: 1%;
}

.gearbuttons {
width: 100%;
height: 25%;
border-radius: 20%;
}

.vertical3slider {
position: relative;
top: 0px;
left: 0px;
border: 2px solid blueviolet;
align-self: center;
height: 100%;
margin: 0 auto;
}

.genericbuttons {
height: 100%;
border-radius: 10%;
}

.vertical2slider {
position: relative;
top: 0px;
left: 0px;
border: 2px solid blueviolet;
align-self: center;
height: 100%;
margin: 0 auto;
}

.verticalslider {
position: relative;
top: 0px;
left: 0px;
border: 2px solid blueviolet;
align-self: center;
height: 100%;
margin: 0 auto;
display: flex;
}

.steering-container {
position: relative;
z-index: 1;
bottom: -50px;
}

.steeringbuttonswrapper {
position: absolute;
top: 21%;
left: 40%;
}

/*    CSS for Outer ring of joystick*/
.steering-outer-circle {
position: relative;
top: 0px;
left: 0px;
border: 2px solid blueviolet;
align-self: center;
width: 300px;
height: 300px;
border-radius: 300px;
margin: 0 auto;
}

/*CSS for inner ring of joystick*/
.steering-inner-circle {
position: absolute;
border: 0px;
top: 50px;
left: 50px;
/*opacity: 0.2;*/
border: solid blueviolet;
width: 250px;
height: 250px;
border-radius: 250px;
}

.steering-knob {
position: absolute;
border: solid blueviolet;
top: -50px;
left: 100px;
background: blueviolet;
/*opacity: 0.2;*/
border: solid blueviolet;
width: 100px;
height: 100px;
border-radius: 100px;
}

.three-slider {
width: 100%;
height: 100%;
}

.three-lights {
width: 49%;
height: 100%;
background: red;
}

.switch {
position: relative;
display: inline-block;
width: 60px;
height: 34px;
}

.svrlight {
background-color: red;
}

.switch input {
display: none;
}

.pwrlight {
position: relative;
height: 26px;
width: 26px;
}

.pwrlight:active {
background-color: red;
border: none;
}

.slider {
position: absolute;
cursor: pointer;
top: 0;
left: 0;
right: 0;
bottom: 0;
background-color: #bf1f1f;
-webkit-transition: .4s;
transition: .4s;
}

.slider:before {
position: absolute;
content: "";
height: 26px;
width: 26px;
left: 4px;
bottom: 4px;
background-color: white;
-webkit-transition: .4s;
transition: .4s;
}

input:checked + .slider {
background-color: #2196F3;
}

input:focus + .slider {
box-shadow: 0 0 1px #2196F3;
}

input:checked + .slider:before {
-webkit-transform: translateX(26px);
-ms-transform: translateX(26px);
transform: translateX(26px);
}

#top-left-4 {
margin: 0;
}

#top-left-5 {
position: absolute;
top: 0%;
right: 0%;
height: 100%
}

.contrast {
color: white;
}
.sensors{
color: white;
}
.sensor-values{
height: fit-content;
}
.row-1 {
position: absolute;
top: 0%;
right: 0;
height: 33%;
width: 100%;
}

.row-2 {
position: absolute;
top: 33.3%;
right: 0;
height: 33%;
width: 100%;
}

.row-3 {
position: absolute;
top: 66.6%;
right: 0;
height: 33%;
width: 100%;
}

.col-1 {
position: absolute;
width: 14%;
right: 85%
}

.col-2 {
position: absolute;
width: 46%;
right: 38%;
}

.col-3 {
position: absolute;
width: 38%;
right: 0;
}
#websocketstatus[type="radio"]:disabled {

color: aqua;
background-color: aqua;
border-color: aquamarine;


}
</style>
<title>myRover</title>
</head>
<body>
<!--<img id="mjpeg_dest"src="/cam_pic_new.php" style="width:100%;"> -->
<!--<div class="joystick-container" id="joystick-container">
<div class="outer" id="outer"><div class="inner" id="inner"></div></div>
</div>-->
<div class="notification-bar" id="notification">
<div class="console" id="mylocalconsole"></div>
<div class="selectables"></div>
<label for="websocketstatus">Connected</label>
<input type="radio" id="websocketstatus" disabled>
<label for="showhidesensors">ShowSensors</label>
<input type="checkbox" id="showhidesensors" onchange="showhidesensordiv()">
<button onclick="myLoop()">Sensor</button>
<input class="smbusnumericals numericals" id="Current">
<input class="smbusnumericals numericals" id="Voltage">
<input class="smbusnumericals numericals" id="Temperature">
<input class="smbusnumericals numericals" id="BatteryCharge">
<input class="smbusnumericals numericals" id="BatteryCapacity">
</div>

<div class="half" id="sensors-top-half" style="visibility: hidden">
<p>
<label class = "sensors">BumpsWheelDrops</label><input class="sensor-values" type="number" id="BumpsWheelDrops">  <br> 

<label class = "sensors">Wall</label><input class="sensor-values" type="number" id="Wall">

<label class = "sensors">CliffLeft</label><input class="sensor-values" type="number" id="CliffLeft">
<br>
<label class = "sensors">CliffFrontLeft</label><input class="sensor-values" type="number" id="CliffFrontLeft">

<label class = "sensors">CliffFrontRight</label><input class="sensor-values" type="number" id="CliffFrontRight">; 

<label class = "sensors">CliffRight</label><input class="sensor-values" type="number" id="CliffRight">; 
<br>
<label class = "sensors">VirtualWall</label><input class="sensor-values" type="number" id="VirtualWall">; 


<label class = "sensors">Overcurrents</label><input class="sensor-values" type="number" id="Overcurrents">; 

<label class = "sensors">DirtDetect</label><input class="sensor-values" type="number" id="DirtDetect">; 
<br>

<label class = "sensors">IrOpcode</label><input class="sensor-values" type="number" id="IrOpcode">; 

<label class = "sensors">Buttons</label><input class="sensor-values" type="number" id="Buttons">; 

<label class = "sensors">Distance</label><input class="sensor-values" type="number" id="Distance">; 
<br>
<label class = "sensors">Angle</label><input class="sensor-values" type="number" id="Angle">; 

<label class = "sensors">ChargingState</label><input class="sensor-values" type="number" id="ChargingState">; 


<label class = "sensors">WallSignal</label><input class="sensor-values" type="number" id="WallSignal">; 
<br>
<label class = "sensors">CliffLeftSignal</label><input class="sensor-values" type="number" id="CliffLeftSignal">; 

<label class = "sensors">CliffFrontLeftSignal</label><input class="sensor-values" type="number" id="CliffFrontLeftSignal">; 

<label class = "sensors">CliffFrontRightSignal</label><input class="sensor-values" type="number" id="CliffFrontRightSignal">; 
<br>
<label class = "sensors">CliffRightSignal</label><input class="sensor-values" type="number" id="CliffRightSignal">; 



<label class = "sensors">ChargerAvailable</label><input class="sensor-values" type="number" id="ChargerAvailable">; 

<label class = "sensors">OpenInterfaceMode</label><input class="sensor-values" type="number" id="OpenInterfaceMode">; 
<br>
<label class = "sensors">SongNumber</label><input class="sensor-values" type="number" id="SongNumber">; 

<label class = "sensors">SongPlaying</label><input class="sensor-values" type="number" id="SongPlaying">; 

<label class = "sensors">OiStreamNumPackets</label><input class="sensor-values" type="number" id="OiStreamNumPackets">; 
<br>
<label class = "sensors">Velocity</label><input class="sensor-values" type="number" id="Velocity">; 

<label class = "sensors">Radius</label><input class="sensor-values" type="number" id="Radius">; 

<label class = "sensors">VelocityRight</label><input class="sensor-values" type="number" id="VelocityRight">; 
<br>
<label class = "sensors">VelocityLeft</label><input class="sensor-values" type="number" id="VelocityLeft">; 

<label class = "sensors">EncoderCountsLeft</label><input class="sensor-values" type="number" id="EncoderCountsLeft">; 

<label class = "sensors">EncoderCountsRight</label><input class="sensor-values" type="number" id="EncoderCountsRight">; 
<br>
<label class = "sensors">LightBumper</label><input class="sensor-values" type="number" id="LightBumper">; 

<label class = "sensors">LightBumpLeft</label><input class="sensor-values" type="number" id="LightBumpLeft">; 

<label class = "sensors">LightBumpFrontLeft</label><input class="sensor-values" type="number" id="LightBumpFrontLeft">; 
<br>
<label class = "sensors">LightBumpCenterLeft</label><input class="sensor-values" type="number" id="LightBumpCenterLeft">; 

<label class = "sensors">LightBumpCenterRight</label><input class="sensor-values" type="number" id="LightBumpCenterRight">; 

<label class = "sensors">LightBumpFrontRight</label><input class="sensor-values" type="number" id="LightBumpFrontRight">; 
<br>
<label class = "sensors">LightBumpRight</label><input class="sensor-values" type="number" id="LightBumpRight">; 

<label class = "sensors">IrOpcodeLeft</label><input class="sensor-values" type="number" id="IrOpcodeLeft">; 

<label class = "sensors">IrOpcodeRight</label><input class="sensor-values" type="number" id="IrOpcodeRight">; 
<br>
<label class = "sensors">LeftMotorCurrent</label><input class="sensor-values" type="number" id="LeftMotorCurrent">; 

<label class = "sensors">RightMotorCurrent</label><input class="sensor-values" type="number" id="RightMotorCurrent">; 

<label class = "sensors">MainBrushCurrent</label><input class="sensor-values" type="number" id="MainBrushCurrent">; 
<br>
<label class = "sensors">SideBrushCurrent</label><input class="sensor-values" type="number" id="SideBrushCurrent">; 

<label class = "sensors">Stasis</label><input class="sensor-values" type="number" id="Stasis">; 
<label class = "sensors" style="visibility: hidden">Unused1</label><input class="sensor-values" type="number" id="Unused1" style="visibility: hidden">; 

<label class = "sensors" style="visibility: hidden">Unused2</label><input class="sensor-values" type="number" id="Unused2" style="visibility: hidden">; 
<label class = "sensors" style="visibility: hidden">Unused3</label><input class="sensor-values" type="number" id="Unused3"style="visibility: hidden">; 
</p>

</div>
<div class="half" id="top-half">
<div class="ten vertical3slider" id="top-left-1" id="gearB">
<button class="gearbuttons" id="forwardgearbuttonB" onclick='doSend(Start)'>Start
</button>
<button class="gearbuttons" id="stopgearbuttonB" onclick='doSend(Stop)'>Stop
</button>
<button class="gearbuttons" id="reversegearbuttonB" onclick='doSend(Safe)'>Safe
</button>
<button class="gearbuttons" id="reversegearbuttonB" onclick='doSend(Full)'>Full
</button>
</div>
<div class="ten vertical2slider" id="top-left-2">
<button class="gearbuttons" id="forwardgearbuttonB" onclick='doSend(Reset)'>Reset
</button>
<button class="gearbuttons" id="forwardgearbuttonB" onclick='doSend(PowerDown)'>Power
</button>
</div>
<div class="ten vertical2slider" id="top-left-3">
<button class="gearbuttons" id="forwardgearbuttonB" onclick='doSend(Clean)'>Clean
</button>
<button class="gearbuttons" id="forwardgearbuttonB" onclick='doSend(MaxClean)'>MaxClean
</button>
<button class="gearbuttons" id="stopgearbuttonB" onclick='doSend(Spot)'>Spot
</button>
<button class="gearbuttons" id="reversegearbuttonB" onclick='doSend(SeekDock)'>Dock
</button>
</div>
<div class="fifty">
<div class="twenty vertical2slider" id="top-left-4">
<button class="gearbuttons" id="connectWS" onclick='doConnect()'>Connect
</button>
<button class="gearbuttons" id="closeWS" onclick='doClose()'>Close
</button>
<button class="gearbuttons" id="reloadfromsource" onclick='location.reload()'>Reload
</button>
<button class="gearbuttons" id="getbatteryinforbtn" onclick='getbatteryinfo'>Battery
</button>
</div>
<div class="eighty" id="top-left-5">
<div class="row-1">
<div class="col-1">
<label class="contrast">Side Brush</label>
</div>
<div class="col-2">
<input type="range" id="sidebrushPWM" min="-127" max="127" value="0" step="5">
<br>
<button id="resetsidebrush" onclick='document.getElementById("sidebrushPWM").value="0"'>Reset</button>
</div>
<div class="col-3">
<input type="radio" id="sidebrushccw" name="sidebrushdirection" checked>
<label class="contrast small-font">ccw</label>
<input type="radio" id="sidebrushcw" name="sidebrushdirection">
<label class="contrast small-font">cw</label>
<br>
<input type="radio" id="sidebrushon" name="sidebrushonoff">
<label class="contrast small-font">on</label>
<input type="radio" id="sidebrushoff" name="sidebrushonoff" checked>
<label class="contrast small-font">off</label>
</div>
</div>
<div class="row-2">
<div class="col-1">
<label class="contrast">Main Brush</label>
</div>
<div class="col-2">
<input type="range" id="mainbrushPWM" min="-127" max="127" value="0" step="5">
<br>
<button id="resetmainbrush" onclick='document.getElementById("mainbrushPWM").value = "0"'>Reset</button>
</div>
<div class="col-3">
<input type="radio" id="mainbrushin" name="mainbrushdirection" checked>
<label class="contrast small-font">in</label>
<input type="radio" id="mainbrushout" name="mainbrushdirection">
<label class="contrast small-font">out</label>
<br>
<input type="radio" id="mainbrushon" name="mainbrushonoff">
<label class="contrast small-font">on</label>
<input type="radio" id="mainbrushoff" name="mainbrushonoff" checked>
<label class="contrast small-font">off</label>
</div>
</div>
<div class="row-3">
<div class="col-1">
<label class="contrast">Vac</label>
</div>
<div class="col-2">
<input type="range" id="vacuumPWM" min="0" max="127" value="0" step="5">
<br>
<button id="sendmotorspwm" onclick='sendPWMMotors(document.getElementById("mainbrushPWM").value,document.getElementById("sidebrushPWM").value,document.getElementById("vacuumPWM").value)'>Send</button>
<button id="allmotorsoff" onclick='sendRunMotorsFullSpeed(0,0,0,0,0)'>OFF</button>
</div>
<div class="col-3">
<input type="radio" id="vacuumon" name="vacuumonoff">
<label class="contrast small-font">on</label>
<input type="radio" id="vacuumoff" name="vacuumonoff" checked>
<label class="contrast small-font">off</label>
<br>
<button id="sendmotors" onclick='sendRunMotorsFullSpeed(document.getElementById("mainbrushout").checked? 1:0, document.getElementById("sidebrushcw").checked? 1:0, document.getElementById("mainbrushon").checked? 1:0,document.getElementById("vacuumon").checked? 1:0,document.getElementById("sidebrushon").checked? 1:0)'>Send</button>
</div>
</div>
</div>
</div>
</div>
<div class="half bottom" id="bottom-half">
<div class="sixty" id="bottom-middle">

<div class="horizontalslider"></div>
<div class="steering-container" id="steering-container">
<div class="steering-outer-circle" id="steering-outer-circle">
<!--                            <div class="steering-inner-circle" id="steering-inner-circle"></div>-->
<div class="steering-knob" id="steering-knob"></div>
<div class="steeringbuttonswrapper"></div>
</div>
</div>
</div>
<div class="twenty vertical3slider" id="gearA">
<div class="slidercolumn" id="slidercolumnA">
<button class="sliderbuttons" id="speedcontrolA">Move
</button>
</div>
</div>
<div class="twenty verticalslider" id="bottom-left">
<div class="sendcommand" id="sendcommand">
<input list="availablecommands" id="mycommands">
<button id="sendcommandbutton" class="genericbutton" onclick='sendcommandfromtext()'>Send</button>
<datalist id="availablecommands"></datalist>
<input type="radio" id="radiusCW" name="radius"><label class="contrast small-font">CW</label><br>
<input type="radio" id="radiusCCW" name="radius"><label class="contrast small-font">CCW</label><br>
<input type="radio" id="radiusOFF" name="radius" checked><label class="contrast small-font">OFF</label>
<button onclick='setradius()'>Set</button>
</div>
</div>
</div>
<div class="detailedlistofcommands">
<!--todo!-->
</div>
</body>
<script>

/*       For eventhandler passive support option, see here: https://developer.mozilla.org/en-US/docs/Web/API/EventTarget/addEventListener
*/

let passiveSupported = false;
try {
const options = {
get passive() {
// This function will be called when the browser
//   attempts to access the passive property.
passiveSupported = true;
return false;
}
};
window.addEventListener("test", null, options);
window.removeEventListener("test", null, options);
} catch (err) {
passiveSupported = false;
}

/* function for 2s complement*/
function createToInt(size) {
if (size < 2) {
throw new Error('Minimum size is 2');
} else if (size > 64) {
throw new Error('Maximum size is 64');
}

// Determine value range
const maxValue = (1 << (size - 1)) - 1;
const minValue = -maxValue - 1;

return (value)=>{
if (value > maxValue || value < minValue) {
throw new Error(`Int${size} overflow`);
}

if (value < 0) {
return (1 << size) + value;
} else {
return value;
}
}
;
}

const toInt16 = createToInt(16);
//using 2 byte numbers for velocity and radius

/*

myval = toInt16(-200).toString(16); // trying to run roomba backwards (-ve velocity) at 200 mm/s, myval 2 complements ff 38 . HIgh byte ff, low byte 38
send as Serial sequence: [137] [Velocity high byte] [Velocity low byte] [Radius high byte] [Radius low byte]


[137] [255] [56] [1] [244]

if (myval.length>2){
parseInt(myval.slice(-2), 16); //gives 56
parseInt(myval.slice(0,myval.length-2), 16); // gives 255
}
else
{
parseInt(myval, 16);
}


*/

//ROOMBA OPCODES
var Start = 128;
var Stop = 173;
var Reset = 7;
var Safe = 131;
var Full = 132;
var Clean = 135;
var MaxClean = 136;
var Spot = 134;
var SeekDock = 143;
var PowerDown = 133;
var Drive = 137;
var DriveDirect = 145;
var DrivePWM = 146;
var RunMotorsFullSpeed = 138;
var PWMMotors = 144;
var Sensors = 142;
//ROOMBA Sensor packets
var RoombaSensorObject  = {
"Group0":                 [0,0,26,0],   //Packet IDs 7 to 26 //[Offset(compared to 80 byte data),packetID,packetdatasize(number of bytes) ,signed]
"Group1":                 [0,1,10,0],   //Packet IDs 7 to 16
"Group2":                 [10,2,6,0],   //Packet IDs 17 to 20
"Group3":                 [16,3,10,0],   //Packet IDs 21 to 26
"Group4":                 [26,4,14,0],   //Packet IDs 27 to 34
"Group5":                 [40,5,12,0],   //Packet IDs 35 to 42
"Group6":                 [0,6,52,0],   //Packet IDs 7 to 42
"Group100":               [0,100,80,0], //Packet IDs 7 to 58 (All sensors)
"Group101":               [52,101,28,0], //Packet IDs 43 to 58
"Group106":               [57,106,12,0], //Packet IDs 46 to 51
"Group107":               [71,107,9,0],  //Packet IDs 54 to 58
"BumpsWheelDrops":        [0,7,1,0],   //1 byte 0-15 [byte number in 80 array,packetID,packetdatasize (number of bytes),signed]
"Wall":                   [1,8,1,0],   //1 byte 0-1
"CliffLeft":              [2,9,1,0],   //1 byte 0-1
"CliffFrontLeft":         [3,10,1,0],  //1 byte 0-1
"CliffFrontRight":        [4,11,1,0],  //1 byte 0-1
"CliffRight":             [5,12,1,0],  //1 byte 0-1
"VirtualWall":            [6,13,1,0],  //1 byte 0-1
"Overcurrents":           [7,14,1,0],  //1 byte 0-29
"DirtDetect":             [8,15,1,0],  //1 byte 0-255
"Unused1":                [9,16,1,0],  //1 byte 0-255
"IrOpcode":               [10,17,1,0],  //1 byte 0-255
"Buttons":                [11,18,1,1], //1 byte 0-255
"Distance":               [12,19,2,1],  //2 bytes -32768 to 32767 mm
"Angle":                  [14,20,2,1],  //2 bytes -32768 to 32767 degrees
"ChargingState":          [16,21,1,0],  //1 byte 0 -6, 0 Not charging, 1 Reconditioning Charging, 2 Full Charging, 3 Trickle Charging, 4 Waiting ,5 Charging Fault Condition
"Voltage":                [17,22,2,0],  //2 bytes 0-65535 mV
"Current":                [19,23,2,1],  //2 bytes -32768 to 32767 mA
"Temperature":            [21,24,1,1],  //1 byte -128 to 127 deg C
"BatteryCharge":          [22,25,2,0],  //2 bytes 0 - 65535 mAh
"BatteryCapacity":        [24,26,2,0],  //2 bytes 0 - 65535 mAh
"WallSignal":             [26,27,2,0],  //2 bytes 0 - 1023
"CliffLeftSignal":        [28,28,2,0],  //2 bytes 0 - 4095
"CliffFrontLeftSignal":   [30,29,2,0],  //2 bytes 0 - 4095
"CliffFrontRightSignal":  [32,30,2,0],  //2 bytes 0 - 4095
"CliffRightSignal":       [34,31,2,0],  //2 bytes 0 - 4095
"Unused2":                [36,32,1,0],  //1 byte 0 - 255
"Unused3":                [37,33,2,0],  //2 bytes 0 - 65535
"ChargerAvailable":       [39,34,1,0],  //1 byte 0-3
"OpenInterfaceMode":      [40,35,1,0],  //1 byte 0-3
"SongNumber":             [41,36,1,0],  //1 byte 0-4
"SongPlaying":            [42,37,1,0],  //1 byte 0-1
"OiStreamNumPackets":     [43,38,1,0],  //1 byte 0-108
"Velocity":               [44,39,2,1],  //2 bytes -500 to 500 mm/s
"Radius":                 [46,40,2,1],  //2 bytes -32768 to 32767 mm
"VelocityRight":          [48,41,2,1],  //2 bytes -500 to 500 mm/s
"VelocityLeft":           [50,42,2,1],  //2 bytes -500 to 500 mm/s
"EncoderCountsLeft":      [52,43,2,1],  //2 bytes -32768 to 32767
"EncoderCountsRight":     [54,44,2,1], //2 bytes -32768 to 32767
"LightBumper":            [56,45,1,0],  //1 byte 0-127
"LightBumpLeft":          [57,46,2,0],  //2 bytes 0-4095
"LightBumpFrontLeft":     [59,47,2,0],  //2 bytes 0-4095
"LightBumpCenterLeft":    [61,48,2,0],  //2 bytes 0-4095
"LightBumpCenterRight":   [63,49,2,0],  //2 bytes 0-4095
"LightBumpFrontRight":    [65,50,2,0],  //2 bytes 0-4095
"LightBumpRight":         [67,51,2,0],  //2 bytes 0-4095
"IrOpcodeLeft":           [69,52,1,0], //1 byte 0-255
"IrOpcodeRight":          [70,53,1,0],  //1 byte 0-255
"LeftMotorCurrent":       [71,54,2,1],  //2 bytes -32768 to 32767mA
"RightMotorCurrent":      [73,55,2,1],  //2 bytes -32768 to 32767 mA
"MainBrushCurrent":       [75,56,2,1],  //2 bytes -32768 to 32767 mA
"SideBrushCurrent":       [77,57,2,1],  //2 bytes -32768 to 32767 mA
"Stasis":                [79,58,1,0]  //1 byte 0-3 bit 0 - stasis toggling yes or no, bit 1 stasis disabled yes or no

}
/*        var Group0=                 [0,0,26,0]   //Packet IDs 7 to 26 //[Offset(compared to 80 byte data),packetID,packetdatasize(number of bytes) ,signed]
var Group1=                 [0,1,10,0]   //Packet IDs 7 to 16
var Group2=                 [10,2,6,0]    //Packet IDs 17 to 20
var Group3=                 [16,3,10,0]   //Packet IDs 21 to 26
var Group4=                 [26,4,14,0]   //Packet IDs 27 to 34
var Group5=                 [40,5,12,0]   //Packet IDs 35 to 42
var Group6=                 [0,6,52,0]   //Packet IDs 7 to 42
var Group100=               [0,100,80,0] //Packet IDs 7 to 58 (All sensors)
var Group101=               [52,101,28,0] //Packet IDs 43 to 58
var Group106=               [57,106,12,0] //Packet IDs 46 to 51
var Group107=               [71,107,9,0]  //Packet IDs 54 to 58
var BumpsWheelDrops =       [0,7,1,0]   //1 byte 0-15 [byte number in 80 array,packetID,packetdatasize (number of bytes),signed]
var Wall =                  [1,8,1,0]   //1 byte 0-1
var CliffLeft =             [2,9,1,0]   //1 byte 0-1
var CliffFrontLeft =        [3,10,1,0]  //1 byte 0-1
var CliffFrontRight=        [4,11,1,0]  //1 byte 0-1
var CliffRight=             [5,12,1,0]  //1 byte 0-1
var VirtualWall=            [6,13,1,0]  //1 byte 0-1
var Overcurrents=           [7,14,1,0]  //1 byte 0-29
var DirtDetect=             [8,15,1,0]  //1 byte 0-255
var Unused1=                [9,16,1,0]  //1 byte 0-255
var IrOpcode=               [10,17,1,0]  //1 byte 0-255
var Buttons=                [11,18,1,1] //1 byte 0-255
var Distance=               [12,19,2,1]  //2 bytes -32768 to 32767 mm
var Angle=                  [14,20,2,1]  //2 bytes -32768 to 32767 degrees
var ChargingState=          [16,21,1,0]  //1 byte 0 -6, 0 Not charging, 1 Reconditioning Charging, 2 Full Charging, 3 Trickle Charging, 4 Waiting ,5 Charging Fault Condition
var Voltage=                [17,22,2,0]  //2 bytes 0-65535 mV
var Current=                [19,23,2,1]  //2 bytes -32768 to 32767 mA
var Temperature=            [21,24,1,1]  //1 byte -128 to 127 deg C
var BatteryCharge=          [22,25,2,0]  //2 bytes 0 - 65535 mAh
var BatteryCapacity=        [24,26,2,0]  //2 bytes 0 - 65535 mAh
var WallSignal=             [26,27,2,0]  //2 bytes 0 - 1023
var CliffLeftSignal=        [28,28,2,0]  //2 bytes 0 - 4095
var CliffFrontLeftSignal=   [30,29,2,0]  //2 bytes 0 - 4095
var CliffFrontRightSignal=  [32,30,2,0]  //2 bytes 0 - 4095
var CliffRightSignal=       [34,31,2,0]  //2 bytes 0 - 4095
var Unused2=                [36,32,1,0]  //1 byte 0 - 255
var Unused3=                [37,33,2,0]  //2 bytes 0 - 65535
var ChargerAvailable=       [39,34,1,0]  //1 byte 0-3
var OpenInterfaceMode=      [40,35,1,0]  //1 byte 0-3
var SongNumber=             [41,36,1,0]  //1 byte 0-4
var SongPlaying=            [42,37,1,0]  //1 byte 0-1
var OiStreamNumPackets=     [43,38,1,0]  //1 byte 0-108
var Velocity=               [44,39,2,1]  //2 bytes -500 to 500 mm/s
var Radius=                 [46,40,2,1]  //2 bytes -32768 to 32767 mm
var VelocityRight=          [48,41,2,1]  //2 bytes -500 to 500 mm/s
var VelocityLeft=           [50,42,2,1]  //2 bytes -500 to 500 mm/s
var EncoderCountsLeft=      [52,43,2,1]  //2 bytes -32768 to 32767
var EncoderCountsRight=     [54,44,2,1]  //2 bytes -32768 to 32767
var LightBumper=            [56,45,1,0]  //1 byte 0-127
var LightBumpLeft=          [57,46,2,0]  //2 bytes 0-4095
var LightBumpFrontLeft=     [59,47,2,0]  //2 bytes 0-4095
var LightBumpCenterLeft=    [61,48,2,0]  //2 bytes 0-4095
var LightBumpCenterRight=   [63,49,2,0]  //2 bytes 0-4095
var LightBumpFrontRight=    [65,50,2,0]  //2 bytes 0-4095
var LightBumpRight=         [67,51,2,0]  //2 bytes 0-4095
var IrOpcodeLeft=           [69,52,1,0]  //1 byte 0-255
var IrOpcodeRight=          [70,53,1,0]  //1 byte 0-255
var LeftMotorCurrent=       [71,54,2,1]  //2 bytes -32768 to 32767mA
var RightMotorCurrent=      [73,55,2,1]  //2 bytes -32768 to 32767 mA
var MainBrushCurrent=       [75,56,2,1]  //2 bytes -32768 to 32767 mA
var SideBrushCurrent=       [77,57,2,1]  //2 bytes -32768 to 32767 mA
var Stasis=                 [79,58,1,0]  //1 byte 0-3 bit 0 - stasis toggling yes or no, bit 1 stasis disabled yes or no*/


//Sensor value arrays
var SensorGroup0=[
"BumpsWheelDrops",
"Wall",
"CliffLeft",
"CliffFrontLeft",
"CliffFrontRight",
"CliffRight",
"VirtualWall",
"Overcurrents",
"DirtDetect",
"Unused1",
"IrOpcode",
"Buttons",                
"Distance",               
"Angle",                 
"ChargingState",          
"Voltage",                
"Current",               
"Temperature",            
"BatteryCharge",          
"BatteryCapacity"
];
var SensorGroup1=[
"BumpsWheelDrops",
"Wall",
"CliffLeft",
"CliffFrontLeft",
"CliffFrontRight",
"CliffRight",
"VirtualWall",
"Overcurrents",
"DirtDetect",
"Unused1"
];
var SensorGroup2=[
"IrOpcode",
"Buttons",                
"Distance",               
"Angle"
];
var SensorGroup3=[
"ChargingState",          
"Voltage",                
"Current",               
"Temperature",            
"BatteryCharge",          
"BatteryCapacity"
];
var SensorGroup4=[
"WallSignal",             
"CliffLeftSignal",        
"CliffFrontLeftSignal",
"CliffFrontRightSignal",
"CliffRightSignal",
"Unused2",
"Unused3",
"ChargerAvailable"
];
var SensorGroup5=[
"OpenInterfaceMode",
"SongNumber",
"SongPlaying",
"OiStreamNumPackets",
"Velocity",
"Radius",
"VelocityRight",
"VelocityLeft"
];
var SensorGroup6=[
"BumpsWheelDrops",
"Wall",
"CliffLeft",
"CliffFrontLeft",
"CliffFrontRight",
"CliffRight",
"VirtualWall",
"Overcurrents",
"DirtDetect",
"Unused1",
"IrOpcode",
"Buttons",                
"Distance",               
"Angle",                 
"ChargingState",          
"Voltage",                
"Current",               
"Temperature",            
"BatteryCharge",          
"BatteryCapacity",        
"WallSignal",             
"CliffLeftSignal",        
"CliffFrontLeftSignal",
"CliffFrontRightSignal",
"CliffRightSignal",
"Unused2",
"Unused3",
"ChargerAvailable",
"OpenInterfaceMode",
"SongNumber",
"SongPlaying",
"OiStreamNumPackets",
"Velocity",
"Radius",
"VelocityRight",
"VelocityLeft"
];

var SensorGroup100=[
"BumpsWheelDrops",
"Wall",
"CliffLeft",
"CliffFrontLeft",
"CliffFrontRight",
"CliffRight",
"VirtualWall",
"Overcurrents",
"DirtDetect",
"Unused1",
"IrOpcode",
"Buttons",                
"Distance",               
"Angle",                 
"ChargingState",          
"Voltage",                
"Current",               
"Temperature",            
"BatteryCharge",          
"BatteryCapacity",        
"WallSignal",             
"CliffLeftSignal",        
"CliffFrontLeftSignal",
"CliffFrontRightSignal",
"CliffRightSignal",
"Unused2",
"Unused3",
"ChargerAvailable",
"OpenInterfaceMode",
"SongNumber",
"SongPlaying",
"OiStreamNumPackets",
"Velocity",
"Radius",
"VelocityRight",
"VelocityLeft",
"EncoderCountsLeft",
"EncoderCountsRight",
"LightBumper",
"LightBumpLeft",
"LightBumpFrontLeft",
"LightBumpCenterLeft",
"LightBumpCenterRight",
"LightBumpFrontRight",
"LightBumpRight",
"IrOpcodeLeft",
"IrOpcodeRight",
"LeftMotorCurrent",
"RightMotorCurrent",
"MainBrushCurrent",
"SideBrushCurrent",
"Stasis"  ];
var SensorGroup101=[
"EncoderCountsLeft",
"EncoderCountsRight",
"LightBumper",
"LightBumpLeft",
"LightBumpFrontLeft",
"LightBumpCenterLeft",
"LightBumpCenterRight",
"LightBumpFrontRight",
"LightBumpRight",
"IrOpcodeLeft",
"IrOpcodeRight",
"LeftMotorCurrent",
"RightMotorCurrent",
"MainBrushCurrent",
"SideBrushCurrent",
"Stasis" 
];
var SensorGroup106=[
"LightBumpLeft",
"LightBumpFrontLeft",
"LightBumpCenterLeft",
"LightBumpCenterRight",
"LightBumpFrontRight",
"LightBumpRight"
];
var SensorGroup107=[
"LeftMotorCurrent",
"RightMotorCurrent",
"MainBrushCurrent",
"SideBrushCurrent",
"Stasis"
];
/*    var valGroup100=[
BumpsWheelDrops,
Wall,
CliffLeft,
CliffFrontLeft,
CliffFrontRight,
CliffRight,
VirtualWall,
Overcurrents,
DirtDetect,
Unused1,
IrOpcode,
Buttons,                
Distance,               
Angle,                 
ChargingState,          
Voltage,                
Current,               
Temperature,            
BatteryCharge,          
BatteryCapacity,        
WallSignal,             
CliffLeftSignal,        
CliffFrontLeftSignal,
CliffFrontRightSignal,
CliffRightSignal,
Unused2,
Unused3,
ChargerAvailable,
OpenInterfaceMode,
SongNumber,
SongPlaying,
OiStreamNumPackets,
Velocity,
Radius,
VelocityRight,
VelocityLeft,
EncoderCountsLeft,
EncoderCountsRight,
LightBumper,
LightBumpLeft,
LightBumpFrontLeft,
LightBumpCenterLeft,
LightBumpCenterRight,
LightBumpFrontRight,
LightBumpRight,
IrOpcodeLeft,
IrOpcodeRight,
LeftMotorCurrent,
RightMotorCurrent,
MainBrushCurrent,
SideBrushCurrent,
Stasis  ];
var valGroup101=[
EncoderCountsLeft,
EncoderCountsRight,
LightBumper,
LightBumpLeft,
LightBumpFrontLeft,
LightBumpCenterLeft,
LightBumpCenterRight,
LightBumpFrontRight,
LightBumpRight,
IrOpcodeLeft,
IrOpcodeRight,
LeftMotorCurrent,
RightMotorCurrent,
MainBrushCurrent,
SideBrushCurrent,
Stasis
];
var valGroup106=[
LightBumpLeft,
LightBumpFrontLeft,
LightBumpCenterLeft,
LightBumpCenterRight,
LightBumpFrontRight,
LightBumpRight
];
var valGroup107=[
LeftMotorCurrent,
RightMotorCurrent,
MainBrushCurrent,
SideBrushCurrent,
Stasis
];*/


var maxFvelocity = 500;
//mm/s
var maxRvelocity = -500;
var maxCWRadius = -2000;
// mm
var maxCCWRadius = 2000;
var velocitySteps = 20;
//steps in mm/s
var currentRadius = 32767;
//default straight
var currentVelocity = 0;
var waitingforsensordata = false;
var lengthofdatarequested = 80;
var currentbufferoffset =0;
var outbuffer=null;
var outbufferview=null;


function uintToInt(uint, nbit) {
nbit = +nbit || 32;
if (nbit > 32) throw new RangeError('uintToInt only supports ints up to 32 bits');
uint <<= 32 - nbit;
uint >>= 32 - nbit;
return uint;
}


function requestSensorDatafromRoomba(packetID,datasize){
doSend(Sensors);
doSend(packetID); // going to request 6 , all sensor data for now - which returns 52 bytes of data. using packetID for future
waitingforsensordata = true;
lengthofdatarequested=datasize;
outbuffer = new ArrayBuffer(lengthofdatarequested);
outbufferview = new Uint8Array(outbuffer);

}
function handleSensorDatafromRoomba(incomingBinaryDataArrayBuffer){

var inbufferview = new Uint8Array(incomingBinaryDataArrayBuffer);                    

//add it to the buffer
outbufferview.set(inbufferview,currentbufferoffset);
//set the offset
currentbufferoffset = currentbufferoffset+inbufferview.byteLength;

//set the request flag to false
if (currentbufferoffset==lengthofdatarequested)
{
waitingforsensordata=false;
currentbufferoffset =0;
//console.log(outbufferview);
for (j=0;j<SensorGroup100.length;j++){document.getElementById(SensorGroup100[j]).value=retrievesensorvalue(RoombaSensorObject[SensorGroup100[j]],RoombaSensorObject["Group100"])} }

}
function retrievesensorvalue(whichsensor,requestedsensorgroup){
//
var mysensorvalue=outbufferview.slice(whichsensor[0]-requestedsensorgroup[0],whichsensor[0]-requestedsensorgroup[0]+whichsensor[2]);
var returnvalue="";
for (i=0;i<mysensorvalue.length;i++){
returnvalue=returnvalue+mysensorvalue[i].toString(2);
//parseInt((myunit16array[0].toString(2)+myunit16array[1].toString(2)),2)
}
return parseInt(returnvalue,2);
}

/*
It takes four data bytes, interpreted as two 16-bit signed
values using two’s complement. (http://en.wikipedia.org/wiki/Two%27s_complement) The first two bytes
specify the average velocity of the drive wheels in millimeters per second (mm/s), with the high byte
being sent first. The next two bytes specify the radius in millimeters at which Roomba will turn. The
longer radii make Roomba drive straighter, while the shorter radii make Roomba turn more. The radius is
measured from the center of the turning circle to the center of Roomba. A Drive command with a
positive velocity and a positive radius makes Roomba drive forward while turning toward the left. A
negative radius makes Roomba turn toward the right. Special cases for the radius make Roomba turn in
place or drive straight, as specified below. A negative velocity makes Roomba drive backward.

*/

function HbyteLbyte(val) {
var myval = toInt16(val).toString(16);
var Hbyte = 0;
var Lbyte = 0;
if (myval.length > 2) {

Lbyte = parseInt(myval.slice(-2), 16);
Hbyte = parseInt(myval.slice(0, myval.length - 2), 16);
} else {

Lbyte = parseInt(myval, 16);
}
return Hbyte + ":" + Lbyte;

}

function sendDrive(velocity, radius) {
/*
Velocity (-500 – 500 mm/s)
Radius (-2000 – 2000 mm) 

Radius Special Cases:
Straight = 32768 or 32767 = 0x8000 or 0x7FFF //int16 overflow with 32768. use 32767
Turn in place clockwise = -1 = 0xFFFF
Turn in place counter-clockwise = 1 = 0x0001

(maxFVelocity<velocity)||(velocity<maxRvelocity)
*/

var myvelocity = HbyteLbyte(velocity);
var myradius = HbyteLbyte(radius);
var myvelocityHbyte = myvelocity.split(":")[0];
var myvelocityLbyte = myvelocity.split(":")[1];
var myradiusHbyte = myradius.split(":")[0];
var myradiusLbyte = myradius.split(":")[1];

//console.log(myvelocityHbyte, myvelocityLbyte, myradiusHbyte, myradiusLbyte);

doSend(Drive);
doSend(myvelocityHbyte);
doSend(myvelocityLbyte);
doSend(myradiusHbyte);
doSend(myradiusLbyte);
}

function sendDriveDirect(rightwheelvelocity, leftwheelvelocity) {
/*Serial sequence: [145] [Right velocity high byte] [Right velocity low byte] [Left velocity high byte]
[Left velocity low byte]*/

var myRvelocity = HbyteLbyte(rightwheelvelocity);
var myLvelocity = HbyteLbyte(leftwheelvelocity);
var myRvelocityHbyte = myRvelocity.split(":")[0];
var myRvelocityLbyte = myRvelocity.split(":")[1];
var myLvelocityHbyte = myLvelocity.split(":")[0];
var myLvelocityLbyte = myRvelocity.split(":")[1];

console.log(myRvelocityHbyte, myRvelocityLbyte, myLvelocityHbyte, myLvelocityLbyte);

doSend(DriveDirect);
doSend(myRvelocityHbyte);
doSend(myRvelocityLbyte);
doSend(myLvelocityHbyte);
doSend(myLvelocityLbyte);
}
function sendDrivePWM(rightwheelpwm, leftwheelpwm) {
/*Serial sequence: [146] [Right PWM high byte] [Right PWM low byte] [Left PWM high byte] [Left PWM
low byte] 
-255 to 255 A positive PWM makes that wheel
drive forward, while a negative PWM makes it drive backward
*/
var myRpwm = HbyteLbyte(rightwheelpwm);
var myLpwm = HbyteLbyte(leftwheelpwm);
var myRpwmHbyte = myRpwm.split(":")[0];
var myRpwmLbyte = myRpwm.split(":")[1];
var myLpwmHbyte = myLpwm.split(":")[0];
var myLpwmLbyte = myLpwm.split(":")[1];

console.log(myRpwmHbyte, myRpwmLbyte, myLpwmHbyte, myLpwmLbyte);
doSend(DrivePWM);
doSend(myRpwmHbyte);
doSend(myRpwmLbyte);
doSend(myLpwmHbyte);
doSend(myLpwmLbyte);

}
function sendRunMotorsFullSpeed(mainbrushoutward, sidebrushCW, mainbrush, vacuum, sidebrush) {
/*
Bits 0-2: 0 = off, 1 = on at 100% pwm duty cycle
Bits 3 & 4: 0 = motor’s default direction, 1 = motor’s opposite direction. Default direction for the side
brush is counterclockwise. Default direction for the main brush/flapper is inward.
To turn on the main brush inward and the side brush clockwise, send: [138] [13]
*/
myval = "000" + mainbrushoutward + sidebrushCW + mainbrush + vacuum + sidebrush;
//console.log(parseInt(myval, 2));
doSend(RunMotorsFullSpeed);
doSend(parseInt(myval, 2));

}
function sendPWMMotors(mainbrushPWM, sidebrushPWM, vacuumPWM) {
/*
[144] [Main Brush PWM] [Side Brush PWM] [Vacuum PWM]
With each data byte, you specify the duty cycle for the low side driver (max 128). For
example, if you want to control a motor with 25% of battery voltage, choose a duty cycle of 128 * 25%
= 32. The main brush and side brush can be run in either direction. The vacuum only runs forward.
Positive speeds turn the motor in its default (cleaning) direction. Default direction for the side brush is
counterclockwise. Default direction for the main brush/flapper is inward.

 Main Brush and Side Brush duty cycle (-127 – 127)
 Vacuum duty cycle (0 – 127)

doSend(144);doSend(-32);doSend(32);doSend(0); //works
*/
doSend(PWMMotors);
doSend(mainbrushPWM);
doSend(sidebrushPWM);
doSend(vacuumPWM);
}

function setradius(){
var myradiusCW = document.getElementById("radiusCW").checked;
var myradiusCCW = document.getElementById("radiusCCW").checked;
var myradiusOFF = document.getElementById("radiusOFF").checked;
if (myradiusCCW == 1){currentRadius=-1;}
if (myradiusCW == 1){currentRadius=1;}
if (myradiusOFF == 1){currentRadius=32767;}
}
function sendcommandfromtext() {

doSend(document.getElementById("mycommands").value);
}

function parse_incoming_websocket_messages(data) {
//Data is S-255:OK or S-255:FAIL or C-F:OK or C-F:FAIL
//Determine if it is OK response or FAIL response, splice it on ":", log it if it is FAIL, proceed if it is OK
var myResponse = data.split(":")[1];
var returningdata = data.split(":")[0];
switch (myResponse) {
case ("FAIL"):
document.getElementById("mylocalconsole").innerHTML = data;
break;
case ("OK"):
var returningdata = data.split(":")[0];
//Now find the first character and do something
response_based_on_first_char(returningdata);
default:
document.getElementById("mylocalconsole").innerHTML = data;
break;

}

}

/*function handleOrientation(event) {
var y = event.gamma;
var z = event.alpha;
var x = event.beta;
//console.log("alpha: " + z + "\n");
//document.getElementById("mylocalconsole").innerHTML = "alpha: " + z + "\n";
//Device has moved more than the step
if(Math.abs(y-previousY)>steeringanlgesteps){

document.getElementById("mylocalconsole").innerHTML = "<W-"+(y+90)+">";  
//previousY=y;
}


}*/

/*
works on chrome 52 on android (GalaxyS5). I think beyond chrome 60 - needs window.isSecureContext to be true - for access to sensors
secure context means everything has to be ssl. Doesn't work even if I save the file locally on the phone and just connect to websocket. On the other hand, saving it locally on the computer and using it - becomes a secure context and sensors can be accessed. Yes, macbook has orientation sensors too.

window.addEventListener('deviceorientation', handleOrientation);
*/
/*    with x1 and y1 being the coordinates of the center of a circle, (x-x1)^2+(y-y1)^2 = radius^2
so for any given value of x, y = sqrt(radius^2-(x-x1)^2)+y1*/

var draggable = document.getElementById('steering-knob');
var outer = document.getElementById('steering-outer-circle');
var touchstartX;
var touchstartY;
//get center coordinates of the steering-outer-circle
var centerofsteeringcircleX;
var centerofsteeringcircleY;
var rect1 = outer.getBoundingClientRect();
var rect;
var myX;
var myY;
var myoffsetfromcontainerX;
var myoffsetfromcontainerY;
var mymaxY;
var myangle;
var steeringanlgesteps = 5;
var previoussentangle = 0;
var steeringrange = 4000;
var steeringservomidangle = 0;
var steeringservomaxangle = 2000;
var steeringservominangle = -2000;
centerofsteeringcircleX = rect1.left + ((rect1.right - rect1.left) / 2);
centerofsteeringcircleY = rect1.top + ((rect1.bottom - rect1.top) / 2);

draggable.addEventListener('touchstart', function(event) {
var touch = event.targetTouches[0];
rect = draggable.getBoundingClientRect();
//Start point -center of steering knob
touchstartX = rect.left + ((rect.width) / 2);
touchstartY = rect.top + ((rect.height) / 2);
rect1 = outer.getBoundingClientRect();
//don't want the steering knob to go below horizon. Find the lowermost allowable steeringknob center Y pixel. it should be steering-knob's radius away from the bottom of the steering container
mymaxY = window.innerHeight - (rect.height) / 2;
//                                myoffsetfromcontainerX=rect.left-rect1.left;
//                                myoffsetfromcontainerY=rect.top-rect1.top;
myoffsetfromcontainerX = draggable.offsetLeft;
myoffsetfromcontainerY = draggable.offsetTop;

//                 console.log("touchstartx="+touchstartX+", touchstartY="+touchstartY+", myoffsetX="+myoffsetfromcontainerX+", myoffsetY="+myoffsetfromcontainerY+", mymaxY="+mymaxY);
}, passiveSupported ? {
passive: true
} : false);
draggable.addEventListener('touchmove', function(event) {
var touch = event.targetTouches[0];
//get the x point of touch;
myX = touch.pageX;
myY = centerofsteeringcircleY - Math.sqrt((150 * 150) - (Math.pow((myX - centerofsteeringcircleX), 2)));

//check if falling below horizon
if (myY < mymaxY) {
draggable.style.left = myoffsetfromcontainerX + (myX - touchstartX) + 'px';
draggable.style.top = myoffsetfromcontainerY + (myY - touchstartY) + 'px';

/*                            calculate angle off center at current position
myangle = -135(Left) to -45 (right). multiplied by 2 to get -270 to -90.
convert to 0-180 by adding 270.
Convert 180 scale to "Range" scale by *Range/180
Shift the mid point of the scale so that Range/2 = midservoangle.*/
//myangle=parseInt((((Math.atan2(myY - centerofsteeringcircleY, myX - centerofsteeringcircleX) * 180 / Math.PI)*2)+270)*(steeringrange/180))+(steeringservomidangle-steeringrange/2);
myangle = parseInt((((Math.atan2(myY - centerofsteeringcircleY, myX - centerofsteeringcircleX) * 180 / Math.PI) * 2) + 270));
if (myangle < 0) {
myangle = 1;
}
if (myangle > 179) {
myangle = 179;
}
myangle = parseInt(myangle * (steeringrange / 180));
if (myangle > 2000) {
myangle = myangle - 4000;
}
//else if (myangle<2000){myangle= 2000-myangle;}  
//else {}
//myangle = 4000-myangle;

}

if (Math.abs(myangle - previoussentangle) > steeringanlgesteps) {

//send command only if it is with in the bounds of the steering servo
//if (myangle >= steeringservominangle && myangle <= steeringservomaxangle){
//console.log(myangle); 
currentRadius = myangle;
sendDrive(currentVelocity, currentRadius);
previoussentangle = myangle;

// }

}
//event.preventDefault();
}, passiveSupported ? {
passive: true
} : false);

draggable.addEventListener('touchend', function(event) {
draggable.style.left = '100px';
draggable.style.top = '-50px';
currentRadius = 32767;
sendDrive(currentVelocity, currentRadius);
}, passiveSupported ? {
passive: true
} : false);

//sliderbasefunction returns at what percent of total slidable height -is the slider at
function sliderbasefunction(event, whocalledme, whichelement) {
var mytotalheight;
var myminY;
var mymaxY;
var mytouchY;
var mystylefromtop;
var mypercentageslide;
var myouterboundrect = document.getElementById('slidercolumn' + whichelement).getBoundingClientRect();
//var myslider = document.getElementById('speedcontrol'+whichelement);
var mysliderrect = whocalledme.getBoundingClientRect();
mytotalheight = myouterboundrect.height;
myminY = myouterboundrect.top + (mysliderrect.height / 2);
mymaxY = myminY + (mytotalheight - mysliderrect.height);
var myslidertouch = event.targetTouches[0];
mytouchY = myslidertouch.pageY;
if (mytouchY >= myminY && mytouchY <= mymaxY) {
//move whocalledme to mytouchY

mypercentageslide = ((mymaxY - mytouchY) / (mymaxY - myminY)) * 100;

if ((80 - mypercentageslide) > 0) {
mystylefromtop = 80 - mypercentageslide;
} else {
mystylefromtop = 0;
}
whocalledme.style.top = mystylefromtop + '%';
//console.log("mytouchY="+mytouchY+" myminY="+myminY+" mymaxy="+mymaxY+" mypercentageslide ="+mypercentageslide+" style from top % ="+mystylefromtop);

} else {

//sliding out of bounds - lower
if (mytouchY >= myminY) {
whocalledme.style.top = 80 + '%';
mypercentageslide = 0;
}//sliding out of bounds - upper
else if (mytouchY <= mymaxY) {

whocalledme.style.top = 0 + '%';
mypercentageslide = 100;
}

}
return Math.round(mypercentageslide);
}
var mysliderA = document.getElementById('speedcontrolA');
var mypreviousPWMAsent = 0;
var mypreviousPWMBsent = 0;
var mypreviousvalueXsent = 0;
var mypreviousvalueYsent = 0;
var mypreviousvalueZsent = 0;
var valuetosendA = 0;
var valuetosendB = 0;
var valuetosendX = 0;
var valuetosendY = 0;
var valuetosendZ = 0;
var servoXmaxangle = 150;
var servoXminangle = 30;
var servoXanglesteps = 5;
var servoYmaxangle = 150;
var servoYminangle = 30;
var servoYanglesteps = 5;
var servoZmaxangle = 150;
var servoZminangle = 30;
var servoZanglesteps = 5;
var motorAmovingforward = false;
var motorAmoving = false;

mysliderA.addEventListener('touchmove', function(event) {
var whatpercentslide = sliderbasefunction(event, this, "A");

//if slider is between 60-100% - move forward with increasing speed. 
//if slider is between 40-0% - move backwards with increasing speed.
//if slider is between 40-60% - stop.

if (whatpercentslide < 40) {

valuetosendA = Math.round(((maxRvelocity) * ((Math.abs(50 - whatpercentslide) * 2) / 100)));
//console.log(valuetosendA);
if (Math.abs(valuetosendA - mypreviousPWMAsent) > velocitySteps) {

console.log(valuetosendA);
sendDrive(valuetosendA, currentRadius);

mypreviousPWMAsent = valuetosendA;
currentVelocity = valuetosendA;
}

} else if (whatpercentslide > 60) {
valuetosendA = Math.round(((maxFvelocity) * ((Math.abs(50 - whatpercentslide) * 2) / 100)));
//console.log(valuetosendA);
if (Math.abs(valuetosendA - mypreviousPWMAsent) > velocitySteps) {

console.log(valuetosendA);
sendDrive(valuetosendA, currentRadius);

mypreviousPWMAsent = valuetosendA;
currentVelocity = valuetosendA;

} else {//stop

}

}

}, passiveSupported ? {
passive: true
} : false);

//when releasing speedcontrol button - it returns to mid position and motor stops.
mysliderA.addEventListener('touchend', function(event) {
motorAmoving = false;
currentVelocity = 0;
sendDrive(currentVelocity, currentRadius);

mysliderA.style.top = 40 + '%';

}, passiveSupported ? {
passive: true
} : false);

var onlongtouch;
var timer;
var touchduration = 750;
//length of time we want the user to touch before we do something

function touchstart(id, val) {
timer = setTimeout(onlongtouch, touchduration, id, val);

}

function touchend() {

//stops short touches from firing the event
if (timer)
clearTimeout(timer);
// clearTimeout, not cleartimeout..
}
/*
//if holding the steering knob for longer than 750 ms, send the current angle. 
onlongtouch=doSend("<W-"+myangle+">");*/

var m = 1;                  //  set your counter to 1

function myLoop() {         //  create a loop function
setTimeout(function() {   //  call a 3s setTimeout when the loop is called
requestSensorDatafromRoomba(RoombaSensorObject["Group100"][1],RoombaSensorObject["Group100"][2]);   //  your code here
m++;                    //  increment the counter
if (m < 1000) {           //  if the counter < 10, call the loop function
myLoop();             //  ..  again which will trigger another 
}                       //  ..  setTimeout()
}, 300)
}

function handlewebsocketstatus(){
if (iswebsocketconnected){
document.getElementById("websocketstatus").checked=true;
}
else
{
document.getElementById("websocketstatus").checked=false;
}
}       

function showhidesensordiv() {
if  (document.getElementById("sensors-top-half").style.visibility == "hidden"){document.getElementById("sensors-top-half").style.visibility = "visible";}
else document.getElementById("sensors-top-half").style.visibility = "hidden";
}  
var websock;
var iswebsocketconnected = false;
function doConnect() {
// websock = new WebSocket('ws://' + window.location.hostname + ':8000/');
websock = new WebSocket('ws://192.168.4.1:8000/');
websock.binaryType = "arraybuffer";
websock.onopen = function(evt) {
console.log('websock open');
iswebsocketconnected = true;
handlewebsocketstatus();

}

websock.onclose = function(evt) {
iswebsocketconnected = false;
console.log('websock close');
handlewebsocketstatus();
//clear all the intervals - doesn't work. still dealing with "ghost" setIntervals

}
websock.onerror = function(evt) {
console.log(evt);
}
websock.onmessage = function(evt) {
//console.log(evt); 
console.log(evt.data);
if (waitingforsensordata) {
handleSensorDatafromRoomba(evt.data);
}
//parse_incoming_websocket_messages(evt.data);
}
}
function doClose() {
//detach steering servo
//attachordetachservos("w","d");

websock.close();
}
function doSend(message) {
if (iswebsocketconnected == true) {
if (websock.readyState == websock.OPEN) {
websock.send(message);
//console.log(message);
} else {

console.log("websocket is in an indeterminate state");
}
}
}
</script>
</html>
)rawliteral";


void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length)
{
//Serial1.printf("webSocketEvent(%d, %d, ...)\r\n", num, type);
switch(type) {
case WStype_DISCONNECTED:
//Serial1.printf("[%u] Disconnected!\r\n", num);
break;
case WStype_CONNECTED:
{
IPAddress ip = webSocket.remoteIP(num);
//Serial1.printf("[%u] Connected from %d.%d.%d.%d url: %s\r\n", num, ip[0], ip[1], ip[2], ip[3], payload);
//itoa( cm, str, 10 );
//  webSocket.sendTXT(num, str, strlen(str));
}
break;
case WStype_TEXT:
{
//Serial1.printf("[%u] get Text: %s\r\n", num, payload);
//Send whatever comes on the WS to Atmega.
char *mystring1 = (char *)payload;
mystring = mystring1;
Serial.write(mystring.toInt());

}
break;
case WStype_BIN:
//Serial1.printf("[%u] get binary length: %u\r\n", num, length);
break;
default:
//Serial1.printf("Invalid WStype [%d]\r\n", type);
break;
}
}

void handleRoot()
{
server.sendHeader("Cache-Control","max-age=604800");
server.send(200, "text/html", INDEX_HTML);

}
void handleIndex()
{
//server.send_P(200, "text/html", INDEX_HTML);
server.sendHeader("Cache-Control","max-age=604800");
server.send(200, "text/html", INDEX_HTML);

}


void handleManifest()
{
server.sendHeader("Cache-Control","max-age=604800");
server.send(200, "application/json", MANIFEST_JSON);

}
void handleCheck()
{
server.sendHeader("Cache-Control","max-age=604800");
server.send(200, "text/html", CHECK_SENSORS);

}

void handleNotFound()
{
String message = "File Not Found\n\n";
message += "URI: ";
message += server.uri();
message += "\nMethod: ";
message += (server.method() == HTTP_GET)?"GET":"POST";
message += "\nArguments: ";
message += server.args();
message += "\n";
for (uint8_t i=0; i<server.args(); i++){
message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
}
server.send(404, "text/plain", message);
}


/*******************Serial Read Functions ************************/
//Serial Read stuff

const byte numChars = 32;
char receivedChars[numChars];



void recvWithStartEndMarkers() {
static boolean recvInProgress = false;
static byte ndx = 0;
char startMarker = '<';
char endMarker = '>';
char rc;

while (Serial.available() > 0 ) {
rc = Serial.read();

if (recvInProgress == true) {
if (rc != endMarker) {
receivedChars[ndx] = rc;
ndx++;
if (ndx >= numChars) {
ndx = numChars - 1;
}
}
else {
receivedChars[ndx] = '\0'; // terminate the string
recvInProgress = false;
webSocket.sendTXT(0,receivedChars,ndx);
//Serial1.println(receivedChars);
//Serial.println(receivedChars);
ndx = 0;

}
}

else if (rc == startMarker) {
recvInProgress = true;
}
}
}




/*******************Serial Read Functions ************************/
/*****generic serial read and send to websocket function ***************/

void recvAndSendtoWebSocket(){
byte  mybytes[64];
if (Serial.available()>0){
int mylength=Serial.available();
for (int p=0;p<mylength;p++) {
mybytes[p] = Serial.read();

}
webSocket.sendBIN(0,mybytes,mylength);
}
}
/*
bool getDataFromROOMBA()(uint8_t* dest, uint8_t len)
{
while (len-- > 0)
{
unsigned long startTime = millis();
while (!Serial.available())
{
// Look for a timeout
if (millis() > startTime + ROOMBA_READ_TIMEOUT)
return false; // Timed out
}
*dest++ = Serial.read();
}
return true;
}
*/
/*****END *** generic serial read and send to websocket function ***************/
void setup()
{ 

//Change Roomba Baud rate to 19200
/*
Use the Baud Rate Change pin (pin 5 on the Mini-DIN connector) to change Roomba’s baud rate. After
turning on Roomba, wait 2 seconds and then pulse the Baud Rate Change low three times. Each pulse
should last between 50 and 500 milliseconds. Roomba will communicate at 19200 baud until the
processor loses battery power or the baud rate is explicitly changed by way of the OI.
*/
delay(5000);
pinMode(2,OUTPUT);
digitalWrite(2, HIGH);
delay(1000);
digitalWrite(2, LOW);
delay(400);
digitalWrite(2, HIGH);
delay(100);
digitalWrite(2, LOW);
delay(400);
digitalWrite(2, HIGH);
delay(100);
digitalWrite(2, LOW);
delay(400);
digitalWrite(2, HIGH);
delay(100);

Serial.begin(19200);

/***************** AP mode*******************/

WiFi.softAP(ssid, password);
//WiFi.printDiag(Serial1);
IPAddress myIP = WiFi.softAPIP();


/***********************************************/

/*****************Client Mode******************/
/*
WiFiMulti.addAP(ssid, password);
while(WiFiMulti.run() != WL_CONNECTED) {
Serial1.print(".");
delay(100);
}
Serial1.println("");
Serial1.print("Connected to ");
Serial1.println(ssid);
Serial1.print("IP address: ");
Serial1.println(WiFi.localIP());
*/
/**********************************************/

server.on("/", handleRoot);
server.on("/index.html",handleIndex);
server.on("/manifest.json",handleManifest);
server.on("/check.html",handleCheck);
server.onNotFound(handleNotFound);
//  server.onNotFound([]() {                              // If the client requests any URI
//    if (!handleFileRead(server.uri()))                  // send it if it exists
//      server.send(404, "text/plain", "404: Not Found"); // otherwise, respond with a 404 (Not Found) error
//  });

server.begin();

webSocket.begin();
webSocket.onEvent(webSocketEvent);


/* ************SPIFFS********************* */
//  if (SPIFFS.begin()){Serial1.println("file system mounted");};
//
//  //Open the "Save.txt" file and check if we were saving before the reset happened
//  File q = SPIFFS.open("/Save.txt", "r");
//  if (q.find("Y")){saveData=true;}
//  q.close();

/*********************************************/

/* ************OTA********************* */

// Port defaults to 8266
// ArduinoOTA.setPort(8266);

// Hostname defaults to esp8266-[ChipID]
// ArduinoOTA.setHostname("myesp8266");

// No authentication by default
// ArduinoOTA.setPassword((const char *)"123");

ArduinoOTA.onStart([]() {
//Serial1.println("Start");
});
ArduinoOTA.onEnd([]() {
//Serial1.println("\nEnd");
});
ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
//Serial1.printf("Progress: %u%%\r", (progress / (total / 100)));
});
ArduinoOTA.onError([](ota_error_t error) {
/*
Serial1.printf("Error[%u]: ", error);
if (error == OTA_AUTH_ERROR) Serial1.println("Auth Failed");
else if (error == OTA_BEGIN_ERROR) Serial1.println("Begin Failed");
else if (error == OTA_CONNECT_ERROR) Serial1.println("Connect Failed");
else if (error == OTA_RECEIVE_ERROR) Serial1.println("Receive Failed");
else if (error == OTA_END_ERROR) Serial1.println("End Failed");
*/
});
ArduinoOTA.begin();

/****************************************************/  


}



void loop()
{
webSocket.loop();
server.handleClient();
ArduinoOTA.handle();
//recvWithStartEndMarkers();
recvAndSendtoWebSocket();

}

