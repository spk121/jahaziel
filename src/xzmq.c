#include <zmq.h>
#include <czmq.h>
#include <glib.h>
#include <stdarg.h>
#include "xzmq.h"

zctx_t *xzctx_new(void)
{
    zctx_t *c;

    c = zctx_new();
    if (c == NULL)
        g_critical ("zctx_new returned NULL");
    return c;
}

void *xzsocket_new (zctx_t *self, int type)
{
    void *sock;
    g_return_val_if_fail (self != NULL, NULL);
    sock = zsocket_new(self, type);
    if (sock == NULL)
        g_critical ("z_socket returned NULL");
    return sock;
}

int xzsocket_connect (void *socket, const char *format, ...)
{
#define ADDR_LEN (100)
  va_list ap;
  char buf[ADDR_LEN+1];
  int i, ret;
 
  g_return_val_if_fail(socket != NULL, -1);
  g_return_val_if_fail(format != NULL, -1);
  g_return_val_if_fail(strlen(format) > 0 && strlen(format) < ADDR_LEN, -1);
  va_start(ap, format); 
  i = vsnprintf(buf, ADDR_LEN+1, format, ap);
  if (i > ADDR_LEN || i <= 0) {
      g_critical("network name bad or too long in %s", __FUNCTION__);
      return -1;
  }
  va_end(ap);
  ret = zsocket_connect(socket, buf);
  if (ret == -1)
      g_critical("zsocket_connect returns -1");
  return ret;
#undef ADDR_LEN
}
