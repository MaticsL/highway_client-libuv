/* Copyright (C) 2015, Hsiang Kao (e0e1e) <0xe0e1e@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
	case TS1_SOCKS4_VER:
		switch (__peekat(0)) {
			case 1: __pop(1); __swi(TS1_SOCKS4_CONNECT);		// CONNECT
			default:
				log(__log_error, "Socks4 - Command [%xh] isn't implemented", __peekat(0));
				__throw;
		}
	case TS1_SOCKS4_CONNECT:
		if (__length >= 2) {
			__dport = ntohs(__peek2at(0)); __pop(2);
			__swi(TS1_SOCKS4_CONNECT_DPORT);
		}
		break;
	case TS1_SOCKS4_CONNECT_DPORT:
		if (__length >= 4) {
			if (!(__peekat(0) || __peekat(1) || __peekat(2)) && __peekat(3)) __dst[0] = '\0';
			else uv_inet_ntop(AF_INET, &__peek4at(0), __dst, sizeof(__dst));
			__pop(4);
			__swi(TS1_SOCKS4_CONNECT_DST);
		}
		break;
	case TS1_SOCKS4_CONNECT_DST:
		{
			int i = 0;
			while(i<__length)
				if (__peekat(i) == '\0') break;
				else ++i;
			if (i < __length) {
				__pop(i+1);					// skip usrid
				__swi(TS1_SOCKS4_CONNECT_USRID);
			}
		}
		break;
	case TS1_SOCKS4_CONNECT_USRID:
		if (__dst[0] == '\0') {
			//  SOCKS4a (specify a destination domain name rather than an IP address)
			int i = 0;
			while(i<__length)
				if (__peekat(i) == '\0') break;
				else ++i;
			if (i < __length) {
				i = 0;
				while ((__dst[i] = __peekat(i)) != '\0') ++i;
				__pop(i+1);
				__swi(TS2_SOCKS4_CONNECT_HTTPSERVER);
			}
		} else __swi(TS2_SOCKS4_CONNECT_HTTPSERVER);
		break;
	case TS2_SOCKS4_CONNECT_HTTPSERVER:
		if (libcurlmgr_tunnel_connecthttpserver(tun)) __throw;
		else __swi_end(TS2_SOCKS4_CONNECT_WAIT_HELLO);
		break;
	case TS2_SOCKS4_CONNECT_WAIT_HELLO:
		log(__log_error, "Socks4 - Undefined behavior [%xh]", __peekat(0));
		__throw;
