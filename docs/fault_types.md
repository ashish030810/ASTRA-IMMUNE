# Fault types and scoring

## Dropout
Switch on the Mega (D2) cuts the ultrasonic echo signal. pulseIn() times out and the Mega reports -1. Any negative value scores 0 trust right away.

## Corruption
Triggered by the button on the Mega (D3) or the "Corrupt Ultrasonic" button on the dashboard, which sends a command through the ESP32 to the Mega. The Mega swaps in a random value from 500-900cm even though the sensor's real range is around 2-400cm. Caught by the range check and the jump check in the scoring function.

## Spoofing
The Uno runs in the background sending fake ultrasonic readings over its own serial line to the ESP32. About 70% of the fake values look plausible (20-150cm), the rest are obvious outliers (600-999cm), and they arrive at irregular intervals instead of the Mega's steady rate. Every packet goes through the same scoring function whether it's real or fake - the outliers get caught easily, the plausible-looking ones are where it's weakest.

## Scoring

Starts at 1.0:
- value outside 2-400cm: -0.7
- jump of more than 150cm since the last trusted reading: -0.5
- clamped at 0
- isolated if score drops below 0.5

## Things worth adding later

- track timing between packets, not just value range (real sensor is steady, spoofed data isn't)
- bring IR into the scoring instead of passing it through untouched
- cross-check ultrasonic against IR
- keep a rolling average of trust score instead of isolating on one bad reading
