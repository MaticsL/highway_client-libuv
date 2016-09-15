/* Copyright (C) 2015, Hsiang Kao (e0e1e) <0xe0e1e@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "highway/exbuf.h"
#include "die.h"

#define front	ex->front
#define back	ex->back

int exbuf_length(_exbuf *ex) {
	return front <= back? back-front: EXBUF_DATASIZ - front + back;
}

#include <string.h>

void exbuf_push(_exbuf *ex, char *buf, int siz)
{
	int remaining = EXBUF_DATASIZ - exbuf_length(ex);
	if (remaining < siz)
		die("exbuf_push: exbuf isn't large enough to contain all the data [siz=%d]", siz);
	else {
		if (front <= back) {		// copy once
			int rl = EXBUF_DATASIZ - back;
			if (siz < rl) {
				memcpy(ex->base + back, buf, siz); back += siz;
				return;
			} else {
				memcpy(ex->base + back, buf, rl);
				siz -= rl, buf += rl;
				back = 0;
			}
		}
		if (siz) {
			memcpy(ex->base + back, buf, siz);
			memcpy(ex->base + EXBUF_DATASIZ + back, buf, siz);
			back += siz;
		}
	}
}
#