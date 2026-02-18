#define _XOPEN_SOURCE 600
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <pty.h>

int sclk, mosi, miso, cs = 1;


sclk = 0; // every change to 5 volt 
void start_transfer() {
    cs = 0;
}


void clock_handler(int sig) {
    sclk = !sclk;
    if (sclk) trigger_data();

}


void trigger_data() {
    // On a defined clock edge, the shift register updates MOSI.
    (void)sig;

    switch (tx_state) {

        case TX_IDLE:
            tx_line = 1;  // idle HIGH
            if (msg_pos < sizeof(msg) - 1) {
                tx_byte = msg[msg_pos++];
                parity = even_parity(tx_byte);
                tx_state = TX_START;
            }
            break;

        case TX_START:
            tx_line = 0;
            bit_pos = 0;
            tx_state = TX_DATA;
            break;

        case TX_DATA:
            tx_line = (tx_byte >> bit_pos) & 1;
            bit_pos++;
            if (bit_pos == 8)
                tx_state = TX_PARITY;
            break;

        case TX_PARITY:
            tx_line = parity;
            tx_state = TX_STOP;
            break;

        case TX_STOP:
            tx_line = 1;
            tx_state = TX_IDLE;
            break;
    }
}

int main() {

    struct sigaction sa_timer;
    struct itimerval timer;

    memset(&sa_timer, 0, sizeof(sa_timer));
    sa_timer.sa_handler = clock_handler;
    sigemptyset(&sa_timer.sa_mask);
    sa_timer.sa_flags = SA_RESTART;

    sigaction(SIGALRM, &sa_timer, NULL);

    timer.it_value.tv_sec = 5;
    timer.it_value.tv_usec = 0;

    timer.it_interval.tv_sec = 5;
    timer.it_interval.tv_usec = 0;

    setitimer(ITIMER_REAL, &timer, NULL);

    while (1) {
        char c = tx_line ? '1' : '0';
        write(1, &c, 1);
        //usleep(bit_us);   // pacing output
        usleep(1000000);
    }

}