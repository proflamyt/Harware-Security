#define _XOPEN_SOURCE 600
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <pty.h>

int sclk, mosi, tx_line, miso, cs = 0;
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


sclk = 0; // every change to 5 volt 
void start_transfer() {
    cs = 0;
}


void clock_handler(int sig) {
    if (!cs) return;
    sclk = !sclk;
    if (sclk) trigger_data();

}

void sigpwr_handler(int signo)
{
    if (signo == SIGPWR)
    {
        cs = 1;
    }
}

static int even_parity(uint8_t b) {
    int p = 0;
    while (b) {
        p ^= 1;
        b &= b - 1;
    }
    return p;
}


void trigger_data() {
    // On a defined clock edge, the shift register updates MOSI.
 
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
    struct sigaction sa;

    sa.sa_handler = sigpwr_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGPWR, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

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
        char d = tx_line ? '1' : '0';
        char c = sclk ? '1' : '0';
        write(1, &c, 1);
        write(1, &d, 1);
        //usleep(bit_us);   // pacing output, 
        usleep(1000000);
    }

}