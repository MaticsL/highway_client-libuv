/* Copyright (C) 2013-2014, Hsiang Kao (e0e1e) <esxgx@163.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef __RECORD_H
#define __RECORD_H

#define _declrec(x)	struct x;	\
	typedef struct x _ ## x;

#define _record(x)		_declrec(x)	\
	struct x {
#define _end_record(x,...)	} __VA_ARGS__;

#include "typedef.h"

#endif	// __FRAME_H
