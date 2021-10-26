#ifndef __KV_ERRNO_H__
#define __KV_ERRNO_H__

static enum map_status{
    MAP_OK,
    MAP_REPLACE,
    MAP_MISSING,
    MAP_OMEM,
    MAP_ERR
}map_status;

static char* status_name[] = {
    [MAP_OK] = "MAP_OK",
    [MAP_REPLACE] = "MAP_REPLACE",
    [MAP_MISSING] = "MAP_MISSING",
    [MAP_OMEM] = "MAP_OMEM",
    [MAP_ERR] = "MAP_ERR"
};

#endif