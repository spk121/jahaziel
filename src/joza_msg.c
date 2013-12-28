/*
    joza_msg - transport for switched virtual call messages

    Copyright 2013 Michael L. Gran <spk121@yahoo.com>

    This file is part of Jozabad.

    Jozabad is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Jozabad is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Jozabad.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
@header
    joza_msg - transport for switched virtual call messages
@discuss
@end
*/

#define _GNU_SOURCE
#include <string.h>
#include <stdint.h>
#include <zmq.h>
#include <czmq.h>
#include "joza_msg.h"



//  --------------------------------------------------------------------------
//  Network data encoding macros

//  Strings are encoded with 1-byte length
#define STRING_MAX  255

//  Raw blocks are encoded with a 2-byte length
#define RAW_MAX 8192

//  Put a block to the frame
#define PUT_BLOCK(host,size) { \
    memcpy (self->needle, (host), size); \
    self->needle += size; \
    }

//  Get a block from the frame
#define GET_BLOCK(host,size) { \
    if (self->needle + size > self->ceiling) \
        goto malformed; \
    memcpy ((host), self->needle, size); \
    self->needle += size; \
    }

//  Put a 1-byte number to the frame
#define PUT_NUMBER1(host) { \
    *(byte *) self->needle = (host); \
    self->needle++; \
    }

//  Put a 2-byte number to the frame
#define PUT_NUMBER2(host) { \
    self->needle [0] = (byte) (((host) >> 8)  & 255); \
    self->needle [1] = (byte) (((host))       & 255); \
    self->needle += 2; \
    }

//  Put a 4-byte number to the frame
#define PUT_NUMBER4(host) { \
    self->needle [0] = (byte) (((host) >> 24) & 255); \
    self->needle [1] = (byte) (((host) >> 16) & 255); \
    self->needle [2] = (byte) (((host) >> 8)  & 255); \
    self->needle [3] = (byte) (((host))       & 255); \
    self->needle += 4; \
    }

//  Put a 8-byte number to the frame
#define PUT_NUMBER8(host) { \
    self->needle [0] = (byte) (((host) >> 56) & 255); \
    self->needle [1] = (byte) (((host) >> 48) & 255); \
    self->needle [2] = (byte) (((host) >> 40) & 255); \
    self->needle [3] = (byte) (((host) >> 32) & 255); \
    self->needle [4] = (byte) (((host) >> 24) & 255); \
    self->needle [5] = (byte) (((host) >> 16) & 255); \
    self->needle [6] = (byte) (((host) >> 8)  & 255); \
    self->needle [7] = (byte) (((host))       & 255); \
    self->needle += 8; \
    }

//  Get a 1-byte number from the frame
#define GET_NUMBER1(host) { \
    if (self->needle + 1 > self->ceiling) \
        goto malformed; \
    (host) = *(byte *) self->needle; \
    self->needle++; \
    }

//  Get a 2-byte number from the frame
#define GET_NUMBER2(host) { \
    if (self->needle + 2 > self->ceiling) \
        goto malformed; \
    (host) = ((uint16_t) (self->needle [0]) << 8) \
           +  (uint16_t) (self->needle [1]); \
    self->needle += 2; \
    }

//  Get a 4-byte number from the frame
#define GET_NUMBER4(host) { \
    if (self->needle + 4 > self->ceiling) \
        goto malformed; \
    (host) = ((uint32_t) (self->needle [0]) << 24) \
           + ((uint32_t) (self->needle [1]) << 16) \
           + ((uint32_t) (self->needle [2]) << 8) \
           +  (uint32_t) (self->needle [3]); \
    self->needle += 4; \
    }

//  Get a 8-byte number from the frame
#define GET_NUMBER8(host) { \
    if (self->needle + 8 > self->ceiling) \
        goto malformed; \
    (host) = ((uint64_t) (self->needle [0]) << 56) \
           + ((uint64_t) (self->needle [1]) << 48) \
           + ((uint64_t) (self->needle [2]) << 40) \
           + ((uint64_t) (self->needle [3]) << 32) \
           + ((uint64_t) (self->needle [4]) << 24) \
           + ((uint64_t) (self->needle [5]) << 16) \
           + ((uint64_t) (self->needle [6]) << 8) \
           +  (uint64_t) (self->needle [7]); \
    self->needle += 8; \
    }

//  Put a string to the frame
#define PUT_STRING(host) { \
    string_size = strlen (host); \
    PUT_NUMBER1 (string_size); \
    memcpy (self->needle, (host), string_size); \
    self->needle += string_size; \
    }

//  Get a string from the frame
#define GET_STRING(host) { \
    GET_NUMBER1 (string_size); \
    if (self->needle + string_size > (self->ceiling)) \
        goto malformed; \
    (host) = (char *) malloc (string_size + 1); \
    memcpy ((host), self->needle, string_size); \
    (host) [string_size] = 0; \
    self->needle += string_size; \
    }

//  Put a raw block to the frame
#define PUT_RAW(host) { \
    string_size = (host)[0] * 256 + (host)[1] + 2; \
    (host) = (uint8_t *) malloc (string_size + 1); \
    memcpy (self->needle, (host), string_size); \
    (host) [string_size] = 0; \
    self->needle += string_size; \
    }

//  Get a raw block from the frame
#define GET_RAW(host) { \
    GET_NUMBER2 (string_size); \
    self->needle -= 2; \
    string_size += 2; \
    if (self->needle + string_size > (self->ceiling)) \
        goto malformed; \
    (host) = (uint8_t *) malloc (string_size + 1); \
    memcpy ((host), self->needle, string_size); \
    (host) [string_size] = 0; \
    self->needle += string_size; \
    }



//  --------------------------------------------------------------------------
//  Create a new joza_msg

joza_msg_t *
joza_msg_new (int id)
{
    joza_msg_t *self = (joza_msg_t *) zmalloc (sizeof (joza_msg_t));
    self->id = id;
    return self;
}


//  --------------------------------------------------------------------------
//  Destroy the joza_msg

void
joza_msg_destroy (joza_msg_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        joza_msg_t *self = *self_p;

        //  Free class properties
        zframe_destroy (&self->address);
        zframe_destroy (&self->data);
        free (self->calling_address);
        free (self->called_address);
        free (self->protocol);
        free (self->host_name);
        zhash_destroy (&self->workers);

        //  Free object itself
        free (self);
        *self_p = NULL;
    }
}


//  --------------------------------------------------------------------------
//  Receive and parse a joza_msg from the socket. Returns new object or
//  NULL if error. Will block if there's no message waiting.

joza_msg_t *
joza_msg_recv (void *input)
{
    assert (input);
    joza_msg_t *self = joza_msg_new (0);
    zframe_t *frame = NULL;
    size_t string_size;
    size_t list_size;
    size_t hash_size;

    //  Read valid message frame from socket; we loop over any
    //  garbage data we might receive from badly-connected peers
    while (true) {
        //  If we're reading from a ROUTER socket, get address
        if (zsockopt_type (input) == ZMQ_ROUTER) {
            zframe_destroy (&self->address);
            self->address = zframe_recv (input);
            if (!self->address)
                goto empty;         //  Interrupted
            if (!zsocket_rcvmore (input))
                goto malformed;
        }
        //  Read and parse command in frame
        frame = zframe_recv (input);
        if (!frame)
            goto empty;             //  Interrupted

        //  Get and check protocol signature
        self->needle = zframe_data (frame);
        self->ceiling = self->needle + zframe_size (frame);
        uint16_t signature;
        GET_NUMBER2 (signature);
        if (signature == (0xAAA0 | 1))
            break;                  //  Valid signature

        //  Protocol assertion, drop message
        while (zsocket_rcvmore (input)) {
            zframe_destroy (&frame);
            frame = zframe_recv (input);
        }
        zframe_destroy (&frame);
    }
    //  Get message id and parse per message type
    GET_NUMBER1 (self->id);

    switch (self->id) {
    case JOZA_MSG_DATA:
        GET_NUMBER1 (self->q);
        GET_NUMBER2 (self->pr);
        GET_NUMBER2 (self->ps);
        //  Get next frame, leave current untouched
        if (!zsocket_rcvmore (input))
            goto malformed;
        self->data = zframe_recv (input);
        break;

    case JOZA_MSG_RR:
        GET_NUMBER2 (self->pr);
        break;

    case JOZA_MSG_RNR:
        GET_NUMBER2 (self->pr);
        break;

    case JOZA_MSG_CALL_REQUEST:
        free (self->calling_address);
        GET_STRING (self->calling_address);
        free (self->called_address);
        GET_STRING (self->called_address);
        GET_NUMBER1 (self->packet);
        GET_NUMBER2 (self->window);
        GET_NUMBER1 (self->throughput);
        //  Get next frame, leave current untouched
        if (!zsocket_rcvmore (input))
            goto malformed;
        self->data = zframe_recv (input);
        break;

    case JOZA_MSG_CALL_ACCEPTED:
        free (self->calling_address);
        GET_STRING (self->calling_address);
        free (self->called_address);
        GET_STRING (self->called_address);
        GET_NUMBER1 (self->packet);
        GET_NUMBER2 (self->window);
        GET_NUMBER1 (self->throughput);
        //  Get next frame, leave current untouched
        if (!zsocket_rcvmore (input))
            goto malformed;
        self->data = zframe_recv (input);
        break;

    case JOZA_MSG_CLEAR_REQUEST:
        GET_NUMBER1 (self->cause);
        GET_NUMBER1 (self->diagnostic);
        break;

    case JOZA_MSG_CLEAR_CONFIRMATION:
        break;

    case JOZA_MSG_RESET_REQUEST:
        GET_NUMBER1 (self->cause);
        GET_NUMBER1 (self->diagnostic);
        break;

    case JOZA_MSG_RESET_CONFIRMATION:
        break;

    case JOZA_MSG_CONNECT:
        free (self->protocol);
        GET_STRING (self->protocol);
        if (strneq (self->protocol, "~SVC"))
            goto malformed;
        GET_NUMBER1 (self->version);
        if (self->version != JOZA_MSG_VERSION)
            goto malformed;
        free (self->calling_address);
        GET_STRING (self->calling_address);
        free (self->host_name);
        GET_STRING (self->host_name);
        GET_NUMBER1 (self->directionality);
        break;

    case JOZA_MSG_CONNECT_INDICATION:
        break;

    case JOZA_MSG_DISCONNECT:
        break;

    case JOZA_MSG_DISCONNECT_INDICATION:
        break;

    case JOZA_MSG_DIAGNOSTIC:
        GET_NUMBER1 (self->cause);
        GET_NUMBER1 (self->diagnostic);
        break;

    case JOZA_MSG_DIRECTORY_REQUEST:
        break;

    case JOZA_MSG_DIRECTORY:
        GET_NUMBER1 (hash_size);
        self->workers = zhash_new ();
        zhash_autofree (self->workers);
        while (hash_size--) {
            char *string;
            GET_STRING (string);
            char *value = strchr (string, '=');
            if (value)
                *value++ = 0;
            zhash_insert (self->workers, string, value);
            free (string);
        }
        break;

    case JOZA_MSG_ENQ:
        break;

    case JOZA_MSG_ACK:
        break;

    default:
        goto malformed;
    }
    //  Successful return
    zframe_destroy (&frame);
    return self;

    //  Error returns
malformed:
    printf ("E: malformed message '%d'\n", self->id);
empty:
    zframe_destroy (&frame);
    joza_msg_destroy (&self);
    return (NULL);
}

//  Count size of key=value pair
static int
s_workers_count (const char *key, void *item, void *argument)
{
    joza_msg_t *self = (joza_msg_t *) argument;
    self->workers_bytes += strlen (key) + 1 + strlen ((char *) item) + 1;
    return 0;
}

//  Serialize workers key=value pair
static int
s_workers_write (const char *key, void *item, void *argument)
{
    joza_msg_t *self = (joza_msg_t *) argument;
    char string [STRING_MAX + 1];
    snprintf (string, STRING_MAX, "%s=%s", key, (char *) item);
    size_t string_size;
    PUT_STRING (string);
    return 0;
}


//  --------------------------------------------------------------------------
//  Send the joza_msg to the socket, and destroy it
//  Returns 0 if OK, else -1

int
joza_msg_send (joza_msg_t **self_p, void *output)
{
    assert (output);
    assert (self_p);
    assert (*self_p);

    //  Calculate size of serialized data
    joza_msg_t *self = *self_p;
    size_t frame_size = 2 + 1;          //  Signature and message ID
    switch (self->id) {
    case JOZA_MSG_DATA:
        //  q is a 1-byte integer
        frame_size += 1;
        //  pr is a 2-byte integer
        frame_size += 2;
        //  ps is a 2-byte integer
        frame_size += 2;
        break;

    case JOZA_MSG_RR:
        //  pr is a 2-byte integer
        frame_size += 2;
        break;

    case JOZA_MSG_RNR:
        //  pr is a 2-byte integer
        frame_size += 2;
        break;

    case JOZA_MSG_CALL_REQUEST:
        //  calling_address is a string with 1-byte length
        frame_size++;       //  Size is one octet
        if (self->calling_address)
            frame_size += strlen (self->calling_address);
        //  called_address is a string with 1-byte length
        frame_size++;       //  Size is one octet
        if (self->called_address)
            frame_size += strlen (self->called_address);
        //  packet is a 1-byte integer
        frame_size += 1;
        //  window is a 2-byte integer
        frame_size += 2;
        //  throughput is a 1-byte integer
        frame_size += 1;
        break;

    case JOZA_MSG_CALL_ACCEPTED:
        //  calling_address is a string with 1-byte length
        frame_size++;       //  Size is one octet
        if (self->calling_address)
            frame_size += strlen (self->calling_address);
        //  called_address is a string with 1-byte length
        frame_size++;       //  Size is one octet
        if (self->called_address)
            frame_size += strlen (self->called_address);
        //  packet is a 1-byte integer
        frame_size += 1;
        //  window is a 2-byte integer
        frame_size += 2;
        //  throughput is a 1-byte integer
        frame_size += 1;
        break;

    case JOZA_MSG_CLEAR_REQUEST:
        //  cause is a 1-byte integer
        frame_size += 1;
        //  diagnostic is a 1-byte integer
        frame_size += 1;
        break;

    case JOZA_MSG_CLEAR_CONFIRMATION:
        break;

    case JOZA_MSG_RESET_REQUEST:
        //  cause is a 1-byte integer
        frame_size += 1;
        //  diagnostic is a 1-byte integer
        frame_size += 1;
        break;

    case JOZA_MSG_RESET_CONFIRMATION:
        break;

    case JOZA_MSG_CONNECT:
        //  protocol is a string with 1-byte length
        frame_size += 1 + strlen ("~SVC");
        //  version is a 1-byte integer
        frame_size += 1;
        //  calling_address is a string with 1-byte length
        frame_size++;       //  Size is one octet
        if (self->calling_address)
            frame_size += strlen (self->calling_address);
        //  host_name is a string with 1-byte length
        frame_size++;       //  Size is one octet
        if (self->host_name)
            frame_size += strlen (self->host_name);
        //  directionality is a 1-byte integer
        frame_size += 1;
        break;

    case JOZA_MSG_CONNECT_INDICATION:
        break;

    case JOZA_MSG_DISCONNECT:
        break;

    case JOZA_MSG_DISCONNECT_INDICATION:
        break;

    case JOZA_MSG_DIAGNOSTIC:
        //  cause is a 1-byte integer
        frame_size += 1;
        //  diagnostic is a 1-byte integer
        frame_size += 1;
        break;

    case JOZA_MSG_DIRECTORY_REQUEST:
        break;

    case JOZA_MSG_DIRECTORY:
        //  workers is an array of key=value strings
        frame_size++;       //  Size is one octet
        if (self->workers) {
            self->workers_bytes = 0;
            //  Add up size of dictionary contents
            zhash_foreach (self->workers, s_workers_count, self);
        }
        frame_size += self->workers_bytes;
        break;

    case JOZA_MSG_ENQ:
        break;

    case JOZA_MSG_ACK:
        break;

    default:
        printf ("E: bad message type '%d', not sent\n", self->id);
        //  No recovery, this is a fatal application error
        assert (false);
    }
    //  Now serialize message into the frame
    zframe_t *frame = zframe_new (NULL, frame_size);
    self->needle = zframe_data (frame);
    size_t string_size;
    int frame_flags = 0;
    PUT_NUMBER2 (0xAAA0 | 1);
    PUT_NUMBER1 (self->id);

    switch (self->id) {
    case JOZA_MSG_DATA:
        PUT_NUMBER1 (self->q);
        PUT_NUMBER2 (self->pr);
        PUT_NUMBER2 (self->ps);
        frame_flags = ZFRAME_MORE;
        break;

    case JOZA_MSG_RR:
        PUT_NUMBER2 (self->pr);
        break;

    case JOZA_MSG_RNR:
        PUT_NUMBER2 (self->pr);
        break;

    case JOZA_MSG_CALL_REQUEST:
        if (self->calling_address) {
            PUT_STRING (self->calling_address);
        } else
            PUT_NUMBER1 (0);    //  Empty string
        if (self->called_address) {
            PUT_STRING (self->called_address);
        } else
            PUT_NUMBER1 (0);    //  Empty string
        PUT_NUMBER1 (self->packet);
        PUT_NUMBER2 (self->window);
        PUT_NUMBER1 (self->throughput);
        frame_flags = ZFRAME_MORE;
        break;

    case JOZA_MSG_CALL_ACCEPTED:
        if (self->calling_address) {
            PUT_STRING (self->calling_address);
        } else
            PUT_NUMBER1 (0);    //  Empty string
        if (self->called_address) {
            PUT_STRING (self->called_address);
        } else
            PUT_NUMBER1 (0);    //  Empty string
        PUT_NUMBER1 (self->packet);
        PUT_NUMBER2 (self->window);
        PUT_NUMBER1 (self->throughput);
        frame_flags = ZFRAME_MORE;
        break;

    case JOZA_MSG_CLEAR_REQUEST:
        PUT_NUMBER1 (self->cause);
        PUT_NUMBER1 (self->diagnostic);
        break;

    case JOZA_MSG_CLEAR_CONFIRMATION:
        break;

    case JOZA_MSG_RESET_REQUEST:
        PUT_NUMBER1 (self->cause);
        PUT_NUMBER1 (self->diagnostic);
        break;

    case JOZA_MSG_RESET_CONFIRMATION:
        break;

    case JOZA_MSG_CONNECT:
        PUT_STRING ("~SVC");
        PUT_NUMBER1 (JOZA_MSG_VERSION);
        if (self->calling_address) {
            PUT_STRING (self->calling_address);
        } else
            PUT_NUMBER1 (0);    //  Empty string
        if (self->host_name) {
            PUT_STRING (self->host_name);
        } else
            PUT_NUMBER1 (0);    //  Empty string
        PUT_NUMBER1 (self->directionality);
        break;

    case JOZA_MSG_CONNECT_INDICATION:
        break;

    case JOZA_MSG_DISCONNECT:
        break;

    case JOZA_MSG_DISCONNECT_INDICATION:
        break;

    case JOZA_MSG_DIAGNOSTIC:
        PUT_NUMBER1 (self->cause);
        PUT_NUMBER1 (self->diagnostic);
        break;

    case JOZA_MSG_DIRECTORY_REQUEST:
        break;

    case JOZA_MSG_DIRECTORY:
        if (self->workers != NULL) {
            PUT_NUMBER1 (zhash_size (self->workers));
            zhash_foreach (self->workers, s_workers_write, self);
        } else
            PUT_NUMBER1 (0);    //  Empty dictionary
        break;

    case JOZA_MSG_ENQ:
        break;

    case JOZA_MSG_ACK:
        break;

    }
    //  If we're sending to a ROUTER, we send the address first
    if (zsockopt_type (output) == ZMQ_ROUTER) {
        assert (self->address);
        if (zframe_send (&self->address, output, ZFRAME_MORE)) {
            zframe_destroy (&frame);
            joza_msg_destroy (self_p);
            return -1;
        }
    }
    //  Now send the data frame
    if (zframe_send (&frame, output, frame_flags)) {
        zframe_destroy (&frame);
        joza_msg_destroy (self_p);
        return -1;
    }

    //  Now send any frame fields, in order
    switch (self->id) {
    case JOZA_MSG_DATA:
        //  If data isn't set, send an empty frame
        if (!self->data)
            self->data = zframe_new (NULL, 0);
        if (zframe_send (&self->data, output, 0)) {
            zframe_destroy (&frame);
            joza_msg_destroy (self_p);
            return -1;
        }
        break;
    case JOZA_MSG_CALL_REQUEST:
        //  If data isn't set, send an empty frame
        if (!self->data)
            self->data = zframe_new (NULL, 0);
        if (zframe_send (&self->data, output, 0)) {
            zframe_destroy (&frame);
            joza_msg_destroy (self_p);
            return -1;
        }
        break;
    case JOZA_MSG_CALL_ACCEPTED:
        //  If data isn't set, send an empty frame
        if (!self->data)
            self->data = zframe_new (NULL, 0);
        if (zframe_send (&self->data, output, 0)) {
            zframe_destroy (&frame);
            joza_msg_destroy (self_p);
            return -1;
        }
        break;
    }
    //  Destroy joza_msg object
    joza_msg_destroy (self_p);
    return 0;
}


//  --------------------------------------------------------------------------
//  Send the DATA to the socket in one step

int
joza_msg_send_data (
    void *output,
    byte q,
    uint16_t pr,
    uint16_t ps,
    zframe_t *data)
{
    joza_msg_t *self = joza_msg_new (JOZA_MSG_DATA);
    joza_msg_set_q (self, q);
    joza_msg_set_pr (self, pr);
    joza_msg_set_ps (self, ps);
    joza_msg_set_data (self, zframe_dup (data));
    return joza_msg_send (&self, output);
}


int
joza_msg_send_addr_data (
    void *output, const zframe_t *addr,
    byte q,
    uint16_t pr,
    uint16_t ps,
    zframe_t *data)
{
    joza_msg_t *self = joza_msg_new (JOZA_MSG_DATA);
    self->address = zframe_dup(addr);
    joza_msg_set_q (self, q);
    joza_msg_set_pr (self, pr);
    joza_msg_set_ps (self, ps);
    joza_msg_set_data (self, zframe_dup (data));
    return joza_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the RR to the socket in one step

int
joza_msg_send_rr (
    void *output,
    uint16_t pr)
{
    joza_msg_t *self = joza_msg_new (JOZA_MSG_RR);
    joza_msg_set_pr (self, pr);
    return joza_msg_send (&self, output);
}


int
joza_msg_send_addr_rr (
    void *output, const zframe_t *addr,
    uint16_t pr)
{
    joza_msg_t *self = joza_msg_new (JOZA_MSG_RR);
    self->address = zframe_dup(addr);
    joza_msg_set_pr (self, pr);
    return joza_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the RNR to the socket in one step

int
joza_msg_send_rnr (
    void *output,
    uint16_t pr)
{
    joza_msg_t *self = joza_msg_new (JOZA_MSG_RNR);
    joza_msg_set_pr (self, pr);
    return joza_msg_send (&self, output);
}


int
joza_msg_send_addr_rnr (
    void *output, const zframe_t *addr,
    uint16_t pr)
{
    joza_msg_t *self = joza_msg_new (JOZA_MSG_RNR);
    self->address = zframe_dup(addr);
    joza_msg_set_pr (self, pr);
    return joza_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the CALL_REQUEST to the socket in one step

int
joza_msg_send_call_request (
    void *output,
    char *calling_address,
    char *called_address,
    byte packet,
    uint16_t window,
    byte throughput,
    zframe_t *data)
{
    joza_msg_t *self = joza_msg_new (JOZA_MSG_CALL_REQUEST);
    joza_msg_set_calling_address (self, calling_address);
    joza_msg_set_called_address (self, called_address);
    joza_msg_set_packet (self, packet);
    joza_msg_set_window (self, window);
    joza_msg_set_throughput (self, throughput);
    joza_msg_set_data (self, zframe_dup (data));
    return joza_msg_send (&self, output);
}


int
joza_msg_send_addr_call_request (
    void *output, const zframe_t *addr,
    char *calling_address,
    char *called_address,
    byte packet,
    uint16_t window,
    byte throughput,
    zframe_t *data)
{
    joza_msg_t *self = joza_msg_new (JOZA_MSG_CALL_REQUEST);
    self->address = zframe_dup(addr);
    joza_msg_set_calling_address (self, calling_address);
    joza_msg_set_called_address (self, called_address);
    joza_msg_set_packet (self, packet);
    joza_msg_set_window (self, window);
    joza_msg_set_throughput (self, throughput);
    joza_msg_set_data (self, zframe_dup (data));
    return joza_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the CALL_ACCEPTED to the socket in one step

int
joza_msg_send_call_accepted (
    void *output,
    char *calling_address,
    char *called_address,
    byte packet,
    uint16_t window,
    byte throughput,
    zframe_t *data)
{
    joza_msg_t *self = joza_msg_new (JOZA_MSG_CALL_ACCEPTED);
    joza_msg_set_calling_address (self, calling_address);
    joza_msg_set_called_address (self, called_address);
    joza_msg_set_packet (self, packet);
    joza_msg_set_window (self, window);
    joza_msg_set_throughput (self, throughput);
    joza_msg_set_data (self, zframe_dup (data));
    return joza_msg_send (&self, output);
}


int
joza_msg_send_addr_call_accepted (
    void *output, const zframe_t *addr,
    char *calling_address,
    char *called_address,
    byte packet,
    uint16_t window,
    byte throughput,
    zframe_t *data)
{
    joza_msg_t *self = joza_msg_new (JOZA_MSG_CALL_ACCEPTED);
    self->address = zframe_dup(addr);
    joza_msg_set_calling_address (self, calling_address);
    joza_msg_set_called_address (self, called_address);
    joza_msg_set_packet (self, packet);
    joza_msg_set_window (self, window);
    joza_msg_set_throughput (self, throughput);
    joza_msg_set_data (self, zframe_dup (data));
    return joza_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the CLEAR_REQUEST to the socket in one step

int
joza_msg_send_clear_request (
    void *output,
    byte cause,
    byte diagnostic)
{
    joza_msg_t *self = joza_msg_new (JOZA_MSG_CLEAR_REQUEST);
    joza_msg_set_cause (self, cause);
    joza_msg_set_diagnostic (self, diagnostic);
    return joza_msg_send (&self, output);
}


int
joza_msg_send_addr_clear_request (
    void *output, const zframe_t *addr,
    byte cause,
    byte diagnostic)
{
    joza_msg_t *self = joza_msg_new (JOZA_MSG_CLEAR_REQUEST);
    self->address = zframe_dup(addr);
    joza_msg_set_cause (self, cause);
    joza_msg_set_diagnostic (self, diagnostic);
    return joza_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the CLEAR_CONFIRMATION to the socket in one step

int
joza_msg_send_clear_confirmation (
    void *output)
{
    joza_msg_t *self = joza_msg_new (JOZA_MSG_CLEAR_CONFIRMATION);
    return joza_msg_send (&self, output);
}


int
joza_msg_send_addr_clear_confirmation (
    void *output, const zframe_t *addr)
{
    joza_msg_t *self = joza_msg_new (JOZA_MSG_CLEAR_CONFIRMATION);
    self->address = zframe_dup(addr);
    return joza_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the RESET_REQUEST to the socket in one step

int
joza_msg_send_reset_request (
    void *output,
    byte cause,
    byte diagnostic)
{
    joza_msg_t *self = joza_msg_new (JOZA_MSG_RESET_REQUEST);
    joza_msg_set_cause (self, cause);
    joza_msg_set_diagnostic (self, diagnostic);
    return joza_msg_send (&self, output);
}


int
joza_msg_send_addr_reset_request (
    void *output, const zframe_t *addr,
    byte cause,
    byte diagnostic)
{
    joza_msg_t *self = joza_msg_new (JOZA_MSG_RESET_REQUEST);
    self->address = zframe_dup(addr);
    joza_msg_set_cause (self, cause);
    joza_msg_set_diagnostic (self, diagnostic);
    return joza_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the RESET_CONFIRMATION to the socket in one step

int
joza_msg_send_reset_confirmation (
    void *output)
{
    joza_msg_t *self = joza_msg_new (JOZA_MSG_RESET_CONFIRMATION);
    return joza_msg_send (&self, output);
}


int
joza_msg_send_addr_reset_confirmation (
    void *output, const zframe_t *addr)
{
    joza_msg_t *self = joza_msg_new (JOZA_MSG_RESET_CONFIRMATION);
    self->address = zframe_dup(addr);
    return joza_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the CONNECT to the socket in one step

int
joza_msg_send_connect (
    void *output,
    char *calling_address,
    char *host_name,
    byte directionality)
{
    joza_msg_t *self = joza_msg_new (JOZA_MSG_CONNECT);
    joza_msg_set_calling_address (self, calling_address);
    joza_msg_set_host_name (self, host_name);
    joza_msg_set_directionality (self, directionality);
    return joza_msg_send (&self, output);
}


int
joza_msg_send_addr_connect (
    void *output, const zframe_t *addr,
    char *calling_address,
    char *host_name,
    byte directionality)
{
    joza_msg_t *self = joza_msg_new (JOZA_MSG_CONNECT);
    self->address = zframe_dup(addr);
    joza_msg_set_calling_address (self, calling_address);
    joza_msg_set_host_name (self, host_name);
    joza_msg_set_directionality (self, directionality);
    return joza_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the CONNECT_INDICATION to the socket in one step

int
joza_msg_send_connect_indication (
    void *output)
{
    joza_msg_t *self = joza_msg_new (JOZA_MSG_CONNECT_INDICATION);
    return joza_msg_send (&self, output);
}


int
joza_msg_send_addr_connect_indication (
    void *output, const zframe_t *addr)
{
    joza_msg_t *self = joza_msg_new (JOZA_MSG_CONNECT_INDICATION);
    self->address = zframe_dup(addr);
    return joza_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the DISCONNECT to the socket in one step

int
joza_msg_send_disconnect (
    void *output)
{
    joza_msg_t *self = joza_msg_new (JOZA_MSG_DISCONNECT);
    return joza_msg_send (&self, output);
}


int
joza_msg_send_addr_disconnect (
    void *output, const zframe_t *addr)
{
    joza_msg_t *self = joza_msg_new (JOZA_MSG_DISCONNECT);
    self->address = zframe_dup(addr);
    return joza_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the DISCONNECT_INDICATION to the socket in one step

int
joza_msg_send_disconnect_indication (
    void *output)
{
    joza_msg_t *self = joza_msg_new (JOZA_MSG_DISCONNECT_INDICATION);
    return joza_msg_send (&self, output);
}


int
joza_msg_send_addr_disconnect_indication (
    void *output, const zframe_t *addr)
{
    joza_msg_t *self = joza_msg_new (JOZA_MSG_DISCONNECT_INDICATION);
    self->address = zframe_dup(addr);
    return joza_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the DIAGNOSTIC to the socket in one step

int
joza_msg_send_diagnostic (
    void *output,
    byte cause,
    byte diagnostic)
{
    joza_msg_t *self = joza_msg_new (JOZA_MSG_DIAGNOSTIC);
    joza_msg_set_cause (self, cause);
    joza_msg_set_diagnostic (self, diagnostic);
    return joza_msg_send (&self, output);
}


int
joza_msg_send_addr_diagnostic (
    void *output, const zframe_t *addr,
    byte cause,
    byte diagnostic)
{
    joza_msg_t *self = joza_msg_new (JOZA_MSG_DIAGNOSTIC);
    self->address = zframe_dup(addr);
    joza_msg_set_cause (self, cause);
    joza_msg_set_diagnostic (self, diagnostic);
    return joza_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the DIRECTORY_REQUEST to the socket in one step

int
joza_msg_send_directory_request (
    void *output)
{
    joza_msg_t *self = joza_msg_new (JOZA_MSG_DIRECTORY_REQUEST);
    return joza_msg_send (&self, output);
}


int
joza_msg_send_addr_directory_request (
    void *output, const zframe_t *addr)
{
    joza_msg_t *self = joza_msg_new (JOZA_MSG_DIRECTORY_REQUEST);
    self->address = zframe_dup(addr);
    return joza_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the DIRECTORY to the socket in one step

int
joza_msg_send_directory (
    void *output,
    zhash_t *workers)
{
    joza_msg_t *self = joza_msg_new (JOZA_MSG_DIRECTORY);
    joza_msg_set_workers (self, zhash_dup (workers));
    return joza_msg_send (&self, output);
}


int
joza_msg_send_addr_directory (
    void *output, const zframe_t *addr,
    zhash_t *workers)
{
    joza_msg_t *self = joza_msg_new (JOZA_MSG_DIRECTORY);
    self->address = zframe_dup(addr);
    joza_msg_set_workers (self, zhash_dup (workers));
    return joza_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the ENQ to the socket in one step

int
joza_msg_send_enq (
    void *output)
{
    joza_msg_t *self = joza_msg_new (JOZA_MSG_ENQ);
    return joza_msg_send (&self, output);
}


int
joza_msg_send_addr_enq (
    void *output, const zframe_t *addr)
{
    joza_msg_t *self = joza_msg_new (JOZA_MSG_ENQ);
    self->address = zframe_dup(addr);
    return joza_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the ACK to the socket in one step

int
joza_msg_send_ack (
    void *output)
{
    joza_msg_t *self = joza_msg_new (JOZA_MSG_ACK);
    return joza_msg_send (&self, output);
}


int
joza_msg_send_addr_ack (
    void *output, const zframe_t *addr)
{
    joza_msg_t *self = joza_msg_new (JOZA_MSG_ACK);
    self->address = zframe_dup(addr);
    return joza_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Duplicate the joza_msg message

joza_msg_t *
joza_msg_dup (joza_msg_t *self)
{
    if (!self)
        return NULL;

    joza_msg_t *copy = joza_msg_new (self->id);
    if (self->address)
        copy->address = zframe_dup (self->address);
    switch (self->id) {
    case JOZA_MSG_DATA:
        copy->q = self->q;
        copy->pr = self->pr;
        copy->ps = self->ps;
        copy->data = zframe_dup (self->data);
        break;

    case JOZA_MSG_RR:
        copy->pr = self->pr;
        break;

    case JOZA_MSG_RNR:
        copy->pr = self->pr;
        break;

    case JOZA_MSG_CALL_REQUEST:
        copy->calling_address = strdup (self->calling_address);
        copy->called_address = strdup (self->called_address);
        copy->packet = self->packet;
        copy->window = self->window;
        copy->throughput = self->throughput;
        copy->data = zframe_dup (self->data);
        break;

    case JOZA_MSG_CALL_ACCEPTED:
        copy->calling_address = strdup (self->calling_address);
        copy->called_address = strdup (self->called_address);
        copy->packet = self->packet;
        copy->window = self->window;
        copy->throughput = self->throughput;
        copy->data = zframe_dup (self->data);
        break;

    case JOZA_MSG_CLEAR_REQUEST:
        copy->cause = self->cause;
        copy->diagnostic = self->diagnostic;
        break;

    case JOZA_MSG_CLEAR_CONFIRMATION:
        break;

    case JOZA_MSG_RESET_REQUEST:
        copy->cause = self->cause;
        copy->diagnostic = self->diagnostic;
        break;

    case JOZA_MSG_RESET_CONFIRMATION:
        break;

    case JOZA_MSG_CONNECT:
        copy->protocol = strdup (self->protocol);
        copy->version = self->version;
        copy->calling_address = strdup (self->calling_address);
        copy->host_name = strdup (self->host_name);
        copy->directionality = self->directionality;
        break;

    case JOZA_MSG_CONNECT_INDICATION:
        break;

    case JOZA_MSG_DISCONNECT:
        break;

    case JOZA_MSG_DISCONNECT_INDICATION:
        break;

    case JOZA_MSG_DIAGNOSTIC:
        copy->cause = self->cause;
        copy->diagnostic = self->diagnostic;
        break;

    case JOZA_MSG_DIRECTORY_REQUEST:
        break;

    case JOZA_MSG_DIRECTORY:
        copy->workers = zhash_dup (self->workers);
        break;

    case JOZA_MSG_ENQ:
        break;

    case JOZA_MSG_ACK:
        break;

    }
    return copy;
}


//  Dump workers key=value pair to stdout
static int
s_workers_dump (const char *key, void *item, void *argument)
{
    joza_msg_t *self = (joza_msg_t *) argument;
    printf ("        %s=%s\n", key, (char *) item);
    return 0;
}


//  --------------------------------------------------------------------------
//  Print contents of message to stdout

void
joza_msg_dump (joza_msg_t *self)
{
    assert (self);
    switch (self->id) {
    case JOZA_MSG_DATA:
        puts ("DATA:");
        printf ("    q=%ld\n", (long) self->q);
        printf ("    pr=%ld\n", (long) self->pr);
        printf ("    ps=%ld\n", (long) self->ps);
        printf ("    data={\n");
        if (self->data) {
            size_t size = zframe_size (self->data);
            byte *data = zframe_data (self->data);
            printf ("        size=%td\n", zframe_size (self->data));
            if (size > 32)
                size = 32;
            int data_index;
            for (data_index = 0; data_index < size; data_index++) {
                if (data_index && (data_index % 4 == 0))
                    printf ("-");
                printf ("%02X", data [data_index]);
            }
        }
        printf ("    }\n");
        break;

    case JOZA_MSG_RR:
        puts ("RR:");
        printf ("    pr=%ld\n", (long) self->pr);
        break;

    case JOZA_MSG_RNR:
        puts ("RNR:");
        printf ("    pr=%ld\n", (long) self->pr);
        break;

    case JOZA_MSG_CALL_REQUEST:
        puts ("CALL_REQUEST:");
        if (self->calling_address)
            printf ("    calling_address='%s'\n", self->calling_address);
        else
            printf ("    calling_address=\n");
        if (self->called_address)
            printf ("    called_address='%s'\n", self->called_address);
        else
            printf ("    called_address=\n");
        printf ("    packet=%ld\n", (long) self->packet);
        printf ("    window=%ld\n", (long) self->window);
        printf ("    throughput=%ld\n", (long) self->throughput);
        printf ("    data={\n");
        if (self->data) {
            size_t size = zframe_size (self->data);
            byte *data = zframe_data (self->data);
            printf ("        size=%td\n", zframe_size (self->data));
            if (size > 32)
                size = 32;
            int data_index;
            for (data_index = 0; data_index < size; data_index++) {
                if (data_index && (data_index % 4 == 0))
                    printf ("-");
                printf ("%02X", data [data_index]);
            }
        }
        printf ("    }\n");
        break;

    case JOZA_MSG_CALL_ACCEPTED:
        puts ("CALL_ACCEPTED:");
        if (self->calling_address)
            printf ("    calling_address='%s'\n", self->calling_address);
        else
            printf ("    calling_address=\n");
        if (self->called_address)
            printf ("    called_address='%s'\n", self->called_address);
        else
            printf ("    called_address=\n");
        printf ("    packet=%ld\n", (long) self->packet);
        printf ("    window=%ld\n", (long) self->window);
        printf ("    throughput=%ld\n", (long) self->throughput);
        printf ("    data={\n");
        if (self->data) {
            size_t size = zframe_size (self->data);
            byte *data = zframe_data (self->data);
            printf ("        size=%td\n", zframe_size (self->data));
            if (size > 32)
                size = 32;
            int data_index;
            for (data_index = 0; data_index < size; data_index++) {
                if (data_index && (data_index % 4 == 0))
                    printf ("-");
                printf ("%02X", data [data_index]);
            }
        }
        printf ("    }\n");
        break;

    case JOZA_MSG_CLEAR_REQUEST:
        puts ("CLEAR_REQUEST:");
        printf ("    cause=%ld\n", (long) self->cause);
        printf ("    diagnostic=%ld\n", (long) self->diagnostic);
        break;

    case JOZA_MSG_CLEAR_CONFIRMATION:
        puts ("CLEAR_CONFIRMATION:");
        break;

    case JOZA_MSG_RESET_REQUEST:
        puts ("RESET_REQUEST:");
        printf ("    cause=%ld\n", (long) self->cause);
        printf ("    diagnostic=%ld\n", (long) self->diagnostic);
        break;

    case JOZA_MSG_RESET_CONFIRMATION:
        puts ("RESET_CONFIRMATION:");
        break;

    case JOZA_MSG_CONNECT:
        puts ("CONNECT:");
        printf ("    protocol=~svc\n");
        printf ("    version=joza_msg_version\n");
        if (self->calling_address)
            printf ("    calling_address='%s'\n", self->calling_address);
        else
            printf ("    calling_address=\n");
        if (self->host_name)
            printf ("    host_name='%s'\n", self->host_name);
        else
            printf ("    host_name=\n");
        printf ("    directionality=%ld\n", (long) self->directionality);
        break;

    case JOZA_MSG_CONNECT_INDICATION:
        puts ("CONNECT_INDICATION:");
        break;

    case JOZA_MSG_DISCONNECT:
        puts ("DISCONNECT:");
        break;

    case JOZA_MSG_DISCONNECT_INDICATION:
        puts ("DISCONNECT_INDICATION:");
        break;

    case JOZA_MSG_DIAGNOSTIC:
        puts ("DIAGNOSTIC:");
        printf ("    cause=%ld\n", (long) self->cause);
        printf ("    diagnostic=%ld\n", (long) self->diagnostic);
        break;

    case JOZA_MSG_DIRECTORY_REQUEST:
        puts ("DIRECTORY_REQUEST:");
        break;

    case JOZA_MSG_DIRECTORY:
        puts ("DIRECTORY:");
        printf ("    workers={\n");
        if (self->workers)
            zhash_foreach (self->workers, s_workers_dump, self);
        printf ("    }\n");
        break;

    case JOZA_MSG_ENQ:
        puts ("ENQ:");
        break;

    case JOZA_MSG_ACK:
        puts ("ACK:");
        break;

    }
}


//  --------------------------------------------------------------------------
//  Get/set the message address

zframe_t *
joza_msg_address (joza_msg_t *self)
{
    assert (self);
    return self->address;
}

const zframe_t *
joza_msg_const_address (const joza_msg_t *self)
{
    assert (self);
    return self->address;
}

void
joza_msg_set_address (joza_msg_t *self, zframe_t *address)
{
    if (self->address)
        zframe_destroy (&self->address);
    self->address = zframe_dup (address);
}


//  --------------------------------------------------------------------------
//  Get/set the joza_msg id

int
joza_msg_id (joza_msg_t *self)
{
    assert (self);
    return self->id;
}

int
joza_msg_const_id (const joza_msg_t *self)
{
    assert (self);
    return self->id;
}

void
joza_msg_set_id (joza_msg_t *self, int id)
{
    self->id = id;
}

//  --------------------------------------------------------------------------
//  Return a printable command string

const char *
joza_msg_const_command (const joza_msg_t *self)
{
    assert (self);
    switch (self->id) {
    case JOZA_MSG_DATA:
        return ("DATA");
        break;
    case JOZA_MSG_RR:
        return ("RR");
        break;
    case JOZA_MSG_RNR:
        return ("RNR");
        break;
    case JOZA_MSG_CALL_REQUEST:
        return ("CALL_REQUEST");
        break;
    case JOZA_MSG_CALL_ACCEPTED:
        return ("CALL_ACCEPTED");
        break;
    case JOZA_MSG_CLEAR_REQUEST:
        return ("CLEAR_REQUEST");
        break;
    case JOZA_MSG_CLEAR_CONFIRMATION:
        return ("CLEAR_CONFIRMATION");
        break;
    case JOZA_MSG_RESET_REQUEST:
        return ("RESET_REQUEST");
        break;
    case JOZA_MSG_RESET_CONFIRMATION:
        return ("RESET_CONFIRMATION");
        break;
    case JOZA_MSG_CONNECT:
        return ("CONNECT");
        break;
    case JOZA_MSG_CONNECT_INDICATION:
        return ("CONNECT_INDICATION");
        break;
    case JOZA_MSG_DISCONNECT:
        return ("DISCONNECT");
        break;
    case JOZA_MSG_DISCONNECT_INDICATION:
        return ("DISCONNECT_INDICATION");
        break;
    case JOZA_MSG_DIAGNOSTIC:
        return ("DIAGNOSTIC");
        break;
    case JOZA_MSG_DIRECTORY_REQUEST:
        return ("DIRECTORY_REQUEST");
        break;
    case JOZA_MSG_DIRECTORY:
        return ("DIRECTORY");
        break;
    case JOZA_MSG_ENQ:
        return ("ENQ");
        break;
    case JOZA_MSG_ACK:
        return ("ACK");
        break;
    }
    return "?";
}

//  --------------------------------------------------------------------------
//  Get/set the q field

byte
joza_msg_q (joza_msg_t *self)
{
    assert (self);
    return self->q;
}

byte
joza_msg_const_q (const joza_msg_t *self)
{
    assert (self);
    return self->q;
}

void
joza_msg_set_q (joza_msg_t *self, byte q)
{
    assert (self);
    self->q = q;
}


//  --------------------------------------------------------------------------
//  Get/set the pr field

uint16_t
joza_msg_pr (joza_msg_t *self)
{
    assert (self);
    return self->pr;
}

uint16_t
joza_msg_const_pr (const joza_msg_t *self)
{
    assert (self);
    return self->pr;
}

void
joza_msg_set_pr (joza_msg_t *self, uint16_t pr)
{
    assert (self);
    self->pr = pr;
}


//  --------------------------------------------------------------------------
//  Get/set the ps field

uint16_t
joza_msg_ps (joza_msg_t *self)
{
    assert (self);
    return self->ps;
}

uint16_t
joza_msg_const_ps (const joza_msg_t *self)
{
    assert (self);
    return self->ps;
}

void
joza_msg_set_ps (joza_msg_t *self, uint16_t ps)
{
    assert (self);
    self->ps = ps;
}


//  --------------------------------------------------------------------------
//  Get/set the data field

zframe_t *
joza_msg_data (joza_msg_t *self)
{
    assert (self);
    return self->data;
}

const zframe_t *
joza_msg_const_data (const joza_msg_t *self)
{
    assert (self);
    return self->data;
}

//  Takes ownership of supplied frame
void
joza_msg_set_data (joza_msg_t *self, zframe_t *frame)
{
    assert (self);
    if (self->data)
        zframe_destroy (&self->data);
    self->data = frame;
}

//  --------------------------------------------------------------------------
//  Get/set the calling_address field

char *
joza_msg_calling_address (joza_msg_t *self)
{
    assert (self);
    return self->calling_address;
}

const char *
joza_msg_const_calling_address (const joza_msg_t *self)
{
    assert (self);
    return self->calling_address;
}

void
joza_msg_set_calling_address (joza_msg_t *self, const char *format, ...)
{
    //  Format into newly allocated string
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    free (self->calling_address);
    self->calling_address = (char *) malloc (STRING_MAX + 1);
    assert (self->calling_address);
    vsnprintf (self->calling_address, STRING_MAX, format, argptr);
    va_end (argptr);
}


//  --------------------------------------------------------------------------
//  Get/set the called_address field

char *
joza_msg_called_address (joza_msg_t *self)
{
    assert (self);
    return self->called_address;
}

const char *
joza_msg_const_called_address (const joza_msg_t *self)
{
    assert (self);
    return self->called_address;
}

void
joza_msg_set_called_address (joza_msg_t *self, const char *format, ...)
{
    //  Format into newly allocated string
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    free (self->called_address);
    self->called_address = (char *) malloc (STRING_MAX + 1);
    assert (self->called_address);
    vsnprintf (self->called_address, STRING_MAX, format, argptr);
    va_end (argptr);
}


//  --------------------------------------------------------------------------
//  Get/set the packet field

byte
joza_msg_packet (joza_msg_t *self)
{
    assert (self);
    return self->packet;
}

byte
joza_msg_const_packet (const joza_msg_t *self)
{
    assert (self);
    return self->packet;
}

void
joza_msg_set_packet (joza_msg_t *self, byte packet)
{
    assert (self);
    self->packet = packet;
}


//  --------------------------------------------------------------------------
//  Get/set the window field

uint16_t
joza_msg_window (joza_msg_t *self)
{
    assert (self);
    return self->window;
}

uint16_t
joza_msg_const_window (const joza_msg_t *self)
{
    assert (self);
    return self->window;
}

void
joza_msg_set_window (joza_msg_t *self, uint16_t window)
{
    assert (self);
    self->window = window;
}


//  --------------------------------------------------------------------------
//  Get/set the throughput field

byte
joza_msg_throughput (joza_msg_t *self)
{
    assert (self);
    return self->throughput;
}

byte
joza_msg_const_throughput (const joza_msg_t *self)
{
    assert (self);
    return self->throughput;
}

void
joza_msg_set_throughput (joza_msg_t *self, byte throughput)
{
    assert (self);
    self->throughput = throughput;
}


//  --------------------------------------------------------------------------
//  Get/set the cause field

byte
joza_msg_cause (joza_msg_t *self)
{
    assert (self);
    return self->cause;
}

byte
joza_msg_const_cause (const joza_msg_t *self)
{
    assert (self);
    return self->cause;
}

void
joza_msg_set_cause (joza_msg_t *self, byte cause)
{
    assert (self);
    self->cause = cause;
}


//  --------------------------------------------------------------------------
//  Get/set the diagnostic field

byte
joza_msg_diagnostic (joza_msg_t *self)
{
    assert (self);
    return self->diagnostic;
}

byte
joza_msg_const_diagnostic (const joza_msg_t *self)
{
    assert (self);
    return self->diagnostic;
}

void
joza_msg_set_diagnostic (joza_msg_t *self, byte diagnostic)
{
    assert (self);
    self->diagnostic = diagnostic;
}


//  --------------------------------------------------------------------------
//  Get/set the host_name field

char *
joza_msg_host_name (joza_msg_t *self)
{
    assert (self);
    return self->host_name;
}

const char *
joza_msg_const_host_name (const joza_msg_t *self)
{
    assert (self);
    return self->host_name;
}

void
joza_msg_set_host_name (joza_msg_t *self, const char *format, ...)
{
    //  Format into newly allocated string
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    free (self->host_name);
    self->host_name = (char *) malloc (STRING_MAX + 1);
    assert (self->host_name);
    vsnprintf (self->host_name, STRING_MAX, format, argptr);
    va_end (argptr);
}


//  --------------------------------------------------------------------------
//  Get/set the directionality field

byte
joza_msg_directionality (joza_msg_t *self)
{
    assert (self);
    return self->directionality;
}

byte
joza_msg_const_directionality (const joza_msg_t *self)
{
    assert (self);
    return self->directionality;
}

void
joza_msg_set_directionality (joza_msg_t *self, byte directionality)
{
    assert (self);
    self->directionality = directionality;
}


//  --------------------------------------------------------------------------
//  Get/set the workers field

zhash_t *
joza_msg_workers (joza_msg_t *self)
{
    assert (self);
    return self->workers;
}

//  Greedy function, takes ownership of workers; if you don't want that
//  then use zhash_dup() to pass a copy of workers

void
joza_msg_set_workers (joza_msg_t *self, zhash_t *workers)
{
    assert (self);
    zhash_destroy (&self->workers);
    self->workers = workers;
}

//  --------------------------------------------------------------------------
//  Get/set a value in the workers dictionary

char *
joza_msg_workers_string (joza_msg_t *self, char *key, char *default_value)
{
    assert (self);
    char *value = NULL;
    if (self->workers)
        value = (char *) (zhash_lookup (self->workers, key));
    if (!value)
        value = default_value;

    return value;
}

uint64_t
joza_msg_workers_number (joza_msg_t *self, char *key, uint64_t default_value)
{
    assert (self);
    uint64_t value = default_value;
    char *string = NULL;
    if (self->workers)
        string = (char *) (zhash_lookup (self->workers, key));
    if (string)
        value = atol (string);

    return value;
}

void
joza_msg_workers_insert (joza_msg_t *self, char *key, char *format, ...)
{
    //  Format string into buffer
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    char *string = (char *) malloc (STRING_MAX + 1);
    assert (string);
    vsnprintf (string, STRING_MAX, format, argptr);
    va_end (argptr);

    //  Store string in hash table
    if (!self->workers) {
        self->workers = zhash_new ();
        zhash_autofree (self->workers);
    }
    zhash_update (self->workers, key, string);
    free (string);
}

size_t
joza_msg_workers_size (joza_msg_t *self)
{
    return zhash_size (self->workers);
}



//  --------------------------------------------------------------------------
//  Selftest

int
joza_msg_test (bool verbose)
{
    printf (" * joza_msg: ");

    //  @selftest
    //  Simple create/destroy test
    joza_msg_t *self = joza_msg_new (0);
    assert (self);
    joza_msg_destroy (&self);

    //  Create pair of sockets we can send through
    zctx_t *ctx = zctx_new ();
    assert (ctx);

    void *output = zsocket_new (ctx, ZMQ_DEALER);
    assert (output);
    zsocket_bind (output, "inproc://selftest");
    void *input = zsocket_new (ctx, ZMQ_ROUTER);
    assert (input);
    zsocket_connect (input, "inproc://selftest");

    //  Encode/send/decode and verify each message type

    self = joza_msg_new (JOZA_MSG_DATA);
    joza_msg_set_q (self, 123);
    joza_msg_set_pr (self, 123);
    joza_msg_set_ps (self, 123);
    joza_msg_set_data (self, zframe_new ("Captcha Diem", 12));
    joza_msg_send (&self, output);

    self = joza_msg_recv (input);
    assert (self);
    assert (joza_msg_q (self) == 123);
    assert (joza_msg_pr (self) == 123);
    assert (joza_msg_ps (self) == 123);
    assert (zframe_streq (joza_msg_data (self), "Captcha Diem"));
    joza_msg_destroy (&self);

    self = joza_msg_new (JOZA_MSG_RR);
    joza_msg_set_pr (self, 123);
    joza_msg_send (&self, output);

    self = joza_msg_recv (input);
    assert (self);
    assert (joza_msg_pr (self) == 123);
    joza_msg_destroy (&self);

    self = joza_msg_new (JOZA_MSG_RNR);
    joza_msg_set_pr (self, 123);
    joza_msg_send (&self, output);

    self = joza_msg_recv (input);
    assert (self);
    assert (joza_msg_pr (self) == 123);
    joza_msg_destroy (&self);

    self = joza_msg_new (JOZA_MSG_CALL_REQUEST);
    joza_msg_set_calling_address (self, "Life is short but Now lasts for ever");
    joza_msg_set_called_address (self, "Life is short but Now lasts for ever");
    joza_msg_set_packet (self, 123);
    joza_msg_set_window (self, 123);
    joza_msg_set_throughput (self, 123);
    joza_msg_set_data (self, zframe_new ("Captcha Diem", 12));
    joza_msg_send (&self, output);

    self = joza_msg_recv (input);
    assert (self);
    assert (streq (joza_msg_calling_address (self), "Life is short but Now lasts for ever"));
    assert (streq (joza_msg_called_address (self), "Life is short but Now lasts for ever"));
    assert (joza_msg_packet (self) == 123);
    assert (joza_msg_window (self) == 123);
    assert (joza_msg_throughput (self) == 123);
    assert (zframe_streq (joza_msg_data (self), "Captcha Diem"));
    joza_msg_destroy (&self);

    self = joza_msg_new (JOZA_MSG_CALL_ACCEPTED);
    joza_msg_set_calling_address (self, "Life is short but Now lasts for ever");
    joza_msg_set_called_address (self, "Life is short but Now lasts for ever");
    joza_msg_set_packet (self, 123);
    joza_msg_set_window (self, 123);
    joza_msg_set_throughput (self, 123);
    joza_msg_set_data (self, zframe_new ("Captcha Diem", 12));
    joza_msg_send (&self, output);

    self = joza_msg_recv (input);
    assert (self);
    assert (streq (joza_msg_calling_address (self), "Life is short but Now lasts for ever"));
    assert (streq (joza_msg_called_address (self), "Life is short but Now lasts for ever"));
    assert (joza_msg_packet (self) == 123);
    assert (joza_msg_window (self) == 123);
    assert (joza_msg_throughput (self) == 123);
    assert (zframe_streq (joza_msg_data (self), "Captcha Diem"));
    joza_msg_destroy (&self);

    self = joza_msg_new (JOZA_MSG_CLEAR_REQUEST);
    joza_msg_set_cause (self, 123);
    joza_msg_set_diagnostic (self, 123);
    joza_msg_send (&self, output);

    self = joza_msg_recv (input);
    assert (self);
    assert (joza_msg_cause (self) == 123);
    assert (joza_msg_diagnostic (self) == 123);
    joza_msg_destroy (&self);

    self = joza_msg_new (JOZA_MSG_CLEAR_CONFIRMATION);
    joza_msg_send (&self, output);

    self = joza_msg_recv (input);
    assert (self);
    joza_msg_destroy (&self);

    self = joza_msg_new (JOZA_MSG_RESET_REQUEST);
    joza_msg_set_cause (self, 123);
    joza_msg_set_diagnostic (self, 123);
    joza_msg_send (&self, output);

    self = joza_msg_recv (input);
    assert (self);
    assert (joza_msg_cause (self) == 123);
    assert (joza_msg_diagnostic (self) == 123);
    joza_msg_destroy (&self);

    self = joza_msg_new (JOZA_MSG_RESET_CONFIRMATION);
    joza_msg_send (&self, output);

    self = joza_msg_recv (input);
    assert (self);
    joza_msg_destroy (&self);

    self = joza_msg_new (JOZA_MSG_CONNECT);
    joza_msg_set_calling_address (self, "Life is short but Now lasts for ever");
    joza_msg_set_host_name (self, "Life is short but Now lasts for ever");
    joza_msg_set_directionality (self, 123);
    joza_msg_send (&self, output);

    self = joza_msg_recv (input);
    assert (self);
    assert (streq (joza_msg_calling_address (self), "Life is short but Now lasts for ever"));
    assert (streq (joza_msg_host_name (self), "Life is short but Now lasts for ever"));
    assert (joza_msg_directionality (self) == 123);
    joza_msg_destroy (&self);

    self = joza_msg_new (JOZA_MSG_CONNECT_INDICATION);
    joza_msg_send (&self, output);

    self = joza_msg_recv (input);
    assert (self);
    joza_msg_destroy (&self);

    self = joza_msg_new (JOZA_MSG_DISCONNECT);
    joza_msg_send (&self, output);

    self = joza_msg_recv (input);
    assert (self);
    joza_msg_destroy (&self);

    self = joza_msg_new (JOZA_MSG_DISCONNECT_INDICATION);
    joza_msg_send (&self, output);

    self = joza_msg_recv (input);
    assert (self);
    joza_msg_destroy (&self);

    self = joza_msg_new (JOZA_MSG_DIAGNOSTIC);
    joza_msg_set_cause (self, 123);
    joza_msg_set_diagnostic (self, 123);
    joza_msg_send (&self, output);

    self = joza_msg_recv (input);
    assert (self);
    assert (joza_msg_cause (self) == 123);
    assert (joza_msg_diagnostic (self) == 123);
    joza_msg_destroy (&self);

    self = joza_msg_new (JOZA_MSG_DIRECTORY_REQUEST);
    joza_msg_send (&self, output);

    self = joza_msg_recv (input);
    assert (self);
    joza_msg_destroy (&self);

    self = joza_msg_new (JOZA_MSG_DIRECTORY);
    joza_msg_workers_insert (self, "Name", "Brutus");
    joza_msg_workers_insert (self, "Age", "%d", 43);
    joza_msg_send (&self, output);

    self = joza_msg_recv (input);
    assert (self);
    assert (joza_msg_workers_size (self) == 2);
    assert (streq (joza_msg_workers_string (self, "Name", "?"), "Brutus"));
    assert (joza_msg_workers_number (self, "Age", 0) == 43);
    joza_msg_destroy (&self);

    self = joza_msg_new (JOZA_MSG_ENQ);
    joza_msg_send (&self, output);

    self = joza_msg_recv (input);
    assert (self);
    joza_msg_destroy (&self);

    self = joza_msg_new (JOZA_MSG_ACK);
    joza_msg_send (&self, output);

    self = joza_msg_recv (input);
    assert (self);
    joza_msg_destroy (&self);

    zctx_destroy (&ctx);
    //  @end

    printf ("OK\n");
    return 0;
}
