/*
    network.c - network interface

    Copyright 2013 Michael L. Gran <spk121@yahoo.com>

    This file is part of Jahaziel

    Jahaziel is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Jahaziel is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Jahaziel.  If not, see <http://www.gnu.org/licenses/>.

*/

/*
  These are functions related to the network interface: ZeroMQ sockets that
  connect to a Jozabad-style broker.
*/
#include <gtk/gtk.h>
#include "joza_msg.h"
#include "xzmq.h"
#include "proc.h"
#include "statusbar.h"
#include "ui.h"
#include "xgtk.h"

enum {UNBOUND, DISCONNECTED, CONNECTING, CONNECTED, DISCONNECTING} net_state = UNBOUND;

#define JAH_PORT 5555
#define JAH_TIMEOUT_CONNECT 1000

zctx_t *ctx = NULL;
void *sock = NULL;

struct proc_connect_data_tag
{
    gchar *server;
    gchar *username;
    gint state;
};

typedef struct proc_connect_data_tag proc_connect_data_t;

static gboolean broker_connect_func (gpointer data)
{
    proc_connect_data_t *wdata = (proc_connect_data_t *) data;
    gint *state = &(wdata ->state);
    int immed = TRUE;

    g_return_val_if_fail (net_state != UNBOUND, FALSE);

    if (g_atomic_int_get(state) == -1)
    {
        // waiting for something to complete
        return TRUE;
    }
    else if (g_atomic_int_get(state) == 0)
    {
        // State 0: A new connection request
        g_atomic_int_set(state, -1);

        // Wait if we're in the middle of something
        if (net_state == DISCONNECTING || net_state == CONNECTING)
            g_atomic_int_set(state, 0);
        else if (net_state == CONNECTED)
            g_atomic_int_set(state, 1);
        else if (net_state == DISCONNECTED)
            g_atomic_int_set(state, 2);
        else
            g_return_val_if_reached(FALSE);

        return TRUE;
    }
    else if (g_atomic_int_get(state) == 1) 
    {
        // State 1: Connect request received whilst already connected
        g_atomic_int_set(state, -1);
        net_state = DISCONNECTING;
        joza_msg_send_disconnect(sock);
        zsocket_destroy(ctx, sock);
        sock = NULL;
        net_state = DISCONNECTED;

        g_atomic_int_set(state, 2);
        return TRUE;
    }
    else if (g_atomic_int_get(state) == 2)
    {
        // State 2: connect a socket to a broker
        g_atomic_int_set(state, -1);
        net_state = CONNECTING;
        gchar *addr = g_strdup_printf("tcp://%s:%d", wdata->server, JAH_PORT);
        int ret;

        pm_queue_proc(proc_status_message_new("connect", addr));
        ret = zsocket_connect(sock, addr); 

        if (ret == -1)
        {
            // Connection failure
            // pop up warning dialog
            // give up
            
            g_atomic_int_set(state, 3);
            gchar *str = g_strdup_printf("Error connecting to '%s': %s",
                                         addr, zmq_strerror (errno));
            xgtk_message_dialog(GTK_WINDOW(ui->main_window), str);
            g_free (str);
        }
        else
        {
            // FIXME: handle directionality correctly
            pm_queue_proc(proc_status_message_new("connect", wdata->username));
            joza_msg_send_connect(sock, wdata->username, 0);
            bool pret = zsocket_poll (sock, JAH_TIMEOUT_CONNECT);
            if (pret == TRUE)
            {
                joza_msg_t *msg = joza_msg_recv (sock);
                if (joza_msg_id(msg) == JOZA_MSG_CONNECT_INDICATION)
                {
                    // success

                    // Enable
                    g_atomic_int_set(state, 4);
                }
                else 
                {
                    if (joza_msg_id(msg) == JOZA_MSG_DIAGNOSTIC)
                        // failure with info
                        // pop up warning dialog
                        ;
                    else if (joza_msg_id(msg) == JOZA_MSG_DISCONNECT_INDICATION)
                        // failure without info
                        // pop up warning dialog
                        ;
                    else
                        // unexpected message
                        // disconnect
                        ;
                    gchar *str = g_strdup_printf("Connection rejected by '%s'", addr);
                    xgtk_message_dialog(GTK_WINDOW(ui->main_window), str);
                    g_free (str);

                    g_atomic_int_set(state, 3);
                } 
            }
            else 
            {
                // timeout
                // pop up warning
                // disconnect
                gchar *str = g_strdup_printf("No response from '%s'", addr);
                xgtk_message_dialog(GTK_WINDOW(ui->main_window), str);
                g_free (str);

                g_atomic_int_set(state, 3);
            }
        }
        g_free(addr);

        return TRUE;
    }
    else if (g_atomic_int_get(state) == 3)
    {
        // Failed to connect
        g_atomic_int_set(state, -1);
        net_state = DISCONNECTED;

        // FIXME: remove this sock as a source of main loop data

        goto clean;
        g_return_val_if_reached (FALSE);
    }
    else if (g_atomic_int_get(state) == 4)
    {
        // Connected
        g_atomic_int_set(state, -1);
        net_state = CONNECTED;
        pm_queue_proc(proc_status_message_new("connect", "Connected"));

        // FIXME: add this socket as a source of main loop data

    clean:
        g_free(wdata->server);
        wdata->server = NULL;
        g_free(wdata->username);
        wdata->username = NULL;
        g_free(wdata);
        wdata = NULL;

        return FALSE;
    }
    else
        g_return_val_if_reached(FALSE);
}

static gboolean proc_connect_func (gpointer data)
{
    proc_connect_data_t *wdata = (proc_connect_data_t *) data;

    g_assert (data != NULL);

    if (net_state == UNBOUND)
    {
        ctx = xzctx_new ();
        net_state = DISCONNECTED;
    }

    if (sock == NULL)
        sock = xzsocket_new(ctx, ZMQ_DEALER);
    
    g_assert(sock != NULL);

    gboolean ret = broker_connect_func(data);
    return ret;
}

GHook *
proc_connect_new (gchar *server, gchar *username)
{
    GHook *hook = pm_proc_new();
    proc_connect_data_t *connect_data = g_new0 (proc_connect_data_t, 1);
    connect_data->server = g_strdup(server);
    connect_data->username = g_strdup(username);
    hook->data = connect_data;
    hook->func = proc_connect_func;
    g_free (server);
    g_free(username);
    return hook;
}


#if 0
static void initialize(int verbose, const char *preface, zctx_t **ctx, void **sock, char *broker,
                       char *calling_address, iodir_t dir)
{
    int ret;

    g_return_if_fail (net_state == UNBOUND);
    
    status_bar_print("Initializing socket.");

    *ctx = xzctx_new ();
    *sock = xzsocket_new(*ctx, ZMQ_DEALER);
    if (sock == NULL)
        status_bar_print("Socket initialization failed");
}

static void broker_input_cb (GObject *source, GAsyncResult *result, gpointer data)
{
    broker_connect_t *s = (broker_connect_t *) data;

    GInputStream *in = G_INPUT_STREAM (source);
    GError *error = NULL;
    gssize nread;
    
    nread = g_input_stream_read_finish (in, result, &error);
    if (nread > 0)
    {
        g_assert_no_error (error);
        g_string_append_len(s->buf, async_read_buffer, nread);
        g_atomic_int_set(&(s->state), 5);
    }
}


static gboolean broker_connect_func (gpointer data)
{
    broker_connect_t *s = (broker_connect_t *) data;
    gint *state = &(s->state);
    int immed = TRUE;

    g_return_val_if_fail (net_state != UNBOUND, FALSE);

    if (g_atomic_int_get(state) == 0)
    {
        // State 0: A new connection request

        // Wait if we're in the middle of something
        if (net_state == DISCONNECTING || net_state == CONNECTING)
            return TRUE;
        else if (net_state == CONNECTED)
            g_atomic_int_set(state, 1);
        else if (net_state == DISCONNECTED)
            g_atomic_int_set(state, 3);

        g_return_val_if_reached(FALSE);
    }
    else if (g_atomic_int_get(state) == 1) 
    {
        // State 1: Connect request received whilst already connected
        g_atomic_int_set(state, 2);

        // If we're connected to the correct broker with the right
        // username and directionality, then do nothing. (Maybe?)

        // Otherwise, tear down this connection.
        // - send a disconnect request to the broker
        // - disconnect the underlying socket
        // - jump to next step
        return (TRUE);
    }
    else if (g_atomic_int_get(state) == 2)
    {
        // State 2: another thread is disconnecting 
        return (TRUE);
    }
    else if (g_atomic_int_get(state) == 3)
    {
        // State 3: Connect a socket to a broker
        g_atomic_int_set(state, 4);

        zsocket_connect(*sock, broker);
        return (TRUE);
    }
    else if (g_atomic_int_get(state) == 4)
    {
        // State 4: send a connect request with a given username and directionaltiy
    }
    else if (g_atomic_int_get(state) == 5)
    {
        // State 5: poll for a connect response or clear request
        bool ret = zsocket_poll (broker_sock, TIMEOUT_CONNECT);
        if (ret == TRUE) {
            joza_msg_t *msg = joza_msg_recv (void *input);
            if (joza_msg_id(msg) == JOZA_MSG_CONNECT_INDICATION)
                // success
                ;
            else if (joza_msg_id(msg) == JOZA_MSG_DISCONNECT_INDICATION) {
                // failure
                ;
            } 
            else {
                // failure
            }

            return (TRUE);
    }
    else if (g_atomic_int_get(state) == 5)
    {
        // State 5: received a connect indication
        g_atomic_int_set(state, 6);
        return (TRUE);
    }
    else if (g_atomic_int_get(state) == 6)
    {
        // State 6: waiting for a connect indication to be processed
    }
    else if (g_atomic_int_get(state) == 7)
    {
        // State 7: received a clear request
    }
    else if (g_atomic_int_get(state) == 8)
    {
        // State 8: waiting for a clear request to be processed
    }
    else if (g_atomic_int_get(state) == 9)
    {
        // State 9: we are complete
        return (FALSE);
    }
    else
        g_return_val_if_reached (FALSE);
}
    

void conct(void)
{
    int ret;
    
    // Part 1

    // Status bar "connection to broker"

    ret = xzsocket_connect(*sock, broker);
    if (ret == -1)
    {
        // Can't connect to broker
        // Popup dialog: can't connect to broker
        ;
    }

    // Part 2

    // Status bar "registering as 'identity' at 'broker'
    ret = joza_msg_send_connect(*sock, identity, dir);
    if (ret != 0)

    if (verbose)
        printf("%s: connecting to broker as %s, (%s)\n",
               preface,
               calling_address,
               iodir_name(dir));
    ret = joza_msg_send_connect(*sock, calling_address, dir);
    if (ret != 0) {
        if (verbose)
            printf("%s: joza_msg_send_connect (%p, %s, %s) returned %d\n", preface,
                   *sock,
                   calling_address,
                   iodir_name(dir),
                   ret);
        exit(1);
    }

    // Broker must respond within timeout
    //timeout1 = true;
    // signal (SIGALRM, catch_alarm);
    //alarm(10);

    if (verbose)
        printf("%s: waiting for connect indication\n", preface);
    joza_msg_t *response = joza_msg_recv(*sock);
    //timeout1 = false;
    if (joza_msg_id(response) != JOZA_MSG_CONNECT_INDICATION) {
        if (verbose) {
            printf("%s: did not receive connect indication\n", preface);
            joza_msg_dump(response);
        }
        exit (1);
    }   
}

#endif
