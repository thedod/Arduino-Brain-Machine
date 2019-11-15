
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
  used a table (chunkybrainwaveTab) that is was too large to transfer into RAM in the original microcontroller,
  we're taking the same approach.  This is accomplished by using the library for PROGMEM.
  (This is used below in the definition for the chunkybrainwaveTab).  Since the
  C compiler assumes that constants are in RAM, rather than in program memory, when accessing
  the chunkybrainwaveTab, we need to use the pgm_read_byte() and pgm_read_dword() macros, and we need
  to use the brainwveTab as an address, i.e., precede it with "&".  For example, to access
  chunkybrainwaveTab[3].bwType, which is a byte, this is how to do it:
   pgm_read_byte( &chunkybrainwaveTab[3].bwType );
  And to access chunkybrainwaveTab[3].bwDuration, which is a double-word, this is how to do it:
   pgm_read_dword( &chunkybrainwaveTab[3].bwDuration );
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

// =============== uncomment for debug options ===============
// --- Serial debugging
// #define DEBUG
// --- Do a short session (usefull when debugging sleep mode)
// #define SHORT_SESSION
// =========================================================================

#define rightEarPin 9 // Define pinout for left ear
#define leftEarPin 10 // Define pinout for left ear
#define rightEyePin 5 // Define pinout for right eye
#define leftEyePin 6 // Define pinout for left eye
#define interruptPin 2 // the input pin where the pushbutton is connected.
#define potPin A0 // user input potentiometer (session selection)

// Common anode. 255 is off
int LEDIntensity = 127;
#define LED_ON (255-LEDIntensity)
#define LED_OFF 255

// ====== Altman's "chunky" frequency-hopping method =======

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
struct chunkyBrainwaveElement {
  char bwType;  // 'a' for Alpha, 'b' for Beta, 't' for Theta,'d' for Delta or 'g' for gamma ('0' signifies last entry in table
  // A, B, T, D and G offer alternating flash instead of concurrent flash.
  unsigned long int bwDuration;  // Duration of this Brainwave Type (divide by 10,000 to get seconds)
};

const chunkyBrainwaveElement chunkybrainwaveTab[] PROGMEM = {
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
Tone rightEarTone;
Tone leftEarTone;
float centralTone = 440.0; //We're starting at this tone and spreading the binaural beat from there.

//Blink statuses for function 'blink_LEDs' and 'alt_blink_LEDS
unsigned long int duration = 0;
unsigned long int onTime = 0;
unsigned long int offTime = 0;


// ====== Mindplace.com "creamy" frequency-transition method =======

struct creamyBrainwaveElement {
  int duration;  // Seconds
  float frequency; // Hz
};

const creamyBrainwaveElement proteusGoodMorning04[] = {
  {0, 8}, {120, 16}, {60, 25}, {60, 25}, {60, 20}, {60, 28}, {60, 20}, {60, 28},
  {60, 20}, {60, 28}, {60, 20}, {60, 28}, {60, 20}, {60, 8}, {60, 20}, {0, 0}
};

const creamyBrainwaveElement proteusGoodNight43[] = {
  {0, 9}, {280, 6}, {300, 3}, {200, 5}, {100, 3}, {20, 2}, {0, 0}
};

const creamyBrainwaveElement proteusVisuals22[] = {
  {0, 8}, {60, 10}, {60, 16}, {60, 20}, {0, 16}, {60, 24}, {0, 20}, {120, 28}, {60, 24},
  {60, 30}, {300, 24}, {30, 12}, {30, 24}, {30, 16}, {120, 30}, {150, 8}, {60, 8}, {0, 0}
};

const creamyBrainwaveElement proteusMeditation10[] = {
  {0, 16}, {60, 16}, {240, 4}, {360, 4}, {120, 2}, {1080, 2}, {60, 8}, {60, 24},
  {120, 24}, {0, 0}
};



/***************************************************
  Session selection potentiometer
***************************************************/

#define NUM_SESSIONS 6
#define SESSION_GOOD_MORNING 0
#define SESSION_GOOD_NIGHT 1
#define SESSION_VISUALS 2
#define SESSION_MEDITATION 3
#define SESSION_MEDITATION_CHUNKY 4
#define SESSION_SETUP 5

int currentSession;

#define NUM_PROTEUS_SESSIONS 4

const creamyBrainwaveElement *proteusSessions[NUM_PROTEUS_SESSIONS] = {
  proteusGoodMorning04, proteusGoodNight43,
  proteusVisuals22, proteusMeditation10
};

int blink_patterns[NUM_SESSIONS] = {
  0B100000000000, // .     SESSION_GOOD_MORNING
  0B100100000000, // ..    SESSION_GOOD_NIGHT
  0B100100100000, // ...   SESSION_VISUALS
  0B111000000000, // -     SESSION_MEDITATION
  0B111001110000, // --    SESSION_MEDITATION_CHUNKY
  0B101010101010, // blink SESSION_SETUP
};

#define BLINK_STATE(n) (((blink_patterns[n]>>n)&1)?HIGH:LOW)

#define BLINK_SEGMENT_DURATION 100 // milliseconds
#define BLINK_NUM_SEGMENTS 12
#define CURRENT_BLINK_SEGMENT ((millis()/BLINK_SEGMENT_DURATION)%BLINK_NUM_SEGMENTS)

int mapPot(int mapMin, int mapMax) {
  // trim a bit of the edge values, because POTs are lousy and might not get there
  return constrain(
           map(analogRead(potPin), 23, 1000, mapMin, mapMax),
           mapMin, mapMax);
}

void setLEDs(int state) {
  analogWrite(rightEyePin, state);
  analogWrite(leftEyePin, state);
}

void blinkSessionSelection(int session) {
  setLEDs(blink_patterns[session] >> CURRENT_BLINK_SEGMENT & 1 ? LED_ON : LED_OFF);
}

/***************************************************
  Button interrupt
***************************************************/
#define STATE_READY 0
#define STATE_RUNNING 1
#define STATE_SLEEPING 2
volatile int machineState = STATE_READY;

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
  pin A0 - session selector potentiometer
  pin 5V - for common anode on LED's
  pin GND - ground for tones
*/
void setup()  {
#ifdef DEBUG
  Serial.begin(9600);
#endif
  rightEarTone.begin(rightEarPin); // Tone rightEarTone begins at pin output rightEarPin
  leftEarTone.begin(leftEarPin); // Tone leftEarTone begins at pin output leftEarPin
  pinMode(rightEyePin, OUTPUT); // Pin output at rightEye
  pinMode(leftEyePin, OUTPUT); // Pin output at leftEye
  pinMode(interruptPin, INPUT_PULLUP); // User input (push button)
  pinMode(potPin, INPUT); // User input (potentiometer)
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  attachInterrupt(digitalPinToInterrupt(interruptPin), buttonInterrupt, FALLING);
}


/***************************************************
   MAIN LOOP - tells our program what to do.
***************************************************/

void loop() {
#ifdef DEBUG
  Serial.println("Waiting for session selection...");
#endif
  while (machineState == STATE_READY) {
    currentSession = mapPot(0, NUM_SESSIONS - 1);
    blinkSessionSelection(currentSession);
    delay(50);
  }
  setLEDs(LED_OFF);

#ifdef DEBUG
  Serial.print("Chose session #");
  Serial.println(currentSession);
#endif
  unsigned long startedAt = millis();
  int j = 0;
  switch (currentSession) {
    case SESSION_SETUP:
      while (machineState == STATE_RUNNING) {
        LEDIntensity = mapPot(31, 255);
        setLEDs(LED_ON);
        delay(50);
      }
#ifdef DEBUG
      Serial.print("Set intensity (0-255) to ");
      Serial.println(LEDIntensity);
#endif
      break;
    case SESSION_GOOD_MORNING:
    case SESSION_GOOD_NIGHT:
    case SESSION_VISUALS:
    case SESSION_MEDITATION:
      creamyBrainwaveElement *session = proteusSessions[currentSession];
      float currentFrequency = 0.0;
      while (session[j].frequency > 0.0) { // 0.0 signifies end of table
#ifdef DEBUG
        Serial.print(j);
        Serial.print(' ');
        Serial.print(currentFrequency);
        Serial.print(" -> ");
        Serial.print(session[j].frequency);
        Serial.print(" in ");
        Serial.print(session[j].duration);
        Serial.print(' ');
        Serial.println(millis() - startedAt);
#endif
#ifdef SHORT_SESSION
        if (j > 2) {
          break;  // when debugging, we don't need to do entire session
        }
#endif
        if (session[j].duration) {
          int steps = session[j].duration * 10; // we split the element to 0.1s long steps
#ifdef DEBUG
          Serial.print(steps);
          Serial.println(" steps");
#endif
          for (int t = 0; t < steps; t++) {
            float freq = currentFrequency + (session[j].frequency - currentFrequency) * (float(t) / steps);
            rightEarTone.play(centralTone - (freq / 2.0));
            leftEarTone.play(centralTone + (freq / 2.0));
            unsigned long halfWaveLength = round(5000.0 / freq);
            if (blink_LEDs(1000, halfWaveLength, halfWaveLength)) { // 1000 decimilisecs = 0.1s
            };
          };
        };
        if (machineState != STATE_RUNNING) {
          break; // interrupt button was pressed
        };
        currentFrequency = session[j].frequency;
        j++;
      };
      break;
    case SESSION_MEDITATION_CHUNKY:
      while (pgm_read_byte(&chunkybrainwaveTab[j].bwType) != '0') {  // '0' signifies end of table
#ifdef DEBUG
        Serial.print(j);
        Serial.print(' ');
        Serial.print(pgm_read_byte(&chunkybrainwaveTab[j].bwType));
        Serial.print(' ');
        Serial.println(millis() - startedAt);
#endif
#ifdef SHORT_SESSION
        if (j > 2) {
          break;  // when debugging, we don't need to do entire session
        }
#endif
        if (do_chunky_brainwave_element(j)) {
          // interrupt button got us out of STATE_RUNNING
          break;
        }
        j++;
      }
      break;
  }
#ifdef DEBUG
  Serial.print("Done #");
  Serial.print(currentSession);
  Serial.print(' ');
  Serial.println(millis());
#endif
  if (machineState != STATE_READY) {
    // Session finished (we're not here due to an interrupt button push)
    // Shut down everything and put the CPU to sleep
    machineState = STATE_SLEEPING;
    setLEDs(LED_OFF);
    rightEarTone.stop();
    leftEarTone.stop();
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
    analogWrite(rightEyePin, LED_ON);
    analogWrite(leftEyePin, LED_ON);
    // turn on LEDs
    if (delay_decimiliseconds(onTime)) {  //   for onTime
      return true;
    }
    analogWrite(rightEyePin, LED_OFF);
    analogWrite(leftEyePin, LED_OFF);
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
    analogWrite(rightEyePin, LED_ON);
    analogWrite(leftEyePin, LED_OFF);
    if (delay_decimiliseconds(onTime)) {  //   for onTime
      return true;
    }
    analogWrite(rightEyePin, LED_OFF);
    analogWrite(leftEyePin, LED_ON);
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

bool do_chunky_brainwave_element(int index) {
  // returns whatever bilnk_LEDs or alt_blink_LEDs returned,
  // i.e. true if interrupt button got us out of STATE_RUNNING
  char brainChr = pgm_read_byte(&chunkybrainwaveTab[index].bwType);

  switch (brainChr) {
    case 'b':
      // Beta
      rightEarTone.play(centralTone - (binauralBeat[0] / 2));
      leftEarTone.play(centralTone + (binauralBeat[0] / 2));
      //  Generate binaural beat of 14.4Hz
      //  delay for the time specified in the table while blinking the LEDs at the correct rate
      return blink_LEDs( pgm_read_dword(&chunkybrainwaveTab[index].bwDuration), 347, 347 );

    case 'B':
      // Beta - with alternating blinks
      rightEarTone.play(centralTone - (binauralBeat[0] / 2));
      leftEarTone.play(centralTone + (binauralBeat[0] / 2));
      //  Generate binaural beat of 14.4Hz
      //  delay for the time specified in the table while blinking the LEDs at the correct rate
      return alt_blink_LEDs( pgm_read_dword(&chunkybrainwaveTab[index].bwDuration), 347, 347 );

    case 'a':
      // Alpha
      rightEarTone.play(centralTone - (binauralBeat[1] / 2));
      leftEarTone.play(centralTone + (binauralBeat[1] / 2));
      // Generates a binaural beat of 11.1Hz
      // delay for the time specified in the table while blinking the LEDs at the correct rate
      return blink_LEDs( pgm_read_dword(&chunkybrainwaveTab[index].bwDuration), 451, 450 );

    case 'A':
      // Alpha
      rightEarTone.play(centralTone - (binauralBeat[1] / 2));
      leftEarTone.play(centralTone + (binauralBeat[1] / 2));
      // Generates a binaural beat of 11.1Hz
      // delay for the time specified in the table while blinking the LEDs at the correct rate
      return alt_blink_LEDs( pgm_read_dword(&chunkybrainwaveTab[index].bwDuration), 451, 450 );

    case 't':
      // Theta
      // start Timer 1 with the correct Offset Frequency for a binaural beat for the Brainwave Type
      //   to Right ear speaker through output OC1A (PB3, pin 15)
      rightEarTone.play(centralTone - (binauralBeat[2] / 2));
      leftEarTone.play(centralTone + (binauralBeat[2] / 2));
      // Generates a binaural beat of 6.0Hz
      // delay for the time specified in the table while blinking the LEDs at the correct rate
      return blink_LEDs( pgm_read_dword(&chunkybrainwaveTab[index].bwDuration), 835, 835 );

    case 'T':
      // Theta
      // start Timer 1 with the correct Offset Frequency for a binaural beat for the Brainwave Type
      //   to Right ear speaker through output OC1A (PB3, pin 15)
      rightEarTone.play(centralTone - (binauralBeat[2] / 2));
      leftEarTone.play(centralTone + (binauralBeat[2] / 2));
      // Generates a binaural beat of 6.0Hz
      // delay for the time specified in the table while blinking the LEDs at the correct rate
      return alt_blink_LEDs( pgm_read_dword(&chunkybrainwaveTab[index].bwDuration), 835, 835 );

    case 'd':
      // Delta
      rightEarTone.play(centralTone - (binauralBeat[3] / 2));
      leftEarTone.play(centralTone + (binauralBeat[3] / 2));
      // Generates a binaural beat of 2.2Hz
      // delay for the time specified in the table while blinking the LEDs at the correct rate
      return blink_LEDs( pgm_read_dword(&chunkybrainwaveTab[index].bwDuration), 2253, 2253 );

    case 'D':
      // Delta
      rightEarTone.play(centralTone - (binauralBeat[3] / 2));
      leftEarTone.play(centralTone + (binauralBeat[3] / 2));
      // Generates a binaural beat of 2.2Hz
      // delay for the time specified in the table while blinking the LEDs at the correct rate
      return alt_blink_LEDs( pgm_read_dword(&chunkybrainwaveTab[index].bwDuration), 2253, 2253 );

    case 'g':
      // Gamma
      rightEarTone.play(centralTone - (binauralBeat[4] / 2));
      leftEarTone.play(centralTone + (binauralBeat[4] / 2));
      // Generates a binaural beat of 40.4Hz
      // delay for the time specified in the table while blinking the LEDs at the correct rate
      return blink_LEDs( pgm_read_dword(&chunkybrainwaveTab[index].bwDuration), 124, 124 );

    case 'G':
      // Gamma
      rightEarTone.play(centralTone - (binauralBeat[4] / 2));
      leftEarTone.play(centralTone + (binauralBeat[4] / 2));
      // Generates a binaural beat of 40.4Hz
      // delay for the time specified in the table while blinking the LEDs at the correct rate
      return alt_blink_LEDs( pgm_read_dword(&chunkybrainwaveTab[index].bwDuration), 124, 124 );

    // this should never be executed, since we catch the end of table in the main loop
    default:
      return true;      // end of table
  }
}


