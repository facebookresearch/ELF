#ifndef __ZWSDECODER_H_INCLUDED__
#define __ZWSDECODER_H_INCLUDED__

#include <czmq.h>

typedef void (*message_callback_t)(void *tag, byte* payload, int length);
typedef void(*close_callback_t)(void *tag, byte* payload, int length);
typedef void(*ping_callback_t)(void *tag, byte* payload, int length);
typedef void(*pong_callback_t)(void *tag, byte* payload, int length);

typedef struct _zwsdecoder_t zwsdecoder_t;

zwsdecoder_t* zwsdecoder_new(void *tag, message_callback_t message_cb, close_callback_t close_cb, ping_callback_t ping_callback, pong_callback_t payload_cb);

void zwsdecoder_destroy(zwsdecoder_t **self_p);

void zwsdecoder_process_buffer(zwsdecoder_t *self, zframe_t* data);

bool zwsdecoder_is_errored(zwsdecoder_t *self);

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif
