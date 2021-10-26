#ifndef LOG_H_
#define LOG_H_

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LOG_LEVEL 3

#define LOG_INFO(x) (((LOG_LEVEL) <= 3) ? printf("[INFO] %s\n", x) : printf(""))
#define LOG_DEBUG(x) (((LOG_LEVEL) <= 2) ? printf("[DEBUG] %s\n", x) : printf(""))
#define LOG_TRACE(x) (((LOG_LEVEL) <= 1) ? printf("[TRACE] %s\n", x) : printf(""))
#define LOG_ERROR(x) (printf("[ERROR] %s\n", x));

#ifdef __cplusplus
};
#endif

#endif /* LOG_H_ */
