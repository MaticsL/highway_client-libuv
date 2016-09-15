/* Copyright (C) 2015, Hsiang Kao (e0e1e) <0xe0e1e@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef __HIGHWAY_TUN_PUB_H
#define __HIGHWAY_TUN_PUB_H

#include "tun.h"

_declrec(envir)
int tunnelmgr(_envir *, char *, u16, _tunnelinfo *);

_declrec(tunnel)
void tunnel_close(_tunnel *);
void tunnel_proxyconn_exit(_tunnel *);
int tunnel_proxyconn_connect_try_reply(_tunnel *, int);
int tunnel_proxyconn_write(_tunnel *, char *, int);
int tunnel_proxyconn_is_tcp_stream(_tunnel *);

#endif
