/* Copyright (C) 2015, Hsiang Kao (e0e1e) <0xe0e1e@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef __HIGHWAY_TUN_LIBCURL_PUB_H
#define __HIGHWAY_TUN_LIBCURL_PUB_H

#include "record.h"

#include <stddef.h>

_declrec(tunnel)
void tunnel_libcurl_close(_tunnel *);
int tunnel_libcurl_proxyconn_exit(_tunnel *);
int tunnel_libcurl_postconn_write(_tunnel *, size_t, char *);
int libcurlmgr_tunnel_connecthttpserver(_tunnel *);

#endif
