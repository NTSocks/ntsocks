/*
 * <p>Title: data_struct.h</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2019 FDU NiSL</p>
 *
 * @author Bob Huang
 * @date 12/23/19 
 * @version 1.0
 */

#ifndef SHM_RING_BUFFER_DATA_STRUCT_H
#define SHM_RING_BUFFER_DATA_STRUCT_H

#include <stdbool.h>

#include "utils.h"

#ifdef __cplusplus
extern "C" {
#endif

#define string char *
#define newString(str) strcpy((char *)malloc(strlen(str) + 1), str)


#define let void *

#ifdef __cplusplus
};
#endif

#endif //SHM_RING_BUFFER_DATA_STRUCT_H
