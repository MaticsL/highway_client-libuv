/* Copyright (C) 2013-2014, Hsiang Kao (e0e1e) <esxgx@163.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef __DEFS_H
#define __DEFS_H


#ifndef NULL
#define NULL	((void *)0)
#endif

#ifndef FALSE
#define FALSE	0
#endif

#ifndef TRUE
#define TRUE	1
#endif

#define UNUSED(x) (void)(x)

#ifndef offsetof
#define offsetof(s, m)   (size_t)&(((s *)0)->m)
#endif

/** 
 * container_of - cast a member of a structure out to the containing structure 
 * @ptr:        the pointer to the member. 
 * @type:       the type of the container struct this is embedded in. 
 * @member:     the name of the member within the struct. 
 * */ 

#define container_of(ptr, type, member) (__extension__({ \
	const typeof( ((type *)0)->member ) *__mptr = (ptr); \
	(type *)( (char *)__mptr - offsetof(type,member) );}))

#undef min
#undef max

#define min(x,y) (__extension__({ \
	typeof(x) _x = (x); \
	typeof(y) _y = (y); \
	(void) (&_x == &_y); \
	_x < _y ? _x : _y; }))

#define max(x,y) (__extension__({ \
	typeof(x) _x = (x); \
	typeof(y) _y = (y); \
	(void) (&_x == &_y); \
	_x > _y ? _x : _y; }))

#endif
