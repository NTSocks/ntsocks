#include <string.h>
#include "common.h"

log_ctx_t log_init(char *filepath, size_t pathlen) {
    if (filepath == NULL || pathlen <= 0) {
        return NULL;
    }

    struct log_context *ctx;
    ctx = (log_ctx_t)malloc(sizeof(struct log_context));
    if (!ctx) {
        return NULL; 
    }

    memset(ctx->filepath, 0, 128);
    memcpy(ctx->filepath, filepath, pathlen);
    ctx->file = fopen(ctx->filepath, "a+");
    if (!ctx->file) {
        free(ctx); 
        return NULL; 
    }

    return ctx;
}

void log_append(log_ctx_t ctx, char *msg, size_t msglen) {
    if (!ctx || !msg || msglen <= 0)   return; 

    fwrite(msg, msglen, 1, ctx->file);
}

void log_destroy(log_ctx_t ctx) {
    if (!ctx || !ctx->file)   return; 

    fclose(ctx->file);
}
