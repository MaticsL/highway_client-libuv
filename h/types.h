/* Copyright (C) 2013-2014, Hsiang Kao (e0e1e) <esxgx@163.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// 标准类型定义
typedef unsigned char	u8;
typedef unsigned short	u16;

typedef signed char	s8;
typedef signed short	s16;

#if __SIZEOF_LONG__ == 4
typedef unsigned long	u32;
typedef signed long	s32;

#elif __SIZEOF_INT__ == 4
typedef unsigned int	u32;
typedef signed int	s32;
#endif

typedef unsigned long long	u64;

