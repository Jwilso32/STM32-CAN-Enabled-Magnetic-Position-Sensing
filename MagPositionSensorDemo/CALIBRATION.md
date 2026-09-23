# Magnetic Gear Calibration

The default calibration table is deliberately disabled. <br>
Uncalibrated firmware enters calibration mode automatically and does not issue P/R/N/D button edges.
It still sends the keypad heartbeat with active-button code `00`.

## First assembled-board workflow

1. Program and debug the board through SWD.
2. Enter calibration mode by either:
   - holding the Nucleo user button during reset on the current bench board;
   - grounding the future active-low `CAL_MODE` strap during reset; or
   - setting `g_force_calibration_mode` to `1` before `App_Init()` in a debug
     session.
3. Watch `g_calibration_telemetry` with STM32CubeIDE Live Expressions. This
   works over SWD and does not require the OLED. Calibration mode also records
   512 samples (about 25.6 seconds) in `g_calibration_capture`. After the sweep,
   halt the MCU before inspecting or exporting the circular buffer. If
   `g_calibration_capture_total_samples` exceeds 512, the oldest record begins
   at `g_calibration_capture_write_index`.
4. If SWO is routed and asynchronous trace is configured, capture the
   `CAL,...` CSV lines from ITM stimulus port 0 at 20 Hz.
5. Sweep the complete mechanism several times in both directions. Also hold
   each labeled P/R/N/D detent for about one second on repeated approaches.
6. For each gear, use the captured **filtered** values to choose inclusive
   X/Y/Z minimum and maximum values that cover the repeated settled samples
   plus reasonable mechanical, temperature, and manufacturing margin. Use raw
   values to understand noise. Do not use the continuous sweep alone to infer
   which cloud belongs to which gear.
7. Enter the four boxes in `Core/Src/gear_calibration.c` and set each zone's
   `enabled` field to `true`.
8. Rebuild and reflash. Startup validation rejects missing, reversed, or
   overlapping boxes.
9. Verify that each detent becomes stable, every between-detent position is
   invalid, boundary dither does not chatter, and the CAN payloads match the
   captured keypad before connecting to the vehicle system.

`hysteresis_counts` expands only the currently held zone. `stable_time_ms`
requires a candidate to remain unchanged before the public gear changes.
`filter_shift = 2` is a first-order IIR factor of 1/4. Tune these after looking
at real traces rather than enlarging zones until they overlap.

## PCB provisions

The final board does not need the OLED. Provide:

- SWDIO, SWCLK, NRST, target VREF, and GND on a Tag-Connect or pogo footprint;
- optional SWO if CSV/ITM logging is desired;
- an active-low, pulled-up `CAL_MODE` test pad or DNP two-pin jumper, sampled
  only at boot;
- accessible CANH, CANL, and ground test points;
- I2C and sensor-supply test points.

The calibration strap selects a safe service mode; it should not write flash by
itself. For the first prototypes, measure, edit the table, and reflash. If every
production unit later needs individual values, add a versioned, CRC-protected,
power-loss-safe flash record and a separately authorized calibration command.
Do not add flash writes until that update and recovery design is defined.

A dedicated diagnostic CAN telemetry frame can be added later, but only after a
non-conflicting diagnostic identifier is allocated for the target network. The
firmware intentionally does not invent one.
