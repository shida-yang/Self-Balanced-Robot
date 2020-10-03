#include "MPU6050.h"

int16_t accel_x_calibration_value,
        accel_y_calibration_value,
        accel_z_calibration_value,
        gyro_x_calibration_value,
        gyro_y_calibration_value,
        gyro_z_calibration_value;

float current_gyro_angle;

int16_t last_accel_x_reading,
        last_accel_y_reading,
        last_accel_z_reading,
        last_gyro_x_reading,
        last_gyro_y_reading,
        last_gyro_z_reading;

void MPU6050_Powerup(){
    EALLOW;
    GpioCtrlRegs.GPCDIR.bit.GPIO95 = 1;
    GpioDataRegs.GPCCLEAR.bit.GPIO95 = 1;
    GpioCtrlRegs.GPCPUD.bit.GPIO95 = 0;

    I2C_Master_Init();

    uint16_t i=0;
    // wake up MPU6050
    I2C_SendBytes(RA_PWR_MGMT_1, &i, 1);

    // set LPF
    i = 0x3;
    I2C_SendBytes(RA_CONFIG, &i, 1);

    // config gyro range
    i=0;
    I2C_SendBytes(RA_GYRO_CONFIG, &i, 1);

    // config accel range
    i=0;
    I2C_SendBytes(RA_ACCEL_CONFIG, &i, 1);

    // take initial average readings to get the calibration offset for gyro
    MPU6050_Calibrate();

    MPU6050_Data_Packet_t dp;
    MPU6050_ReadAccelGyro(&dp);

    last_accel_x_reading = dp.accel_x;
    last_accel_y_reading = dp.accel_y;
    last_accel_z_reading = dp.accel_z;
    last_gyro_x_reading = dp.gyro_x;
    last_gyro_y_reading = dp.gyro_y;
    last_gyro_z_reading = dp.gyro_z;

    current_gyro_angle = MPU6050_GetAccelAngle();

    InitTimer0();
}

void MPU6050_ReadAccelGyro(MPU6050_Data_Packet_t* dp){
    uint16_t data_array[12];
    I2C_ReadBytes(RA_ACCEL_XOUT_H, data_array, 6);
    I2C_ReadBytes(RA_GYRO_XOUT_H, &data_array[6], 6);

    dp->accel_x = (int16_t)((data_array[1]<<8) | data_array[0]);
    dp->accel_y = (int16_t)((data_array[3]<<8) | data_array[2]);
    dp->accel_z = (int16_t)((data_array[5]<<8) | data_array[4]);
    dp->gyro_x = (int16_t)((data_array[7]<<8) | data_array[6]);
    dp->gyro_y = (int16_t)((data_array[9]<<8) | data_array[8]);
    dp->gyro_z = (int16_t)((data_array[11]<<8) | data_array[10]);
}

float MPU6050_GetPitchAngle(){
    return current_gyro_angle;
}

static void MPU6050_Calibrate(){
    uint16_t testing_points = 500;
    int32_t sum_accel_x = 0, sum_accel_y = 0, sum_accel_z = 0,
                sum_gyro_x = 0, sum_gyro_y = 0, sum_gyro_z = 0;
    MPU6050_Data_Packet_t dp;

    for(int i=0; i<testing_points; i++){
        MPU6050_ReadAccelGyro(&dp);
        sum_gyro_x += dp.gyro_x;
        sum_gyro_y += dp.gyro_y;
        sum_gyro_z += dp.gyro_z;
        sum_accel_x += dp.accel_x;
        sum_accel_y += dp.accel_y;
        sum_accel_z += dp.accel_z;
        DELAY_US(2000);
    }

    gyro_x_calibration_value = sum_gyro_x / testing_points;
    gyro_y_calibration_value = sum_gyro_y / testing_points;
    gyro_z_calibration_value = sum_gyro_z / testing_points;
    accel_x_calibration_value = sum_accel_x / testing_points;
    accel_y_calibration_value = sum_accel_y / testing_points;
    accel_z_calibration_value = sum_accel_z / testing_points;
}

static float MPU6050_GetAccelAngle(){
    float adjusted_accel_y_reading = last_accel_y_reading - accel_y_calibration_value;
    if(adjusted_accel_y_reading > 16384) adjusted_accel_y_reading = 16384;
    if(adjusted_accel_y_reading < -16384) adjusted_accel_y_reading = -16384;
    return asinf(adjusted_accel_y_reading / 16384.0) * 57.2958; // 360/(2*pi) = 57.2958
}

static void I2C_Master_Init(){
    InitI2CGpio();

    EALLOW;

    // Enable Clock for I2C
    CpuSysRegs.PCLKCR9.bit.I2C_A = 1;

    // Put I2C into Reset Mode
    I2caRegs.I2CMDR.bit.IRS = 0;

    // Configure I2C clock to 400kHz
    // Assume system clock = 200MHz
    I2caRegs.I2CPSC.bit.IPSC = 19;
    I2caRegs.I2CCLKL = 7;
    I2caRegs.I2CCLKH = 8;

    // Set slave address
    I2caRegs.I2CSAR.bit.SAR = MPU6050_SLAVE_ADDR;

    // Release from Reset Mode
    I2caRegs.I2CMDR.bit.IRS = 1;

}

static void I2C_SendBytes(Uint16 RA, Uint16 * const values, Uint16 length){
    // Set to Master, Repeat Mode, TRX, FREE, Start
    I2caRegs.I2CMDR.all = 0x66A0;

    // wait for start condition to send
    while(I2caRegs.I2CMDR.bit.STT);

    // wait if transmitter is not ready
    while(!I2caRegs.I2CSTR.bit.XRDY);

    // send MPU6050 internal register address
    I2caRegs.I2CDXR.bit.DATA = RA;

    for(int i=0; i<length; i++){
        // wait if transmitter is not ready
        while(!I2caRegs.I2CSTR.bit.XRDY);
        // send data
        I2caRegs.I2CDXR.bit.DATA = values[i];
    }

    // wait if transmitter is not ready
    while(!I2caRegs.I2CSTR.bit.XRDY);

    // stop condition
    I2caRegs.I2CMDR.bit.STP = 1;
    while(I2caRegs.I2CMDR.bit.STP);
}

static void I2C_ReadBytes(Uint16 RA, Uint16 * const values, Uint16 length){
    // Set to Master, Repeat Mode, TRX, FREE, Start
    I2caRegs.I2CMDR.all = 0x66A0;

    // wait for start condition to send
    while(I2caRegs.I2CMDR.bit.STT);

    // send MPU6050 internal register address
    I2caRegs.I2CDXR.bit.DATA = RA;

    // wait if transmitter is not ready
    while(!I2caRegs.I2CSTR.bit.XRDY);

    // Set to Master, Repeat Mode, receive, FREE, Start
    I2caRegs.I2CMDR.all = 0x64A0;

    // wait for start condition to send
    while(I2caRegs.I2CMDR.bit.STT);

    for(int i=0; i<length; i++){
        // wait for received data to be ready
        while(!I2caRegs.I2CSTR.bit.RRDY);
        values[i] = I2caRegs.I2CDRR.bit.DATA;
    }

    // send NACK
    I2caRegs.I2CMDR.bit.NACKMOD = 1;

    // wait for NACK to send
    while(I2caRegs.I2CMDR.bit.NACKMOD);

    // stop condition
    I2caRegs.I2CMDR.bit.STP = 1;
}

static void InitI2CGpio(){
    EALLOW;

    GpioCtrlRegs.GPDDIR.bit.GPIO105 = 1;
    GpioDataRegs.GPDCLEAR.bit.GPIO105 = 1;
    int i;
    for(i=0; i<10; i++){
        GpioDataRegs.GPDSET.bit.GPIO105 = 1;
        DELAY_US(1000);
        GpioDataRegs.GPDCLEAR.bit.GPIO105 = 1;
        DELAY_US(1000);
    }

    GpioCtrlRegs.GPDPUD.bit.GPIO104 = 0;    // Enable pull-up for GPIO104 (SDAA)
    GpioCtrlRegs.GPDPUD.bit.GPIO105 = 0;       // Enable pull-up for GPIO105 (SCLA)

    GpioCtrlRegs.GPDQSEL1.bit.GPIO104 = 3;  // Asynch input GPIO104 (SDAA)
    GpioCtrlRegs.GPDQSEL1.bit.GPIO105 = 3;  // Asynch input GPIO105 (SCLA)

    GpioCtrlRegs.GPDGMUX1.bit.GPIO104 = 0;   // Configure GPIO104 for SDAA operation
    GpioCtrlRegs.GPDGMUX1.bit.GPIO105 = 0;   // Configure GPIO105 for SCLA operation
    GpioCtrlRegs.GPDMUX1.bit.GPIO104 = 1;
    GpioCtrlRegs.GPDMUX1.bit.GPIO105 = 1;

}

static void InitTimer0(){
    CPUTimer_stopTimer(CPUTIMER0_BASE);

    // set timer to overflow every 2ms (400000 clock cycles at 200MHz)
    CPUTimer_setPreScaler(CPUTIMER0_BASE, 399);
    CPUTimer_setPeriod(CPUTIMER0_BASE, 999);

    // enable timer0 interrupt
    Interrupt_register(INT_TIMER0, &cpuTimer0ISR);

    CPUTimer_enableInterrupt(CPUTIMER0_BASE);
    Interrupt_enable(INT_TIMER0);

    CPUTimer_startTimer(CPUTIMER0_BASE);
}

__interrupt void cpuTimer0ISR(void){
    GpioDataRegs.GPCTOGGLE.bit.GPIO95 = 1;

    MPU6050_Data_Packet_t dp;
    MPU6050_ReadAccelGyro(&dp);

    last_accel_x_reading = dp.accel_x;
    last_accel_y_reading = dp.accel_y;
    last_accel_z_reading = dp.accel_z;
    last_gyro_x_reading = dp.gyro_x;
    last_gyro_y_reading = dp.gyro_y;
    last_gyro_z_reading = dp.gyro_z;

    float delta_gyro_x_angle = ((float)last_gyro_x_reading - (float)gyro_x_calibration_value) * 0.00001567;

    float accel_angle = MPU6050_GetAccelAngle();

//    accel_angle = 0;

    float adjusted_gyro_z_reading = last_gyro_z_reading - gyro_z_calibration_value;

//    adjusted_gyro_z_reading = 0;

    current_gyro_angle = (current_gyro_angle + delta_gyro_x_angle + adjusted_gyro_z_reading * 0.0000001576) * 0.9990 + (accel_angle) * 0.0010;

    CPUTimer_clearOverflowFlag(CPUTIMER0_BASE);
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP1);
}