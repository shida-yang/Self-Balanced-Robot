#include <F28x_Project.h>
#include "driverlib.h"

#define LEFT_DIR_PIN        44
#define LEFT_STEP_PIN       45
#define RIGHT_DIR_PIN       46
#define RIGHT_STEP_PIN      47
#define A4988_ENABLE_PIN    48
#define A4988_SLEEP_PIN     49

typedef enum MOTOR_SIDE{
    LEFT,
    RIGHT
} MOTOR_SIDE_t;

typedef enum MOTOR_DIR{
    FORWARD = 1,
    BACKWARD = 0
} MOTOR_DIR_t;

void A4988_INIT();
void set_dir(MOTOR_SIDE_t side, MOTOR_DIR_t dir);
void step_high(MOTOR_SIDE_t side);
void step_low(MOTOR_SIDE_t side);
void enable_motors();
void disable_motors();
void wake_motors();
void sleep_motors();
