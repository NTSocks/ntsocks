#ifndef MEASURE_H_
#define MEASURE_H_

#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include "sys/time.h"

#define MEASUREMENTS 200
#define USECSTEP 10
#define USECSTART 100

double sample_get_cpu_mhz();

size_t get_cycles();
//static double proc_get_cpu_mhz(int no_cpu_freq_warn)
double proc_get_cpu_mhz();
double get_cpu_mhz();
#endif // MEASURE_H_