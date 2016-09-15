/* Copyright (C) 2015, Hsiang Kao (e0e1e) <0xe0e1e@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <uv.h>
#include "highway/envir_impl.h"

_envir default_envir;

int __libcurlmgr_init(_envir *);

_envir *envir_default(void)
{
	static int runonce = 0;
	if (!runonce) {
		default_envir.loop = uv_default_loop();
		if (__libcurlmgr_init(&default_envir)) exit(-1);
		runonce = 1;
	}
	return &default_envir;
}
