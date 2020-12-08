#include "MPU6050.h"

float current_gyro_angle;

MPU6050_Data_Packet_t last_dp = {}, calibration_dp = {};

uint16_t calibration_count = MPU6050_CALIBRATION_COUNT;
int32_t sum_accel_x = 0, sum_accel_y = 0, sum_accel_z = 0,
            sum_gyro_x = 0, sum_gyro_y = 0, sum_gyro_z = 0;

uint16_t update_accel_gyro_i2c_counter;
UPDATE_ACCEL_GYRO_STATE_t update_accel_gyro_i2c_state = IDLEING;

void MPU6050_Powerup(){

    I2C_Master_Init();

    uint16_t i=0;
    // wake up MPU6050
    I2C_SendBytes_Polling(RA_PWR_MGMT_1, &i, 1);

    // set LPF
    i = 0x3;
    I2C_SendBytes_Polling(RA_CONFIG, &i, 1);

    // config gyro range
    i= (1<<3);
    I2C_SendBytes_Polling(RA_GYRO_CONFIG, &i, 1);

    // config accel range
    i=0;
    I2C_SendBytes_Polling(RA_ACCEL_CONFIG, &i, 1);

    // take initial average readings to get the calibration offset for gyro
    MPU6050_Calibrate();

//    MPU6050_ReadAccelGyro_Polling(&last_dp);
//
//    current_gyro_angle = MPU6050_GetAccelAngle();

    InitTimer0();
//    Interrupt_enable(INT_I2CA);
}

static void MPU6050_ReadAccelGyro_Polling(MPU6050_Data_Packet_t* dp){
    uint16_t data_array[12];
    I2C_ReadBytes_Polling(RA_ACCEL_XOUT_H, data_array, 6);
    I2C_ReadBytes_Polling(RA_GYRO_XOUT_H, &data_array[6], 6);

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
    int32_t testing_points = 500;
    int32_t discard_points = 200;
    int32_t sum_accel_x = 0, sum_accel_y = 0, sum_accel_z = 0,
                sum_gyro_x = 0, sum_gyro_y = 0, sum_gyro_z = 0;
    MPU6050_Data_Packet_t dp;

    for(int i=0; i<testing_points + discard_points; i++){
        if(i>=discard_points){
            MPU6050_ReadAccelGyro_Polling(&dp);
            sum_gyro_x += dp.gyro_x;
            sum_gyro_y += dp.gyro_y;
            sum_gyro_z += dp.gyro_z;
            sum_accel_x += dp.accel_x;
            sum_accel_y += dp.accel_y;
            sum_accel_z += (dp.accel_z - ACCEL_RES);
        }
        DELAY_US(2000);
    }

    calibration_dp.gyro_x = sum_gyro_x / (testing_points);
    calibration_dp.gyro_y = sum_gyro_y / (testing_points);
    calibration_dp.gyro_z = sum_gyro_z / (testing_points);
    calibration_dp.accel_x = sum_accel_x / (testing_points);
    calibration_dp.accel_y = sum_accel_y / (testing_points);
    calibration_dp.accel_z = sum_accel_z / (testing_points);
}

static float MPU6050_GetAccelAngle(){
    float adjusted_accel_x_reading = last_dp.accel_x - calibration_dp.accel_x;
    if(adjusted_accel_x_reading > ACCEL_RES) adjusted_accel_x_reading = ACCEL_RES;
    if(adjusted_accel_x_reading < -ACCEL_RES) adjusted_accel_x_reading = -ACCEL_RES;
    return -asinf(adjusted_accel_x_reading / ACCEL_RES) * 57.2958; // 360/(2*pi) = 57.2958
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

    // set up but disable I2CA interrupts
    I2caRegs.I2CIER.all = I2C_INT_RX_DATA_RDY | I2C_INT_TX_DATA_RDY | I2C_INT_STOP_CONDITION;
    Interrupt_register(INT_I2CA, &i2caISR);
//    Interrupt_disable(INT_I2CA);
    Interrupt_enable(INT_I2CA);
}

static void I2C_SendBytes_Polling(Uint16 RA, Uint16 * const values, Uint16 length){
    // Set to Master, Repeat Mode, TRX, FREE, Start
    while(I2caRegs.I2CMDR.bit.MST);
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

static void I2C_ReadBytes_Polling(Uint16 RA, Uint16 * const values, Uint16 length){
    // Set to Master, Repeat Mode, TRX, FREE, Start
    while(I2caRegs.I2CMDR.bit.MST);
    I2caRegs.I2CMDR.all = 0x66A0;

    // send MPU6050 internal register address
    I2caRegs.I2CDXR.bit.DATA = RA;

    // wait if transmitter is not ready
    while(!I2caRegs.I2CSTR.bit.XRDY);

    // Set to Master, Repeat Mode, receive, FREE, Start
    I2caRegs.I2CMDR.all = 0x64A0;

    for(int i=0; i<length; i++){
        // wait for received data to be ready
        while(!I2caRegs.I2CSTR.bit.RRDY);
        values[i] = I2caRegs.I2CDRR.bit.DATA;
    }

    // stop condition
    I2caRegs.I2CMDR.bit.STP = 1;
    while(!I2caRegs.I2CSTR.bit.SCD);
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

__interrupt void i2caISR(void){
//    I2caRegs.I2CSTR.all |= I2C_INT_RX_DATA_RDY | I2C_INT_STOP_CONDITION;
//    I2caRegs.I2CISRC.all;

    switch(update_accel_gyro_i2c_state){
    case IDLEING:
        break;
    case SEND_ACCEL_RA:
        break;
    case SWITCH_TO_RX_ACCEL:
        // Set to Master, Repeat Mode, receive, FREE, Start
        I2caRegs.I2CMDR.all = 0x64A0;
        update_accel_gyro_i2c_state = RX_ACCEL_LOOP;
        break;
    case RX_ACCEL_LOOP:
        switch(update_accel_gyro_i2c_counter){
        case 0:
            last_dp.accel_x = (I2caRegs.I2CDRR.bit.DATA << 8);
            update_accel_gyro_i2c_counter++;
            break;
        case 1:
            last_dp.accel_x |= I2caRegs.I2CDRR.bit.DATA;
            update_accel_gyro_i2c_counter++;
            break;
        case 2:
            last_dp.accel_y = (I2caRegs.I2CDRR.bit.DATA << 8);
            update_accel_gyro_i2c_counter++;
            break;
        case 3:
            last_dp.accel_y |= I2caRegs.I2CDRR.bit.DATA;
            update_accel_gyro_i2c_counter++;
            break;
        case 4:
            last_dp.accel_z = (I2caRegs.I2CDRR.bit.DATA << 8);
            update_accel_gyro_i2c_counter++;
            break;
        case 5:
            last_dp.accel_z |= I2caRegs.I2CDRR.bit.DATA;
            I2caRegs.I2CMDR.bit.STP = 1;
            update_accel_gyro_i2c_state = STOP_ACCEL;
            break;
        default:
            break;
        }
        break;
    case STOP_ACCEL:
        update_accel_gyro_i2c_state = SEND_GYRO_RA;
        break;
    case SEND_GYRO_RA:
        // initiate I2C communication to read gyro
        update_accel_gyro_i2c_counter = 0;
        while(I2caRegs.I2CMDR.bit.MST);
        // Set to Master, Repeat Mode, TRX, FREE, Start
        I2caRegs.I2CMDR.all = 0x66A0;
        // send MPU6050 internal register address
        I2caRegs.I2CDXR.bit.DATA = RA_GYRO_XOUT_H;
        update_accel_gyro_i2c_state = SWITCH_TO_RX_GYRO;
        break;
    case SWITCH_TO_RX_GYRO:
        // Set to Master, Repeat Mode, receive, FREE, Start
        I2caRegs.I2CMDR.all = 0x64A0;
        update_accel_gyro_i2c_state = RX_GYRO_LOOP;
        break;
    case RX_GYRO_LOOP:
        switch(update_accel_gyro_i2c_counter){
        case 0:
            last_dp.gyro_x = (I2caRegs.I2CDRR.bit.DATA << 8);
            update_accel_gyro_i2c_counter++;
            break;
        case 1:
            last_dp.gyro_x |= I2caRegs.I2CDRR.bit.DATA;
            update_accel_gyro_i2c_counter++;
            break;
        case 2:
            last_dp.gyro_y = (I2caRegs.I2CDRR.bit.DATA << 8);
            update_accel_gyro_i2c_counter++;
            break;
        case 3:
            last_dp.gyro_y |= I2caRegs.I2CDRR.bit.DATA;
            update_accel_gyro_i2c_counter++;
            break;
        case 4:
            last_dp.gyro_z = (I2caRegs.I2CDRR.bit.DATA << 8);
            update_accel_gyro_i2c_counter++;
            break;
        case 5:
            last_dp.gyro_z |= I2caRegs.I2CDRR.bit.DATA;
            I2caRegs.I2CMDR.bit.STP = 1;
            update_accel_gyro_i2c_state = STOP_GYRO;
            break;
        default:
            break;
        }
        break;
    case STOP_GYRO:
        if(calibration_count > 0){
            sum_gyro_x += last_dp.gyro_x;
            sum_gyro_y += last_dp.gyro_y;
            sum_gyro_z += last_dp.gyro_z;
            sum_accel_x += last_dp.accel_x;
            sum_accel_y += last_dp.accel_y;
            sum_accel_z += (last_dp.accel_z - ACCEL_RES);
            calibration_count--;
            if(calibration_count == 0){
                calibration_dp.gyro_x = sum_gyro_x / (MPU6050_CALIBRATION_COUNT);
                calibration_dp.gyro_y = sum_gyro_y / (MPU6050_CALIBRATION_COUNT);
                calibration_dp.gyro_z = sum_gyro_z / (MPU6050_CALIBRATION_COUNT);
                calibration_dp.accel_x = sum_accel_x / (MPU6050_CALIBRATION_COUNT);
                calibration_dp.accel_y = sum_accel_y / (MPU6050_CALIBRATION_COUNT);
                calibration_dp.accel_z = sum_accel_z / (MPU6050_CALIBRATION_COUNT);
                current_gyro_angle = MPU6050_GetAccelAngle();
            }
        }
        else{
            // calculate current tile angle
            float delta_gyro_y_angle = (float)(last_dp.gyro_y - calibration_dp.gyro_y) * TIMER0_PER / GYRO_RES;
            float accel_angle = MPU6050_GetAccelAngle();
            float adjusted_gyro_z_reading = last_dp.gyro_z - calibration_dp.gyro_z;
            current_gyro_angle = (current_gyro_angle + delta_gyro_y_angle - adjusted_gyro_z_reading * 0) * 0.9990 + (accel_angle) * 0.0010;
            update_accel_gyro_i2c_state = IDLEING;
        }
        break;
    default:
        break;
    }

    I2caRegs.I2CSTR.all |= I2C_INT_RX_DATA_RDY | I2C_INT_STOP_CONDITION;
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP8);
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

    // initiate I2C communication to read accel
    update_accel_gyro_i2c_state = SEND_ACCEL_RA;
    update_accel_gyro_i2c_counter = 0;
    // Set to Master, Repeat Mode, TRX, FREE, Start
    I2caRegs.I2CMDR.all = 0x66A0;
    // send MPU6050 internal register address
    I2caRegs.I2CDXR.bit.DATA = RA_ACCEL_XOUT_H;
    update_accel_gyro_i2c_state = SWITCH_TO_RX_ACCEL;

    CPUTimer_clearOverflowFlag(CPUTIMER0_BASE);
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP1);
}
