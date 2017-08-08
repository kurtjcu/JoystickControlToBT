
#include "./pt/pt.h"   // include protothread library

#define BAUD_RATE 38600 //Baud rate for the Windows Driver

#define POT_1 A0
#define POT_2 A1
  
#define MOTOR_PIN_1 5 //Pin supports PWM
#define MOTOR_PIN_2 6 //Pin that supports PWM
#define TOGGLE_PIN 4

//change this to switch between x-y and x-z planes of motion
int switchState = 0;

int pot1 = 0;
int pot2 = 0;
int thetaX = 0;
int thetaY = 0;
int thetaZ = 0;
int newthetaX = 0;
int newthetaY = 0;
int newthetaZ = 0;
//note that pot1 and motor1 is always x
//and pot2 and motor2 is always y or z depending on state
int newTorque1 = 0;
int newTorque2 = 0;
int motorTorque1 = 0;
int motorTorque2 = 0;



//Motor 1
int in1 = 8;
int in2 = 7;
//Motor 2
int in3 = 10;
int in4 = 9;

char str[255];

String inputString = "";
boolean stringComplete = false;



static struct pt pt1, pt2, pt3, pt4; // each protothread needs one of these
/*
 * Priorities of tasks:
 * First task: Read the serial input from CHAI3D
 * Second task: Generate Motor Torque
 * Third task: Read the angle 
 * Fourth task: Send serial output of angle. 
 */


void setup() {
  
  pinMode(MOTOR_PIN_1, OUTPUT);
  pinMode(MOTOR_PIN_2, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);
  pinMode(TOGGLE_PIN, INPUT);
  inputString.reserve(200);
  
  PT_INIT(&pt1);  // initialise the two
  PT_INIT(&pt2);  // protothread variables
  PT_INIT(&pt3);  // initialise the two
  PT_INIT(&pt4);  // protothread variables
  Serial.begin(BAUD_RATE, SERIAL_8E1);
}


/* This function measures the potentiometers and calculates the angle*/
static int protothreadAngle(struct pt *pt, int interval) {
  static unsigned long timestamp = 0;
  PT_BEGIN(pt);
  while(1) { // never stop 
    /* each time the function is called the second boolean
    *  argument "millis() - timestamp > interval" is re-evaluated
    *  and if false the function exits after that. */
    PT_WAIT_UNTIL(pt, millis() - timestamp > interval );
    timestamp = millis(); // take a new timestamp

    switchState = digitalRead(TOGGLE_PIN);
    newthetaX = analogRead(POT_1); //Top potentiometer
    pot2 = analogRead(POT_2); //Top potentiometer
    if(switchState == 0){
      //move on x and y axis
      newthetaY = pot2;
      newthetaZ = 512; //pot zero
    }else{
      //move on x and z axis
      newthetaY = 512; //pot zero
      newthetaZ = pot2;
    }
  }
  PT_END(pt);
}
/* Generate the motor Torque*/
static int protothreadMotor (struct pt *pt) {
  //static unsigned long timestamp = 0;
  PT_BEGIN(pt);
  while(1) {
    //Wait Until condition can also be changed to "when motorTorque value changed"
    PT_WAIT_UNTIL(pt, newTorque1 != motorTorque1 || newTorque2 != motorTorque2); 
    motorTorque1 = newTorque1;
    motorTorque2 = newTorque2;
    
    if(motorTorque1 < 0)
    {
      //Motor A Clockwise
      digitalWrite(in2, LOW);
      digitalWrite(in1, HIGH);
    }
    else {
      //Motor A Counter-clockwise
      digitalWrite(in1, LOW);
      digitalWrite(in2, HIGH);
    }
    
    if(motorTorque2 > 0)
    {
      //Motor B Clockwise
      digitalWrite(in4, LOW);
      digitalWrite(in3, HIGH);
    }
    else {
      //Motor B Counter-clockwise
      digitalWrite(in3, LOW);
      digitalWrite(in4, HIGH);
    }
    
    analogWrite(MOTOR_PIN_1, abs(motorTorque1));
    analogWrite(MOTOR_PIN_2, abs(motorTorque2));

  }
  PT_END(pt);
}

/* Receive the String from the driver for motor*/
static int protothreadInput(struct pt *pt, int interval) {
  static unsigned long timestamp = 0;
  PT_BEGIN(pt);
  while(1) {
    PT_WAIT_UNTIL(pt, (Serial.available() > 0) && (millis() - timestamp > interval) ); // What should the condition be?
    timestamp = millis();
    //Read from the serial input buffer
    inputString = Serial.readStringUntil('\n');
    inputString.toCharArray(str,255);
    //Process Strings to get each motorTorquePercent
    sscanf(str,"%i-%i", &newTorque1, &newTorque2);
  }
  PT_END(pt);
}

/* Send the Angle position over serial*/
static int protothreadOutput(struct pt *pt, int interval) {
  static unsigned long timestamp = 0;
  PT_BEGIN(pt);
  while(1) {
    PT_WAIT_UNTIL(pt, newthetaX != thetaX || newthetaY != thetaY || newthetaZ != thetaZ); // What should the condition be?
    timestamp = millis();
    //Send the angle positions
    char outputString[10];
    thetaX = newthetaX;
    thetaY = newthetaY;
    thetaZ = newthetaZ;
    sprintf(outputString, "%d,%d,%d", thetaX, thetaY, thetaZ);
    Serial.println(outputString);
  }
  PT_END(pt);
}

void loop() {
  //Starting protothreads, and setting time interval (subject to change)
  protothreadAngle(&pt1, 1);     //Angle Proto-Thread
  protothreadInput(&pt3, 1);     //Serial Input Proto-Thread
  protothreadMotor(&pt2);         //Motor Proto-Thread
  protothreadOutput(&pt4, 1);  //Serial Output Proto-Thread
}
