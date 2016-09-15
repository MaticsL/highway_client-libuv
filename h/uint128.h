/* Copyright (C) 2013-2014, Hsiang Kao (e0e1e) <esxgx@163.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef __UINT128_H
#define __UINT128_H

#include "frame.h"

_DFa(u128)
	union {
		u64	data64[2];
		u64	data32[4];
		u16	data16[8];
	};
_DFz(u128)

typedef _u128	u128_t;

#endif
