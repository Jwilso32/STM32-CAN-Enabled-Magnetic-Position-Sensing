#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/* Cooperative task periods. All values are in milliseconds. */
#ifndef APP_SENSOR_PERIOD_MS
#define APP_SENSOR_PERIOD_MS                 5U
#endif

#ifndef APP_DISPLAY_PERIOD_MS
#define APP_DISPLAY_PERIOD_MS               50U
#endif

#ifndef APP_CALIBRATION_LOG_PERIOD_MS
#define APP_CALIBRATION_LOG_PERIOD_MS       50U
#endif

/* 512 samples at 20 Hz retain about 25.6 seconds of sweep history in RAM. */
#ifndef APP_CALIBRATION_CAPTURE_SAMPLES
#define APP_CALIBRATION_CAPTURE_SAMPLES     512U
#endif

#if APP_CALIBRATION_CAPTURE_SAMPLES == 0
#error "APP_CALIBRATION_CAPTURE_SAMPLES must be greater than zero"
#endif

#ifndef APP_SENSOR_RETRY_PERIOD_MS
#define APP_SENSOR_RETRY_PERIOD_MS          1000U
#endif

/* Keypad protocol timing captured from the original hardware. */
#ifndef APP_KEYPAD_HEARTBEAT_PERIOD_MS
#define APP_KEYPAD_HEARTBEAT_PERIOD_MS      100U
#endif

#ifndef APP_KEYPAD_PRESS_DURATION_MS
#define APP_KEYPAD_PRESS_DURATION_MS        170U
#endif

/*
 * Development features. Set these to 0 for the custom production PCB.
 * The OLED code remains available for a bench build but is not initialized
 * or referenced by the application when APP_ENABLE_BENCH_DISPLAY is 0.
 */
#ifndef APP_ENABLE_BENCH_DISPLAY
#define APP_ENABLE_BENCH_DISPLAY            1U
#endif

#ifndef APP_ENABLE_ITM_TRACE
#define APP_ENABLE_ITM_TRACE                1U
#endif

/* Use the Nucleo B1 button as the boot-time calibration request on the bench. */
#ifndef APP_USE_NUCLEO_CALIBRATION_BUTTON
#define APP_USE_NUCLEO_CALIBRATION_BUTTON   1U
#endif

#endif /* APP_CONFIG_H */
