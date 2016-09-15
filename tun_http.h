/* Copyright (C) 2015-2016, Hsiang Kao (e0e1e) <0xe0e1e@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

	case TS1_HTTP_PARSE_HEADER:
		if (!tunnel_proxyconn_http_parse_header(tun))
			__swi(TS1_HTTP_CONNECT_HTTPSERVER);
		break;

	case TS1_HTTP_CONNECT_HTTPSERVER:
		if (libcurlmgr_tunnel_connecthttpserver(tun))
			tunnel_proxyconn_http_connect_reply(tun, 0);
		else __swi_end(TS2_HTTP_HEADER_WAIT_HELLO);
		break;

	case TS2_HTTP_HEADER_WAIT_HELLO:
		__throw;
