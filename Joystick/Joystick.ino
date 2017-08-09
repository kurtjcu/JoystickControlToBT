
/* 2017-08-10
 * This file was created by Kurt@nqmakersupplies.com.au
 * Uses protothreads from http://dunkels.com/adam/pt/
 * Reads the analog value from two post and sends them via serial, used with a paired set of HC-05 BT modules. 
 * https://nqmakersupplies.com.au/shop/product/hc-05-bluetooth-rx-or-tx-142
 * 
 * Desigend to use with https://github.com/kurtjcu/ZumoRCViaBT
 */


#include "./pt/pt.h"   // include protothread library

#define BAUD_RATE 9600 //Baud rate for the Windows Driver

#define POT_1 A0
#define POT_2 A1
  

int pot1 = 0;
int pot2 = 0;
int thetaX = 0;
int thetaY = 0;
int newthetaX = 0;
int newthetaY = 0;

char str[255];

String inputString = "";
boolean stringComplete = false;

static struct pt pt1, pt2; // each protothread needs one of these
/*
 * Priorities of tasks:
 * First task: Read the angle 
 * Second task: Send serial output of angle. 
 */

void setup() {
  
  inputString.reserve(100); 
  
  PT_INIT(&pt1);  // initialise the two
  PT_INIT(&pt2);  // protothread variables
  
  Serial.begin(BAUD_RATE);
}

/* This function measures the potentiometers */
static int protothreadAngle(struct pt *pt, int interval) {
  static unsigned long timestamp = 0;
  PT_BEGIN(pt);
  while(1) { // never stop 
    /* each time the function is called the second boolean
    *  argument "millis() - timestamp > interval" is re-evaluated
    *  and if false the function exits after that. */
    PT_WAIT_UNTIL(pt, millis() - timestamp > interval );
    timestamp = millis(); // take a new timestamp

    newthetaX = analogRead(POT_1); //X potentiometer
    newthetaY = analogRead(POT_2); //Y potentiometer
  }
  PT_END(pt);
}


/* Send the Joystick position over serial*/
static int protothreadOutput(struct pt *pt, int interval) {
  static unsigned long timestamp = 0;
  PT_BEGIN(pt);
  while(1) {
    PT_WAIT_UNTIL(pt, millis() - timestamp > interval ); 
    timestamp = millis();
    //Send the angle positions
    char outputString[10];
    thetaX = newthetaX;
    thetaY = newthetaY;
    sprintf(outputString, "%d-%d", thetaX, thetaY);
    Serial.println(outputString);
  }
  PT_END(pt);
}

void loop() {
  //Starting protothreads, and setting time interval (subject to change)
  protothreadAngle(&pt1, 10);     //Angle Proto-Thread
  protothreadOutput(&pt2, 100);  //Serial Output Proto-Thread
}
