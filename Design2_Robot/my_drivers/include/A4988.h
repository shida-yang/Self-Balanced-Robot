#include <F28x_Project.h>
#include "driverlib.h"

#define LEFT_DIR_PIN        95
#define LEFT_STEP_PIN       139
#define RIGHT_DIR_PIN       52
#define RIGHT_STEP_PIN      97
#define A4988_ENABLE_PIN    94
#define A4988_SLEEP_PIN     65

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
