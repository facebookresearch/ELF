#include "zwsdecoder.h"

typedef enum
{
	opcode_continuation = 0, opcode_text = 0x01, opcode_binary = 0x02, opcode_close = 0x08, opcode_ping = 0x09, opcode_pong = 0xA
} opcode_t;

typedef enum
{
	new_message, second_byte, short_size, short_size_2,
	long_size, long_size_2, long_size_3, long_size_4, long_size_5, long_size_6, long_size_7, long_size_8,
	mask, mask_2, mask_3, mask_4, begin_payload, payload, error
} state_t;

struct _zwsdecoder_t
{
	state_t state;

	opcode_t opcode;
	bool is_masked;
	byte mask[4];
	byte *payload;
	int payload_length;
	int payload_index;
	void *tag;
	message_callback_t message_cb;
	close_callback_t close_cb;
	ping_callback_t ping_cb;
	pong_callback_t pong_cb;
};

// private methods
static void zwsdecoder_process_byte(zwsdecoder_t *self, byte b);
static state_t zwsdecoder_next_state(zwsdecoder_t *self);
static void invoke_new_message(zwsdecoder_t *self);


zwsdecoder_t* zwsdecoder_new(void *tag, message_callback_t message_cb, close_callback_t close_cb, ping_callback_t ping_cb, pong_callback_t pong_cb)
{
	zwsdecoder_t *self = zmalloc(sizeof(zwsdecoder_t));

	self->state = new_message;
	self->tag = tag;
	self->message_cb = message_cb;
	self->close_cb = close_cb;
	self->ping_cb = ping_cb;
	self->pong_cb = pong_cb;
	self->payload = NULL;

	return self;
}

void zwsdecoder_destroy(zwsdecoder_t **self_p)
{
	zwsdecoder_t *self = *self_p;
	free(self);
	*self_p = NULL;
}

void zwsdecoder_process_buffer(zwsdecoder_t *self, zframe_t* data)
{
	int i = 0;

	byte* buffer = zframe_data(data);
	int buffer_length = zframe_size(data);

	int bytes_to_read;

	while (i < buffer_length)
	{
		switch (self->state)
		{
		case error:
			return;
		case begin_payload:

			self->payload_index = 0;
			self->payload = zmalloc(sizeof(byte) * (self->payload_length + 4)); // +4 extra bytes in case we have to inflate it

			// continue to payload
		case payload:
			bytes_to_read = self->payload_length - self->payload_index;

			if (bytes_to_read > (buffer_length - i))
			{
				bytes_to_read = buffer_length - i;
			}

			memcpy(self->payload + self->payload_index, buffer + i, bytes_to_read);

			if (self->is_masked)
			{
				for (int j = self->payload_index; j < self->payload_index + bytes_to_read; j++)
				{
					self->payload[j] = self->payload[j] ^ self->mask[(j) % 4];
				}
			}

			self->payload_index += bytes_to_read;
			i += bytes_to_read;

			if (self->payload_index < self->payload_length)
			{
				self->state = payload;
			}
			else
			{
				// temp(this, new MessageEventArgs(m_opcode, m_payload, m_more));

				self->state = new_message;
				invoke_new_message(self);
			}

			break;
		default:
			zwsdecoder_process_byte(self, buffer[i]);
			i++;
			break;
		}
	}
}

static void zwsdecoder_process_byte(zwsdecoder_t *self, byte b)
{
	bool final;

	switch (self->state)
	{
	case new_message:
		final = (b & 0x80) != 0; // final bit
		self->opcode = b & 0xF; // opcode bit

		// not final messages are currently not supported
		if (!final)
		{
			self->state = error;
		}
		// check that the opcode is supported
		else if (self->opcode != opcode_binary && self->opcode != opcode_close && self->opcode != opcode_ping && self->opcode != opcode_pong)
		{
			self->state = error;
		}
		else
		{
			self->state = second_byte;
		}
		break;
	case second_byte:
		self->is_masked = (b & 0x80) != 0;

		byte length = (byte)(b & 0x7F);

		if (length < 126)
		{
			self->payload_length = length;
			self->state = zwsdecoder_next_state(self);
		}
		else if (length == 126)
		{
			self->state = short_size;
		}
		else
		{
			self->state = long_size;
		}
		break;
	case mask:
		self->mask[0] = b;
		self->state = mask_2;
		break;
	case mask_2:
		self->mask[1] = b;
		self->state = mask_3;
		break;
	case mask_3:
		self->mask[2] = b;
		self->state = mask_4;
		break;
	case mask_4:
		self->mask[3] = b;
		self->state = zwsdecoder_next_state(self);
		break;
	case short_size:
		self->payload_length = b << 8;
		self->state = short_size_2;
		break;
	case short_size_2:
		self->payload_length |= b;
		self->state = zwsdecoder_next_state(self);
		break;
	case long_size:
		self->payload_length = 0;

		// must be zero, max message size is MaxInt
		if (b != 0)
			self->state = error;
		else
			self->state = long_size_2;
		break;
	case long_size_2:
		// must be zero, max message size is MaxInt
		if (b != 0)
			self->state = error;
		else
			self->state = long_size_3;
		break;
	case long_size_3:
		// must be zero, max message size is MaxInt
		if (b != 0)
			self->state = error;
		else
			self->state = long_size_4;
		break;
	case long_size_4:
		// must be zero, max message size is MaxInt
		if (b != 0)
			self->state = error;
		else
			self->state = long_size_5;
		break;
	case long_size_5:
		self->payload_length |= b << 24;
		self->state = long_size_6;
		break;
	case long_size_6:
		self->payload_length |= b << 16;
		self->state = long_size_7;
		break;
	case long_size_7:
		self->payload_length |= b << 8;
		self->state = long_size_8;
		break;
	case long_size_8:
		self->payload_length |= b;
		self->state = zwsdecoder_next_state(self);
		break;
	case begin_payload:
	case payload:
	case error:
	    ;
	    // XXXX unhandled
	}
}

static state_t zwsdecoder_next_state(zwsdecoder_t *self)
{
	if ((self->state == long_size_8 || self->state == second_byte || self->state == short_size_2) && self->is_masked)
	{
		return mask;
	}
	else
	{
		if (self->payload_length == 0)
		{
			invoke_new_message(self);

			return new_message;
		}
		else
			return begin_payload;

	}
}

static void invoke_new_message(zwsdecoder_t *self)
{
	switch (self->opcode)
	{
	case opcode_binary:
		self->message_cb(self->tag, self->payload, self->payload_length);
		break;
	case opcode_close:
		self->close_cb(self->tag, self->payload, self->payload_length);
		break;
	case opcode_ping:
		self->ping_cb(self->tag, self->payload, self->payload_length);
		break;
	case opcode_pong:
		self->pong_cb(self->tag, self->payload, self->payload_length);
		break;
	default:
		assert(false);
	}

	if (self->payload != NULL)
	{
		free(self->payload);
		self->payload = NULL;
	}
}

bool zwsdecoder_is_errored(zwsdecoder_t *self)
{
	return self->state == error;
}
