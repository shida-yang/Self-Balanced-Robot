#include <F28x_Project.h>
#include "driverlib.h"
#include "interrupt.h"
#include <math.h>

#define MPU6050_SLAVE_ADDR  0x68

#define RA_CONFIG           0x1A
#define RA_GYRO_CONFIG      0x1B
#define RA_ACCEL_CONFIG     0x1C

#define RA_ACCEL_XOUT_H     0x3B
#define RA_ACCEL_XOUT_L     0x3C
#define RA_ACCEL_YOUT_H     0x3D
#define RA_ACCEL_YOUT_L     0x3E
#define RA_ACCEL_ZOUT_H     0x3F
#define RA_ACCEL_ZOUT_L     0x40

#define RA_GYRO_XOUT_H      0x43
#define RA_GYRO_XOUT_L      0x44
#define RA_GYRO_YOUT_H      0x45
#define RA_GYRO_YOUT_L      0x46
#define RA_GYRO_ZOUT_H      0x47
#define RA_GYRO_ZOUT_L      0x48

#define RA_PWR_MGMT_1       0x6B

#define ACCEL_RES           16384.0
#define GYRO_RES            65.5
#define TIMER0_PER          0.002

#define MPU6050_CALIBRATION_COUNT   500

typedef struct MPU6050_Data_Packet{
    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;
    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;
} MPU6050_Data_Packet_t;

typedef enum UPDATE_ACCEL_GYRO_STATE{
    IDLEING,
    SEND_ACCEL_RA,
    SWITCH_TO_RX_ACCEL,
    RX_ACCEL_LOOP,
    STOP_ACCEL,
    SEND_GYRO_RA,
    SWITCH_TO_RX_GYRO,
    RX_GYRO_LOOP,
    STOP_GYRO
} UPDATE_ACCEL_GYRO_STATE_t;

void MPU6050_Powerup();
float MPU6050_GetPitchAngle();

static void MPU6050_Calibrate();
static float MPU6050_GetAccelAngle();
static void MPU6050_ReadAccelGyro_Polling(MPU6050_Data_Packet_t* dp);

static void I2C_Master_Init();
static void I2C_SendBytes_Polling(Uint16 RA, Uint16 * const values, Uint16 length);
static void I2C_ReadBytes_Polling(Uint16 RA, Uint16 * const values, Uint16 length);
static void InitI2CGpio();
__interrupt void i2caISR(void);

static void InitTimer0();
__interrupt void cpuTimer0ISR(void);
