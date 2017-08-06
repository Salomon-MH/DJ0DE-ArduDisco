//Basic LED visualizer code by Michael Bartlett
//Extended, changed and optimized by Daniel Salomon (github.com/salomon-mh)

///// Changelog:

/////// 25.07.2017: - Initial Copy from Michael Bartlett
///////             - Extending Comments
///////             - Added StaticGreen as ColorPalette
///////             - Addeded IRLib2 as infrared library
///////             ---> Untested code, just experimental.
///////             
/////// 26.07.2017: - Initial testing and refining
///////             - Replacing physical button presses with IR remote. Works, but just in theory.
///////             - Replacing the real sound input with a random number.
///////             - Adding workaround for IR beeing 0 after NeoPixel's show() command
///////             ---> IR just works now because of a dirty hack. Better than nothing!
///////             
/////// 27.07.2017: - Added optical "alerts" if started recieving IR
///////             - Added optical "alerts" if recieved valid IR
///////             - Refined workaround for IR
///////             ---> IR now works relatively good and has fancy alerts. Still looking for a real "clean" fix though.
///////             ---> NOTE: This still uses no valid sound input, just random numbers. Real code comes after I have everything at hand.
///////             
/////// 28.07.2017: - Starting Code for dual Arduino setup 
///////             - Removed unneccessary code
///////             - Added decoding method
///////             - Untested, as I do not own two Arduinos yet. Use with caution. Dont forget to connect grounds between the Arduinos!
///////
/////// 29.07.2017: - Changed code for 5 pin decoding
///////             - Added microphone support
///////             
/////// 03.08.2017: - Added function for central key management
///////             - Added support for 3 seperate equally long strips, including seperate brightness levels etc.
///////             ---> NOTE: Code **wont** work for strips with different LED ammount or probably for one strip any longer too. Use older version instead, see GitHub history.
///////             
/////// 05.08.2017: - Bug fixes for starng selection, strang copying, strang moving, ... (By adding a 'virtual' second equal LED strang)
///////             
/////// 06.08.2017: - Added background light: A permanent light, which remains even when there is no visualization. Reason: Dark room during silence.
///////             - background light / backlight is slowly "pusling"
///////             - flexible ammount of copies/'instances'
///////             


//////////Libraries
#include <Adafruit_NeoPixel.h>  //Library to simplify interacting with the LED strand
#ifdef __AVR__
//#include <avr/power.h>   //Includes the library for power reduction registers if your chip supports them. 
                         //More info: http://www.nongnu.org/avr-libc/user-manual/group__avr__power.htlm
#endif

//////////Constants
//PINs
#define LED_PIN   2  //Pin for the pixel strand. Does not have to be analog.
#define AUDIO_PIN 7  //Pin for the envelope of the sound detector

#define INPUT_COUNT 5  //How many wires are conected for transmitting? Reading the values in the code has to be changed too when changing this!
#define INPUT_PIN1  8  //The input pins from IRArdu to control the lights
#define INPUT_PIN2  10  //WARNING: DO NOT FORGET TO CONNECT GROUNDS BETWEEN THE ARDUINOS.
#define INPUT_PIN3  13
#define INPUT_PIN4  11
#define INPUT_PIN5  12

//Others
#define LED_TOTAL 60  //Change this to the number of LEDs in your strand.
#define LED_HALF  LED_TOTAL/2
#define KEYRECIEVE_NOTIFICATION_TIME 100 //How long the white flash is shown after IR detected. 0 completely disables this fuction.
#define SERIALDEBUGGING 1 //Should serial send debugging information? 0=FALSE 1=TRUE
#define AUDIO_SAMLPING 255 //Audio sampling rate (For me 256 Bits is enough, 1024 Bits work good though too but is not necessary)
#define VISUALS   7 //Ammount of effects existing

//I have 3 LED strips which are all supposed to show the same effects.
//I'm going to define where which strip starts and ends here so the Ardu only has to calculate the effects one time.
//LEDSTRANG1 **has to** be the largest strang.
//Planning to get these obsolete. - UPDATE: These are now obsolete and are used nowhere, so they are commented out.
/*#define LEDSTRANG1_START 0
#define LEDSTRANG1_END 19
#define LEDSTRANG1_HALF (LEDSTRANG1_END-LEDSTRANG1_START)/2
#define LEDSTRANG2_START 20
#define LEDSTRANG2_END 39
#define LEDSTRANG3_START 40
#define LEDSTRANG3_END 59*/


//////////<Globals>
//  These values either need to be remembered from the last pass of loop() or 
//  need to be accessed by several functions in one pass, so they need to be global.

Adafruit_NeoPixel strand = Adafruit_NeoPixel(LED_TOTAL, 4, NEO_GRB + NEO_KHZ800);  // **DUMMY** LED strand object by NeoPixel library used for visualization
Adafruit_NeoPixel strandReal = Adafruit_NeoPixel(LED_TOTAL, LED_PIN, NEO_GRB + NEO_KHZ800);  //LED strand object by NeoPixel library used to show colors

//LED Color
uint16_t gradient = 0; //Used to iterate and loop through each color palette gradually
uint8_t palette = 0;  //Holds the current color palette.
uint8_t visual = 0;   //Holds the current visual being displayed.
float shuffleTime = 0;  //Holds how many seconds of runtime ago the last shuffle was (if shuffle mode is on).
bool shuffle = false;  //Toggles shuffle mode.
double brightness0 = 1;   //Used for adjusting the max brightness.
double brightness1 = 1;
double brightness2 = 1;
double brightness3 = 1;
bool isStaticLight = false; //Different Color behavior for static light.
uint8_t staticRed = 255; //Values for color for static light.
uint8_t staticGreen = 255;
uint8_t staticBlue = 255;
uint8_t staticState = 0; //Value for current color state for static light.
float timeBacklightChanged = 0;
bool negBacklightChange = true;

uint8_t selectedStrip = 0;
//bool isWholeVisualization = false; //OBSOLETE + UNUSED: Defines if strangs should be handled as one or as seperate ones
uint8_t virtualStripCount = 9; //Defines, how many times the vis should be copied. Makes isWholeVisualization obsolete.
uint8_t staticBacklight = 5;
bool staticBacklightEnabled = true; //Defines a static backlight so the room is never completely dark
bool shiftOneRight = false; //Shifts all strangs one to the right. So it is 3->1->2

//IMPORTANT:
//  This array holds the "threshold" of each color function (i.e. the largest number they take before repeating).
//  The values are in the same order as in ColorPalette()'s switch case (Rainbow() is first, etc). This is simply to
//  keep "gradient" from overflowing, the color functions themselves can take any positive value. For example, the
//  largest value Rainbow() takes before looping is 1529, so "gradient" should reset after 1529, as listed.
//     Make sure you add/remove values accordingly if you add/remove a color function in the switch-case in ColorPalette().
uint16_t thresholds[] = {1529, 1019, 764, 764, 764, 1274, 764};

//Sound
uint8_t volume = 0;    //Holds the volume level read from the sound detector.
uint8_t last = 0;      //Holds the value of volume from the previous loop() pass.
float maxVol = 15;     //Holds the largest volume recorded thus far to proportionally adjust the visual's responsiveness.
float avgVol = 0;      //Holds the "average" volume-level to proportionally adjust the visual experience.
float avgBump = 0;     //Holds the "average" volume-change to trigger a "bump."
bool bump = false;     //Used to pass if there was a "bump" in volume

//Temporary storage for input signal
int recevInput[INPUT_COUNT];
uint16_t decodedInput = 0;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//NOTE: The reason "average" is quoted is because it is not a true mathematical average. This is because I have
//      found what I call a "sequenced average" is more successful in execution than a real average. The difference
//      is that the sequenced average doesn't use the pool of all values recorded thus far, but rather averages the
//      last average and the current value received (in sequence). Concretely:
//
//          True average: (1 + 2 + 3) / 3 = 2
//          Sequenced: (1 + 2) / 2 = 1.5 --> (1.5 + 3) / 2 = 2.25  (if 1, 2, 3 was the order the values were received)
//
//      All "averages" in the program operate this way. The difference is subtle, but the reason is that sequenced
//      averages are more adaptive to changes in the overall volume. In other words, if you went from loud to quiet,
//      the sequenced average is more likely to show an accurate and proportional adjustment more fluently.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//For Traffic() visual
//int8_t pos[LED_TOTAL] = { -2};    //Stores a population of color "dots" to iterate across the LED strand.
//uint8_t rgb[LED_TOTAL][3] = {0};  //Stores each dot's specific RGB values.
int8_t pos[LED_TOTAL/3] = { -2};    //Stores a population of color "dots" to iterate across the LED strand.
uint8_t rgb[LED_TOTAL/3][3] = {0};  //Stores each dot's specific RGB values.


//For Snake() visual
bool left = false;  //Determines the direction of iteration. Recycled in PaletteDance()
int8_t dotPos = 0;  //Holds which LED in the strand the dot is positioned at. Recycled in most other visuals.
float timeBump = 0; //Holds the time (in runtime seconds) the last "bump" occurred.
float avgTime = 0;  //Holds the "average" amount of time between each "bump" (used for pacing the dot's movement).

//For LoopThrough() visual
int loopthroughcounter = 0;

//////////</Globals>


//////////<Standard Functions>

void setup() {    //Like it's named, this gets ran before any other function.

  Serial.begin(9600); //Sets data rate for serial data transmission.

  //Defines the input pins to be input.
  pinMode(INPUT_PIN1, INPUT);
  pinMode(INPUT_PIN2, INPUT);
  pinMode(INPUT_PIN3, INPUT);
  pinMode(INPUT_PIN4, INPUT);
  pinMode(INPUT_PIN5, INPUT);

  //Write a "LOW" value to the input pins.
  /*digitalWrite(INPUT_PIN1, LOW);
  digitalWrite(INPUT_PIN2, LOW);
  digitalWrite(INPUT_PIN3, LOW);
  digitalWrite(INPUT_PIN4, LOW);
  digitalWrite(INPUT_PIN5, LOW);*/
  timeBacklightChanged = millis();
  
  strand.begin(); //Initialize the LED strand object.
  strandReal.begin();
  strandReal.show();  //Show a blank strand, just to get the LED's ready for use.

}


void loop() {  //This is where the magic happens. This loop produces each frame of the visual.

  decodeInput(); //Decodes the input from binary to integer

  //Record the volume level from the sound detector
  volume = 0;
  int volumetickcnt = 0;
  while(!digitalRead(AUDIO_PIN) || volumetickcnt < AUDIO_SAMLPING) {
    if(volume < AUDIO_SAMLPING && !digitalRead(AUDIO_PIN)) {
      volume++;
    } //else break;
    volumetickcnt++;
  }

  //Alternative method for recording. Isnt that fancy as the previous one though, so it's commented.
  /*while(!digitalRead(AUDIO_PIN) && volume < 400) {
        volume++;
    }*/
  //Would be a working frequency filter for the alternative recording method.
  //This one interferes with the "bump" of the original code, so it's just nice to know that it works.
  /*if (volume > 200 && volume < 300) {volume = volume;}
  else volume = 0;*/

  
  //Serial.println(volume);
  
  //"Virtual" music for testing.
  //volume = 12 + random(15);
  
  //brightness0 = analogRead(KNOB_PIN) / 1023.0; //Record how far the trimpot is twisted
  //knob's default value is 1 now. Value of knob is defined by CycleBrightness()!!!

  //Sets a threshold for volume.
  //  In practice I've found noise can get up to 15, so if it's lower, the visual thinks it's silent.
  //  Also if the volume is less than average volume / 2 (essentially an average with 0), it's considered silent.
  if (volume < avgVol / 2.0 || volume < 15) volume = 0;

  else avgVol = (avgVol + volume) / 2.0; //If non-zeo, take an "average" of volumes.

  //If the current volume is larger than the loudest value recorded, overwrite
  if (volume > maxVol) maxVol = volume;

  //Check the Cycle* functions for specific instructions if you didn't include buttons in your design.
  ////////////////////////////////////////////////////////////////////////////////////////////////////
  //CyclePalette();  //Changes palette for shuffle mode or button press.
  
  //CycleVisual();   //Changes visualization for shuffle mode or button press.

  //ToggleShuffle(); //Toggles shuffle mode. Delete this if you didn't use buttons.
  
  //CycleBrightness();
  ProcessInput(); //Calls specific functions depending on input given by IRArdu
  isStaticLight = false; //Resets static light option.
  ////////////////////////////////////////////////////////////////////////////////////////////////////

  //This is where "gradient" is modulated to prevent overflow.
  if (gradient > thresholds[palette]) {
    gradient %= thresholds[palette] + 1;

    //Everytime a palette gets completed is a good time to readjust "maxVol," just in case
    //  the song gets quieter; we also don't want to lose brightness intensity permanently
    //  because of one stray loud sound.
    maxVol = (maxVol + volume) / 2.0;
  }

  //If there is a decent change in volume since the last pass, average it into "avgBump"
  if (volume - last > 10) avgBump = (avgBump + (volume - last)) / 2.0;

  //If there is a notable change in volume, trigger a "bump"
  //  avgbump is lowered just a little for comparing to make the visual slightly more sensitive to a beat.
  bump = (volume - last > avgBump * .9);  

  //If a "bump" is triggered, average the time between bumps
  if (bump) {
    avgTime = (((millis() / 1000.0) - timeBump) + avgTime) / 2.0;
    timeBump = millis() / 1000.0;
  }

  Visualize();   //Calls the appropriate visualization to be displayed with the globals as they are.
  controlBacklight();
  CopyLEDContentAndApplyBrightness();
  strandReal.show(); //This command actually shows the lights. NEW: Called here, not in the visualizations.
  
  gradient++;    //Increments gradient

  last = volume; //Records current volume for next pass

  delay(30);     //Paces visuals so they aren't too fast to be enjoyable
}

//////////</Standard Functions>


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////       VISUAL       //////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


//This function calls the appropriate visualization based on the value of "visual"
void Visualize() {
  switch (visual) {
    case 0: return Pulse();
    case 1: return PalettePulse();
    case 2: return Traffic();
    case 3: return Snake();
    case 4: return PaletteDance();
    case 5: return Glitter();
    case 6: return Paintball();
    case 7: return StaticLight()/*LoopThrough()*/;
    default: return Pulse();
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
//NOTE: The strand displays RGB values as a 32 bit unsigned integer (uint32_t), which is why ColorPalette()
//      and all associated color functions' return types are uint32_t. This value is a composite of 3
//      unsigned 8bit integer (uint8_t) values (0-255 for each of red, blue, and green). You'll notice the
//      function split() (listed below) is used to dissect these 8bit values from the 32-bit color value.
//////////////////////////////////////////////////////////////////////////////////////////////////////////


//This function calls the appropriate color palette based on "palette"
//  If a negative value is passed, returns the appropriate palette withe "gradient" passed.
//  Otherwise returns the color palette with the passed value (useful for fitting a whole palette on the strand).
uint32_t ColorPalette(float num) {
  switch (palette) {
    case 0: return (num < 0) ? Rainbow(gradient) : Rainbow(num);
    case 1: return (num < 0) ? Sunset(gradient) : Sunset(num);
    case 2: return (num < 0) ? Ocean(gradient) : Ocean(num);
    case 3: return (num < 0) ? PinaColada(gradient) : PinaColada(num);
    case 4: return (num < 0) ? Sulfur(gradient) : Sulfur(num);
    case 5: return (num < 0) ? NoGreen(gradient) : NoGreen(num);
  case 6: return (num < 0) ? StaticGreen(gradient) : StaticGreen(num);
    default: return Rainbow(gradient);
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//NOTE: All of these visualizations feature some aspect that affects brightness based on the volume relative to
//      maxVol, so that louder = brighter. Initially, I did simple proportions (volume/maxvol), but I found this
//      to be visually indistinct. I then tried an exponential method (raising the value to the power of
//      volume/maxvol). While this was more visually satisfying, I've opted for a balance between the two. You'll
//      notice something like pow(volume/maxVol, 2.0) in the functions below. This simply squares the ratio of
//      volume to maxVol to get a more exponential curve, but not as exaggerated as an actual exponential curve.
//      In essence, this makes louder volumes brighter, and lower volumes dimmer, to be more visually distinct.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////
////////////////////        PULSE       ////////////////////
////////////////////////////////////////////////////////////
//Pulse from center of the strand

void Pulse() {

  fade(0.75);   //Listed below, this function simply dims the colors a little bit each pass of loop()

  //Advances the palette to the next noticeable color if there is a "bump"
  if (bump) gradient += thresholds[palette] / 24;

  //If it's silent, we want the fade effect to take over, hence this if-statement
  if (volume > 0) {
    uint32_t col = ColorPalette(-1); //Our retrieved 32-bit color

    //These variables determine where to start and end the pulse since it starts from the middle of the strand.
    //  The quantities are stored in variables so they only have to be computed once (plus we use them in the loop).
    int start = getStripMid(1) - (getStripMid(1) * (volume / maxVol));
    int finish = getStripMid(1) + (getStripMid(1) * (volume / maxVol)) + getStripEnd(1) % 2;
    //Listed above, LED_HALF is simply half the number of LEDs on your strand. ? this part adjusts for an odd quantity.

    for (int i = start; i < finish; i++) {

      //"damp" creates the fade effect of being dimmer the farther the pixel is from the center of the strand.
      //  It returns a value between 0 and 1 that peaks at 1 at the center of the strand and 0 at the ends.
      float damp = sin((i - start) * PI / float(finish - start));

      //Squaring damp creates more distinctive brightness.
      damp = pow(damp, 2.0);

      //Fetch the color at the current pixel so we can see if it's dim enough to overwrite.
      uint32_t col2 = strand.getPixelColor(i);

      //Takes advantage of one for loop to do the following:
      // Appropriatley adjust the brightness of this pixel using location, volume, and "knob"
      // Take the average RGB value of the intended color and the existing color, for comparison
      uint8_t colors[3];
      float avgCol = 0, avgCol2 = 0;
      for (int k = 0; k < 3; k++) {
        colors[k] = split(col, k) * damp * brightness0 * pow(volume / maxVol, 2);
        avgCol += colors[k];
        avgCol2 += split(col2, k);
      }
      avgCol /= 3.0, avgCol2 /= 3.0;

      //Compare the average colors as "brightness". Only overwrite dim colors so the fade effect is more apparent.
      if (avgCol > avgCol2) strand.setPixelColor(i, strand.Color(colors[0], colors[1], colors[2]));
    }
  }
  
}



////////////////////////////////////////////////////////////
////////////////////    PALETTEPULSE    ////////////////////
////////////////////////////////////////////////////////////
//Same as Pulse(), but colored the entire pallet instead of one solid color

void PalettePulse() {
  fade(0.75);
  if (bump) gradient += thresholds[palette] / 24;
  if (volume > 0) {
    int start = getStripMid(1) - (getStripMid(1) * (volume / maxVol));
    int finish = getStripMid(1) + (getStripMid(1) * (volume / maxVol)) + getStripEnd(1) % 2;
	  for (int i = start; i < finish; i++) {
      float damp = sin((i - start) * PI / float(finish - start));
      damp = pow(damp, 2.0);

      //This is the only difference from Pulse(). The color for each pixel isn't the same, but rather the
      //  entire gradient fitted to the spread of the pulse, with some shifting from "gradient".
      int val = thresholds[palette] * (i - start) / (finish - start);
      val += gradient;
      uint32_t col = ColorPalette(val);

      uint32_t col2 = strand.getPixelColor(i);
      uint8_t colors[3];
      float avgCol = 0, avgCol2 = 0;
      for (int k = 0; k < 3; k++) {
        colors[k] = split(col, k) * damp * brightness0 * pow(volume / maxVol, 2);
        avgCol += colors[k];
        avgCol2 += split(col2, k);
      }
      avgCol /= 3.0, avgCol2 /= 3.0;
      if (avgCol > avgCol2) strand.setPixelColor(i, strand.Color(colors[0], colors[1], colors[2]));
    }
  }
  //strand.show();
}


////////////////////////////////////////////////////////////
////////////////////       TRAFFIC      ////////////////////
////////////////////////////////////////////////////////////
//Dots racing into each other

void Traffic() {

  //fade() actually creates the trail behind each dot here, so it's important to include.
  fade(0.8);

  //Create a dot to be displayed if a bump is detected.
  if (bump) {

    //This mess simply checks if there is an open position (-2) in the pos[] array.
    int8_t slot = 0;
    for (slot; slot < sizeof(pos); slot++) {
      if (pos[slot] < -1) break;
      else if (slot + 1 >= sizeof(pos)) {
        slot = -3;
        break;
      }
    }

    //If there is an open slot, set it to an initial position on the strand.
    if (slot != -3) {

      //Evens go right, odds go left, so evens start at 0, odds at the largest position.
      pos[slot] = (slot % 2 == 0) ? -1 : getStripEnd(1);

      //Give it a color based on the value of "gradient" during its birth.
      uint32_t col = ColorPalette(-1);
      gradient += thresholds[palette] / 24;
      for (int j = 0; j < 3; j++) {
        rgb[slot][j] = split(col, j);
      }
    }
  }

  //Again, if it's silent we want the colors to fade out.
  if (volume > 0) {

    //If there's sound, iterate each dot appropriately along the strand.
    for (int i = 0; i < sizeof(pos); i++) {

      //If a dot is -2, that means it's an open slot for another dot to take over eventually.
      if (pos[i] < -1) continue;

      //As above, evens go right (+1) and odds go left (-1)
      pos[i] += (i % 2) ? -1 : 1;

      //Odds will reach -2 by subtraction, but if an even dot goes beyond the LED strip, it'll be purged.
      if (pos[i] >= getStripEnd(1)) pos[i] = -2;

      //Set the dot to its new position and respective color.
      //  I's old position's color will gradually fade out due to fade(), leaving a trail behind it.
      strand.setPixelColor( pos[i], strand.Color(
                              float(rgb[i][0]) * pow(volume / maxVol, 2.0) * brightness0,
                              float(rgb[i][1]) * pow(volume / maxVol, 2.0) * brightness0,
                              float(rgb[i][2]) * pow(volume / maxVol, 2.0) * brightness0)
                          );
    }
  }
  //strand.show(); //Again, don't forget to actually show the lights!
}



////////////////////////////////////////////////////////////
////////////////////        SNAKE       ////////////////////
////////////////////////////////////////////////////////////
//Dot sweeping back and forth to the beat

void Snake() {
  if (bump) {

    //Change color a little on a bump
    gradient += thresholds[palette] / 30;

    //Change the direction the dot is going to create the illusion of "dancing."
    left = !left;
  }

  fade(0.975); //Leave a trail behind the dot.

  uint32_t col = ColorPalette(-1); //Get the color at current "gradient."

  //The dot should only be moved if there's sound happening.
  //  Otherwise if noise starts and it's been moving, it'll appear to teleport.
  if (volume > 0) {

    //Sets the dot to appropriate color and intensity
    strand.setPixelColor(dotPos, strand.Color(
                           float(split(col, 0)) * pow(volume / maxVol, 1.5) * brightness0,
                           float(split(col, 1)) * pow(volume / maxVol, 1.5) * brightness0,
                           float(split(col, 2)) * pow(volume / maxVol, 1.5) * brightness0)
                        );

    //This is where "avgTime" comes into play.
    //  That variable is the "average" amount of time between each "bump" detected.
    //  So we can use that to determine how quickly the dot should move so it matches the tempo of the music.
    //  The dot moving at normal loop speed is pretty quick, so it's the max speed if avgTime < 0.15 seconds.
    //  Slowing it down causes the color to update, but only change position every other amount of loops.
    if (avgTime < 0.15)                                               dotPos += (left) ? -1 : 1;
    else if (avgTime >= 0.15 && avgTime < 0.5 && gradient % 2 == 0)   dotPos += (left) ? -1 : 1;
    else if (avgTime >= 0.5 && avgTime < 1.0 && gradient % 3 == 0)    dotPos += (left) ? -1 : 1;
    else if (gradient % 4 == 0)                                       dotPos += (left) ? -1 : 1;
  }

  //strand.show(); // Display the lights

  //Check if dot position is out of bounds.
  if (dotPos < 0)    dotPos = getStripEnd(1);
  else if (dotPos >= getStripEnd(1)+1)  dotPos = getStripStart(1);
}


////////////////////////////////////////////////////////////
////////////////////    PALETTEDANCE    ////////////////////
////////////////////////////////////////////////////////////
//Projects a whole palette which oscillates to the beat, similar to the snake but a whole gradient instead of a dot

void PaletteDance() {
  //This is the most calculation-intensive visual, which is why it doesn't need delayed.

  if (bump) left = !left; //Change direction of iteration on bump

  //Only show if there's sound.
  
  int boundLow = getStripStart(1);
  int boundHigh = getStripEnd(1)+1;
  
  if (volume > avgVol) {

    //This next part is convoluted, here's a summary of what's happening:
    //  First, a sin wave function is introduced to change the brightness of all the pixels (stored in "sinVal")
    //      This is to make the dancing effect more obvious. The trick is to shift the sin wave with the color so it all appears
    //      to be the same object, one "hump" of color. "dotPos" is added here to achieve this effect.
    //  Second, the entire current palette is proportionally fitted to the length of the LED strand (stored in "val" each pixel).
    //      This is done by multiplying the ratio of position and the total amount of LEDs to the palette's threshold.
    //  Third, the palette is then "shifted" (what color is displayed where) by adding "dotPos."
    //      "dotPos" is added to the position before dividing, so it's a mathematical shift. However, "dotPos"'s range is not
    //      the same as the range of position values, so the function map() is used. It's basically a built in proportion adjuster.
    //  Lastly, it's all multiplied together to get the right color, and intensity, in the correct spot.
    //      "gradient" is also added to slowly shift the colors over time.
    for (int i = boundLow; i < boundHigh; i++) {

      float sinVal = abs(sin(
                           (i + dotPos) *
                           (PI / float(boundHigh / 1.25) )
                         ));
      sinVal *= sinVal;
      sinVal *= volume / maxVol;
      sinVal *= brightness0;

      unsigned int val = float(thresholds[palette] + 1)
                         //map takes a value between -LED_TOTAL and +LED_TOTAL and returns one between 0 and LED_TOTAL
                         * (float(i + map(dotPos, -1 * (boundHigh - 1), boundHigh - 1, 0, boundHigh - 1))
                            / float(boundHigh))
                         + (gradient);

      val %= thresholds[palette]; //make sure "val" is within range of the palette

      uint32_t col = ColorPalette(val); //get the color at "val"

      strand.setPixelColor(i, strand.Color(
                             float(split(col, 0))*sinVal,
                             float(split(col, 1))*sinVal,
                             float(split(col, 2))*sinVal)
                          );
    }

    //After all that, appropriately reposition "dotPos."
    dotPos += (left) ? -1 : 1;
  }

  //If there's no sound, fade.
  else  fade(0.8);

  //strand.show(); //Show lights.

  //Loop "dotPos" if it goes out of bounds.
  if (dotPos < boundLow) dotPos = boundHigh - boundHigh / 6;
  else if (dotPos >= boundHigh - boundHigh / 6)  dotPos = boundLow;
}


////////////////////////////////////////////////////////////
////////////////////      GLITTER       ////////////////////
////////////////////////////////////////////////////////////
//Creates white sparkles on a color palette to the beat

void Glitter() {

  //This visual also fits a whole palette on the entire strip
  //  This just makes the palette cycle more quickly so it's more visually pleasing
  gradient += thresholds[palette] / 204;
  
  int boundLow = getStripStart(1);
  int boundHigh = getStripEnd(1)+1;
  
  //"val" is used again as the proportional value to pass to ColorPalette() to fit the whole palette.
  for (int i = boundLow; i < boundHigh; i++) {
    unsigned int val = float(thresholds[palette] + 1) *
                       (float(i) / float(boundHigh))
                       + (gradient);
    val %= thresholds[palette];
    uint32_t  col = ColorPalette(val);

    //We want the sparkles to be obvious, so we dim the background color.
    strand.setPixelColor(i, strand.Color(
                           split(col, 0) / 6.0 * brightness0,
                           split(col, 1) / 6.0 * brightness0,
                           split(col, 2) / 6.0 * brightness0)
                        );
  }

  //Create sparkles every bump
  if (bump) {

    //Random generator needs a seed, and micros() gives a large range of values.
    //  micros() is the amount of microseconds since the program started running.
    randomSeed(micros());

    //Pick a random spot on the strand.
    dotPos = random(boundHigh - 1);

    //Draw  sparkle at the random position, with appropriate brightness.
    strand.setPixelColor(dotPos, strand.Color(
                           255.0 * pow(volume / maxVol, 2.0) * brightness0,
                           255.0 * pow(volume / maxVol, 2.0) * brightness0,
                           255.0 * pow(volume / maxVol, 2.0) * brightness0
                         ));
  }
  bleed(dotPos);
  //strand.show(); //Show the lights.
}


////////////////////////////////////////////////////////////
////////////////////      PAINTBALL     ////////////////////
////////////////////////////////////////////////////////////
// Recycles Glitter()'s random positioning; simulates "paintballs" of
//  color splattering randomly on the strand and bleeding together.

void Paintball() {

  //If it's been twice the average time for a "bump" since the last "bump," start fading.
  if ((millis() / 1000.0) - timeBump > avgTime * 2.0) fade(0.99);

  //Bleeds colors together. Operates similarly to fade. For more info, see its definition below
  bleed(dotPos);

  //Create a new paintball if there's a bump (like the sparkles in Glitter())
  if (bump) {

    //Random generator needs a seed, and micros() gives a large range of values.
    //  micros() is the amount of microseconds since the program started running.
    randomSeed(micros());

    //Pick a random spot on the strip. Random was already reseeded above, so no real need to do it again.
    dotPos = random(getStripEnd(1));

    //Grab a random color from our palette.
    uint32_t col = ColorPalette(random(thresholds[palette]));

    //Array to hold final RGB values
    uint8_t colors[3];

    //Relates brightness of the color to the relative volume and potentiometer value.
    for (int i = 0; i < 3; i++) colors[i] = split(col, i) * pow(volume / maxVol, 2.0) * brightness0;

    //Splatters the "paintball" on the random position.
    strand.setPixelColor(dotPos, strand.Color(colors[0], colors[1], colors[2]));

    //This next part places a less bright version of the same color next to the left and right of the
    //  original position, so that the bleed effect is stronger and the colors are more vibrant.
    for (int i = 0; i < 3; i++) colors[i] *= .8;
    strand.setPixelColor(dotPos - 1, strand.Color(colors[0], colors[1], colors[2]));
    strand.setPixelColor(dotPos + 1, strand.Color(colors[0], colors[1], colors[2]));
  }
  //strand.show(); //Show lights.
}


////////////////////////////////////////////////////////////
////////////////////    Static Light    ////////////////////
////////////////////////////////////////////////////////////
// Provides a static light. SELFMADE.

void StaticLight() {
  isStaticLight = true;
  for (int i = 0; i < getStripEnd(1)+1; i++) {
    strand.setPixelColor(i, strand.Color(staticRed*brightness0,staticGreen*brightness0,staticBlue*brightness0));
  }
  //strand.show();
}

////////////////////////////////////////////////////////////
////////////////////    Loop through    ////////////////////
////////////////////////////////////////////////////////////
// Cycle through all LEDs seperately. SELFMADE.

void LoopThrough() {
  isStaticLight = true;
  for (int i = 0; i < getStripEnd(1)+1; i++) {
    strand.setPixelColor(i, strand.Color(0,0,0));
  }
  strand.setPixelColor(loopthroughcounter, strand.Color(staticRed*brightness0,staticGreen*brightness0,staticBlue*brightness0));
  if(loopthroughcounter+1 > getStripEnd(1)) loopthroughcounter = 0;
  else loopthroughcounter++;
  delay(250);
  //strand.show();
}




//////////</Visual Functions>


//////////<Helper Functions>

void CyclePalette() {

  //IMPORTANT: Delete this whole if-block if you didn't use buttons//////////////////////////////////

  //If the binary number 1 is recieved via inputs, action is performed
    if (SERIALDEBUGGING) Serial.println("Changing Palette!");
    showKeyRecieved();
    if (isStaticLight) {
    switch (staticState)
    {
    case 0: staticRed = 255; staticGreen = 0; staticBlue = 0; staticState = 1; break;//Switch to Red 1
    case 1: staticRed = 0; staticGreen = 255; staticBlue = 0; staticState = 2; break;//Switch to Green 2
    case 2: staticRed = 0; staticGreen = 0; staticBlue = 255; staticState = 3; break;//Switch to Blue 3
    case 3: staticRed = 255; staticGreen = 100; staticBlue = 0; staticState = 4; break;//Switch to Orange 4
    case 4: staticRed = 0; staticGreen = 255; staticBlue = 255; staticState = 5; break;//Switch to Aqua 5
    case 5: staticRed = 72; staticGreen = 0; staticBlue = 255; staticState = 6; break;//Switch to Purple 6
    case 6: staticRed = 255; staticGreen = 216; staticBlue = 0; staticState = 7; break;//Switch to Yellow 7
    case 7: staticRed = 255; staticGreen = 0; staticBlue = 110; staticState = 8; break;//Switch to Pink 8
    case 8: staticRed = 50; staticGreen = 205; staticBlue = 50; staticState = 9; break;//Switch to Lime 9
    case 9: staticRed = 255; staticGreen = 255; staticBlue = 255; staticState = 0; break;//Switch to White 0
    default: staticRed = 255; staticGreen = 255; staticBlue = 255; staticState = 0; break;//Switch to White 0
    }
    if (SERIALDEBUGGING) Serial.print("Switched static light to ");
    if (SERIALDEBUGGING) Serial.println(staticState);
  } else {
  
  palette++;     //This is this button's purpose, to change the color palette.

    //If palette is larger than the population of thresholds[], start back at 0
    //  This is why it's important you add a threshold to the array if you add a
    //  palette, or the program will cylce back to Rainbow() before reaching it.
    if (palette >= sizeof(thresholds) / 2) palette = 0;

    gradient %= thresholds[palette]; //Modulate gradient to prevent any overflow that may occur.

    //The button is close to the microphone on my setup, so the sound of pushing it is
    //  relatively loud to the sound detector. This causes the visual to think a loud noise
    //  happened, so the delay simply allows the sound of the button to pass unabated.
    //delay(350);

    maxVol = avgVol;  //Set max volume to average for a fresh experience.
  }
  ///////////////////////////////////////////////////////////////////////////////////////////////////

  //If shuffle mode is on, and it's been 30 seconds since the last shuffle, and then a modulo
  //  of gradient to get a random decision between palette or visualization shuffle
  /*if (shuffle && millis() / 1000.0 - shuffleTime > 30 && gradient % 2) {

    shuffleTime = millis() / 1000.0; //Record the time this shuffle happened.

    palette++;
    if (palette >= sizeof(thresholds) / 2) palette = 0;
    gradient %= thresholds[palette];
    maxVol = avgVol;  //Set the max volume to average for a fresh experience.
  }*/
}


void CycleVisual() {

    if (SERIALDEBUGGING) Serial.println("Changing Visual!");
    showKeyRecieved();
    visual++;     //The purpose of this button: change the visual mode
    //Serial.println(visual);

    gradient = 0; //Prevent overflow

    //Resets "visual" if there are no more visuals to cycle through.
    if (visual > VISUALS) visual = 0;
    //This is why you should change "VISUALS" if you add a visual, or the program loop over it.

    //Resets the positions of all dots to nonexistent (-2) if you cycle to the Traffic() visual.
    if (visual == 1) memset(pos, -2, sizeof(pos));

    //Gives Snake() and PaletteDance() visuals a random starting point if cycled to.
    int boundLow = getStripStart(1);
    int boundHigh = getStripEnd(1)+1;
	
	if (visual == 2 || visual == 3) {
      randomSeed(analogRead(0));
      dotPos = random(boundHigh);
    }

    //Like before, this delay is to prevent a button press from affecting "maxVol."
    //delay(350);

    maxVol = avgVol; //Set max volume to average for a fresh experience

  ///////////////////////////////////////////////////////////////////////////////////////////////////

  //If shuffle mode is on, and it's been 30 seconds since the last shuffle, and then a modulo
  //  of gradient WITH INVERTED LOGIC to get a random decision between what to shuffle.
  //  This guarantees one and only one of these shuffles will occur.
  /*if (shuffle && millis() / 1000.0 - shuffleTime > 30 && !(gradient % 2)) {

    shuffleTime = millis() / 1000.0; //Record the time this shuffle happened.

    visual++;
    gradient = 0;
    if (visual > VISUALS) visual = 0;
    if (visual == 1) memset(pos, -2, sizeof(pos));
    if (visual == 2 || visual == 3) {
      randomSeed(analogRead(0));
      dotPos = random(strand.numPixels());
    }
    maxVol = avgVol;
  }*/
}


void CycleBrightness() {

  //If infrared is recieved and matches with IR_CHANGEBRIGHTNESS, action is performed
    if (SERIALDEBUGGING) Serial.println("Changing Brightness!");
    showKeyRecieved();
	if (selectedStrip == 0) {
		switch ((int)(brightness0*10)) {
			case 10: brightness0 = 0.9; break;
			case 9: brightness0 = 0.8; break;
			case 8: brightness0 = 0.7; break;
			case 7: brightness0 = 0.6; break;
			case 6: brightness0 = 0.5; break;
			case 5: brightness0 = 0.4; break;
			case 4: brightness0 = 0.3; break;
			case 3: brightness0 = 0.2; break;
			case 2: brightness0 = 0.1; break;
			case 1: brightness0 = 0.0; break;
			case 0: brightness0 = 1; break;
			default: brightness0 = 1.0; break;
		}
	} else if (selectedStrip == 1) {
		switch ((int)(brightness1*10)) {
			case 10: brightness1 = 0.9; break;
			case 9: brightness1 = 0.8; break;
			case 8: brightness1 = 0.7; break;
			case 7: brightness1 = 0.6; break;
			case 6: brightness1 = 0.5; break;
			case 5: brightness1 = 0.4; break;
			case 4: brightness1 = 0.3; break;
			case 3: brightness1 = 0.2; break;
			case 2: brightness1 = 0.1; break;
			case 1: brightness1 = 0.0; break;
			case 0: brightness1 = 1; break;
			default: brightness1 = 1.0; break;
		}
	} else if (selectedStrip == 2) {
		switch ((int)(brightness2*10)) {
			case 10: brightness2 = 0.9; break;
			case 9: brightness2 = 0.8; break;
			case 8: brightness2 = 0.7; break;
			case 7: brightness2 = 0.6; break;
			case 6: brightness2 = 0.5; break;
			case 5: brightness2 = 0.4; break;
			case 4: brightness2 = 0.3; break;
			case 3: brightness2 = 0.2; break;
			case 2: brightness2 = 0.1; break;
			case 1: brightness2 = 0.0; break;
			case 0: brightness2 = 1; break;
			default: brightness2 = 1.0; break;
		}
	} else if (selectedStrip == 3) {
		switch ((int)(brightness3*10)) {
			case 10: brightness3 = 0.9; break;
			case 9: brightness3 = 0.8; break;
			case 8: brightness3 = 0.7; break;
			case 7: brightness3 = 0.6; break;
			case 6: brightness3 = 0.5; break;
			case 5: brightness3 = 0.4; break;
			case 4: brightness3 = 0.3; break;
			case 3: brightness3 = 0.2; break;
			case 2: brightness3 = 0.1; break;
			case 1: brightness3 = 0.0; break;
			case 0: brightness3 = 1; break;
			default: brightness3 = 1.0; break;
		}
	}
}

void turnOff() {
	showKeyRecieved();
    if(brightness0 == 0) brightness0 = 1;
    else brightness0 = 0;
}

void turnOn() {
	showKeyRecieved();
    if(brightness0 == 0) brightness0 = 1;
    else brightness0 = 0;
}

void CycleSelection() {
	if (SERIALDEBUGGING) Serial.println("Switching selection!");
	if (selectedStrip == 3) selectedStrip = 0;
	else selectedStrip++;
	showSelected();
}

void showSelected() {
	int strangstart = 0;
	int strangend = 0;
	if (selectedStrip == 0) {strangstart = 0; strangend = strand.numPixels()-1;}
	else {strangstart = getStripStart(selectedStrip, 3); strangend = getStripEnd(selectedStrip, 3);}
	
	for (int i = strangstart; i < strangend; i++) {
			strandReal.setPixelColor(i, strand.Color(0,0,0));
	}
	for (int i = 0; i < 3; i++) {
		strandReal.setPixelColor(strangstart, strand.Color(0,0,255));
		strandReal.setPixelColor(strangend, strand.Color(0,0,255));
    if (selectedStrip == 0) {
      strandReal.setPixelColor(getStripEnd(1, 3), strand.Color(0,0,255));
      strandReal.setPixelColor(getStripStart(2, 3), strand.Color(0,0,255));
      strandReal.setPixelColor(getStripEnd(2, 3), strand.Color(0,0,255));
      strandReal.setPixelColor(getStripStart(3, 3), strand.Color(0,0,255));
    }
		strandReal.show();
		delay(300);
		decodeInput();
		if (ProcessInput()) break; //Breaks if new key is detected
		strandReal.setPixelColor(strangstart, strand.Color(0,0,0));
		strandReal.setPixelColor(strangend, strand.Color(0,0,0));
		if (selectedStrip == 0) {
      strandReal.setPixelColor(getStripEnd(1, 3), strand.Color(0,0,0));
      strandReal.setPixelColor(getStripStart(2, 3), strand.Color(0,0,0));
      strandReal.setPixelColor(getStripEnd(2, 3), strand.Color(0,0,0));
      strandReal.setPixelColor(getStripStart(3, 3), strand.Color(0,0,0));
    }
		strandReal.show();
		delay(150);
		decodeInput();
		if (ProcessInput()) break; //Breaks if new key is detected
	}
}

void controlBacklight() {
  //staticBacklight
  if (timeBacklightChanged+350 < millis()) {
    //uint8_t whattodo = random(2);
    timeBacklightChanged = millis();
    if (!negBacklightChange) staticBacklight++;
    else staticBacklight--;
    if (staticBacklight < 5) {staticBacklight = 5; negBacklightChange = false;}
    else if (staticBacklight > 12) {staticBacklight = 12; negBacklightChange = true;}
  }
}

void CopyLEDContentAndApplyBrightness() {
	//Copy all content. Also applys staticBacklight.
  for(int i = 0; i < strand.numPixels(); i++)
    {
      //Retrieve the color at the current position.
      uint32_t col = strand.getPixelColor(i);
      float colors[3]; //Array of the three RGB values
      for (int j = 0; j < 3; j++) colors[j] = split(col, j);
    
      strandReal.setPixelColor(i, colorCap(colors[0] + staticBacklight * 1 ), colorCap(colors[1] + staticBacklight), colorCap(colors[2] + staticBacklight * 0.4));
    }
  
	if (virtualStripCount > 1) { //Copies the visualization to all 'instances'
		for(int i = 0; i <= (getStripEnd(1) - getStripStart(1)); i++)
		{
			//Retrieve the color at the current position.
			uint32_t col = strand.getPixelColor(getStripStart(1)+i);
			float colors[3]; //Array of the three RGB values
			for (int j = 0; j < 3; j++) colors[j] = split(col, j);
		    
			for (int stranginforloop = 1; stranginforloop <= virtualStripCount+1; stranginforloop++) {
				if ((int)stranginforloop/3 <= 1) strandReal.setPixelColor(getStripStart(stranginforloop)+i, colorCap(colors[0] * brightness1 + staticBacklight * 1), colorCap(colors[1] * brightness1 + staticBacklight), colorCap(colors[2] * brightness1 + staticBacklight * 0.4));
				else if ((int)stranginforloop/3 == 2) strandReal.setPixelColor(getStripStart(stranginforloop)+i, colorCap(colors[0] * brightness2 + staticBacklight * 1), colorCap(colors[1] * brightness2 + staticBacklight), colorCap(colors[2] * brightness2 + staticBacklight * 0.4));
				else if ((int)stranginforloop/3 >= 3) strandReal.setPixelColor(getStripStart(stranginforloop)+i, colorCap(colors[0] * brightness3 + staticBacklight * 1), colorCap(colors[1] * brightness3 + staticBacklight), colorCap(colors[2] * brightness3 + staticBacklight * 0.4));
			}
		}
		
	} else if (shiftOneRight /*&& isWholeVisualization*/) { //Shifts all LED-blocks one to the right, so the visualization with all LEDs working together 
															//works right in my setup. Basically moves the middle of the animation.
		for(int i = 0; i <= (getStripEnd(1, 3) - getStripStart(1, 3)); i++) {
			uint32_t col1 = strand.getPixelColor(getStripStart(1, 3)+i);
			float colors1[3]; //Array of the three RGB values for strang 1
			for (int j = 0; j < 3; j++) colors1[j] = split(col1, j);
			uint32_t col2 = strand.getPixelColor(getStripStart(2, 3)+i);
			float colors2[3]; //Array of the three RGB values for strang 2
			for (int j = 0; j < 3; j++) colors2[j] = split(col2, j);
			uint32_t col3 = strand.getPixelColor(getStripStart(3, 3)+i);
			float colors3[3]; //Array of the three RGB values for strang 3
			for (int j = 0; j < 3; j++) colors3[j] = split(col3, j);
			
			strandReal.setPixelColor(getStripStart(1, 3)+i, colorCap(colors2[0] * brightness1 + staticBacklight * 1), colorCap(colors2[1] * brightness1 + staticBacklight), colorCap(colors2[2] * brightness1 + staticBacklight * 0.4));
			strandReal.setPixelColor(getStripStart(2, 3)+i, colorCap(colors3[0] * brightness2 + staticBacklight * 1), colorCap(colors3[1] * brightness2 + staticBacklight), colorCap(colors3[2] * brightness2 + staticBacklight * 0.4));
			strandReal.setPixelColor(getStripStart(3, 3)+i, colorCap(colors1[0] * brightness3 + staticBacklight * 1), colorCap(colors1[1] * brightness3 + staticBacklight), colorCap(colors1[2] * brightness3 + staticBacklight * 0.4));
		}
	}
	
}

uint8_t colorCap(uint16_t givenColor) {
  if (givenColor < 256) return givenColor;
  else return 255;
}

uint16_t getStripStart(uint8_t stripNo) {
	//Dynamically returns strip start for strip no. 'stripNo' - makes LEDSTRANG1_START etc obsolete.
	return (LED_TOTAL/virtualStripCount)*(stripNo-1); //Example: LED_TOTAL=60, virtualStripCount=3, Results: 1:0 2:20 3:40
}

uint16_t getStripStart(uint8_t stripNo, uint8_t customStripCount) {
	//Dynamically returns strip start for strip no. 'stripNo' - makes LEDSTRANG1_START etc obsolete.
	return (LED_TOTAL/customStripCount)*(stripNo-1); //Example: LED_TOTAL=60, virtualStripCount=3, Results: 1:0 2:20 3:40
}

uint16_t getStripEnd(uint8_t stripNo) {
	//Dynamically returns strip end for strip no. 'stripNo' - makes LEDSTRANG1_END etc obsolete.
	return (getStripStart(stripNo+1) - 1); //Example: LED_TOTAL=60, virtualStripCount=3, Results: 1:19 2:39 3:59
	//This forces getStripStart to calculate the start position of a none-existing strip **once** (the last call), so dont force impossible results to return -1!
}

uint16_t getStripEnd(uint8_t stripNo, uint8_t customStripCount) {
	//Dynamically returns strip end for strip no. 'stripNo' - makes LEDSTRANG1_END etc obsolete.
	return (getStripStart(stripNo+1, customStripCount) - 1); //Example: LED_TOTAL=60, virtualStripCount=3, Results: 1:19 2:39 3:59
	//This forces getStripStart to calculate the start position of a none-existing strip **once** (the last call), so dont force impossible results to return -1!
}

uint16_t getStripMid(uint8_t stripNo) {
	//Dynamically returns strip middle for strip no. 'stripNo' - makes LEDSTRANG1_HALF etc obsolete.
	return getStripStart(stripNo) + (LED_TOTAL/virtualStripCount/2); 
}


//NEW: Central input management
bool ProcessInput() {
	switch(decodedInput) {
		case 1: CycleVisual(); return true;
		case 2: CyclePalette(); return true;
		case 3: CycleBrightness(); return true;
		case 4: /*CycleSelection();*/ return true; //Too many clicks after another might cause a stack overflow, but you have to be a total idiot to trigger that
		case 5: return true;
		case 6: return true;
		case 7: /*turnOff();*/ CycleSelection(); return true;
		case 8: turnOn(); return true;
		case 9: return true;
		case 10: return true;
		case 11: return true;
		//...
	}
}



// VISUAL EFFECT TO SHOW THAT A KEYSTROKE IS NOTICED.
void showKeyRecieved() {
	if (KEYRECIEVE_NOTIFICATION_TIME != 0)
	{
		for (int i = 1; i < strand.numPixels()-1; i++) {
			strandReal.setPixelColor(i, strand.Color(0,0,0));
		}
		strandReal.setPixelColor(getStripStart(1, 3), strand.Color(0,255,0));
		strandReal.setPixelColor(getStripEnd(1, 3), strand.Color(0,255,0));
		strandReal.setPixelColor(getStripStart(2, 3), strand.Color(0,255,0));
		strandReal.setPixelColor(getStripEnd(2, 3), strand.Color(0,255,0));
		strandReal.setPixelColor(getStripStart(3, 3), strand.Color(0,255,0));
		strandReal.setPixelColor(getStripEnd(3, 3), strand.Color(0,255,0));
		strandReal.show();
		delay(KEYRECIEVE_NOTIFICATION_TIME);
		for (int i = 0; i < strand.numPixels(); i++) {
			strandReal.setPixelColor(i, strand.Color(0,0,0));
		}
		strandReal.show();
	}

}

// VISUAL EFFECT TO SHOW THAT LISTENING TO IR STARTED (LEGACY AND NO LONGER NEEDED)
/*void showIRisListening() {
  for (int i = 1; i < strand.numPixels()-1; i++) {
    strand.setPixelColor(i, strand.Color(0,0,0));
  }
  strand.setPixelColor(0, strand.Color(60,0,0));
  strand.setPixelColor(strand.numPixels()-1, strand.Color(60,0,0));
  strand.show();
}*/

// Methods to decode the input given by IRArdu
void decodeInput() {
	recevInput[0] = digitalRead(INPUT_PIN1); //Read the values from the PINs.
  recevInput[1] = digitalRead(INPUT_PIN2);
  recevInput[2] = digitalRead(INPUT_PIN3);
	recevInput[3] = 0; //digitalRead(INPUT_PIN4);
  recevInput[4] = 0; //digitalRead(INPUT_PIN5); //Add or remove lines if you want more transfer PINs from IRArdu to LEDArdu. With 3 7 values are possible, with 4 15, with 5 31, with 6 63 and so on.
  if (SERIALDEBUGGING && (recevInput[0] || recevInput[1] || recevInput[2])) {Serial.print("Raw Input: "); Serial.print(recevInput[0]); Serial.print(recevInput[1]); Serial.println(recevInput[2]);}
	decodedInput = 0;
	for (int i = 0; i < INPUT_COUNT; i++) { //Decode the input.
		if (recevInput[i])
		{
			decodedInput += (pow(2, i)+0.1); //Adds the equivalent number to the result. Pin1: +1, Pin2: +2, Pin3: +4, ...
      if (SERIALDEBUGGING) {Serial.print("Input "); Serial.print(i); Serial.print(" is true, adding "); Serial.print(pow(2, i)); Serial.print(" to "); Serial.println(decodedInput);}
		}
	} 
	
	if (SERIALDEBUGGING && decodedInput != 0) {Serial.print("Decoded Input: "); Serial.println(decodedInput);}
}


//IMPORTANT: Delete this function  if you didn't use buttons./////////////////////////////////////////
// void ToggleShuffle() {
  // if (!digitalRead(BUTTON_3)) {

    // shuffle = !shuffle; //This button's purpose: toggle shuffle mode.

    /* This delay is to prevent the button from taking another reading while you're pressing it */
    // delay(500);

    /* Reset these things for a fresh experience. */
    // maxVol = avgVol;
    // avgBump = 0;
  // }
// }
//////////////////////////////////////////////////////////////////////////////////////////////////////


//Fades lights by multiplying them by a value between 0 and 1 each pass of loop().
void fade(float damper) {

  //"damper" must be between 0 and 1, or else you'll end up brightening the lights or doing nothing.

  int boundLow = 0;
  int boundHigh = strand.numPixels();
  
  
  for (int i = boundLow; i < boundHigh; i++) {

    //Retrieve the color at the current position.
    uint32_t col = strand.getPixelColor(i);

    //If it's black, you can't fade that any further.
    if (col == 0) continue;

    float colors[3]; //Array of the three RGB values

    //Multiply each value by "damper"
    for (int j = 0; j < 3; j++) colors[j] = split(col, j) * damper;

    //Set the dampened colors back to their spot.
    strand.setPixelColor(i, strand.Color(colors[0] , colors[1], colors[2]));
  }
}


//"Bleeds" colors currently in the strand by averaging from a designated "Point"
void bleed(uint8_t Point) {
	
  int boundLow = 1;
  int boundHigh = strand.numPixels();
  
	
  for (int i = boundLow; i < boundHigh; i++) {

    //Starts by look at the pixels left and right of "Point"
    //  then slowly works its way out
    int sides[] = {Point - i, Point + i};

    for (int i = 0; i < 2; i++) {

      //For each of Point+i and Point-i, the pixels to the left and right, plus themselves, are averaged together.
      //  Basically, it's setting one pixel to the average of it and its neighbors, starting on the left and right
      //  of the starting "Point," and moves to the ends of the strand
      int point = sides[i];
      uint32_t colors[] = {strand.getPixelColor(point - 1), strand.getPixelColor(point), strand.getPixelColor(point + 1)  };

      //Sets the new average values to just the central point, not the left and right points.
      strand.setPixelColor(point, strand.Color(
                             float( split(colors[0], 0) + split(colors[1], 0) + split(colors[2], 0) ) / 3.0,
                             float( split(colors[0], 1) + split(colors[1], 1) + split(colors[2], 1) ) / 3.0,
                             float( split(colors[0], 2) + split(colors[1], 2) + split(colors[2], 2) ) / 3.0)
                          );
    }
  }
}


//As mentioned above, split() gives you one 8-bit color value
//from the composite 32-bit value that the NeoPixel deals with.
//This is accomplished with the right bit shift operator, ">>"
uint8_t split(uint32_t color, uint8_t i ) {

  //0 = Red, 1 = Green, 2 = Blue

  if (i == 0) return color >> 16;
  if (i == 1) return color >> 8;
  if (i == 2) return color >> 0;
  return -1;
}

//////////</Helper Functions>


//////////<Palette Functions>

//These functions simply take a value and return a gradient color
//  in the form of an unsigned 32-bit integer

//The gradients return a different, changing color for each multiple of 255
//  This is because the max value of any of the 3 RGB values is 255, so it's
//  an intuitive cutoff for the next color to start appearing.
//  Gradients should also loop back to their starting color so there's no jumps in color.

uint32_t Rainbow(unsigned int i) {
  if (i > 1529) return Rainbow(i % 1530);
  if (i > 1274) return strand.Color(255, 0, 255 - (i % 255));   //violet -> red
  if (i > 1019) return strand.Color((i % 255), 0, 255);         //blue -> violet
  if (i > 764) return strand.Color(0, 255 - (i % 255), 255);    //aqua -> blue
  if (i > 509) return strand.Color(0, 255, (i % 255));          //green -> aqua
  if (i > 255) return strand.Color(255 - (i % 255), 255, 0);    //yellow -> green
  return strand.Color(255, i, 0);                               //red -> yellow
}

uint32_t Sunset(unsigned int i) {
  if (i > 1019) return Sunset(i % 1020);
  if (i > 764) return strand.Color((i % 255), 0, 255 - (i % 255));          //blue -> red
  if (i > 509) return strand.Color(255 - (i % 255), 0, 255);                //purple -> blue
  if (i > 255) return strand.Color(255, 128 - (i % 255) / 2, (i % 255));    //orange -> purple
  return strand.Color(255, i / 2, 0);                                       //red -> orange
}

uint32_t Ocean(unsigned int i) {
  if (i > 764) return Ocean(i % 765);
  if (i > 509) return strand.Color(0, i % 255, 255 - (i % 255));  //blue -> green
  if (i > 255) return strand.Color(0, 255 - (i % 255), 255);      //aqua -> blue
  return strand.Color(0, 255, i);                                 //green -> aqua
}

uint32_t PinaColada(unsigned int i) {
  if (i > 764) return PinaColada(i % 765);
  if (i > 509) return strand.Color(255 - (i % 255) / 2, (i % 255) / 2, (i % 255) / 2);  //red -> half white
  if (i > 255) return strand.Color(255, 255 - (i % 255), 0);                            //yellow -> red
  return strand.Color(128 + (i / 2), 128 + (i / 2), 128 - i / 2);                       //half white -> yellow
}

uint32_t Sulfur(unsigned int i) {
  if (i > 764) return Sulfur(i % 765);
  if (i > 509) return strand.Color(i % 255, 255, 255 - (i % 255));   //aqua -> yellow
  if (i > 255) return strand.Color(0, 255, i % 255);                 //green -> aqua
  return strand.Color(255 - i, 255, 0);                              //yellow -> green
}

uint32_t NoGreen(unsigned int i) {
  if (i > 1274) return NoGreen(i % 1275);
  if (i > 1019) return strand.Color(255, 0, 255 - (i % 255));         //violet -> red
  if (i > 764) return strand.Color((i % 255), 0, 255);                //blue -> violet
  if (i > 509) return strand.Color(0, 255 - (i % 255), 255);          //aqua -> blue
  if (i > 255) return strand.Color(255 - (i % 255), 255, i % 255);    //yellow -> aqua
  return strand.Color(255, i, 0);                                     //red -> yellow
}

//NOTE: This is an example of a non-gradient palette: you will get straight red, white, or blue
//      This works fine, but there is no gradient effect, this was merely included as an example.
//      If you wish to include it, put it in the switch-case in ColorPalette() and add its
//      threshold (764) to thresholds[] at the top.
uint32_t StaticGreen(unsigned int i) {
  if (i > 764) return StaticGreen(i % 765);
  if (i > 509) return strand.Color(40, 255, 100);      //Extron-green
  if (i > 255) return strand.Color(100, 255, 0);  //yellowish-green
  return strand.Color(0, 255, 0);                   //green only
}

//////////</Palette Functions>
