# P10 10-Second Challenge 🎯

An Arduino Uno based timing game built with a custom-driven 32×16 single-color P10 LED matrix.

The goal is simple: press the button to start the timer and press it again as close as possible to **10.00 seconds**.

- **9.95 – 10.05 seconds:** WIN 🙂
- Anything else: LOSE 🙁
- If the button is not pressed within 20 seconds, the game automatically ends.

When the player wins, the display shows a happy face and an active buzzer plays a victory pattern. When the player loses, a sad face is displayed with a different buzzer pattern.

## 🔧 Hardware

- Arduino Uno
- 32×16 single-color red P10 LED matrix
- DP4536 controller
- 8× DP5125 LED driver ICs
- Push button
- Active buzzer
- 5V power supply

## 🔌 Arduino Pinout

| Function | Arduino Pin |
|---|---|
| Button | D2 |
| Active Buzzer | D3 |
| P10 A | D6 |
| P10 B | D7 |
| P10 LAT | D8 |
| P10 OE | D9 |
| P10 DATA | D11 |
| P10 CLK | D13 |

The button is connected between **D2 and GND** and uses the Arduino's internal pull-up resistor.

## 🧠 P10 Panel Reverse Engineering

The P10 panel used in this project did not work correctly with the standard display library and required a custom low-level driver.

The panel contains:

- 1× DP4536
- 8× DP5125
- 32×16 LEDs
- 1/4 scan architecture

The panel was investigated experimentally using an oscilloscope and individual LED tests.

The Arduino directly controls:

- DATA
- CLK
- LAT
- OE
- A
- B

Each scan transfers **128 bits** to the panel.

The physical LED mapping was determined manually by activating individual bits and observing their corresponding LED positions.

For this panel:

- DATA LOW = LED ON
- DATA HIGH = LED OFF
- OE HIGH = Display enabled
- OE LOW = Display disabled

This custom mapping allows the complete 32×16 panel to be controlled directly from an Arduino Uno without using the standard P10 display library.

## ⏱️ Game Logic

1. The display initially shows `00.00`.
2. Pressing the button starts the timer.
3. Pressing the button again stops the timer.
4. A result between `9.95` and `10.05` seconds is considered a win.
5. A happy or sad face is displayed for two seconds.
6. After the animation, the final result remains on the display.
7. Pressing the button again starts a new game.
8. The game automatically ends at `20.00` seconds if the player does not press the button.

## 🔊 Sound

An active buzzer provides different beep patterns for winning and losing.

Because an active buzzer generates its own tone, different rhythms are used instead of different musical frequencies.

## 🚧 Project Status

Working prototype.

Future improvements may include:

- Permanent enclosure
- Improved wiring
- Front panel design
- Statistics / game counter
- Additional animations

## 👨‍💻 Author

Built by Abdullah Kutlu. 
