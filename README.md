# MIDIfy — DIY MIDI DJ Controller

A custom-built MIDI DJ controller using an Arduino Uno and repurposed hard disk drive (HDD) components. Physical inputs — buttons, potentiometers, and a scratch mechanism — are converted into MIDI signals that control a DAW in real time.

Built as a group academic project during B.Sc. Electronics at Christ University.

---

## What it does

- 3 push buttons send **MIDI Note ON/OFF** messages — mapped to cue points or pads in Ableton Live
- 2 potentiometers/sliders send **MIDI Control Change (CC)** messages — mapped to volume, pitch, or effects
- An HDD platter mechanism simulates **vinyl scratching** using repurposed disk components
- All inputs are debounced and noise-filtered for stable, reliable MIDI output

---

## Hardware used

| Component | Purpose |
|---|---|
| Arduino Uno (ATmega328) | Microcontroller — reads inputs and sends MIDI |
| 3x Push buttons | Trigger MIDI notes (cue points, pads) |
| 2x Potentiometers / sliders | Send MIDI CC for faders and knobs |
| HDD platter + arm | Scratch mechanism — simulates vinyl |
| Resistors | Pull-up/pull-down for button circuits |

---

## Software & Libraries

- [Arduino IDE](https://www.arduino.cc/en/software)
- [Hairless MIDI](https://projectgus.github.io/hairless-midiserial/) — bridges Arduino serial to MIDI
- [loopMIDI](https://www.tobias-erichsen.de/software/loopmidi.html) — virtual MIDI port on Windows
- [Ableton Live 11](https://www.ableton.com/) — DAW used for mapping and performance
- [MIDI.h](https://github.com/FortySevenEffects/arduino_midi_library) — Arduino MIDI library
- [ResponsiveAnalogRead](https://github.com/dxinteractive/ResponsiveAnalogRead) — smooths potentiometer noise

---

## How it works

```
Physical input
      │
      ▼
Arduino reads pin (digital or analog)
      │
      ├── Button pressed?  →  Send MIDI Note ON  (velocity 127)
      │   Button released? →  Send MIDI Note OFF (velocity 0)
      │
      └── Pot moved?       →  Map raw value (0–1023) to MIDI (0–127)
                               Send MIDI Control Change (CC)
      │
      ▼
Serial → Hairless MIDI → loopMIDI → Ableton Live
```

Potentiometers are smoothed using `ResponsiveAnalogRead` to eliminate jitter. A movement threshold (`varThreshold = 20`) and a timeout window (`TIMEOUT = 300ms`) ensure CC messages are only sent when the pot is genuinely being moved.

---

## Pin mapping

| Pin | Component | MIDI message |
|---|---|---|
| D2 | Button 1 | Note 36 (C2) ON/OFF |
| D3 | Button 2 | Note 37 (C#2) ON/OFF |
| D4 | Button 3 | Note 38 (D2) ON/OFF |
| A0 | Potentiometer 1 | CC 1 (0–127) |
| A1 | Potentiometer 2 | CC 2 (0–127) |

---

## How to set up and run

1. Install the Arduino IDE and required libraries (MIDI.h, ResponsiveAnalogRead)
2. Open `midify.ino` and set `#define ATMEGA328` at the top to target Arduino Uno
3. Upload the sketch to your Arduino Uno
4. Open Hairless MIDI — select your Arduino's COM port as the serial input
5. Open loopMIDI — create a virtual MIDI port
6. In Hairless MIDI, route output to the loopMIDI port
7. Open Ableton Live → go to Preferences → MIDI → enable the loopMIDI port
8. Use MIDI Map mode in Ableton to assign buttons and pots to controls
9. Play

---

## To customise

- Change `note = 36` to shift which MIDI notes the buttons trigger
- Change `cc = 1` to shift which CC numbers the pots control
- Adjust `debounceDelay` if buttons feel unresponsive or double-trigger
- Adjust `varThreshold` and `snapMultiplier` to tune pot sensitivity

---

## Project context

Built as part of a B.Sc. Electronics group project at Christ University (2023–2024). The goal was to demonstrate hardware-software integration — combining circuit design, microcontroller programming, and real-world MIDI protocol to build a functional musical instrument.

---

## Skills demonstrated

`Arduino` `C++` `Embedded Systems` `MIDI Protocol` `Analog Signal Processing` `Hardware-Software Integration` `Circuit Design` `Ableton Live`
