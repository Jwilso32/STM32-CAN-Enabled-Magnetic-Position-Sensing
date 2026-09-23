# Magnetic Gear Selector Firmware

This project reads the AH4931Q magnetic sensor, classifies a stable Park,
Reverse, Neutral, or Drive position, and emulates the original keypad over
Classic CAN.

## Application structure

- `Core/Src/main.c` contains only CubeMX hardware initialization and the
  top-level `App_Init()` / `App_Run()` calls.
- `app.c` owns the cooperative schedule and application state.
- `ah4931q.c` is only the I2C sensor driver.
- `gear_selector.c` filters samples, applies calibrated 3-axis windows,
  hysteresis, and transition stability time.
- `gear_calibration.c` is the only file that should need data changes after a
  mechanical calibration sweep.
- `keypad_can.c` owns the captured keypad wire protocol and virtual-button
  state machine.
- `calibration_mode.c` exposes live raw/filtered telemetry and CSV trace output.
- `app_display.c` contains the optional test-bench OLED and is not part of the
  production behavior.
- `board_io.c` isolates the present Nucleo button from the future PCB
  calibration strap.

The scheduler is deliberately foreground-only. ISRs do not compete with the
application for the FDCAN transmit FIFO, and each task uses elapsed-time checks
so a delayed loop does not generate catch-up bursts.

## Keypad CAN protocol

All outgoing keypad frames use extended ID `0x18EFFF21`, Classic CAN, and an
8-byte payload. `0x18EF2100` is the observed VCU-to-keypad response direction.

```text
Heartbeat:   04 1B F9 <counter> <active button> 00 FF 21
Button edge: 04 1B <button> 01 <pressed> 21 FF FF
```

Button codes are Park `01`, Reverse `02`, Neutral `03`, and Drive `04`.
A stable gear transition creates an approximately 170 ms virtual press. At
least one 100 ms heartbeat carries the active code before the release edge.

## Build variants

Development features are controlled in `Core/Inc/app_config.h`:

- Set `APP_ENABLE_BENCH_DISPLAY` to `0` for the final PCB.
- Set `APP_USE_NUCLEO_CALIBRATION_BUTTON` to `0` after a `CAL_MODE` GPIO is
  defined in CubeMX for the custom board.
- Set `APP_ENABLE_ITM_TRACE` to `0` if SWO/ITM is not routed or used.

See [CALIBRATION.md](CALIBRATION.md) before enabling any gear zone.
