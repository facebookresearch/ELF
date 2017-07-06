#include "zwssock.h"
#include "zwshandshake.h"
#include "zwsdecoder.h"

#include <czmq.h>
#include <string.h>
#include <zlib.h>

struct _zwssock_t
{
	zctx_t *ctx;                //  Our parent context
	void *control;              //  Control to/from agent
	void *data;                 //  Data to/from agent
};

//  This background thread does all the real work
static void s_agent_task(void *args, zctx_t *ctx, void *control);

//  --------------------------------------------------------------------------
//  Constructor

zwssock_t* zwssock_new_router(zctx_t *ctx)
{
	zwssock_t *self = (zwssock_t *)zmalloc(sizeof(zwssock_t));

	assert(self);

	self->ctx = ctx;
	self->control = zthread_fork(self->ctx, s_agent_task, NULL);

	//  Create separate data socket, send address on control socket
	self->data = zsocket_new(self->ctx, ZMQ_PAIR);
	assert(self->data);
	int rc = zsocket_bind(self->data, "inproc://data-%p", self->data);
	assert(rc != -1);
	zstr_sendf(self->control, "inproc://data-%p", self->data);

	return self;
}

//  --------------------------------------------------------------------------
//  Destructor

void zwssock_destroy(zwssock_t **self_p)
{
	assert(self_p);
	if (*self_p) {
		zwssock_t *self = *self_p;
		zstr_send(self->control, "TERMINATE");

		free(zstr_recv(self->control));
		free(self);
		*self_p = NULL;
	}
}

int zwssock_bind(zwssock_t *self, char *endpoint)
{
	assert(self);
	return zstr_sendx(self->control, "BIND", endpoint, NULL);
}

int zwssock_send(zwssock_t *self, zmsg_t **msg_p)
{
	assert(self);
	assert(zmsg_size(*msg_p) > 0);

	return zmsg_send(msg_p, self->data);
}

zmsg_t * zwssock_recv(zwssock_t *self)
{
	assert(self);
	zmsg_t *msg = zmsg_recv(self->data);
	return msg;
}

zmsg_t * zwssock_recv_nowait(zwssock_t *self)
{
	assert(self);
	zmsg_t *msg = zmsg_recv_nowait(self->data);
	return msg;
}

void* zwssock_handle(zwssock_t *self)
{
	assert(self);
	return self->data;
}


//  *************************    BACK END AGENT    *************************

typedef struct {
	zctx_t *ctx;                //  CZMQ context
	void *control;              //  Control socket back to application
	void *data;                 //  Data socket to application
	void *stream;               //  stream socket to server
	zhash_t *clients;           //  Clients known so far
	bool terminated;            //  Agent terminated by API
} agent_t;

static agent_t *
s_agent_new(zctx_t *ctx, void *control)
{
	agent_t *self = (agent_t *)zmalloc(sizeof(agent_t));
	self->ctx = ctx;
	self->control = control;
	self->stream = zsocket_new(ctx, ZMQ_STREAM);

	//  Connect our data socket to caller's endpoint
	self->data = zsocket_new(ctx, ZMQ_PAIR);
	char *endpoint = zstr_recv(self->control);
	int rc = zsocket_connect(self->data, "%s", endpoint);
	assert(rc != -1);
	free(endpoint);

	self->clients = zhash_new();
	return self;
}

static void
s_agent_destroy(agent_t **self_p)
{
	assert(self_p);
	if (*self_p) {
		agent_t *self = *self_p;
		zhash_destroy(&self->clients);
		free(self);
		*self_p = NULL;
	}
}

//  This section covers a single client connection
typedef enum {
	closed = 0,
	connected = 1,              //  Ready for messages
	exception = 2               //  Failed due to some error
} state_t;

typedef struct {
	agent_t *agent;             //  Agent for this client
	state_t state;              //  Current state
	zframe_t *address;          //  Client address identity
	char *hashkey;              //  Key into clients hash
	zwsdecoder_t* decoder;
	unsigned char client_max_window_bits; // Requested compression factor by the server for the client
	unsigned char server_max_window_bits; // Requested compression factor by the client for the server
	z_stream permessage_deflate_client;   // The client advertised the permessage-deflate extension
	z_stream permessage_deflate_server;   // The client advertised the permessage-deflate extension

	zmsg_t *outgoing_msg;		// Currently outgoing message, if not NULL final frame was not yet arrived
} client_t;

static client_t *
client_new(agent_t *agent, zframe_t *address)
{
	client_t *self = (client_t *)zmalloc(sizeof(client_t));
	assert(self);
	self->agent = agent;
	self->address = zframe_dup(address);
	self->hashkey = zframe_strhex(address);
	self->state = closed;
	self->decoder = NULL;
	self->client_max_window_bits = 10;
	self->server_max_window_bits = 10;
	self->permessage_deflate_client.zalloc   = Z_NULL;
	self->permessage_deflate_client.zfree    = Z_NULL;
	self->permessage_deflate_client.opaque   = Z_NULL;
	self->permessage_deflate_client.avail_in = 0;
	self->permessage_deflate_client.next_in  = Z_NULL;
	self->permessage_deflate_server.zalloc   = Z_NULL;
	self->permessage_deflate_server.zfree    = Z_NULL;
	self->permessage_deflate_server.opaque   = Z_NULL;
	self->permessage_deflate_server.avail_in = 0;
	self->permessage_deflate_server.next_in  = Z_NULL;
	self->outgoing_msg = NULL;
	return self;
}

static void
client_destroy(client_t **self_p)
{
	assert(self_p);
	if (*self_p) {
		client_t *self = *self_p;
		zframe_destroy(&self->address);

		if (self->decoder != NULL)
		{
			zwsdecoder_destroy(&self->decoder);
		}

		if (self->client_max_window_bits > 0)
		{
			inflateEnd(&self->permessage_deflate_client);
		}

		if (self->server_max_window_bits > 0)
		{
			deflateEnd(&self->permessage_deflate_server);
		}

		if (self->outgoing_msg != NULL)
		{
			zmsg_destroy(&self->outgoing_msg);
		}

		free(self->hashkey);
		free(self);
		*self_p = NULL;
	}
}

#define CHUNK 8192

void router_message_received(void *tag, byte* payload, int length)
{
	client_t *self = (client_t *)tag;
	bool more;

	if (self->outgoing_msg == NULL)
	{
		self->outgoing_msg = zmsg_new();
		zmsg_addstr(self->outgoing_msg, self->hashkey);
	}

	if (self->client_max_window_bits > 0) {
		unsigned char out[CHUNK];
		byte* outgoingData = (byte*)zmalloc(length + 4);
		bool more_parsed = false;

		/* 7.2.2.  Decompression */
		memcpy(outgoingData, payload, length);
		outgoingData[length + 0] = 0x00;
		outgoingData[length + 1] = 0x00;
		outgoingData[length + 2] = 0xff;
		outgoingData[length + 3] = 0xff;

		self->permessage_deflate_client.avail_in = length + 4;
		self->permessage_deflate_client.next_in = outgoingData;

		do {
			self->permessage_deflate_client.avail_out = CHUNK;
			self->permessage_deflate_client.next_out = out;

			int ret = inflate(&self->permessage_deflate_client, Z_NO_FLUSH);
			assert(ret != Z_STREAM_ERROR);
			switch (ret) {
				case Z_NEED_DICT:
					ret = Z_DATA_ERROR;
				case Z_DATA_ERROR:
				case Z_MEM_ERROR:
					{
						inflateEnd(&self->permessage_deflate_client);
						zmsg_destroy(&self->outgoing_msg);

						/* close the client connection */
						self->state = exception;
						zframe_t *address = zframe_dup(self->address);
						zframe_send(&address, self->agent->stream, ZFRAME_MORE);
						zmq_send(self->agent->stream, NULL, 0, 0);
						return;
					}
			}
			unsigned int length_inflated = CHUNK - self->permessage_deflate_client.avail_out;
			if (!more_parsed) {
				more_parsed = true;
				more = (out[0] == 1);
				zmsg_addmem(self->outgoing_msg, &out[1], length_inflated - 1);
			} else {
				zmsg_addmem(self->outgoing_msg, out, length_inflated);
			}

		} while (self->permessage_deflate_client.avail_out == 0);

		free(outgoingData);
	} else {
		more = (payload[0] == 1);
		zmsg_addmem(self->outgoing_msg, &payload[1], length - 1);
	}

	if (!more)
	{
		zmsg_send(&self->outgoing_msg, self->agent->data);
	}
}

void close_received(void *tag, byte* payload, int length)
{
	// TODO: close received
}

void ping_received(void *tag, byte* payload, int length)
{
	// TODO: implement ping
}

void pong_received(void *tag, byte* payload, int length)
{
	// TOOD: implement pong
}

static void not_acceptable(zframe_t *_address, void *dest) {
	zframe_t *address = zframe_dup(_address);
	zframe_send(&address, dest, ZFRAME_MORE + ZFRAME_REUSE);
	zstr_send (dest, "HTTP/1.1 406 Not Acceptable\r\n\r\n");

	zframe_send(&address, dest, ZFRAME_MORE);
	zmq_send(dest, NULL, 0, 0);
}

static void client_data_ready(client_t * self)
{
	zframe_t* data;
	zwshandshake_t * handshake;

	data = zframe_recv(self->agent->stream);

	switch (self->state)
	{
	case closed:
		// TODO: we might didn't receive the entire request, make the zwshandshake able to handle multiple inputs

		handshake = zwshandshake_new();
		if (zwshandshake_parse_request(handshake, data))
		{
			// request is valid, getting the response
			zframe_t* response = zwshandshake_get_response(handshake, &self->client_max_window_bits, &self->server_max_window_bits);
			if (response)
			{
				zframe_t *address = zframe_dup(self->address);

				zframe_send(&address, self->agent->stream, ZFRAME_MORE);
				zframe_send(&response, self->agent->stream, 0);

				free(response);

				if (self->client_max_window_bits > 0) {
					int ret = inflateInit2(&self->permessage_deflate_client, -self->client_max_window_bits);
					if (ret != Z_OK) {
						self->client_max_window_bits = 0;
						self->state = exception;
						not_acceptable(self->address, self->agent->stream);
					}

					ret = deflateInit2(&self->permessage_deflate_server, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -self->server_max_window_bits, 8, Z_DEFAULT_STRATEGY);
					if (ret != Z_OK) {
						self->server_max_window_bits = 0;
						self->state = exception;
						not_acceptable(self->address, self->agent->stream);
					}
				}

				self->decoder = zwsdecoder_new(self, &router_message_received, &close_received, &ping_received, &pong_received);
				self->state = connected;
			}
			else
			{
				// request is invalid, for example the message does not contain the required headers
				self->state = exception;
				not_acceptable(self->address, self->agent->stream);
			}
		}
		else
		{
			// request is invalid
			self->state = exception;
		}
		zwshandshake_destroy(&handshake);

		break;
	case connected:
		zwsdecoder_process_buffer(self->decoder, data);

		if (zwsdecoder_is_errored(self->decoder))
		{
			self->state = exception;
		}

		break;

	case exception:
		// ignoring the message
		break;
	}

	zframe_destroy(&data);
}

//  Callback when we remove client from 'clients' hash table
static void
client_free(void *argument)
{
	client_t *client = (client_t *)argument;
	client_destroy(&client);
}

static int
s_agent_handle_control(agent_t *self)
{
	//  Get the whole message off the control socket in one go
	zmsg_t *request = zmsg_recv(self->control);
	char *command = zmsg_popstr(request);
	if (!command)
		return -1;                  //  Interrupted

	if (streq(command, "BIND")) {
		char *endpoint = zmsg_popstr(request);
		puts(endpoint);
		int rc = zsocket_bind(self->stream, "%s", endpoint);
		assert(rc != -1);
		free(endpoint);
	}
	else if (streq(command, "UNBIND")) {
		char *endpoint = zmsg_popstr(request);
		int rc = zsocket_unbind(self->stream, "%s", endpoint);
		assert(rc != -1);
		free(endpoint);
	}
	else if (streq(command, "TERMINATE")) {
		self->terminated = true;
		zstr_send(self->control, "OK");
	}
	free(command);
	zmsg_destroy(&request);
	return 0;
}

//  Handle a message from the server

static int
s_agent_handle_router(agent_t *self)
{
	zframe_t *address = zframe_recv(self->stream);
	char *hashkey = zframe_strhex(address);
	client_t *client = zhash_lookup(self->clients, hashkey);
	if (client == NULL) {
		client = client_new(self, address);

		zhash_insert(self->clients, hashkey, client);
		zhash_freefn(self->clients, hashkey, client_free);
	}
	free(hashkey);
	zframe_destroy(&address);

	client_data_ready(client);

	//  If client is misbehaving, remove it
	if (client->state == exception)
		zhash_delete(self->clients, client->hashkey);

	return 0;
}

static void
compute_frame_header(byte header, int payloadLength, int *frameSize, int *payloadStartIndex, byte *outgoingData) {
	*frameSize = 2 + payloadLength;
	*payloadStartIndex = 2;

	if (payloadLength > 125)
	{
		*frameSize += 2;
		*payloadStartIndex += 2;

		if (payloadLength > 0xFFFF) // 2 bytes max value
		{
			*frameSize += 6;
			*payloadStartIndex += 6;
		}
	}

	outgoingData[0] = header;

	// No mask
	outgoingData[1] = 0x00;

	if (payloadLength <= 125)
	{
		outgoingData[1] |= (byte)(payloadLength & 127);
	}
	else if (payloadLength <= 0xFFFF) // maximum size of short
	{
		outgoingData[1] |= 126;
		outgoingData[2] = (payloadLength >> 8) & 0xFF;
		outgoingData[3] = payloadLength & 0xFF;
	}
	else
	{
		outgoingData[1] |= 127;
		outgoingData[2] = 0;
		outgoingData[3] = 0;
		outgoingData[4] = 0;
		outgoingData[5] = 0;
		outgoingData[6] = (payloadLength >> 24) & 0xFF;
		outgoingData[7] = (payloadLength >> 16) & 0xFF;
		outgoingData[8] = (payloadLength >> 8) & 0xFF;
		outgoingData[9] = payloadLength & 0xFF;
	}
}

static int
s_agent_handle_data(agent_t *self)
{
	//  First frame is client address (hashkey)
	//  If caller sends unknown client address, we discard the message
	//  The assert disappears when we start to timeout clients...
	zmsg_t *request = zmsg_recv(self->data);
	char *hashkey = zmsg_popstr(request);
	client_t *client = zhash_lookup(self->clients, hashkey);

	zframe_t* address;

	if (client) {
		//  Each frame is a full ZMQ message with identity frame
		while (zmsg_size(request)) {
			zframe_t *receivedFrame = zmsg_pop(request);
			bool more = false;

			if (zmsg_size(request))
				more = true;

			if (client->server_max_window_bits > 0)
			{
				byte byte_no_more = 0;
				byte byte_more = 1;

				int frameSize = zframe_size(receivedFrame);

				// This assumes that a compressed message is never longer than 64 bytes plus the original message. A better assumption without realloc would be great.
				unsigned int available = frameSize + 64 + 10;
				byte *compressedPayload = (byte*)zmalloc(available);
				client->permessage_deflate_server.avail_in = 1;
				client->permessage_deflate_server.next_in  = (more ? &byte_more : &byte_no_more);
				client->permessage_deflate_server.avail_out = available;
				client->permessage_deflate_server.next_out = &compressedPayload[10];

				deflate(&client->permessage_deflate_server, Z_NO_FLUSH);

				client->permessage_deflate_server.avail_in = frameSize;
				client->permessage_deflate_server.next_in  = zframe_data(receivedFrame);

				deflate(&client->permessage_deflate_server, Z_SYNC_FLUSH);
				assert(client->permessage_deflate_server.avail_in == 0);

				int payloadLength = available - client->permessage_deflate_server.avail_out;
				payloadLength -= 4; /* skip the 0x00 0x00 0xff 0xff */

				byte initialHeader[10];
				int payloadStartIndex;
				compute_frame_header((byte)0xC2, payloadLength, &frameSize, &payloadStartIndex, initialHeader); // 0xC2 = Final, RSV1, Binary

				// We reserved 10 extra bytes before the compressed message, here we prepend the header before it, prevents allocation of the message and a memcpy of the compressed content
				byte* outgoingData = &compressedPayload[10 - payloadStartIndex];
				memcpy(outgoingData, initialHeader, payloadStartIndex);

				address = zframe_dup(client->address);

				zframe_send(&address, self->stream, ZFRAME_MORE);
				zsocket_sendmem(self->stream, outgoingData, frameSize, 0);

				free(compressedPayload);
				zframe_destroy(&receivedFrame);
            }
			else
			{
				int payloadLength = zframe_size(receivedFrame) + 1;
				byte* outgoingData = (byte*)zmalloc(payloadLength + 10); /* + 10 = max size of header */

				int frameSize, payloadStartIndex;
				compute_frame_header(0x82, payloadLength, &frameSize, &payloadStartIndex, outgoingData); /* 0x82 = Binary and Final */

				// more byte
				outgoingData[payloadStartIndex] = (byte)(more ? 1 : 0);
				payloadStartIndex++;

				// payload
				memcpy(outgoingData + payloadStartIndex, zframe_data(receivedFrame), zframe_size(receivedFrame));

				address = zframe_dup(client->address);

				zframe_send(&address, self->stream, ZFRAME_MORE);
				zsocket_sendmem(self->stream, outgoingData, frameSize, 0);

				free(outgoingData);
				zframe_destroy(&receivedFrame);
			}

			// TODO: check return code, on return code different than 0 or again set exception
		}
	}

	free(hashkey);
	zmsg_destroy(&request);
	return 0;
}

void s_agent_task(void *args, zctx_t *ctx, void *control)
{
	//  Create agent instance as we start this task
	agent_t *self = s_agent_new(ctx, control);
	if (!self)                  //  Interrupted
		return;

	//  We always poll all three sockets
	zmq_pollitem_t pollitems[] = {
			{ self->control, 0, ZMQ_POLLIN, 0 },
			{ self->stream, 0, ZMQ_POLLIN, 0 },
			{ self->data, 0, ZMQ_POLLIN, 0 }
	};
	while (!zctx_interrupted) {
		if (zmq_poll(pollitems, 3, -1) == -1)
			break;              //  Interrupted

		if (pollitems[0].revents & ZMQ_POLLIN)
			s_agent_handle_control(self);
		if (pollitems[1].revents & ZMQ_POLLIN)
			s_agent_handle_router(self);
		if (pollitems[2].revents & ZMQ_POLLIN)
			s_agent_handle_data(self);

		if (self->terminated)
			break;
	}
	//  Done, free all agent resources
	s_agent_destroy(&self);
}
