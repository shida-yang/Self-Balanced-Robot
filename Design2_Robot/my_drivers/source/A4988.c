#include "A4988.h"

void A4988_INIT(){
    // all pins are output
    my_GPIO_setDirectionMode(LEFT_DIR_PIN, 1);
    my_GPIO_setDirectionMode(LEFT_STEP_PIN, 1);
    my_GPIO_setDirectionMode(RIGHT_DIR_PIN, 1);
    my_GPIO_setDirectionMode(RIGHT_STEP_PIN, 1);
    my_GPIO_setDirectionMode(A4988_ENABLE_PIN, 1);
    my_GPIO_setDirectionMode(A4988_SLEEP_PIN, 1);

    // set default values of pins
    set_dir(LEFT, FORWARD);
    set_dir(RIGHT, FORWARD);
    step_low(LEFT);
    step_low(RIGHT);
    wake_motors();
    DELAY_US(5000);
    enable_motors();

}

void set_dir(MOTOR_SIDE_t side, MOTOR_DIR_t dir){
    if(side == LEFT){
        my_GPIO_writePin(LEFT_DIR_PIN, dir);
    }
    else{
        my_GPIO_writePin(RIGHT_DIR_PIN, dir);
    }
}

void step_high(MOTOR_SIDE_t side){
    if(side == LEFT){
        my_GPIO_writePin(LEFT_STEP_PIN, 1);
    }
    else{
        my_GPIO_writePin(RIGHT_STEP_PIN, 1);
    }
}

void step_low(MOTOR_SIDE_t side){
    if(side == LEFT){
        my_GPIO_writePin(LEFT_STEP_PIN, 0);
    }
    else{
        my_GPIO_writePin(RIGHT_STEP_PIN, 0);
    }
}

void enable_motors(){
    my_GPIO_writePin(A4988_ENABLE_PIN, 0);
}

void disable_motors(){
    my_GPIO_writePin(A4988_ENABLE_PIN, 1);
}

void wake_motors(){
    my_GPIO_writePin(A4988_SLEEP_PIN, 1);
}

void sleep_motors(){
    my_GPIO_writePin(A4988_SLEEP_PIN, 0);
}
