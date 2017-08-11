//Code by Daniel Salomon

///// Changelog:

/////// 28.07.2017: - First testing
///////             - Binary encoding seems to work perfectly fine
///////             
/////// 29.07.2017: - Changed output to 5 pins   
///////
/////// 30.07.2017: - Final adjustments            


//////////Libraries
#include <IRLibAll.h>     //Includes the library for infrared receiving

//////////Constants
//PINs
#define IR_PIN    3  //Pin for the Infrared reciever
#define OUT_PIN1  8  //The output pins
#define OUT_PIN2  9
#define OUT_PIN3  10
#define OUT_PIN4  11
#define OUT_PIN5  12

//Settings
#define SERIALDEBUGGING 1 //Should serial send debugging information? 0=FALSE 1=TRUE

//Digital output timeout time
#define OUTPUT_TIME 70 //70 is a bit more than 2x the delay in loop() for the LED Arduino. Should be timeout enough!

//IR HEX values
uint32_t IR_signal_01 = 0xFFA05F; //Sends signal 1 and so on binary. What will happen at keypress is decided by LEDArdu itself.
uint32_t IR_signal_02 = 0xFF20DF;
uint32_t IR_signal_03 = 0xFF609F;
uint32_t IR_signal_04 = 0xFFE01F;
uint32_t IR_signal_05 = 0xFF906F;
uint32_t IR_signal_06 = 0xFF10EF;
uint32_t IR_signal_07 = 0xFF50AF;
uint32_t IR_signal_08 = 0xFFD02F;
uint32_t IR_signal_09 = 0xFFB04F;
uint32_t IR_signal_10 = 0xFF30CF;
uint32_t IR_signal_11 = 0xFF708F;
uint32_t IR_signal_12 = 0xFFF00F;
uint32_t IR_signal_13 = 0xFFA857;
uint32_t IR_signal_14 = 0xFF28D7;
uint32_t IR_signal_15 = 0xFF6897;
uint32_t IR_signal_16 = 0xFFE817;
uint32_t IR_signal_17 = 0xFF9867;
uint32_t IR_signal_18 = 0xFF18E7;
uint32_t IR_signal_19 = 0xFF58A7;
uint32_t IR_signal_20 = 0xFFD827;
uint32_t IR_signal_21 = 0xFF8877;
uint32_t IR_signal_22 = 0xFF08F7;
uint32_t IR_signal_23 = 0xFF48B7;
uint32_t IR_signal_24 = 0xFFC837;

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
  pinMode(OUT_PIN4, OUTPUT);
  pinMode(OUT_PIN5, OUTPUT);

  //Write a "LOW" value to the pins to initialize.
  digitalWrite(OUT_PIN1, LOW);
  digitalWrite(OUT_PIN2, LOW);
  digitalWrite(OUT_PIN3, LOW);
  digitalWrite(OUT_PIN4, LOW);
  digitalWrite(OUT_PIN5, LOW);
  
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
	if(irValue == IR_signal_01) recievedValue = 1;
  else if(irValue == IR_signal_02) recievedValue = 2;
  else if(irValue == IR_signal_03) recievedValue = 3;
  else if(irValue == IR_signal_04) recievedValue = 4;
  else if(irValue == IR_signal_05) recievedValue = 5;
  else if(irValue == IR_signal_06) recievedValue = 6;
  else if(irValue == IR_signal_07) recievedValue = 7;
  else if(irValue == IR_signal_08) recievedValue = 8;
  else if(irValue == IR_signal_09) recievedValue = 9;
  else if(irValue == IR_signal_10) recievedValue = 10;
  else if(irValue == IR_signal_11) recievedValue = 11;
  else if(irValue == IR_signal_12) recievedValue = 12;
  else if(irValue == IR_signal_13) recievedValue = 13;
  else if(irValue == IR_signal_14) recievedValue = 14;
  else if(irValue == IR_signal_15) recievedValue = 15;
  else if(irValue == IR_signal_16) recievedValue = 16;
  else if(irValue == IR_signal_17) recievedValue = 17;
  else if(irValue == IR_signal_18) recievedValue = 18;
  else if(irValue == IR_signal_19) recievedValue = 19;
  else if(irValue == IR_signal_20) recievedValue = 20;
  else if(irValue == IR_signal_21) recievedValue = 21;
  else if(irValue == IR_signal_22) recievedValue = 22;
  else if(irValue == IR_signal_23) recievedValue = 23;
  else if(irValue == IR_signal_24) recievedValue = 24;
  

  if (SERIALDEBUGGING) Serial.println(irValue, HEX); 
  //Serial.println(recievedValue);
	if (recievedValue != -1) {
		encodeSignal_helper(recievedValue);
    if (SERIALDEBUGGING) Serial.println(recievedValue);
	}
}

void encodeSignal_helper(int value) {
	int output[5];
	for (int i = 0; i < 5; i++) {
		output[i] = value%2;
		value = value/2;
	} //if value=1 output is 100, for 2 it is 010, for 3 it is 110 etc.
	if (output[0]) {digitalWrite(OUT_PIN1, HIGH);}
  if (output[1]) {digitalWrite(OUT_PIN2, HIGH);}
  if (output[2]) {digitalWrite(OUT_PIN3, HIGH);}
  if (output[3]) {digitalWrite(OUT_PIN4, HIGH);}
  if (output[4]) {digitalWrite(OUT_PIN5, HIGH);}
  if (SERIALDEBUGGING) {
    Serial.print(output[0]);
    Serial.print(output[1]);
    Serial.print(output[2]);
    Serial.print(output[3]);
    Serial.println(output[4]);
  }
}

void resetOutput() {
	delay(OUTPUT_TIME);
	  digitalWrite(OUT_PIN1, LOW);
    digitalWrite(OUT_PIN2, LOW);
    digitalWrite(OUT_PIN3, LOW);
    digitalWrite(OUT_PIN4, LOW);
    digitalWrite(OUT_PIN5, LOW);
}

//////////</Standard Functions>


