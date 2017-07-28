//Basic visualizer code by Michael Bartlett
//Extended by Daniel Salomon

///// Changelog:

/////// 28.07.2017: - First testing
///////             
///////             
///////             


//////////Libraries
#include <IRLibAll.h>     //Includes the library for infrared receiving

//////////Constants
//PINs
#define IR_PIN    3  //Pin for the Infrared reciever
#define OUT_PIN1  5
#define OUT_PIN2  5
#define OUT_PIN3  5

//IR HEX values
uint32_t IR_CHANGEVISUAL = 0x20DF04FB;//0x20DF04FB; //Example HEX value, change to actual values!
uint32_t IR_CHANGECOLOR = 0x20DF847B;//0x20DF847B;
uint32_t IR_CHANGEBRIGHTNESS = 0x20DFF906;//0x20DFF906;

//////////<Globals>
//  These values either need to be remembered from the last pass of loop() or 
//  need to be accessed by several functions in one pass, so they need to be global.

//Infrared
IRrecv irrecv(IR_PIN);
IRdecode irDecoder;
uint32_t irValue;

//////////</Globals>


//////////<Standard Functions>

void setup() {    //Like it's named, this gets ran before any other function.

  Serial.begin(9600); //Sets data rate for serial data transmission.

  //Defines the buttons pins to be input.
  //pinMode(BUTTON_1, INPUT); pinMode(BUTTON_2, INPUT); pinMode(BUTTON_3, INPUT);

  //Write a "HIGH" value to the button pins.
  //digitalWrite(BUTTON_1, HIGH); digitalWrite(BUTTON_2, HIGH); digitalWrite(BUTTON_3, HIGH);
  
  irrecv.enableIRIn(); // Start the infrared receiver
}


void loop() {  //This is where the magic happens. This loop produces each frame of the visual.

  irValue = 0;
  if (irrecv.getResults())
  {
     irDecoder.decode();

     if (irDecoder.value == 0) { //As the animation interrupts IR recieving, you have to press the same button two times in a row
       showIRisListening();
       for (int retryCnt = 0; retryCnt < 20; retryCnt++) { //I honestly have absolutely **NO IDEA** why this works, but it works like a charm!
        irrecv.disableIRIn();
        irrecv.enableIRIn();
        delay(200);
        irrecv.getResults();
        irDecoder.decode();
        if (irDecoder.value == IR_CHANGEVISUAL || irDecoder.value == IR_CHANGECOLOR || irDecoder.value == IR_CHANGEBRIGHTNESS) {
          retryCnt = 20;
          irValue = irDecoder.value;
        }
       }
     }
     //irValue = irDecoder.value;

     /**if (irDecoder.value != 0xFFFFFFFF)
     {
      irValue = irDecoder.value; //Detect the HEX results recieved.
      //Serial.println(irValue, HEX);
     }**/
     
     irrecv.enableIRIn(); // Receive the next infrared value
  } //irResults.value should be IR_CHANGEBRIGHTNESS, IR_CHANGECOLOR or IR_CHANGEVISUAL now, otherwise nothing should happen.
  
  //volume = analogRead(AUDIO_PIN);       //Record the volume level from the sound detector
     //Paces visuals so they aren't too fast to be enjoyable
}

//////////</Standard Functions>


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////       VISUAL       //////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



