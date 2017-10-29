#include <termios.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include <pthread.h>
#include "cmd_def.h"

#define timeout_ms  (100)

extern int serial_handle;

/* this variable is our reference to the rx thread */
extern pthread_t rx_thread;

void uart_close();

void uart_tx(uint8 len1,uint8* data1,uint16 len2,uint8* data2);

int uart_open(char *port);

void *uart_rx(void *);