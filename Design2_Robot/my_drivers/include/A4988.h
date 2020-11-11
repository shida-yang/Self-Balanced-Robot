#include <F28x_Project.h>
//#include "driverlib.h"

#define RIGHT_DIR_PIN        95
#define RIGHT_STEP_PIN       139
#define LEFT_DIR_PIN       40
#define LEFT_STEP_PIN      41
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

/*
 * GPIO Codes
 */
#define GPIOCTRL_BASE               0x00007C00U // GPIO 0 to 31 Mux A Configuration Registers
#define GPIO_O_GPBCTRL              0x40U
#define GPIO_O_GPACTRL              0x0U
#define GPIO_CTRL_REGS_STEP         ((GPIO_O_GPBCTRL - GPIO_O_GPACTRL) / 2U)
#define GPIO_O_GPADIR               0xAU
#define GPIO_GPxDIR_INDEX           (GPIO_O_GPADIR / 2U)

#define GPIODATA_BASE               0x00007F00U // GPIO 0 to 31 Mux A Data Registers
#define GPIO_O_GPBDAT               0x8U
#define GPIO_O_GPADAT               0x0U
#define GPIO_DATA_REGS_STEP         ((GPIO_O_GPBDAT - GPIO_O_GPADAT) / 2U)
#define GPIO_O_GPASET               0x2U
#define GPIO_O_GPACLEAR             0x4U
#define GPIO_GPxSET_INDEX           (GPIO_O_GPASET / 2U)
#define GPIO_GPxCLEAR_INDEX         (GPIO_O_GPACLEAR / 2U)


static void my_GPIO_setDirectionMode(uint32_t pin, uint16_t pinIO)
{
    volatile uint32_t *gpioBaseAddr;
    uint32_t pinMask;

    gpioBaseAddr = (uint32_t *)GPIOCTRL_BASE +
                   ((pin / 32U) * GPIO_CTRL_REGS_STEP);
    pinMask = (uint32_t)1U << (pin % 32U);

    EALLOW;

    //
    // Set the data direction
    //
    if(pinIO == 1)
    {
        //
        // Output
        //
        gpioBaseAddr[GPIO_GPxDIR_INDEX] |= pinMask;
    }
    else
    {
        //
        // Input
        //
        gpioBaseAddr[GPIO_GPxDIR_INDEX] &= ~pinMask;
    }

    EDIS;
}

static inline void my_GPIO_writePin(uint32_t pin, uint32_t outVal)
{
    volatile uint32_t *gpioDataReg;
    uint32_t pinMask;

    gpioDataReg = (uint32_t *)((uintptr_t)GPIODATA_BASE) +
                  ((pin / 32U) * GPIO_DATA_REGS_STEP);

    pinMask = (uint32_t)1U << (pin % 32U);

    if(outVal == 0U)
    {
        gpioDataReg[GPIO_GPxCLEAR_INDEX] = pinMask;
    }
    else
    {
        gpioDataReg[GPIO_GPxSET_INDEX] = pinMask;
    }
}
