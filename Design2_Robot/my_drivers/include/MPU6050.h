#include <F28x_Project.h>

#define MPU6050_SLAVE_ADDR  0x68

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

#define RA_PWR_MGMT_1   0x6B

typedef struct MPU6050_Data_Packet{
    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;
    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;
} MPU6050_Data_Packet_t;

void MPU6050_Powerup();
void MPU6050_ReadAccelGyro(MPU6050_Data_Packet_t* dp);

static void I2C_Master_Init();
static void I2C_SendBytes(Uint16 RA, Uint16 * const values, Uint16 length);
static void I2C_ReadBytes(Uint16 RA, Uint16 * const values, Uint16 length);
static void InitI2CGpio();
