#include <ctype.h>
#include <czmq.h>

#include "zwshandshake.h"

typedef enum
{
	initial = 0,
	request_line_G,
	request_line_GE,
	request_line_GET,
	request_line_GET_space,
	request_line_resource,
	request_line_resource_space,
	request_line_H,
	request_line_HT,
	request_line_HTT,
	request_line_HTTP,
	request_line_HTTP_slash,
	request_line_HTTP_slash_1,
	request_line_HTTP_slash_1_dot,
	request_line_HTTP_slash_1_dot_1,
	request_line_cr,
	header_field_begin_name,
	header_field_name,
	header_field_colon,
	header_field_value_trailing_space,
	header_field_value,
	header_field_cr,
	end_line_cr,
	complete,

	error = -1
} state_t;

struct _zwshandshake_t
{
	state_t state;
	zhash_t* header_fields;
};

bool zwshandshake_validate(zwshandshake_t *self);

zwshandshake_t* zwshandshake_new()
{
	zwshandshake_t* self = zmalloc(sizeof(zwshandshake_t));
	self->state = initial;
	self->header_fields = zhash_new();

	return self;
}

void s_free_item(void *data)
{
	free(data);
}

void zwshandshake_destroy(zwshandshake_t **self_p)
{
	zwshandshake_t *self = *self_p;

	zhash_destroy(&self->header_fields);

	free(self);
	*self_p = NULL;
}

bool zwshandshake_parse_request(zwshandshake_t *self, zframe_t* data)
{
	// the parse method is not fully implemented and therefore not secured.
	// for the purpose of ZWS prototyoe only the request-line, upgrade header and Sec-WebSocket-Key are validated.

	// one of the ommissions in this parser in the fact that http-header contents may be spread over multiple lines
	// the current implementation would only keep the last line.

	char *request = zframe_strdup(data);
	int length = strlen(request);

	char *field_name;
	char *field_value;

	size_t name_begin, name_length, value_begin, value_length;

	for (size_t i = 0; i < length; i++)
	{
		char c = request[i];

		switch (self->state)
		{
		case initial:
			if (c == 'G')
				self->state = request_line_G;
			else
				self->state = error;
			break;
		case request_line_G:
			if (c == 'E')
				self->state = request_line_GE;
			else
				self->state = error;
			break;
		case request_line_GE:
			if (c == 'T')
				self->state = request_line_GET;
			else
				self->state = error;
			break;
		case request_line_GET:
			if (c == ' ')
				self->state = request_line_GET_space;
			else
				self->state = error;
			break;
		case request_line_GET_space:
			if (c == '\r' || c == '\n')
				self->state = error;
			// TODO: instead of check what is not allowed check what is allowed
			if (c != ' ')
				self->state = request_line_resource;
			else
				self->state = request_line_GET_space;
			break;
		case request_line_resource:
			if (c == '\r' || c == '\n')
				self->state = error;
			else if (c == ' ')
				self->state = request_line_resource_space;
			else
				self->state = request_line_resource;
			break;
		case request_line_resource_space:
			if (c == 'H')
				self->state = request_line_H;
			else
				self->state = error;
			break;
		case request_line_H:
			if (c == 'T')
				self->state = request_line_HT;
			else
				self->state = error;
			break;
		case request_line_HT:
			if (c == 'T')
				self->state = request_line_HTT;
			else
				self->state = error;
			break;
		case request_line_HTT:
			if (c == 'P')
				self->state = request_line_HTTP;
			else
				self->state = error;
			break;
		case request_line_HTTP:
			if (c == '/')
				self->state = request_line_HTTP_slash;
			else
				self->state = error;
			break;
		case request_line_HTTP_slash:
			if (c == '1')
				self->state = request_line_HTTP_slash_1;
			else
				self->state = error;
			break;
		case request_line_HTTP_slash_1:
			if (c == '.')
				self->state = request_line_HTTP_slash_1_dot;
			else
				self->state = error;
			break;
		case request_line_HTTP_slash_1_dot:
			if (c == '1')
				self->state = request_line_HTTP_slash_1_dot_1;
			else
				self->state = error;
			break;
		case request_line_HTTP_slash_1_dot_1:
			if (c == '\r')
				self->state = request_line_cr;
			else
				self->state = error;
			break;
		case request_line_cr:
			if (c == '\n')
				self->state = header_field_begin_name;
			else
				self->state = error;
			break;
		case header_field_begin_name:
			switch (c)
			{
			case '\r':
				self->state = end_line_cr;
				break;
			case '\n':
				self->state = error;
				break;
			default:
				name_begin = i;
				self->state = header_field_name;
				break;
			}
			break;
		case header_field_name:
			if (c == '\r' || c == '\n')
				self->state = error;
			else if (c == ':')
			{
				name_length = i - name_begin;
				self->state = header_field_colon;
			}
			else
				self->state = header_field_name;
			break;
		case header_field_colon:
		case header_field_value_trailing_space:
			if (c == '\n')
				self->state = error;
			else if (c == '\r')
				self->state = header_field_cr;
			else if (c == ' ')
				self->state = header_field_value_trailing_space;
			else
			{
				value_begin = i;
				self->state = header_field_value;
			}
			break;
		case header_field_value:
			if (c == '\n')
				self->state = error;
			else if (c == '\r')
			{
				value_length = i - value_begin;

				field_name = zmalloc(sizeof(char) * (name_length + 1));
				field_value = zmalloc(sizeof(char) * (value_length + 1));

				strncpy(field_name, request + name_begin, name_length);
				strncpy(field_value, request + value_begin, value_length);

				field_name[name_length] = '\0';
				field_value[value_length] = '\0';

				for (size_t j = 0; j < name_length; j++)
				{
					field_name[j] = tolower(field_name[j]);
				}

				zhash_insert(self->header_fields, field_name, field_value);
				zhash_freefn(self->header_fields, field_name, &s_free_item);

				free(field_name);

				self->state = header_field_cr;
			}
			else
				self->state = header_field_value;
			break;
		case header_field_cr:
			if (c == '\n')
				self->state = header_field_begin_name;
			else
				self->state = error;
			break;
		case end_line_cr:
			if (c == '\n')
			{
				self->state = complete;
				free(request);
				return zwshandshake_validate(self);
			}
			break;
		case error:
			free(request);
			return false;
		default:
			assert(false);
			free(request);
			return false;
		}
	}

	free(request);
	return false;
}

bool zwshandshake_validate(zwshandshake_t *self)
{
	// TODO: validate that the request is valid
	return true;
}

int encode_base64(uint8_t *in, int in_len, char* out, int out_len)
{
	static const uint8_t base64enc_tab[64] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	unsigned ii, io;
	uint_least32_t v;
	unsigned rem;

	for (io = 0, ii = 0, v = 0, rem = 0; ii<in_len; ii++) {
		uint8_t ch;
		ch = in[ii];
		v = (v << 8) | ch;
		rem += 8;
		while (rem >= 6) {
			rem -= 6;
			if (io >= out_len) return -1; /* truncation is failure */
			out[io++] = base64enc_tab[(v >> rem) & 63];
		}
	}
	if (rem) {
		v <<= (6 - rem);
		if (io >= out_len) return -1; /* truncation is failure */
		out[io++] = base64enc_tab[v & 63];
	}
	while (io & 3) {
		if (io >= out_len) return -1; /* truncation is failure */
		out[io++] = '=';
	}
	if (io >= out_len) return -1; /* no room for null terminator */
	out[io] = 0;
	return io;

}

zframe_t* zwshandshake_get_response(zwshandshake_t *self, unsigned char *client_max_window_bits, unsigned char *server_max_window_bits)
{
	const char * sec_websocket_key_name = "sec-websocket-key";
	const char * magic_string = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

	char * key = zhash_lookup(self->header_fields, sec_websocket_key_name);
	if (key == NULL) return NULL;

	int len = strlen(key) + strlen(magic_string);

	char plain[150];

	strcpy(plain, key);
	strcat(plain, magic_string);

	zdigest_t* digest = zdigest_new();
	zdigest_update(digest, (byte *) plain, len);

	byte* hash = zdigest_data(digest);

	char accept_key[150];

	int accept_key_len = encode_base64(hash, zdigest_size(digest), accept_key, 150);

	zdigest_destroy (&digest);

	if (accept_key_len == -1) return NULL;

	// this is a proof of concept implementation, similar to zwshandshake_parse_request
	// it only implements the bare minimum to get compression going, consider it a TODO
	// to implement a proper parsing of both HTTP Headers as sec-websocket-extensions list.

	/* Implementation of permessage-deflate, client_max_window_bits and server_max_window_bits */
	const char * sec_websocket_extensions_name = "sec-websocket-extensions";
	bool extension_permessage_deflate = false;
	bool extension_client_max_window_bits = false;
	bool extension_server_max_window_bits = false;

	char * key_extensions = zhash_lookup(self->header_fields, sec_websocket_extensions_name);
	if (key_extensions && strstr(key_extensions, "permessage-deflate") != NULL &&
		*client_max_window_bits > 0 && *server_max_window_bits > 0) {

		extension_permessage_deflate = true;
		if (strstr(key_extensions, "client_max_window_bits") != NULL) {
			extension_client_max_window_bits = true;
		} else {
			*client_max_window_bits = 15;
		}

		char *server_max_window_bits = strstr(key_extensions, "server_max_window_bits=");
		if (server_max_window_bits) {
			extension_server_max_window_bits = true;

			long int server_max_window_bits_candidate = strtol(server_max_window_bits + sizeof("server_max_window_bits="), NULL, 10);
			if (server_max_window_bits_candidate >= 8 || server_max_window_bits_candidate <= 15) {
				*server_max_window_bits = (unsigned char) (server_max_window_bits_candidate & 15);
			}

			/* in all other cases, we keep the value in server_max_window_bits */
		}
	} else {
		*client_max_window_bits = 0;
		*server_max_window_bits = 0;
	}

	char extension[128] = { 0 };

	if (extension_permessage_deflate) {
		if (extension_client_max_window_bits && extension_server_max_window_bits) {
			snprintf(extension, 128, "Sec-WebSocket-Extensions: permessage-deflate; client_max_window_bits=%d; server_max_window_bits=%d\r\n", *client_max_window_bits, *server_max_window_bits);
		} else if (extension_client_max_window_bits) {
			snprintf(extension, 128, "Sec-WebSocket-Extensions: permessage-deflate; client_max_window_bits=%d\r\n", *client_max_window_bits);
		} else if (extension_server_max_window_bits) {
			snprintf(extension, 128, "Sec-WebSocket-Extensions: permessage-deflate; server_max_window_bits=%d\r\n", *server_max_window_bits);
		} else {
			strncpy(extension, "Sec-WebSocket-Extensions: permessage-deflate\r\n", 128);
		}
	}

	char response[256];

	int response_len = snprintf(response, 256, "HTTP/1.1 101 Switching Protocols\r\n"
	                                           "Upgrade: websocket\r\n"
	                                           "Connection: Upgrade\r\n"
	                                           "Sec-WebSocket-Accept: %s\r\n"
	                                           "Sec-WebSocket-Protocol: WSNetMQ\r\n"
	                                           "%s\r\n", accept_key, extension);

	zframe_t *zframe_response = zframe_new(response, response_len);

	return zframe_response;
}

