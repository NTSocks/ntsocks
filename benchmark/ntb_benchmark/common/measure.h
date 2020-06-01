#ifndef MEASURE_H_
#define MEASURE_H_

#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include "sys/time.h"

size_t get_cycles() {
    uint64_t low, high;
    size_t val;
    asm volatile ("rdtsc" : "=a" (low), "=d" (high));
    val = (size_t)((high << 32) | low);
    return val;
}

#define MEASUREMENTS 200
#define USECSTEP 10
#define USECSTART 100
double sample_get_cpu_mhz() {
    struct timeval tv1, tv2;
    size_t start;
    double sx = 0, sy = 0, sxx = 0, syy = 0, sxy = 0;
    double tx, ty;
    int i;

    /* Regression: y = a + b x */
    long x[MEASUREMENTS];
    size_t y[MEASUREMENTS];
    double b; /* cycles per microsecond */
    double r_2;

    for (i = 0; i < MEASUREMENTS; ++i) {
        start = get_cycles();

        if (gettimeofday(&tv1, NULL)) {
            fprintf(stderr, "gettimeofday failed.\n");
            return 0;
        }

        do {
            if (gettimeofday(&tv2, NULL)) {
                fprintf(stderr, "gettimeofday failed.\n");
                return 0;
            }
        } while ((tv2.tv_sec - tv1.tv_sec) * 1000000 +
                (tv2.tv_usec - tv1.tv_usec) < USECSTART + i * USECSTEP);

        x[i] = (tv2.tv_sec - tv1.tv_sec) * 1000000 +
            tv2.tv_usec - tv1.tv_usec;
        y[i] = get_cycles() - start;
    }

    for (i = 0; i < MEASUREMENTS; ++i) {
        tx = x[i];
        ty = y[i];
        sx += tx;
        sy += ty;
        sxx += tx * tx;
        syy += ty * ty;
        sxy += tx * ty;
    }

    b = (MEASUREMENTS * sxy - sx * sy) / (MEASUREMENTS * sxx - sx * sx);

    r_2 = (MEASUREMENTS * sxy - sx * sy) * (MEASUREMENTS * sxy - sx * sy) /
        (MEASUREMENTS * sxx - sx * sx) /
        (MEASUREMENTS * syy - sy * sy);

    if (r_2 < 0.9) {
        fprintf(stderr,"Correlation coefficient r^2: %g < 0.9\n", r_2);
        return 0;
    }

    return b;
}
#undef MEASUREMENTS
#undef USECSTEP
#undef USECSTART

//static double proc_get_cpu_mhz(int no_cpu_freq_warn)
double proc_get_cpu_mhz() {
    FILE* f;
    char buf[256];
    double mhz = 0.0;
    int print_flag = 0;
    double delta;

    f = fopen("/proc/cpuinfo","r");

    if (!f)
        return 0.0;
    while(fgets(buf, sizeof(buf), f)) {
        double m;
        int rc;

        rc = sscanf(buf, "cpu MHz : %lf", &m);

        if (rc != 1)
            continue;

        if (mhz == 0.0) {
            mhz = m;
            continue;
        }
        delta = mhz > m ? mhz - m : m - mhz;
        if ((delta / mhz > 0.02) && (print_flag ==0)) {
            print_flag = 1;
            /*
            if (!no_cpu_freq_warn) {
                fprintf(stderr, "Conflicting CPU frequency values"
                        " detected: %lf != %lf. CPU Frequency is not max.\n", mhz, m);
            }
            */
            continue;
        }
    }

    fclose(f);
    return mhz;
}

double get_cpu_mhz() {
    double sample, proc, delta;
    sample = sample_get_cpu_mhz();
    proc = proc_get_cpu_mhz();
    if (!proc || !sample)
        return 0;

    delta = proc > sample ? proc - sample : sample - proc;
    if (delta / proc > 0.02) {
        return sample;
    }
    return proc;
}

#endif // MEASURE_H_