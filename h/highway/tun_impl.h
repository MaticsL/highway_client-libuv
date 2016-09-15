/* Copyright (C) 2015, Hsiang Kao (e0e1e) <0xe0e1e@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef __HIGHWAY_TUN_IMPL_H
#define __HIGHWAY_TUN_IMPL_H

#include <uv.h>
#include "tun.h"

#include <curl/curl.h>
#include "list.h"
#include "exbuf.h"

_declrec(envir)

_record(tunnel)
	uv_tcp_t conn;
	uv_shutdown_t shutdown;
// �����趨���ǿ��ǵ������߳̽�����һ������tunnel������
// �̣���libuv�����������ڸ��ؾ���
	_envir *env;
	_tunnelinfo *info;

	int ev;

	int dport;
	char dst[256];

	// ʣ�ೢ�Ի���, ����recvconn��postconn
	int remaining_retries;

	// the session cookie
	int cookieslength;
	char cookies[256];

// tun_libcurl���
	CURL *recvconn;
	_list_node_d post;
	void (*libcurl_socket_complete_cb)(void *, CURL *, CURLMsg *);

	_exbuf eb;					// buffer
	char *ob;
_end_record(tunnel)

#endif
