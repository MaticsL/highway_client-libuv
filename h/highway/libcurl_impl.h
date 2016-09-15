/* Copyright (C) 2015, Hsiang Kao (e0e1e) <0xe0e1e@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef __HIGHWAY_LIBCURL_IMPL_H
#define __HIGHWAY_LIBCURL_IMPL_H

#include <uv.h>
#include <curl/curl.h>

#include "record.h"

_record(libcurlmgr)
	uv_timer_t timeout;
	CURLM *curlm;
_end_record(libcurlmgr)

#endif
