/*
    xzmq.h - simplified ZeroMQ header

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
#include <czmq.h>

zctx_t *xzctx_new(void);
void *xzsocket_new (zctx_t *self, int type);
int xzsocket_connect (void *socket, const char *format, ...);

