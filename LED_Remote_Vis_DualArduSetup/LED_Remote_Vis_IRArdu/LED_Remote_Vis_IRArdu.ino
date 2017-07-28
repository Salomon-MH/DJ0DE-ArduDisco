//Code by Daniel Salomon

///// Changelog:

/////// 28.07.2017: - First testing
///////             - Binary encoding seems to work perfectly fine
///////             
///////             


//////////Libraries
#include <IRLibAll.h>     //Includes the library for infrared receiving

//////////Constants
//PINs
#define IR_PIN    3  //Pin for the Infrared reciever
#define OUT_PIN1  5  //The output pins
#define OUT_PIN2  6
#define OUT_PIN3  7

//Digital output timeout time
#define OUTPUT_TIME 70 //70 is a bit more than 2x the delay in loop() for the LED Arduino. Should be timeout enough!

//IR HEX values
uint32_t IR_signal_1 = 0x20DF04FB; //Used to be IR_CHANGEVISUAL //LG TV remote HEX values, change to your actual values!
uint32_t IR_signal_2 = 0x20DF847B; //Used to be IR_CHANGECOLOR
uint32_t IR_signal_3 = 0x20DFF906; //Used to be IR_CHANGEBRIGHTNESS

//Infrared
IRrecv irrecv(IR_PIN);
IRdecode irDecoder;
uint32_t irValue;


//////////<Standard Functions>

void setup() {    //Like it's named, this gets ran before any other function.

  Serial.begin(9600); //Sets data rate for serial data transmission.

  //Defines the pins to be output.
  pinMode(OUT_PIN1, OUTPUT);
  pinMode(OUT_PIN2, OUTPUT);
  pinMode(OUT_PIN3, OUTPUT);

  //Write a "LOW" value to the pins to initialize.
  digitalWrite(OUT_PIN1, LOW);
  digitalWrite(OUT_PIN2, LOW);
  digitalWrite(OUT_PIN3, LOW);
  
  irrecv.enableIRIn(); // Start the infrared receiver
}


void loop() {

  irValue = 0;
  if (irrecv.getResults())
  {
     irDecoder.decode();

        if (irDecoder.value != 0xFFFFFFFF) {
			irValue = irDecoder.value; //Saves the HEX result recieved.
        }
     
     irrecv.enableIRIn(); // Begin receiving the next infrared value
  }
  //irResults.value should be a valid IR signal now, otherwise nothing will happen.

  if (irValue != 0) {
    encodeSignal();
    resetOutput();
  }
  
}

void encodeSignal() {
	int recievedValue = -1;
	if(irValue == IR_signal_1) recievedValue = 1;
  else if(irValue == IR_signal_2) recievedValue = 2;
  else if(irValue == IR_signal_3) recievedValue = 3;
  
  Serial.println(recievedValue);
	if (recievedValue != -1) {
		encodeSignal_helper(recievedValue);
	}
}

void encodeSignal_helper(int value) {
	int output[3];
	for (int i = 0; i < 3; i++) {
		output[i] = value%2;
		value = value/2;
	} //if value=1 output is 100, for 2 it is 010, for 3 it is 110 etc.
	if (output[0]) digitalWrite(OUT_PIN1, HIGH);
  if (output[1]) digitalWrite(OUT_PIN2, HIGH);
  if (output[2]) digitalWrite(OUT_PIN3, HIGH);
}

void resetOutput() {
	delay(OUTPUT_TIME);
	  digitalWrite(OUT_PIN1, LOW);
    digitalWrite(OUT_PIN2, LOW);
    digitalWrite(OUT_PIN3, LOW);
}

//////////</Standard Functions>


