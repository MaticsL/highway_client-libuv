/* Copyright (C) 2015, Hsiang Kao (e0e1e) <0xe0e1e@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef __HIGHWAY_ENCRYPT_H
#define __HIGHWAY_ENCRYPT_H

#include "record.h"

_declrec(methodinfo)
_declrec(cipher)

_record(methodinfo_0)
	int (*key_length)(_methodinfo *);
	int (*iv_length)(_methodinfo *);
	_cipher *(*cipher_init)(_methodinfo *, char *, char *, int);
	int (*cipher_update)(_cipher *, char *, int, char *);
	int (*cipher_final)(_cipher *, char *);
	void (*cipher_exit)(_cipher *);
_end_record(methodinfo_0)

#endif
