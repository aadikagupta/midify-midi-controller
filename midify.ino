/*
 * MIDIfy — DIY MIDI DJ Controller
 * ================================
 * Converts physical hardware inputs (buttons and potentiometers)
 * into MIDI signals that can control any DAW (e.g. Ableton Live).
 *
 * Hardware used:
 *   - Arduino Uno (ATmega328)
 *   - 3 push buttons (connected to digital pins 2, 3, 4)
 *   - 2 potentiometers / sliders (connected to analog pins A0, A1)
 *   - Repurposed HDD components for the scratch mechanism
 *
 * MIDI Output:
 *   - Buttons send Note ON / Note OFF messages
 *   - Potentiometers send Control Change (CC) messages
 *
 * Library dependencies:
 *   - MIDI.h          (for ATmega328 serial MIDI)
 *   - MIDIUSB         (for ATmega32U4 USB MIDI)
 *   - ResponsiveAnalogRead (smooths out potentiometer noise)
 *
 * Board target: Arduino Uno (ATmega328) with Hairless MIDI bridge
 * Baud rate: 115200
 */

// ─────────────────────────────────────────────────────────────
// BUTTONS
// ─────────────────────────────────────────────────────────────

const int N_BUTTONS = 3;                                 // Total number of buttons
const int BUTTON_ARDUINO_PIN[N_BUTTONS] = { 2, 3, 4 };  // Digital pins each button is wired to

int buttonCState[N_BUTTONS] = {};  // Current state of each button (HIGH or LOW)
int buttonPState[N_BUTTONS] = {};  // Previous state — used to detect a state change

// Pin 13 has a built-in LED and uses a pull-down resistor (unlike others which use pull-up)
// Uncomment the line below if you are using pin 13:
// #define pin13 1
byte pin13index = 12;  // Index position of pin 13 in the BUTTON_ARDUINO_PIN array (if used)

// Debounce: prevents false triggers from button bounce noise
unsigned long lastDebounceTime[N_BUTTONS] = { 0 };  // Timestamp of the last detected state change
unsigned long debounceDelay = 50;                    // Minimum time (ms) between valid button reads

#endif  // end USING_BUTTONS


// ─────────────────────────────────────────────────────────────
// POTENTIOMETERS
// ─────────────────────────────────────────────────────────────

#ifdef USING_POTENTIOMETERS

const int N_POTS = 2;                             // Total number of potentiometers (rotary or slide)
const int POT_ARDUINO_PIN[N_POTS] = { A0, A1 };  // Analog pins each potentiometer is wired to

int potCState[N_POTS] = { 0 };  // Current raw analog reading of each pot
int potPState[N_POTS] = { 0 };  // Previous raw reading — used to calculate movement
int potVar = 0;                  // Absolute difference between current and previous reading

int midiCState[N_POTS] = { 0 };  // Current MIDI value (0–127) mapped from pot reading
int midiPState[N_POTS] = { 0 };  // Previous MIDI value — only send CC if this has changed

// Timeout logic: keeps reading the pot for a set time after it stops moving
// This avoids sending stale values if the pot settles slowly
const int TIMEOUT = 300;              // How long (ms) to keep reading after the pot goes still
const int varThreshold = 20;          // Minimum change in raw reading to be treated as intentional movement
boolean potMoving = true;             // Flag: is the pot currently moving?
unsigned long PTime[N_POTS] = { 0 }; // Timestamp of the last detected pot movement
unsigned long timer[N_POTS] = { 0 }; // Elapsed time since last movement — compared against TIMEOUT

int reading = 0;  // Raw analog reading before responsive smoothing

// ResponsiveAnalogRead smooths noisy potentiometer signals
// snapMultiplier: lower = smoother but slower response; higher = faster but noisier
float snapMultiplier = 0.01;
ResponsiveAnalogRead responsivePot[N_POTS] = {};  // Array of smoothed pot readers (filled in setup)

// Calibration range of the physical potentiometer
// Adjust potMin/potMax if your pot doesn't reach full 0–1023 range
int potMin = 10;
int potMax = 1023;

#endif  // end USING_POTENTIOMETERS


// ─────────────────────────────────────────────────────────────
// MIDI CONFIGURATION
// ─────────────────────────────────────────────────────────────

byte midiCh = 0;  // MIDI channel (0 = channel 1 in MIDIUSB; use 1 for MIDI.h library)
byte note = 36;   // Starting MIDI note number for buttons (36 = C2); each button offsets by +i
byte cc = 1;      // Starting MIDI CC number for pots; each pot offsets by +i


// ─────────────────────────────────────────────────────────────
// SETUP — runs once when the Arduino powers on
// ─────────────────────────────────────────────────────────────

void setup() {

  // Serial baud rate — must match your MIDI bridge software (e.g. Hairless MIDI)
  // Use 31250 for hardware MIDI; 115200 for Hairless MIDI over USB
  Serial.begin(115200);

#ifdef DEBUG
  Serial.println("Debug mode");
  Serial.println();
#endif

#ifdef USING_BUTTONS
  // Set each button pin as INPUT with internal pull-up resistor
  // This means the pin reads HIGH when unpressed and LOW when pressed
  for (int i = 0; i < N_BUTTONS; i++) {
    pinMode(BUTTON_ARDUINO_PIN[i], INPUT_PULLUP);
  }

#ifdef pin13
  // Pin 13 uses a pull-down resistor — set as plain INPUT (no pull-up)
  pinMode(BUTTON_ARDUINO_PIN[pin13index], INPUT);
#endif

#endif  // end USING_BUTTONS

#ifdef USING_POTENTIOMETERS
  // Initialise each ResponsiveAnalogRead object with smoothing settings
  for (int i = 0; i < N_POTS; i++) {
    responsivePot[i] = ResponsiveAnalogRead(0, true, snapMultiplier);
    responsivePot[i].setAnalogResolution(1023);  // Arduino Uno ADC range: 0–1023
  }
#endif
}


// ─────────────────────────────────────────────────────────────
// LOOP — runs continuously after setup
// ─────────────────────────────────────────────────────────────

void loop() {

#ifdef USING_BUTTONS
  buttons();        // Check and handle button state changes
#endif

#ifdef USING_POTENTIOMETERS
  potentiometers(); // Check and handle pot movements
#endif
}


// ─────────────────────────────────────────────────────────────
// BUTTONS — reads each button and sends MIDI Note ON/OFF
// ─────────────────────────────────────────────────────────────

#ifdef USING_BUTTONS

void buttons() {

  for (int i = 0; i < N_BUTTONS; i++) {

    buttonCState[i] = digitalRead(BUTTON_ARDUINO_PIN[i]);  // Read current pin state

#ifdef pin13
    // Invert pin 13 reading because it uses pull-down (LOW = unpressed, HIGH = pressed)
    // All other pins use pull-up (HIGH = unpressed, LOW = pressed)
    if (i == pin13index) {
      buttonCState[i] = !buttonCState[i];
    }
#endif

    // Only process if the debounce delay has passed since the last change
    if ((millis() - lastDebounceTime[i]) > debounceDelay) {

      // Detect a state change (press or release)
      if (buttonPState[i] != buttonCState[i]) {
        lastDebounceTime[i] = millis();  // Record the time of this change

        if (buttonCState[i] == LOW) {
          // Button pressed — send Note ON (velocity 127 = full strength)

#ifdef ATMEGA328
          MIDI.sendNoteOn(note + i, 127, midiCh);   // note, velocity, channel
#elif ATMEGA32U4
          noteOn(midiCh, note + i, 127);             // channel, note, velocity
          MidiUSB.flush();
#elif TEENSY
          usbMIDI.sendNoteOn(note + i, 127, midiCh);
#elif DEBUG
          Serial.print(i);
          Serial.println(": button on");
#endif

        } else {
          // Button released — send Note ON with velocity 0 (standard Note OFF equivalent)

#ifdef ATMEGA328
          MIDI.sendNoteOn(note + i, 0, midiCh);
#elif ATMEGA32U4
          noteOn(midiCh, note + i, 0);
          MidiUSB.flush();
#elif TEENSY
          usbMIDI.sendNoteOn(note + i, 0, midiCh);
#elif DEBUG
          Serial.print(i);
          Serial.println(": button off");
#endif
        }

        buttonPState[i] = buttonCState[i];  // Update previous state for next comparison
      }
    }
  }
}

#endif  // end USING_BUTTONS


// ─────────────────────────────────────────────────────────────
// POTENTIOMETERS — reads each pot and sends MIDI CC messages
// ─────────────────────────────────────────────────────────────

#ifdef USING_POTENTIOMETERS

void potentiometers() {

  for (int i = 0; i < N_POTS; i++) {

    // Step 1: Read and smooth the raw analog value
    reading = analogRead(POT_ARDUINO_PIN[i]);
    responsivePot[i].update(reading);
    potCState[i] = responsivePot[i].getValue();  // Smoothed value

    // Step 2: Also take a direct raw reading (used for movement detection)
    potCState[i] = analogRead(POT_ARDUINO_PIN[i]);

    // Step 3: Map the raw reading (potMin–potMax) to MIDI range (0–127)
    midiCState[i] = map(potCState[i], potMin, potMax, 0, 127);

    // Clamp MIDI value to valid range in case of noise at the extremes
    if (midiCState[i] < 0)   { midiCState[i] = 0; }
    if (midiCState[i] > 127) { midiCState[i] = 0; }

    // Step 4: Detect whether the pot is moving
    potVar = abs(potCState[i] - potPState[i]);  // How much has it moved since last loop?

    if (potVar > varThreshold) {  // Movement exceeds noise threshold — pot is being turned
      PTime[i] = millis();        // Record the time of this movement
    }

    timer[i] = millis() - PTime[i];  // How long since the last detected movement?

    // Keep the gate open for TIMEOUT ms after the last movement
    potMoving = (timer[i] < TIMEOUT);

    // Step 5: If pot is moving and MIDI value has changed, send Control Change
    if (potMoving && midiPState[i] != midiCState[i]) {

#ifdef ATMEGA328
      MIDI.sendControlChange(cc + i, midiCState[i], midiCh);  // cc number, value, channel
#elif ATMEGA32U4
      controlChange(midiCh, cc + i, midiCState[i]);           // channel, CC number, value
      MidiUSB.flush();
#elif TEENSY
      usbMIDI.sendControlChange(cc + i, midiCState[i], midiCh);
#elif DEBUG
      Serial.print("Pot: ");
      Serial.print(i);
      Serial.print(" ");
      Serial.println(midiCState[i]);
#endif

      potPState[i] = potCState[i];    // Store current raw reading for next comparison
      midiPState[i] = midiCState[i];  // Store current MIDI value for next comparison
    }
  }
}

#endif  // end USING_POTENTIOMETERS


// ─────────────────────────────────────────────────────────────
// ATmega32U4 MIDI HELPER FUNCTIONS (Micro, Pro Micro, Leonardo)
// ─────────────────────────────────────────────────────────────

#ifdef ATMEGA32U4

// Send a Note ON message over USB MIDI
void noteOn(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = { 0x09, 0x90 | channel, pitch, velocity };
  MidiUSB.sendMIDI(noteOn);
}

// Send a Note OFF message over USB MIDI
void noteOff(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOff = { 0x08, 0x80 | channel, pitch, velocity };
  MidiUSB.sendMIDI(noteOff);
}

// Send a Control Change (CC) message over USB MIDI
void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = { 0x0B, 0xB0 | channel, control, value };
  MidiUSB.sendMIDI(event);
}

#endif  // end ATMEGA32U4

