/* Copyright (C) 2015, Hsiang Kao (e0e1e) <0xe0e1e@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef __HIGHWAY_EXBUF_H
#define __HIGHWAY_EXBUF_H

#include "record.h"

//
// include/curl/curl.h: #define CURL_MAX_WRITE_SIZE 16384
// lib/urldata.h: #define BUFSIZE CURL_MAX_WRITE_SIZE
//
// void (*uv_alloc_cb)(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
// A suggested size (65536 at the moment) is provided, but it doesn't need to be honored.
//

// slightly large than twice the maximum size of curl/uv recv buffers(= 64KB)
#define EXBUF_DATASIZ		(132*1024)

_record(exbuf)
	char base[EXBUF_DATASIZ << 1];
	int front, back;
_end_record(exbuf)

#define exbuf_peekat(a, x)		((a)->base[(a)->front+(x)])
#define exbuf_peek2at(a, x)		(*(u16 *)&exbuf_peekat(a,x))
#define exbuf_peek4at(a, x)		(*(u32 *)&exbuf_peekat(a,x))
#define exbuf_clear(a)		((a)->front = (a)->back = 0)

static inline void exbuf_pop(_exbuf *ex, int siz) {
	ex->front = (ex->front + siz) % EXBUF_DATASIZ;
}
int exbuf_length(_exbuf *ex);
void exbuf_push(_exbuf *, char *, int);

#endif
