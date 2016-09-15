/* Copyright (C) 2015, Hsiang Kao (e0e1e) <0xe0e1e@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
	case TS1_SOCKS5_VER:
		{
			int methods = __peekat(0);
			if (__length >= methods+1) {
				int i;
				for(i=1; i<=methods; ++i)
					if (__peekat(i) == 0) break;
				__pop(1 + methods);
				
				char buf[] = {5, 0};
				
				if (i<=methods)
					if (tunnel_proxyconn_write(tun, buf, sizeof(buf))>=0)
						__swi(TS1_SOCKS5_METHOD);
				buf[1] = 0xff;
				tunnel_proxyconn_write(tun, buf, sizeof(buf));
				__throw;
			}
		}
		break;
	case TS1_SOCKS5_METHOD:
		if (__length >= 1) {
			switch(__peekat(0)) {
				case 5:	 __pop(1); __swi(TS1_SOCKS5_VER2);
				default:
					log(__log_error, "Socks5 - invaild ver2 [%xh]", __peekat(0));
					__throw;
			}
		}
		break;
	case TS1_SOCKS5_VER2:
		switch(__peekat(0)) {
			case 1: __pop(1); __swi(TS1_SOCKS5_CONNECT_CMD);
			default:
				log(__log_error, "Socks5 - Command [%xh] isn't implemented", __peekat(0));
				__throw;
		}
	case TS1_SOCKS5_CONNECT_CMD:
		if (__length >= 2) {
			int atyp = __peekat(1);
			__pop(2);
			switch(atyp) {
				case 1:		__swi(TS1_SOCKS5_CONNECT_ATYP_0_IPV4);
				case 3:		__swi(TS1_SOCKS5_CONNECT_ATYP_0_DOMAINNAME);
				default:	__throw;
			}
		}
		break;
	case TS1_SOCKS5_CONNECT_ATYP_0_IPV4:
		if (__length >= 4) {
			uv_inet_ntop(AF_INET, &__peek4at(0), __dst, sizeof(__dst));
			__pop(4);
			__swi(TS1_SOCKS5_CONNECT_ATYP_1);
		}
		break;
	case TS1_SOCKS5_CONNECT_ATYP_0_DOMAINNAME:
		{
			int octets = __peekat(0);
			if (__length >= octets+1) {
				memcpy(__dst, &__peekat(1), octets);
				__dst[octets] = '\0';
				__pop(1 + octets);
				__swi(TS1_SOCKS5_CONNECT_ATYP_1);
			}
		}
		break;
	case TS1_SOCKS5_CONNECT_ATYP_1:
		if (__length >= 2) {
			__dport = ntohs(__peek2at(0)); __pop(2);
			__swi(TS2_SOCKS5_CONNECT_HTTPSERVER);
		}
		break;
	case TS2_SOCKS5_CONNECT_HTTPSERVER:
		if (libcurlmgr_tunnel_connecthttpserver(tun)) __throw;
		else __swi_end(TS2_SOCKS5_CONNECT_WAIT_HELLO);
		break;
	case TS2_SOCKS5_CONNECT_WAIT_HELLO:
		log(__log_error, "Socks5 - Undefined behavior [%xh]", __peekat(0));
		__throw;
	