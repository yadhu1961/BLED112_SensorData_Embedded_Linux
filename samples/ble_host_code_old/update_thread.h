#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include "encrypt.h"
#include "uart.h"
#include "profile.h"

#ifndef timer_INCLUDED
#define timer_INCLUDED

extern pthread_t update_values_thread;

int update_thread_init();
void *update_attribute_thread(void *argptr);
void print_data(const char *tittle, const void* data, int len);
void invalidate_values(uint8_t flag);

#endif