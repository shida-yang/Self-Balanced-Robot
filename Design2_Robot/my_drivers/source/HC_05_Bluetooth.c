/*
 * HC_05_Bluetooth.c
 *
 *  Created on: Sep 17, 2020
 *      Author: syang
 */

// Communicate with the HC-05 Bluetooth module using SCIA
#include "HC_05_Bluetooth.h"

// two receive buffers for Bluetooth
char BT_RX_BUF[2][RX_BUF_LEN];
// two transmit buffers for Bluetooth
char BT_TX_BUF[2][TX_BUF_LEN];

// indicate which buf is currently busy
bool curr_filling_rx_buf, curr_reading_tx_buf;

// index used by ISRs to fill/read buf
uint16_t rx_filling_index, tx_reading_index;

// locks to control receive/transmit flow
bool rx_buf_ready[2];
bool tx_buf_ready[2];

void HC_05_init(){
    HC_05_GPIO_init();
    HC_05_SCI_init();

    curr_filling_rx_buf = 0;
    curr_reading_tx_buf = 0;

    rx_filling_index = 0;
    tx_reading_index = 0;

    rx_buf_ready[0] = 0;
    rx_buf_ready[1] = 0;
    tx_buf_ready[0] = 1;
    tx_buf_ready[1] = 1;
}

bool HC_05_read_string(char* data_str){
    bool ret_val = true;

    // store curr_reading_tx_buf to prevent change during operation
    bool temp_curr_filling_rx_buf = curr_filling_rx_buf;

    // if there is no new data to read, return fail to indicate NO DATA READ
    if(rx_buf_ready[!temp_curr_filling_rx_buf] == 0){
        ret_val = false;
    }

    // determine which rx buf is completed
    char* completed_rx_buf = BT_RX_BUF[!temp_curr_filling_rx_buf];

    // copy string byte-by-byte
    int i = 0;
    while(completed_rx_buf[i] != '\0' && i < 31){
        data_str[i] = completed_rx_buf[i];
        i++;
    }
    // fill in terminating NULL
    data_str[i] = '\0';

    rx_buf_ready[temp_curr_filling_rx_buf] = 0;
    return ret_val;
}

bool HC_05_send_string(char* data_str){
    bool ret_val = true;

    // store curr_reading_tx_buf to prevent change during operation
    bool temp_curr_reading_tx_buf = curr_reading_tx_buf;

    // if current buf is not ready, return fail to indicate OVERWRITTING data
    if(tx_buf_ready[!temp_curr_reading_tx_buf] == 0){
        ret_val = false;
    }

    // determine which tx buf is completed
    char* completed_tx_buf = BT_TX_BUF[!temp_curr_reading_tx_buf];

    // copy string byte-by-byte
    int i = 0;
    while(data_str[i] != '\0' && i < 31){
        completed_tx_buf[i] = data_str[i];
        // indicate new data is available for transfer in the buf
        tx_buf_ready[!temp_curr_reading_tx_buf] = 0;
        i++;
    }
    // fill in terminating NULL
    completed_tx_buf[i] = '\0';
    // indicate new data is available for transfer in the buf
    tx_buf_ready[!temp_curr_reading_tx_buf] = 0;

    // if the SCI transmitter is ready, initiate a new transmission
    if(SciaRegs.SCICTL2.bit.TXRDY == 1){
        // transmit only when the current buf contains more than NULL
        if(i != 0){
            curr_reading_tx_buf = !temp_curr_reading_tx_buf;
            HC_05_send_byte(completed_tx_buf[0]);
            tx_reading_index = 1;
        }
    }

    return ret_val;
}

__interrupt void HC_05_RX_ISR(void){
    // determine which rx buf is being filled
    char* filling_rx_buf = BT_RX_BUF[curr_filling_rx_buf];

    // read current byte from SCIRX buffer
    char curr_byte = HC_05_read_byte();

    // if buffer full, don't fill in new data
    if(rx_filling_index == RX_BUF_LEN - 1){
        filling_rx_buf[rx_filling_index] = '\0';
    }
    // if still have space in buffer, fill in current byte
    else{
        filling_rx_buf[rx_filling_index] = curr_byte;
        rx_filling_index++;
        if(curr_byte == RX_TERM_CHAR){
            filling_rx_buf[rx_filling_index] = '\0';
            rx_filling_index++;
        }

    }

    // if current byte is NULL, switch to another buffer
    if(curr_byte == '\0' || curr_byte == RX_TERM_CHAR){
        rx_buf_ready[curr_filling_rx_buf] = 1;
        curr_filling_rx_buf = !curr_filling_rx_buf;
        rx_filling_index = 0;
    }

    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP9);
}

__interrupt void HC_05_TX_ISR(void){
    // do not transmit when this buffer does not have new data
    if(tx_buf_ready[curr_reading_tx_buf] == 1){
        curr_reading_tx_buf = !curr_reading_tx_buf;
        Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP9);
        return;
    }

    // determine which tx buf is being filled
    char* reading_tx_buf = BT_TX_BUF[curr_reading_tx_buf];

    char curr_byte;

    // if reaches the end of table, transmit NULL
    if(tx_reading_index == TX_BUF_LEN - 1){
        curr_byte = '\0';
    }
    // if still have data in buffer, transmit the data
    else{
        curr_byte = reading_tx_buf[tx_reading_index];
        tx_reading_index++;
    }

    // if current byte is NULL, switch to another buffer
    if(curr_byte == '\0'){
        // mark there is no data in the buf
        tx_buf_ready[curr_reading_tx_buf] = 1;
        curr_reading_tx_buf = !curr_reading_tx_buf;
        tx_reading_index = 0;
    }

    // write current byte to SCITX buffer
    HC_05_send_byte(curr_byte);

    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP9);
}

// GPIO8 = SCITXDA(O)
// GPIO9 = SCIRXDA(I)
// GPIO10 = HC-05 EN (O)
// GPIO11 = HC-05 STATE (I)
static void HC_05_GPIO_init(){
    // enable pullups
    GpioCtrlRegs.GPAPUD.bit.GPIO8 = 0;
    GpioCtrlRegs.GPAPUD.bit.GPIO9 = 0;
    GpioCtrlRegs.GPAPUD.bit.GPIO10 = 0;
    GpioCtrlRegs.GPAPUD.bit.GPIO11 = 0;

    // GPIO8 = SCITXDA(O)
    // GPIO9 = SCIRXDA(I)
    GpioCtrlRegs.GPAGMUX1.bit.GPIO8 = 1;
    GpioCtrlRegs.GPAGMUX1.bit.GPIO9 = 1;
    GpioCtrlRegs.GPAMUX1.bit.GPIO8 = 2;
    GpioCtrlRegs.GPAMUX1.bit.GPIO9 = 2;

    // asynch qual
    GpioCtrlRegs.GPAQSEL1.bit.GPIO9 = 3;
    GpioCtrlRegs.GPAQSEL1.bit.GPIO11 = 3;

    // direction of EN and STATE
    GpioCtrlRegs.GPADIR.bit.GPIO10 = 1;
    GpioCtrlRegs.GPADIR.bit.GPIO11 = 0;
}

/*
 * HC-05 has default command baud = 38400 and default data baud = 9600
 *
 *  strcpy(data, "AT+UART=115200,0,0\r\n");
 *  HC_05_send_string(data);
 *  DELAY_US(100000);
 *  HC_05_read_string(data2);
 *
 *  These change the data baud to 115200
 */
static void HC_05_SCI_init(){
    SciaRegs.SCICTL1.bit.SWRESET = 0;

    // set BAUD rate to 9600
//    SciaRegs.SCILBAUD.all = 650 & 0xFF;
//    SciaRegs.SCIHBAUD.all = 650 >> 8;

    // set BAUD to 38400
//    SciaRegs.SCILBAUD.all = 162;
//    SciaRegs.SCIHBAUD.all = 0;

    // set BAUD to 115200
    SciaRegs.SCILBAUD.all = 53;
    SciaRegs.SCIHBAUD.all = 0;

    // 8 data bit, 1 stop bit, no parity
    SciaRegs.SCICCR.bit.SCICHAR = 7;
    SciaRegs.SCICCR.bit.STOPBITS = 0;
    SciaRegs.SCICCR.bit.PARITYENA = 0;

    // allow SCI to complete current transfer if hits breakpoint
    SciaRegs.SCIPRI.bit.FREESOFT = 1;

    // enable receive interrupt
    SciaRegs.SCICTL2.bit.RXBKINTENA = 1;
    Interrupt_register(INT_SCIA_RX, &HC_05_RX_ISR);
    Interrupt_enable(INT_SCIA_RX);

    // enable transmit interrupt
    SciaRegs.SCICTL2.bit.TXINTENA = 1;
    Interrupt_register(INT_SCIA_TX, &HC_05_TX_ISR);
    Interrupt_enable(INT_SCIA_TX);

    // enable TX and RX
    SciaRegs.SCICTL1.bit.TXENA = 1;
    SciaRegs.SCICTL1.bit.RXENA = 1;

    // end software reset
    SciaRegs.SCICTL1.bit.SWRESET = 1;
}

static char HC_05_read_byte(){
    return SciaRegs.SCIRXBUF.all;
}

static void HC_05_send_byte(char data){
    SciaRegs.SCITXBUF.all = data;
}
