#include <F28x_Project.h>
#include "HC_05_Bluetooth.h"
#include "MPU6050.h"
#include "A4988.h"
#include "interrupt.h"

//#define PID_P   70
//#define PID_I   0
//#define PID_D   0

float PID_P = 400;
float PID_I = 15;
float PID_D = 200;

#define A4988_PULSE_ON_US       20
#define PID_UPDATE_TIME_US      4000
#define MAIN_LOOP_PERIOD_COUNT  (PID_UPDATE_TIME_US/A4988_PULSE_ON_US)
#define tilt_ANGLE_BT_UPDATE_TIME_US    500000
#define tilt_ANGLE_BT_UPDATE_COUNT      (tilt_ANGLE_BT_UPDATE_TIME_US/A4988_PULSE_ON_US)

#define MIN_TIME_BETWEEN_PULSE  200
//#define PID_OUT_IGNORE_THRESHOLD    1

float PID_OUT_IGNORE_THRESHOLD = 100;
float SELF_ADJUST_COEFFICIENT = 0;
float PID_I_ACCUM_CAP = 350;
float BRAKE_COEFFICIENT = 0;

void init_main_timer();
__interrupt void cpuTimer1ISR(void);

int inttostring(char str[], int num);
bool parse_js_values();

char BT_SEND_BUF[32];
char BT_RECEIVE_BUF[32];

float pid_i_accum, pid_out, out_left, out_right;
float tilt_angle, balance_angle, angle_stop_adjustment, error, last_error;
int32_t left_pulse, left_pulse_count, left_pulse_target,
        right_pulse, right_pulse_count, right_pulse_target;
float js_angle = 0, js_force = 0;

uint32_t system_counter;
bool main_loop_flag, force_update_pulse_target, bt_send_tilt_angle_flag;

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
    A4988_INIT();

    //enable global interrupt
    Interrupt_enableMaster();

    init_main_timer();

    system_counter = 0;

    balance_angle = 0;
    angle_stop_adjustment = 0;
    pid_i_accum = 0;
    last_error = 0;
    // initiate pulse counts to skip the first iteration
    left_pulse_count = 3;
    right_pulse_count = 3;
    // make the first pulse run 10ms empty to wait for initial motor output
    left_pulse_target = 50000 / A4988_PULSE_ON_US;
    right_pulse_target = 50000 / A4988_PULSE_ON_US;

    main_loop_flag = 1;
    force_update_pulse_target = 0;
    bt_send_tilt_angle_flag = 0;

    while(1){
        if(main_loop_flag){
            // get current tilt angle
            tilt_angle = MPU6050_GetPitchAngle();

            // calculate PID output
            error = tilt_angle - balance_angle + angle_stop_adjustment;
//            if(fabs(pid_out)>(PID_OUT_IGNORE_THRESHOLD+10))
                error += pid_out * BRAKE_COEFFICIENT ;

            pid_i_accum += PID_I * error;
            if(pid_i_accum>PID_I_ACCUM_CAP) pid_i_accum = PID_I_ACCUM_CAP;
            else if(pid_i_accum<-PID_I_ACCUM_CAP) pid_i_accum = -PID_I_ACCUM_CAP;

            if(pid_out == 0){
                pid_out = PID_P * error + pid_i_accum + PID_D * (error - last_error);
                if(fabs(pid_out)<PID_OUT_IGNORE_THRESHOLD){
                    pid_out=0;
                }
                else{
                    force_update_pulse_target = 1;
                }
            }
            else{
                pid_out = PID_P * error + pid_i_accum + PID_D * (error - last_error);
                if(fabs(pid_out)<PID_OUT_IGNORE_THRESHOLD){
                    pid_out=0;
                }
            }



            // calculate left output
            out_left = pid_out;

            // calculate right output
            out_right = pid_out;

            // calculate left pulses
            left_pulse = 500000 / A4988_PULSE_ON_US / out_left;

            // calculate right pulses
            right_pulse = 100000 / A4988_PULSE_ON_US / out_right;

            if(force_update_pulse_target){
                force_update_pulse_target = 0;
                if(left_pulse < 0){
                    set_dir(LEFT, BACKWARD);
                    set_dir(RIGHT, BACKWARD);
                    left_pulse_target = left_pulse<(-MIN_TIME_BETWEEN_PULSE/A4988_PULSE_ON_US)?-left_pulse:(MIN_TIME_BETWEEN_PULSE/A4988_PULSE_ON_US); // update target
                    right_pulse_target = left_pulse_target;
                }
                else{
                    set_dir(LEFT, FORWARD);
                    set_dir(RIGHT, FORWARD);
                    left_pulse_target = left_pulse>(MIN_TIME_BETWEEN_PULSE/A4988_PULSE_ON_US)?left_pulse:(MIN_TIME_BETWEEN_PULSE/A4988_PULSE_ON_US); // update target
                    right_pulse_target = left_pulse_target;
                }

//                if(right_pulse < 0){
//                    right_pulse_target = right_pulse<(-MIN_TIME_BETWEEN_PULSE/A4988_PULSE_ON_US)?-right_pulse:(MIN_TIME_BETWEEN_PULSE/A4988_PULSE_ON_US); // update target
//                }
//                else{
//                    right_pulse_target = right_pulse>(MIN_TIME_BETWEEN_PULSE/A4988_PULSE_ON_US)?right_pulse:(MIN_TIME_BETWEEN_PULSE/A4988_PULSE_ON_US); // update target
//                }
            }

            last_error = error;

            if(pid_out!=0)
                angle_stop_adjustment -= fabs(pid_out)/pid_out*SELF_ADJUST_COEFFICIENT;

            // update JS value
            HC_05_read_string(BT_RECEIVE_BUF);
            parse_js_values();

            main_loop_flag = 0;
        }

        if(bt_send_tilt_angle_flag){
            float temp_tilt_angle = tilt_angle;
            char sign;
            // get sign
            if(temp_tilt_angle < 0){
                sign = '-';
                temp_tilt_angle = -temp_tilt_angle;
            }
            else{
                sign = ' ';
            }
            // get int part
            uint16_t int_part = (uint16_t)temp_tilt_angle;
            // get decimal part
            uint16_t decimal_part = (uint16_t)((temp_tilt_angle - (float)int_part)*1000);
            uint16_t pos = 0;
            BT_SEND_BUF[0] = sign;
            pos++;
            pos += inttostring(&BT_SEND_BUF[pos], int_part);
            BT_SEND_BUF[pos] = '.';
            pos++;
            if(decimal_part < 100){
                BT_SEND_BUF[pos] = '0';
                pos++;
            }
            if(decimal_part < 10){
                BT_SEND_BUF[pos] = '0';
                pos++;
            }
            pos += inttostring(&BT_SEND_BUF[pos], decimal_part);
            BT_SEND_BUF[pos] = '\0';
            HC_05_send_string(BT_SEND_BUF);
            bt_send_tilt_angle_flag = 0;
        }

    }

    return 0;
}

void init_main_timer(){
    CPUTimer_stopTimer(CPUTIMER1_BASE);

    // set timer to overflow every A4988_PULSE_ON_US us
    CPUTimer_setPreScaler(CPUTIMER1_BASE, 199);
    CPUTimer_setPeriod(CPUTIMER1_BASE, A4988_PULSE_ON_US - 1);

    // enable timer1 interrupt
    Interrupt_register(INT_TIMER1, &cpuTimer1ISR);

    CPUTimer_enableInterrupt(CPUTIMER1_BASE);
    Interrupt_enable(INT_TIMER1);

    CPUTimer_startTimer(CPUTIMER1_BASE);
}

__interrupt void cpuTimer1ISR(void){
    system_counter ++;
    if(system_counter % MAIN_LOOP_PERIOD_COUNT == 0){
        main_loop_flag = 1;
    }

    if(system_counter % tilt_ANGLE_BT_UPDATE_COUNT == 0){
        bt_send_tilt_angle_flag = 1;
    }

    // handle left pulses
    // need to handle this first in case of 0 target
    if(left_pulse_count >= left_pulse_target){
        left_pulse_count = 0;           // reset count
        // update direction
        if(left_pulse < 0){
            set_dir(LEFT, BACKWARD);
            set_dir(RIGHT, BACKWARD);
            left_pulse_target = left_pulse<(-MIN_TIME_BETWEEN_PULSE/A4988_PULSE_ON_US)?-left_pulse:(MIN_TIME_BETWEEN_PULSE/A4988_PULSE_ON_US); // update target
            right_pulse_target = left_pulse_target;
        }
        else{
            set_dir(LEFT, FORWARD);
            left_pulse_target = left_pulse>(MIN_TIME_BETWEEN_PULSE/A4988_PULSE_ON_US)?left_pulse:(MIN_TIME_BETWEEN_PULSE/A4988_PULSE_ON_US); // update target
            set_dir(RIGHT, FORWARD);
            right_pulse_target = left_pulse_target;
        }
    }
    else if(left_pulse_count == 1){
        step_high(LEFT);
        step_high(RIGHT);
    }
    else if(left_pulse_count == 2){
        step_low(LEFT);
        step_low(RIGHT);
    }
    left_pulse_count ++;

    // handle right pulses
    // need to handle this first in case of 0 target
//    if(right_pulse_count >= right_pulse_target){
//        right_pulse_count = 0;           // reset count
//        // update direction
//        if(right_pulse < 0){
//            set_dir(RIGHT, BACKWARD);
//            right_pulse_target = right_pulse<(-MIN_TIME_BETWEEN_PULSE/A4988_PULSE_ON_US)?-right_pulse:(MIN_TIME_BETWEEN_PULSE/A4988_PULSE_ON_US); // update target
//        }
//        else{
//            set_dir(RIGHT, FORWARD);
//            right_pulse_target = right_pulse>(MIN_TIME_BETWEEN_PULSE/A4988_PULSE_ON_US)?right_pulse:(MIN_TIME_BETWEEN_PULSE/A4988_PULSE_ON_US); // update target
//        }
//    }
//    else if(right_pulse_count == 1){
//        step_high(RIGHT);
//    }
//    else if(right_pulse_count == 2){
//        step_low(RIGHT);
//    }
//    right_pulse_count ++;
}

int inttostring(char str[], int num)
{
    int i, rem, len = 0, n;

    if(num == 0){
        str[0] = '0';
        return 1;
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
    return len;
}

bool parse_js_values(){
    int i=0;
    bool parsing_angle = 1;

    float temp_angle = 0, temp_force = 0;

    // check starting parenthesis
    if(BT_RECEIVE_BUF[i] != '('){
        return false;
    }
    i++;

    // loop till the end
    while(BT_RECEIVE_BUF[i] != NULL){
        switch(BT_RECEIVE_BUF[i]){
        case ')':
            // no ',' met, wrong format
            if(parsing_angle){
                return false;
            }
            break;
        case ',':
            parsing_angle = 0;
            break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            if(parsing_angle){
                temp_angle = temp_angle * 10 + (BT_RECEIVE_BUF[i] - '0');
            }
            else{
                temp_force = temp_force * 10 + (BT_RECEIVE_BUF[i] - '0');
            }
            break;
        // invalid char
        default:
            return false;
        }
        i++;
    }

    // value out of range
    if(temp_angle > 359 || temp_angle <0 || temp_force > 100 || temp_force < 0){
        return false;
    }

    js_angle = temp_angle;
    js_force = temp_force;
    return true;
}
