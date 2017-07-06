#include <czmq.h>

#include "zwssock.h"

static char *listen_on = (char *)"tcp://127.0.0.1:8000";

int main(int argc, char **argv)
{
	zctx_t *ctx;
	zwssock_t *sock;

	char *l =  argc > 1 ? argv[1] : listen_on;

	int major, minor, patch;
	zmq_version (&major, &minor, &patch);
	printf("built with: ØMQ=%d.%d.%d czmq=%d.%d.%d\n",
	       major, minor, patch,
	       CZMQ_VERSION_MAJOR, CZMQ_VERSION_MINOR,CZMQ_VERSION_PATCH);


	ctx = zctx_new();
	sock = zwssock_new_router(ctx);

	zwssock_bind(sock, l);

	zmsg_t* msg;
	zframe_t *id;

	while (!zctx_interrupted)
	{
		msg = zwssock_recv_nowait(sock);

		if (!msg)
			break;

		// first message is the routing id
		id = zmsg_pop(msg);

		while (zmsg_size(msg) != 0)
		{
			char * str = zmsg_popstr(msg);

			printf("%s\n", str);

			free(str);
		}

		zmsg_destroy(&msg);

		msg = zmsg_new();

		zmsg_push(msg, id);
		zmsg_addstr(msg, "hello back");

		int rc = zwssock_send(sock, &msg);
		if (rc != 0)
			zmsg_destroy(&msg);
	}

	zwssock_destroy(&sock);
	zctx_destroy(&ctx);
}
