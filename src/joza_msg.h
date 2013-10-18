/*
    joza_msg.h - transport for switched virtual call messages

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

#ifndef __JOZA_MSG_H_INCLUDED__
#define __JOZA_MSG_H_INCLUDED__

#ifdef  __cplusplus
extern "C" {
#else
#include <stdbool.h>
#endif

#define _GNU_SOURCE
#include <string.h>
#include <stdint.h>
#include <zmq.h>
#include <czmq.h>

/*  These are the joza_msg messages
    DATA - Binary data message.
        q             number 1
        pr            number 2
        ps            number 2
        data          frame
    RR - Tells peer the lowest send sequence number that it may send in its DATA packet.
        pr            number 2
    RNR - Tells peer to stop sending data.
        pr            number 2
    CALL_REQUEST - Call a peer. Negotiate the type of connection requested.
        calling_address  string
        called_address  string
        packet        number 1
        window        number 2
        throughput    number 1
        data          frame
    CALL_ACCEPTED - Answer the call.
        calling_address  string
        called_address  string
        packet        number 1
        window        number 2
        throughput    number 1
        data          frame
    CLEAR_REQUEST - Request call termination
        cause         number 1
        diagnostic    number 1
    CLEAR_CONFIRMATION - Accept call termination
    RESET_REQUEST - Tell the peer to restart flow control
        cause         number 1
        diagnostic    number 1
    RESET_CONFIRMATION - Tell the peer that we have restarted flow control
    CONNECT - Client node requests connection to the broker.
        protocol      string
        version       number 1
        calling_address  string
        directionality  number 1
    CONNECT_INDICATION - Broker tells node that it has been connected.
    DISCONNECT - Node tells broker that it is disconnecting.
    DISCONNECT_INDICATION - Broker tells node that it has been disconnected
    DIAGNOSTIC - This is the Switched Virtual Call protocol version 1
        cause         number 1
        diagnostic    number 1
    DIRECTORY_REQUEST - This is the Switched Virtual Call protocol version 1
    DIRECTORY - This is the Switched Virtual Call protocol version 1
        workers       dictionary
*/

#define JOZA_MSG_VERSION                    1
#define JOZA_MSG_COUNT                      16

#define JOZA_MSG_DATA                       0
#define JOZA_MSG_RR                         1
#define JOZA_MSG_RNR                        2
#define JOZA_MSG_CALL_REQUEST               3
#define JOZA_MSG_CALL_ACCEPTED              4
#define JOZA_MSG_CLEAR_REQUEST              5
#define JOZA_MSG_CLEAR_CONFIRMATION         6
#define JOZA_MSG_RESET_REQUEST              7
#define JOZA_MSG_RESET_CONFIRMATION         8
#define JOZA_MSG_CONNECT                    9
#define JOZA_MSG_CONNECT_INDICATION         10
#define JOZA_MSG_DISCONNECT                 11
#define JOZA_MSG_DISCONNECT_INDICATION      12
#define JOZA_MSG_DIAGNOSTIC                 13
#define JOZA_MSG_DIRECTORY_REQUEST          14
#define JOZA_MSG_DIRECTORY                  15

//  Structure of our class

struct _joza_msg_t {
    zframe_t *address;          //  Address of peer if any
    int id;                     //  joza_msg message ID
    byte *needle;               //  Read/write pointer for serialization
    byte *ceiling;              //  Valid upper limit for read pointer
    byte q;
    uint16_t pr;
    uint16_t ps;
    zframe_t *data;
    char *calling_address;
    char *called_address;
    byte packet;
    uint16_t window;
    byte throughput;
    byte cause;
    byte diagnostic;
    char *protocol;
    byte version;
    byte directionality;
    zhash_t *workers;
    size_t workers_bytes;       //  Size of dictionary content
};

//  Opaque class structure
typedef struct _joza_msg_t joza_msg_t;

//  @interface
//  Create a new joza_msg
joza_msg_t *
joza_msg_new (int id);

//  Destroy the joza_msg
void
joza_msg_destroy (joza_msg_t **self_p);

//  Receive and parse a joza_msg from the input
joza_msg_t *
joza_msg_recv (void *input);

//  Send the joza_msg to the output, and destroy it
int
joza_msg_send (joza_msg_t **self_p, void *output);

//  Send the DATA to the output in one step
int
joza_msg_send_data (void *output,
                    byte q,
                    uint16_t pr,
                    uint16_t ps,
                    zframe_t *data);

int
joza_msg_send_addr_data (void *output, const zframe_t *addr,
                         byte q,
                         uint16_t pr,
                         uint16_t ps,
                         zframe_t *data);

//  Send the RR to the output in one step
int
joza_msg_send_rr (void *output,
                  uint16_t pr);

int
joza_msg_send_addr_rr (void *output, const zframe_t *addr,
                       uint16_t pr);

//  Send the RNR to the output in one step
int
joza_msg_send_rnr (void *output,
                   uint16_t pr);

int
joza_msg_send_addr_rnr (void *output, const zframe_t *addr,
                        uint16_t pr);

//  Send the CALL_REQUEST to the output in one step
int
joza_msg_send_call_request (void *output,
                            char *calling_address,
                            char *called_address,
                            byte packet,
                            uint16_t window,
                            byte throughput,
                            zframe_t *data);

int
joza_msg_send_addr_call_request (void *output, const zframe_t *addr,
                                 char *calling_address,
                                 char *called_address,
                                 byte packet,
                                 uint16_t window,
                                 byte throughput,
                                 zframe_t *data);

//  Send the CALL_ACCEPTED to the output in one step
int
joza_msg_send_call_accepted (void *output,
                             char *calling_address,
                             char *called_address,
                             byte packet,
                             uint16_t window,
                             byte throughput,
                             zframe_t *data);

int
joza_msg_send_addr_call_accepted (void *output, const zframe_t *addr,
                                  char *calling_address,
                                  char *called_address,
                                  byte packet,
                                  uint16_t window,
                                  byte throughput,
                                  zframe_t *data);

//  Send the CLEAR_REQUEST to the output in one step
int
joza_msg_send_clear_request (void *output,
                             byte cause,
                             byte diagnostic);

int
joza_msg_send_addr_clear_request (void *output, const zframe_t *addr,
                                  byte cause,
                                  byte diagnostic);

//  Send the CLEAR_CONFIRMATION to the output in one step
int
joza_msg_send_clear_confirmation (void *output);

int
joza_msg_send_addr_clear_confirmation (void *output, const zframe_t *addr);

//  Send the RESET_REQUEST to the output in one step
int
joza_msg_send_reset_request (void *output,
                             byte cause,
                             byte diagnostic);

int
joza_msg_send_addr_reset_request (void *output, const zframe_t *addr,
                                  byte cause,
                                  byte diagnostic);

//  Send the RESET_CONFIRMATION to the output in one step
int
joza_msg_send_reset_confirmation (void *output);

int
joza_msg_send_addr_reset_confirmation (void *output, const zframe_t *addr);

//  Send the CONNECT to the output in one step
int
joza_msg_send_connect (void *output,
                       char *calling_address,
                       byte directionality);

int
joza_msg_send_addr_connect (void *output, const zframe_t *addr,
                            char *calling_address,
                            byte directionality);

//  Send the CONNECT_INDICATION to the output in one step
int
joza_msg_send_connect_indication (void *output);

int
joza_msg_send_addr_connect_indication (void *output, const zframe_t *addr);

//  Send the DISCONNECT to the output in one step
int
joza_msg_send_disconnect (void *output);

int
joza_msg_send_addr_disconnect (void *output, const zframe_t *addr);

//  Send the DISCONNECT_INDICATION to the output in one step
int
joza_msg_send_disconnect_indication (void *output);

int
joza_msg_send_addr_disconnect_indication (void *output, const zframe_t *addr);

//  Send the DIAGNOSTIC to the output in one step
int
joza_msg_send_diagnostic (void *output,
                          byte cause,
                          byte diagnostic);

int
joza_msg_send_addr_diagnostic (void *output, const zframe_t *addr,
                               byte cause,
                               byte diagnostic);

//  Send the DIRECTORY_REQUEST to the output in one step
int
joza_msg_send_directory_request (void *output);

int
joza_msg_send_addr_directory_request (void *output, const zframe_t *addr);

//  Send the DIRECTORY to the output in one step
int
joza_msg_send_directory (void *output,
                         zhash_t *workers);

int
joza_msg_send_addr_directory (void *output, const zframe_t *addr,
                              zhash_t *workers);

//  Duplicate the joza_msg message
joza_msg_t *
joza_msg_dup (joza_msg_t *self);

//  Print contents of message to stdout
void
joza_msg_dump (joza_msg_t *self);

//  Get/set the message address
zframe_t *
joza_msg_address (joza_msg_t *self);
const zframe_t *
joza_msg_const_address (const joza_msg_t *self);
void
joza_msg_set_address (joza_msg_t *self, zframe_t *address);

//  Get the joza_msg id and printable command
int
joza_msg_id (joza_msg_t *self);
int
joza_msg_const_id (const joza_msg_t *self);
void
joza_msg_set_id (joza_msg_t *self, int id);
const char *
joza_msg_const_command (const joza_msg_t *self);

//  Get/set the q field
byte
joza_msg_q (joza_msg_t *self);
byte
joza_msg_const_q (const joza_msg_t *self);
void
joza_msg_set_q (joza_msg_t *self, byte q);

//  Get/set the pr field
uint16_t
joza_msg_pr (joza_msg_t *self);
uint16_t
joza_msg_const_pr (const joza_msg_t *self);
void
joza_msg_set_pr (joza_msg_t *self, uint16_t pr);

//  Get/set the ps field
uint16_t
joza_msg_ps (joza_msg_t *self);
uint16_t
joza_msg_const_ps (const joza_msg_t *self);
void
joza_msg_set_ps (joza_msg_t *self, uint16_t ps);

//  Get/set the data field
zframe_t *
joza_msg_data (joza_msg_t *self);
const zframe_t *
joza_msg_const_data (const joza_msg_t *self);
void
joza_msg_set_data (joza_msg_t *self, zframe_t *frame);

//  Get/set the calling_address field
char *
joza_msg_calling_address (joza_msg_t *self);
const char *
joza_msg_const_calling_address (const joza_msg_t *self);
void
joza_msg_set_calling_address (joza_msg_t *self, const char *format, ...);

//  Get/set the called_address field
char *
joza_msg_called_address (joza_msg_t *self);
const char *
joza_msg_const_called_address (const joza_msg_t *self);
void
joza_msg_set_called_address (joza_msg_t *self, const char *format, ...);

//  Get/set the packet field
byte
joza_msg_packet (joza_msg_t *self);
byte
joza_msg_const_packet (const joza_msg_t *self);
void
joza_msg_set_packet (joza_msg_t *self, byte packet);

//  Get/set the window field
uint16_t
joza_msg_window (joza_msg_t *self);
uint16_t
joza_msg_const_window (const joza_msg_t *self);
void
joza_msg_set_window (joza_msg_t *self, uint16_t window);

//  Get/set the throughput field
byte
joza_msg_throughput (joza_msg_t *self);
byte
joza_msg_const_throughput (const joza_msg_t *self);
void
joza_msg_set_throughput (joza_msg_t *self, byte throughput);

//  Get/set the cause field
byte
joza_msg_cause (joza_msg_t *self);
byte
joza_msg_const_cause (const joza_msg_t *self);
void
joza_msg_set_cause (joza_msg_t *self, byte cause);

//  Get/set the diagnostic field
byte
joza_msg_diagnostic (joza_msg_t *self);
byte
joza_msg_const_diagnostic (const joza_msg_t *self);
void
joza_msg_set_diagnostic (joza_msg_t *self, byte diagnostic);

//  Get/set the directionality field
byte
joza_msg_directionality (joza_msg_t *self);
byte
joza_msg_const_directionality (const joza_msg_t *self);
void
joza_msg_set_directionality (joza_msg_t *self, byte directionality);

//  Get/set the workers field
zhash_t *
joza_msg_workers (joza_msg_t *self);
void
joza_msg_set_workers (joza_msg_t *self, zhash_t *workers);

//  Get/set a value in the workers dictionary
char *
joza_msg_workers_string (joza_msg_t *self, char *key, char *default_value);
uint64_t
joza_msg_workers_number (joza_msg_t *self, char *key, uint64_t default_value);
void
joza_msg_workers_insert (joza_msg_t *self, char *key, char *format, ...);
size_t
joza_msg_workers_size (joza_msg_t *self);

//  Self test of this class
int
joza_msg_test (bool verbose);
//  @end

#ifdef  __cplusplus
}
#endif

#endif
