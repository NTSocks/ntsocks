/*
 * <p>Title: utils.h</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 FDU NiSL</p>
 *
 * @author Bob Huang
 * @date 12/23/19 
 * @version 1.0
 */

#ifndef SHM_RING_BUFFER_UTILS_H
#define SHM_RING_BUFFER_UTILS_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NEW(type) (type *)malloc(sizeof(type))
#define APPLY(task, ...) task(__VA_ARGS__)
#define ABS(n) n > 0 ? n : -n
#define RANGE(min, max, number) number < min ? min : number > max ? max : number
#define EQUAL(a, b) a == b ? 0 : a < b ? -1 : 1
#define log printf

// uuid
char * createUUID();

#ifdef __cplusplus
};
#endif

#endif //SHM_RING_BUFFER_UTILS_H
