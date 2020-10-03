

/**
 * main.c
 */

#include <F28x_Project.h>
#include "HC_05_Bluetooth.h"
#include "MPU6050.h"
#include "interrupt.h"

#include "string.h"

char data[32];
char data2[32];
bool send_success;

MPU6050_Data_Packet_t dp;
float angle;

void tostring(char str[], int num)
{
    int i, rem, len = 0, n;

    if(num == 0){
        str[0] = '0';
        str[1] = '\n';
        str[2] = '\r';
        str[3] = '\0';
        return;
    }

    n = num;
    while (n != 0)
    {
        len++;
        n /= 10;
    }
    for (i = 0; i < len; i++)
    {
        rem = num % 10;
        num = num / 10;
        str[len - (i + 1)] = rem + '0';
    }
    str[len] = '\n';
    str[len + 1] = '\r';
    str[len + 2] = '\0';
}

int main(void){

    //init system clocks and get board speed running at 200 MHz
    InitSysCtrl();
    EALLOW;
    //disable all interrupt
    Interrupt_initModule();
    //init interrupt table
    Interrupt_initVectorTable();
    // initialize the timer structs and setup the timers in their default states
    InitCpuTimers();

    EALLOW;
    HC_05_init();

    MPU6050_Powerup();

    //enable global interrupt
    Interrupt_enableMaster();

    while(1){
        // testing HC-05
//         uint8_t i = 0;
//         for(i=0; i<256; i++){
//             tostring(data, i);
//             send_success = HC_05_send_string(data);
//             if(i%40==0)HC_05_read_string(data2);
//             DELAY_US(1000);
//         }

        // testing MPU6050
        angle = MPU6050_GetPitchAngle();
        DELAY_US(5000);
    }

    return 0;
}

