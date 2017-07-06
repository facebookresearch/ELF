#ifndef __ZWSHANDSHAKE_H_INCLUDED__
#define __ZWSHANDSHAKE_H_INCLUDED__

#include <czmq.h>

typedef struct _zwshandshake_t zwshandshake_t;

zwshandshake_t* zwshandshake_new();

void zwshandshake_destroy(zwshandshake_t **self_p);

bool zwshandshake_parse_request(zwshandshake_t *self, zframe_t* data);

zframe_t* zwshandshake_get_response(zwshandshake_t *self, unsigned char *client_max_window_bits, unsigned char *server_max_window_bits);

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif
