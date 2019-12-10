
#ifndef _SCD30_INTERNAL_H_
#define _SCD30_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/*** I2C address ***/
//#define SCD30_I2C_ADDRESS               0x61

/*** Read ***/
#define SCD30_VERSION                   0xD100
#define SCD30_STATUS                    0x0202  // 1: available. 0: not.
/*** Read (data) ***/
#define SCD30_DATA                      0x0300
/*** CMD ***/
#define SCD30_START                     0x0010
#define SCD30_STOP                      0x0104
#define SCD30_RESET                     0xD304
/*** Parameters ***/
#define SCD30_INTERVAL                  0x4600  // Measurement Interval, 2~1800 s.
#define SCD30_ASC                       0x5306  // De-Activate Automatic Self-Calibration
#define SCD30_FRC                       0x5204  // Forced Recalibration
#define SCD30_T_OFFSET                  0x5403  // Temperature Offset, 0.01 Celsius.
#define SCD30_A_OFFSET                  0x5102  // Altitude Compensation, 1 Meter.

/****** Default Value ******/
#define SCD30_DEF_PRESSURE              0x03f5  // Ambient Pressure: 1013 mBar (1013.25)


#ifdef __cplusplus
}
#endif

#endif