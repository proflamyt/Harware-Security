#define _XOPEN_SOURCE 600
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <pty.h>
#include <stdbool.h>

/* ── The two I2C wires (just variables in our simulation) ── */
int SDA = 1;   /* data  wire: 1 = HIGH, 0 = LOW */
int SCL = 1;   /* clock wire: 1 = HIGH, 0 = LOW */
int SWITCH_SCL =  1, ack = 0;
int bit_pos = 0;

enum {
    START,
    DATA,
    ACK,
    STOP
};

char msg[] = "\x90flag{practice}"
int tx_state =  START;
char BYTE;
int msg_pos = 0;
int write = 0;
bool trigger_stop = false;




void clock_handler(int sig) {
    if (SWITCH_SCL) return;
    if (SCL && trigger_stop) {
        SDA = 1;
        SWITCH_SCL = 1;
    }
    SCL = !SCL;
    if (!SCL) trigger_data();
}

void trigger_data() {
    switch (tx_state) {
        case START:
            SDA = 1; SCL = 1;   /* both high = bus idle */
            printf("[START] SDA drops LOW while SCL is HIGH\n");
            SDA = 0;           
            SWITCH_SCL = 0; 
            tx_state = DATA; 
            BYTE = msg[msg_pos];
            break;
// On a defined clock edge, the shift register updates MOSI.
        case DATA:
            if (bit_pos == 8) {
                BYTE = msg[msg_pos++];
                tx_state = ACK;
                write = true;
                bit_pos = 0;
            }
            SDA = (BYTE >> bit_pos) & 1;
            bit_pos++;
            break;
        /* ── ACK: receiver pulls SDA LOW for one clock pulse to say "got it" ── */
        case ACK:
            if (ack && msg_pos >= strlen(msg) - 1) {
                tx_state = STOP;
                break;
            }
            if (ack) {
                SDA = 1; 
                tx_state = DATA;
                break;
            }
            SDA = 0
            ack = 1
            break;
        
        /* ── STOP condition: SDA rises HIGH while SCL is HIGH ── */
        case STOP:
            trigger_stop = true;
            break;
    }
}



/* ═══════════════════════════════════════════════════════════
 *  MAIN — runs a simple I2C transaction
 *
 *  We send:
 *    1. START
 *    2. Address byte  (slave address 0x48 + WRITE bit = 0x90)
 *    3. ACK
 *    4. Data byte 1   (0xAB)
 *    5. ACK
 *    6. Data byte 2   (0x4F)
 *    7. ACK
 *    8. STOP
 * ═══════════════════════════════════════════════════════════ */
int main(void)
{

    struct sigaction sa_timer;
    struct itimerval timer;
    struct sigaction sa;

    printf("╔══════════════════════════════════════════╗\n");
    printf("║   Simple I2C Bit-by-Bit Simulation       ║\n");
    printf("╚══════════════════════════════════════════╝\n\n");


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

    /* ── Step 1: START ── */
    i2c_start();

    /* ── Step 2: Address byte ──
     * I2C address is 7 bits. The 8th bit (LSB) is the R/W flag.
     * Slave address = 0x48 = 0b1001000
     * Shift left by 1:      0b10010000 = 0x90
     * OR with 0 (WRITE):    0b10010000 = 0x90  ← this is what we send */



    while (1) {

        char d = SDA ? '1' : '0';
        char c = SCL ? '1' : '0';
        write(1, &c, 1);
        write(1, &d, 1);
        //usleep(bit_us);   // pacing output, 
        usleep(1000000);
    }


    printf("\nDone. Total bytes sent: 3 (1 address + 2 data)\n");
    return 0;
}