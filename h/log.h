/* Copyright (C) 2013-2014, Hsiang Kao (e0e1e) <esxgx@163.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef __LOG_H
#define __LOG_H

#define log(x, ...)	do{\
	char buf[512];			\
	sprintf(buf, __VA_ARGS__);	\
	x(buf);	 \
}while(0)

#include <stdio.h>

#define __log_error(x)		fprintf(stderr, "%s\n", x)
#define __log_info(x)		printf("%s\n", x)

#endif
