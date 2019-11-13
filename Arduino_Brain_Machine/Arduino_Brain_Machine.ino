
/***************************************************
  Sketch: Sound & Light Machine for Arduino
  Author: Chris Sparnicht - http://low.li
  Creation Date: 2011.01.31
  Last Modification Date: 2011.02.12
  License: Creative Commons 2.5 Attrib. & Share Alike

  Derivation and Notes:
  Make sure you have audio stereo 10K Ohm potentiometer to reduce
  the volume of the audio with your headset. If you don't,
  you might damage your ear drums, your arduino or your headset.

  Included with this sketch is a png diagram.

  This arduino sketch is based on the original Sound & Light Machine
  by - Mitch Altman - 19-Mar-07 as featured in Make Magazine 10.
  http://makezine.com/10/brainwave/

  See notes in code below for how I adapted Mitch Altman's version for Arduino

  The sleep coding comes partially from here:
  http://www.arduino.cc/playground/Learning/ArduinoSleepCode
***************************************************/

/***************************************************
  SOME INFORMATION ABOUT PROGMEM:
  First, you have to use #include <avr/pgmspace.h> for table arrays - PROGMEM
  The Arduino compiler creates code that will transfer all constants into RAM when the microcontroller
  resets.  This hardward could probably hold all this data in memory, but since the original system
  used a table (brainwaveTab) that is was too large to transfer into RAM in the original microcontroller,
  we're taking the same approach.  This is accomplished by using the library for PROGMEM.
  (This is used below in the definition for the brainwaveTab).  Since the
  C compiler assumes that constants are in RAM, rather than in program memory, when accessing
  the brainwaveTab, we need to use the pgm_read_byte() and pgm_read_dword() macros, and we need
  to use the brainwveTab as an address, i.e., precede it with "&".  For example, to access
  brainwaveTab[3].bwType, which is a byte, this is how to do it:
   pgm_read_byte( &brainwaveTab[3].bwType );
  And to access brainwaveTab[3].bwDuration, which is a double-word, this is how to do it:
   pgm_read_dword( &brainwaveTab[3].bwDuration );
 ***************************************************/

/***************************************************
  LIBRARIES - Define necessary libraries here.
***************************************************/
#include <avr/pgmspace.h> // for arrays - PROGMEM 
#include <avr/sleep.h> // A library to control the sleep mode
#include <avr/power.h> // A library to control power
#include <Tone.h> // Download from https://github.com/bhagman/Tone

/***************************************************
  GLOBALS
  We isolate calls to pins with these globals so we can change
  which pin we'll use i one please, rather than having to search and replace
  in many places.
***************************************************/

// =============== uncomment if you want Serial debug reports ===============
// #define DEBUG
// =========================================================================

#define rightEyeRed 5 // Define pinout for right eye
#define leftEyeRed 6 // Define pinout for left eye
#define rightEarLow 9 // Define pinout for left ear
#define lefttEarLow 10 // Define pinout for left ear
#define interruptPin 2 // the input pin where the pushbutton is connected.

// Common anode. 255 is off
#define LED_INTENSITY 192 // 0 to 255
#define LED_ON (255-LED_INTENSITY)
#define LED_OFF 255

/***************************************************
  BRAINWAVE TABLE
  See 'Some information about PROGMEM' above.
  Table of values for meditation start with
  lots of Beta (awake / conscious)
  add Alpha (dreamy / trancy to connect with
      subconscious Theta that'll be coming up)
  reduce Beta (less conscious)
  start adding Theta (more subconscious)
  pulse in some Delta (creativity)
  and then reverse the above to come up refreshed
***************************************************/
struct brainwaveElement {
  char bwType;  // 'a' for Alpha, 'b' for Beta, 't' for Theta,'d' for Delta or 'g' for gamma ('0' signifies last entry in table
  // A, B, T, D and G offer alternating flash instead of concurrent flash.
  unsigned long int bwDuration;  // Duration of this Brainwave Type (divide by 10,000 to get seconds)
} const brainwaveTab[] PROGMEM = {
  { 'b', 600000 },
  { 'a', 100000 },
  { 'b', 200000 },
  { 'a', 150000 },
  { 'b', 150000 },
  { 'a', 200000 },
  { 'b', 100000 },
  { 'a', 300000 },
  { 'b', 50000 },
  { 'a', 600000 },
  { 't', 100000 },
  { 'A', 300000 },
  { 't', 200000 },
  { 'a', 200000 },
  { 't', 300000 },
  { 'A', 150000 },
  { 't', 600000 },
  { 'a', 100000 },
  { 'b', 10000 },
  { 'a', 50000 },
  { 'T', 550000 },
  { 'd', 10000 },
  { 't', 450000 },
  { 'd', 50000 },
  { 'T', 350000 },
  { 'd', 100000 },
  { 't', 250000 },
  { 'd', 150000 },
  { 'g', 10000 },
  { 'T', 50000 },
  { 'g', 10000 },
  { 'd', 300000 },
  { 'g', 50000 },
  { 'd', 600000 },
  { 'g', 100000 },
  { 'D', 300000 },
  { 'g', 50000 },
  { 'd', 150000 },
  { 'g', 10000 },
  { 't', 100000 },
  { 'D', 100000 },
  { 't', 200000 },
  { 'a', 10000 },
  { 'd', 100000 },
  { 't', 300000 },
  { 'a', 50000 },
  { 'B', 10000 },
  { 'a', 100000 },
  { 't', 220000 },
  { 'A', 150000 },
  { 'b', 10000 },
  { 'a', 300000 },
  { 'b', 50000 },
  { 'a', 200000 },
  { 'B', 120000 },
  { 'a', 150000 },
  { 'b', 200000 },
  { 'a', 100000 },
  { 'b', 250000 },
  { 'A', 50000 },
  { 'b', 600000 },
  { '0', 0 }
};


/***************************************************
  VARIABLES for tone generator
  The difference in Hz between two close tones can
  cause a 'beat'. The brain recognizes a pulse
  between the right ear and the left ear due
  to the difference between the two tones.
  Instead of assuming that one ear will always
  have a specific tone, we assume a central tone
  and create tones on the fly half the beat up
  and down from the central tone.
  If we set a central tone of 200, we can expect
  the following tones to be generated:
  Hz:      R Ear    L Ear    Beat
  Beta:    192.80   207.20   14.4
  Alpha:   194.45   205.55   11.1
  Theta:   197.00   203.00    6.0
  Delta:   198.90   201.10    2.2

  You can use any central tone you like. I think a
  lower tone between 100 and 220 is easier on the ears
  than higher tones for a meditation or relaxation.
  Others prefer something around 440 (A above Middle C).
  Isolating the central tone makes it easy for
  the user to choose a preferred frequency base.

***************************************************/
float binauralBeat[] = { 14.4, 11.1, 6.0, 2.2, 40.4 }; // For beta, alpha, gamma and delta beat differences.
Tone rightEar;
Tone leftEar;
float centralTone = 440.0; //We're starting at this tone and spreading the binaural beat from there.

//Blink statuses for function 'blink_LEDs' and 'alt_blink_LEDS
unsigned long int duration = 0;
unsigned long int onTime = 0;
unsigned long int offTime = 0;

#define STATE_READY 0
#define STATE_RUNNING 1
#define STATE_SLEEPING 2
volatile int machineState = STATE_READY;


/***************************************************
  Button interrupt function
***************************************************/
void buttonInterrupt()
{
  switch (machineState) {
    case STATE_READY:
      machineState = STATE_RUNNING;
      break;
    case STATE_RUNNING:
    case STATE_SLEEPING:
      machineState = STATE_READY; // Back to the normal ready/running cycle
      break;
  }
}

/***************************************************
  SETUP defines pins and tones.
  Arduino pins we'll use:
  pin  2 - push button (interrupt)
  pin  5 - right ear
  pin  6 - left ear
  pin  9 - Left eye LED1
  pin 10 - Right eye LED2
  pin 11 - Button input
  pin 5V - for common anode on LED's
  pin GND - ground for tones
*/
void setup()  {
#ifdef DEBUG
  Serial.begin(9600);
#endif
  rightEar.begin(rightEarLow); // Tone rightEar begins at pin output rightEarLow
  leftEar.begin(lefttEarLow); // Tone leftEar begins at pin output leftEarLow
  pinMode(rightEyeRed, OUTPUT); // Pin output at rightEyeRed
  pinMode(leftEyeRed, OUTPUT); // Pin output at leftEyeRed
  pinMode(interruptPin, INPUT_PULLUP); // Pin input at interruptPin
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  attachInterrupt(digitalPinToInterrupt(interruptPin), buttonInterrupt, FALLING);
}


/***************************************************
   MAIN LOOP - tells our program what to do.
***************************************************/

void loop() {
#ifdef DEBUG
  Serial.println("Waiting for button...");
#endif
  analogWrite(rightEyeRed, LED_ON);
  analogWrite(leftEyeRed, LED_ON);
  while (machineState == STATE_READY) {
    delay(50); // here we can read session selection POT once we have one
  }

  int j = 0;
  while (pgm_read_byte(&brainwaveTab[j].bwType) != '0') {  // '0' signifies end of table
#ifdef DEBUG
    Serial.print(j);
    Serial.print(' ');
    Serial.println(millis());
    // uncomment if debugging sleep mode and in a hurry ;)
    // if (j > 2) {
    //   break;  // when debugging, we don't need to do entire session
    // }
      */
#endif
    if (do_brainwave_element(j)) {
      // interrupt button got us out of STATE_RUNNING
      break;
    }
    j++;
  }
#ifdef DEBUG
  Serial.print("Done ");
  Serial.println(millis());
#endif
  // Shut down everything and put the CPU to sleep
  analogWrite(rightEyeRed, LED_OFF);
  analogWrite(leftEyeRed, LED_OFF);
  rightEar.stop();
  leftEar.stop();
  if (machineState != STATE_READY) {
    // Session finished (we're not here due to an interrupt button push)
    machineState = STATE_SLEEPING;
#ifdef DEBUG
    Serial.println("Sleeping...");
    delay(1000); // let the dust settle...
#endif
    sleep_enable();          // enables the sleep bit in the mcucr register so sleep is possible. just a safety pin
    sleep_mode();            // here the device is actually put to sleep
    // THE PROGRAM CONTINUES FROM HERE AFTER WAKING UP
    sleep_disable();            // first thing after waking from sleep: disable sleep...
    delay(1000); // let the dust settle...
#ifdef DEBUG
    Serial.println("Woke up.");
#endif
  }
}

/***************************************************
  This function delays the specified number of 1/10 milliseconds
***************************************************/

bool delay_decimiliseconds(unsigned long int dms) {
  // returns true if interrupt button got us out of STATE_RUNNING
  // https://www.arduino.cc/reference/en/language/functions/time/delaymicroseconds/ says
  // "Currently, the largest value that will produce an accurate delay is 16383"
  // so we do each decimilisecond as a separate call
  for (int i = 0; i < dms; i++) {
    delayMicroseconds(100); //
    if (machineState != STATE_RUNNING) {
      return true; // there was an interrupt
    }
  }
  return false;
}

/***************************************************
  This function blinks the LEDs
  (connected to Pin 6, Pin 5 -
  for Left eye, Right eye, respectively)
  and keeps them blinking for the Duration specified
  (Duration given in 1/10 millisecs).
  This function also acts as a delay for the Duration specified.
  In this particular instance, digitalWrites are set
  for common anode, so "on" = LOW and "off" = HIGH.
***************************************************/

bool blink_LEDs( unsigned long int duration, unsigned long int onTime, unsigned long int offTime) {
  // returns true if interrupt button got us out of STATE_RUNNING
  for (int i = 0; i < (duration / (onTime + offTime)); i++) {
    analogWrite(rightEyeRed, LED_ON);
    analogWrite(leftEyeRed, LED_ON);
    // turn on LEDs
    if (delay_decimiliseconds(onTime)) {  //   for onTime
      return true;
    }
    analogWrite(rightEyeRed, LED_OFF);
    analogWrite(leftEyeRed, LED_OFF);
    // turn off LEDs
    if (delay_decimiliseconds(offTime)) { //   for offTime
      return true;
    }
  }
  return false;
}

bool alt_blink_LEDs( unsigned long int duration, unsigned long int onTime, unsigned long int offTime) {
  // returns true if interrupt button got us out of STATE_RUNNING
  for (int i = 0; i < (duration / (onTime + offTime)); i++) {
    analogWrite(rightEyeRed, LED_ON);
    analogWrite(leftEyeRed, LED_OFF);
    if (delay_decimiliseconds(onTime)) {  //   for onTime
      return true;
    }
    analogWrite(rightEyeRed, LED_OFF);
    analogWrite(leftEyeRed, LED_ON);
    if (delay_decimiliseconds(offTime)) { //   for offTime
      return true;
    }
  }
  return false;
}
/***************************************************
  This function starts with a central audio frequency and
  splits the difference between two tones
  to create a binaural beat (between Left and Right ears)
  for a Brainwave Element.
  (See notes above for beat creation method.)
***************************************************/

bool do_brainwave_element(int index) {
  // returns whatever bilnk_LEDs or alt_blink_LEDs returned,
  // i.e. true if interrupt button got us out of STATE_RUNNING
  char brainChr = pgm_read_byte(&brainwaveTab[index].bwType);

  switch (brainChr) {
    case 'b':
      // Beta
      rightEar.play(centralTone - (binauralBeat[0] / 2));
      leftEar.play(centralTone + (binauralBeat[0] / 2));
      //  Generate binaural beat of 14.4Hz
      //  delay for the time specified in the table while blinking the LEDs at the correct rate
      return blink_LEDs( pgm_read_dword(&brainwaveTab[index].bwDuration), 347, 347 );

    case 'B':
      // Beta - with alternating blinks
      rightEar.play(centralTone - (binauralBeat[0] / 2));
      leftEar.play(centralTone + (binauralBeat[0] / 2));
      //  Generate binaural beat of 14.4Hz
      //  delay for the time specified in the table while blinking the LEDs at the correct rate
      return alt_blink_LEDs( pgm_read_dword(&brainwaveTab[index].bwDuration), 347, 347 );

    case 'a':
      // Alpha
      rightEar.play(centralTone - (binauralBeat[1] / 2));
      leftEar.play(centralTone + (binauralBeat[1] / 2));
      // Generates a binaural beat of 11.1Hz
      // delay for the time specified in the table while blinking the LEDs at the correct rate
      return blink_LEDs( pgm_read_dword(&brainwaveTab[index].bwDuration), 451, 450 );

    case 'A':
      // Alpha
      rightEar.play(centralTone - (binauralBeat[1] / 2));
      leftEar.play(centralTone + (binauralBeat[1] / 2));
      // Generates a binaural beat of 11.1Hz
      // delay for the time specified in the table while blinking the LEDs at the correct rate
      return alt_blink_LEDs( pgm_read_dword(&brainwaveTab[index].bwDuration), 451, 450 );

    case 't':
      // Theta
      // start Timer 1 with the correct Offset Frequency for a binaural beat for the Brainwave Type
      //   to Right ear speaker through output OC1A (PB3, pin 15)
      rightEar.play(centralTone - (binauralBeat[2] / 2));
      leftEar.play(centralTone + (binauralBeat[2] / 2));
      // Generates a binaural beat of 6.0Hz
      // delay for the time specified in the table while blinking the LEDs at the correct rate
      return blink_LEDs( pgm_read_dword(&brainwaveTab[index].bwDuration), 835, 835 );

    case 'T':
      // Theta
      // start Timer 1 with the correct Offset Frequency for a binaural beat for the Brainwave Type
      //   to Right ear speaker through output OC1A (PB3, pin 15)
      rightEar.play(centralTone - (binauralBeat[2] / 2));
      leftEar.play(centralTone + (binauralBeat[2] / 2));
      // Generates a binaural beat of 6.0Hz
      // delay for the time specified in the table while blinking the LEDs at the correct rate
      return alt_blink_LEDs( pgm_read_dword(&brainwaveTab[index].bwDuration), 835, 835 );

    case 'd':
      // Delta
      rightEar.play(centralTone - (binauralBeat[3] / 2));
      leftEar.play(centralTone + (binauralBeat[3] / 2));
      // Generates a binaural beat of 2.2Hz
      // delay for the time specified in the table while blinking the LEDs at the correct rate
      return blink_LEDs( pgm_read_dword(&brainwaveTab[index].bwDuration), 2253, 2253 );

    case 'D':
      // Delta
      rightEar.play(centralTone - (binauralBeat[3] / 2));
      leftEar.play(centralTone + (binauralBeat[3] / 2));
      // Generates a binaural beat of 2.2Hz
      // delay for the time specified in the table while blinking the LEDs at the correct rate
      return alt_blink_LEDs( pgm_read_dword(&brainwaveTab[index].bwDuration), 2253, 2253 );

    case 'g':
      // Gamma
      rightEar.play(centralTone - (binauralBeat[4] / 2));
      leftEar.play(centralTone + (binauralBeat[4] / 2));
      // Generates a binaural beat of 40.4Hz
      // delay for the time specified in the table while blinking the LEDs at the correct rate
      return blink_LEDs( pgm_read_dword(&brainwaveTab[index].bwDuration), 124, 124 );

    case 'G':
      // Gamma
      rightEar.play(centralTone - (binauralBeat[4] / 2));
      leftEar.play(centralTone + (binauralBeat[4] / 2));
      // Generates a binaural beat of 40.4Hz
      // delay for the time specified in the table while blinking the LEDs at the correct rate
      return alt_blink_LEDs( pgm_read_dword(&brainwaveTab[index].bwDuration), 124, 124 );

    // this should never be executed, since we catch the end of table in the main loop
    default:
      return true;      // end of table
  }
}


