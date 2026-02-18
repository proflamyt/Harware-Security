#define _XOPEN_SOURCE 600
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <pty.h>

#define BAUD 9600

enum {
    TX_IDLE,
    TX_START,
    TX_DATA,
    TX_PARITY,
    TX_STOP
};

volatile sig_atomic_t tx_line = 1;   // idle = HIGH

const uint8_t msg[] = "flag";

int tx_state = TX_IDLE;
uint8_t tx_byte = 0;
int parity = 0;
int bit_pos = 0;
int msg_pos = 0;

int master, slave;
char slave_name[100];

static int even_parity(uint8_t b) {
    int p = 0;
    while (b) {
        p ^= 1;
        b &= b - 1;
    }
    return p;
}

void timer_handler(int sig) {
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

int main(void) {

    struct sigaction sa_timer;
    struct itimerval timer;

    memset(&sa_timer, 0, sizeof(sa_timer));
    sa_timer.sa_handler = timer_handler;
    sigemptyset(&sa_timer.sa_mask);
    sa_timer.sa_flags = SA_RESTART;

    sigaction(SIGALRM, &sa_timer, NULL);

    /* bit duration */
    int bit_us = 1000000 / BAUD;

    // timer.it_value.tv_sec = 0;
    // timer.it_value.tv_usec = bit_us;
    // timer.it_interval.tv_sec = 0;
    // timer.it_interval.tv_usec = bit_us;

    timer.it_value.tv_sec = 5;
    timer.it_value.tv_usec = 0;

    timer.it_interval.tv_sec = 5;
    timer.it_interval.tv_usec = 0;

    setitimer(ITIMER_REAL, &timer, NULL);

    if (openpty(&master, &slave, slave_name, NULL, NULL) == -1) {
        perror("openpty");
        exit(1);
    }

    printf("UART TX on PTY: %s\n", slave_name);

    while (1) {
        char c = tx_line ? '1' : '0';
        write(1, &c, 1);
        //usleep(bit_us);   // pacing output
        usleep(1000000);
    }
}
