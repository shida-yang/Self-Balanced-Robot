#include "MPU6050.h"

void MPU6050_Powerup(){
    I2C_Master_Init();

//    for(int i=0; i<128; i++){
//        I2caRegs.I2CSAR.bit.SAR = i;
//
//        // transmitter mode
//        I2caRegs.I2CMDR.bit.TRX = 1;
//
//        // start condition
//        I2caRegs.I2CMDR.bit.STT = 1;
//
//        // wait for start condition to send
//        while(I2caRegs.I2CMDR.bit.STT);
//
//        if(I2caRegs.I2CSTR.bit.NACK == 0){
//            int a=i;
//        }
//    }

    uint16_t i=0;
    I2C_SendBytes(RA_PWR_MGMT_1, &i, 1);
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
