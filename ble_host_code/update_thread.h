#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include "encrypt.h"
#include "uart.h"
#include "profile.h"

#ifndef timer_INCLUDED
#define timer_INCLUDED

#define MAX_ATTRIBUTE_PAYLOAD 56
#define DATA_ATTRIBUTE_SIZE 241

extern pthread_t update_values_thread;
extern char *data_file;
extern int data_update_period;
extern int key_update_period;

int update_thread_init();
void *update_attribute_thread(void *argptr);
void print_data(const char *tittle, const void* data, int len);
void invalidate_values(uint8_t flag);

#endif