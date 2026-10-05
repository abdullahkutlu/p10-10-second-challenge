# P10 LED Matrix Reverse Engineering

## Overview

This document describes the reverse-engineering process of the 32×16 single-color P10 LED matrix used in the **P10 10-Second Challenge** project.

The panel did not operate correctly with the standard P10 display library. Instead of replacing the panel, I investigated its hardware, control signals, scan architecture and physical LED mapping.

The final result was a custom Arduino Uno display driver that directly controls the panel without using a standard P10 display library.

---

## Panel Hardware

The display used in this project is a **32×16 single-color red P10 LED matrix**.

The PCB is marked:

`P10-SMD2835-V9-1`

The board contains:

- 32×16 red LEDs
- 1× DP4536 IC
- 8× DP5125 LED driver ICs
- Two HUB-style 16-pin connectors
- 5 V power input
- 1/4 scan architecture

The panel contains a total of:

`32 × 16 = 512 LEDs`

---

## IC Identification

The back of the PCB was inspected to understand the hardware architecture of the module.

Two types of integrated circuits were identified.

### DP4536

The board contains **one DP4536** IC located near the HUB interface section.

The panel receives the following control signals from the Arduino:

- A
- B
- LAT
- OE
- DATA / DR
- CLK

The behavior of these signals was investigated experimentally rather than assuming that the panel followed the behavior expected by a standard P10 library.

### DP5125

The PCB contains **eight DP5125 LED driver ICs**.

Each DP5125 provides 16 outputs.

Therefore:

`8 × 16 = 128 outputs`

This matched the experimental observation that the panel requires **128 serial data bits for every scan state**.

The eight DP5125 ICs form the serial LED driver chain used by the matrix.

---

## Initial Problem

The first attempts to control the panel using a standard P10 display library did not produce a correct image.

At this point, several possible causes had to be investigated:

- Incorrect wiring
- Incorrect HUB connector
- Missing Arduino signals
- Incorrect OE polarity
- Incorrect DATA polarity
- Incorrect scan configuration
- Different internal LED mapping

Instead of continuing to modify library settings blindly, the panel was tested directly at signal level.

---

## Oscilloscope Testing

A **FNIRSI DSO-TC3 oscilloscope** was used to verify that the Arduino Uno was generating the required digital control signals.

The oscilloscope ground was connected to the common ground of the Arduino and the LED panel.

Each signal line was then measured individually.

| Signal | Arduino Uno Pin | Result |
|---|---:|---|
| A | D6 | Working |
| B | D7 | Working |
| LAT | D8 | Working |
| OE | D9 | Working |
| DATA / DR | D11 | Working |
| CLK | D13 | Working |

The measured digital signals switched between approximately **0 V and 5 V**.

The CLK line showed repeated pulse activity, confirming that the Arduino was generating clock pulses.

Activity was also observed on:

- DATA
- LAT
- OE
- A
- B

This test was important because it confirmed that the Arduino and the signal wiring were functioning.

The display problem therefore appeared to be related to the panel interface, signal polarity, scan structure or internal LED mapping.

---

## Input Connector Discovery

The P10 module contains two HUB-style 16-pin connectors.

During testing, it was discovered that the two connectors could not simply be used interchangeably.

For this particular panel, the **left-side connector when looking at the back of the PCB** was identified experimentally as the input connector.

Using the other connector did not produce the expected display output.

After moving the Arduino signals to the correct connector, the panel responded to the raw test signals.

---

## Signal Polarity Testing

The next step was determining how the OE and DATA signals behaved.

### OE Polarity

The initial software assumptions about the Output Enable signal did not match this panel.

Testing showed:

- `OE HIGH` = display enabled
- `OE LOW` = display disabled

This behavior was confirmed experimentally by changing the OE state while transmitting known LED patterns.

### DATA Polarity

The DATA signal was also found to use inverted LED logic relative to the initial implementation.

For this panel:

- `DATA LOW` = LED ON
- `DATA HIGH` = LED OFF

Correcting both the OE and DATA behavior was a major step toward controlling the panel successfully.

---

## 128-Bit Shift Test

The panel contains eight DP5125 driver ICs.

Since each driver has 16 outputs:

`8 × 16 = 128`

A raw Arduino test program was created to transmit exactly **128 bits for each scan state**.

The basic sequence was:

1. Disable the display.
2. Select a scan address using A and B.
3. Shift 128 bits using DATA and CLK.
4. Pulse LAT to latch the data.
5. Enable the display.
6. Repeat for the next scan state.

When all 128 transmitted bits represented the ON state, the complete panel successfully illuminated.

This confirmed several important characteristics simultaneously:

- Arduino communication was working
- The panel itself was working
- The correct HUB connector was being used
- 128 bits per scan was correct
- OE polarity was correct
- DATA polarity was correct

However, displaying normal text still produced an incorrect image.

This indicated that the remaining problem was the relationship between the serial bit order and the physical LEDs.

---

## Scan Architecture

Testing showed that the module uses a **1/4 scan architecture**.

Two address lines are used:

- A
- B

These provide four possible scan states.

| A | B | Scan |
|---|---|---:|
| 0 | 0 | 0 |
| 1 | 0 | 1 |
| 0 | 1 | 2 |
| 1 | 1 | 3 |

The Arduino continuously cycles through these four scan states.

For each scan state, 128 bits are transmitted to the driver chain.

Rapidly repeating all four scan states creates the complete 32×16 image through multiplexing and persistence of vision.

---

## Physical LED Mapping Experiment

Knowing that 128 bits were required was not enough.

The exact relationship between those 128 bits and the physical LEDs still had to be determined.

A diagnostic program was therefore created that transmitted 128 bits while activating **only one bit at a time**.

The position of the illuminated LED was then observed and recorded.

Several experimentally confirmed positions for scan 0 were:

| Shift Bit | Physical Position |
|---:|---|
| 0 | Row 4, Column 32 |
| 1 | Row 4, Column 31 |
| 7 | Row 4, Column 25 |
| 8 | Row 8, Column 32 |
| 15 | Row 8, Column 25 |
| 16 | Row 12, Column 32 |
| 31 | Row 16, Column 25 |
| 32 | Row 4, Column 24 |
| 64 | Row 4, Column 16 |

These measurements revealed that the shift register did not correspond to a simple 32-pixel left-to-right row.

---

## Discovered Bit Organization

The 128-bit stream is organized into four main blocks.

Each block controls an **8-column-wide section** of the matrix.

The column order is reversed relative to normal framebuffer coordinates.

The blocks correspond to:

- Block 0 → Columns 32 to 25
- Block 1 → Columns 24 to 17
- Block 2 → Columns 16 to 9
- Block 3 → Columns 8 to 1

Inside each block are four groups of eight bits.

For scan 0, the first block behaves as follows:

| Bits | Physical Position |
|---|---|
| 0–7 | Row 4, Columns 32→25 |
| 8–15 | Row 8, Columns 32→25 |
| 16–23 | Row 12, Columns 32→25 |
| 24–31 | Row 16, Columns 32→25 |

The next 32-bit block repeats the same row structure for the next eight columns.

This pattern continues across the entire display.

---

## Final Mapping Formula

After the physical mapping was understood, the following mapping logic was implemented in the custom driver:

```cpp
for (byte block = 0; block < 4; block++) {

    for (byte rowGroup = 0; rowGroup < 4; rowGroup++) {

        int y = (3 - scan) + rowGroup * 4;

        for (byte p = 0; p < 8; p++) {

            int x = 31 - block * 8 - p;

            bool led = frame[y][x];

            digitalWrite(PIN_DATA, led ? LOW : HIGH);

            digitalWrite(PIN_CLK, HIGH);
            digitalWrite(PIN_CLK, LOW);
        }
    }
}
```

This converts normal framebuffer coordinates into the physical bit order required by the panel.

---

## Display Refresh Sequence

The final refresh sequence works as follows.

First, the display output is disabled:

```cpp
digitalWrite(PIN_OE, LOW);
```

The scan address is selected:

```cpp
digitalWrite(PIN_A, scan & 1);
digitalWrite(PIN_B, (scan >> 1) & 1);
```

The 128 bits are then shifted into the DP5125 driver chain.

After all bits have been transmitted, LAT is pulsed:

```cpp
digitalWrite(PIN_LAT, HIGH);
delayMicroseconds(1);
digitalWrite(PIN_LAT, LOW);
```

The display is then enabled:

```cpp
digitalWrite(PIN_OE, HIGH);
```

After a short display period, OE is disabled and the next scan begins.

All four scan states are refreshed continuously.

---

## Custom Framebuffer

Instead of making the game logic work directly with the unusual physical LED organization, a normal 32×16 framebuffer was implemented:

```cpp
bool frame[16][32];
```

This allows the rest of the program to work with conventional X/Y pixel coordinates.

For example:

```cpp
pixel(x, y);
```

can represent a normal physical pixel location.

The low-level refresh routine is responsible for converting these logical framebuffer coordinates into the unusual serial bit order required by the panel.

This separates the application logic from the panel hardware implementation.

---

## First Successful Text Test

After implementing the discovered mapping, a scrolling text test was performed across the entire display.

The message:

`GOTVEREN MUSTAFA`

was successfully displayed and scrolled across the 32×16 matrix.

This test confirmed that:

- The scan order was correct
- The 128-bit shift length was correct
- The physical row mapping was correct
- The physical column mapping was correct
- DATA polarity was correct
- OE polarity was correct
- The custom framebuffer driver was functioning

At this point, the P10 panel could be controlled reliably without the standard P10 display library.

---

## Final Arduino Connections

The final signal connections are:

| Function | Arduino Uno Pin |
|---|---:|
| Push Button | D2 |
| Active Buzzer | D3 |
| P10 A | D6 |
| P10 B | D7 |
| P10 LAT | D8 |
| P10 OE | D9 |
| P10 DATA / DR | D11 |
| P10 CLK | D13 |
| Ground | GND |

The push button is connected between **D2 and GND**.

The Arduino uses:

```cpp
pinMode(PIN_BUTTON, INPUT_PULLUP);
```

so an external button pull-up resistor is not required.

---

## Power Supply Testing

The P10 panel was also tested using an external regulated 5 V power source.

During normal stopwatch operation, measured current consumption reached approximately:

`0.16 A`

The display was noticeably brighter when powered directly from the external 5 V supply compared with powering it through the Arduino Uno.

For the final system, the P10 panel is therefore powered directly from a regulated **5 V supply**.

The Arduino and P10 panel use a **common ground**.

---

## Final Application: 10-Second Challenge

After the custom display driver was working correctly, it was used to build the final application.

The game is a **10-second timing challenge**.

The player presses a button once to start the timer and presses it again when they believe exactly 10 seconds have passed.

The display shows:

`SS.CC`

where:

- `SS` = seconds
- `CC` = centiseconds

Examples:

`00.00`

`09.87`

`10.00`

`12.43`

The winning interval is:

`9.95 s – 10.05 s`

inclusive.

---

## Winning

If the measured time is between **9.95 and 10.05 seconds**, the player wins.

The system then:

1. Stops the timer.
2. Displays a happy face.
3. Plays the winning buzzer pattern.
4. Keeps the face on screen for approximately two seconds.
5. Displays `10.00`.
6. Keeps the result on the display until another game is started.

---

## Losing

If the player stops outside the winning interval, the system:

1. Stops the timer.
2. Displays a sad face.
3. Plays the losing buzzer pattern.
4. Keeps the face on screen for approximately two seconds.
5. Displays the player's actual stopping time.
6. Keeps the result on screen until another game is started.

If the player does not stop the timer before **20.00 seconds**, the game automatically ends as a loss.

---

## Software Architecture

The final game uses a simple state machine:

```cpp
enum GameState {
    READY,
    RUNNING,
    RESULT_FACE,
    RESULT_TIME
};
```

This allows the display refresh, timer, button handling and buzzer logic to operate without long blocking delays.

The display therefore continues to refresh while the rest of the application is running.

Button debouncing is also handled in software using `millis()`.

---

## Project Result

What initially appeared to be a simple Arduino LED matrix project required investigation of the actual panel hardware.

The development process included:

- Inspecting the P10 PCB
- Identifying the DP4536 and eight DP5125 ICs
- Verifying Arduino signals with an oscilloscope
- Testing approximately 0–5 V digital activity
- Identifying the correct HUB input connector
- Determining OE polarity experimentally
- Determining DATA polarity experimentally
- Discovering the 1/4 scan architecture
- Determining the 128-bit shift structure
- Activating individual bits to map physical LEDs
- Deriving the panel's custom coordinate mapping
- Implementing a custom framebuffer
- Writing a low-level Arduino display driver
- Successfully displaying scrolling text
- Building the final 10-second timing game

The final solution directly controls the 32×16 P10 LED matrix from an Arduino Uno without relying on the standard P10 display library.
For the complete reverse-engineering process, oscilloscope testing, IC analysis and physical LED mapping, see:
