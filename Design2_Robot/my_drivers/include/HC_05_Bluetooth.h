/*
 * HC_05_Bluetooth.h
 *
 *  Created on: Sep 17, 2020
 *      Author: syang
 */

//#ifndef MY_DRIVERS_INCLUDE_HC_05_BLUETOOTH_H_
//#define MY_DRIVERS_INCLUDE_HC_05_BLUETOOTH_H_

#include <F28x_Project.h>
#include "interrupt.h"

#define RX_BUF_LEN 32
#define TX_BUF_LEN 32

void HC_05_init();
bool HC_05_read_string(char* data_str);
bool HC_05_send_string(char* data_str);

__interrupt void HC_05_RX_ISR(void);
__interrupt void HC_05_TX_ISR(void);

static void HC_05_GPIO_init();
static void HC_05_SCI_init();
static char HC_05_read_byte();
static void HC_05_send_byte(char data);

//#endif /* MY_DRIVERS_INCLUDE_HC_05_BLUETOOTH_H_ */
