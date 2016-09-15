/* Copyright (C) 2015, Hsiang Kao (e0e1e) <0xe0e1e@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* http://tools.ietf.org/html/rfc2616#section-13.5.1 */

int
tunnel_proxyconn_http_connect_reply(_tunnel *tun, int ok)
{
	static char badgateway[] = "HTTP/1.1 502 Bad Gateway\r\n"
		"Content-Length: 11\r\n"
		"Connection: close\r\n\r\nBad Gateway";
	static char established[] =
		"HTTP/1.0 200 Connection established\r\n\r\n";

	if (ok) {
		if (tun->ob != NULL) {
			int ret = tunnel_libcurl_postconn_write(tun,
				*(u32 *)tun->ob, tun->ob+4);
			free(tun->ob);
			if (ret < 0) goto __error;
		} else if (tunnel_proxyconn_write(tun, established,
			sizeof(established) - 1) < 0)
			goto __error;
		tun->ev = TSx_TCP_STREAM;
		tunnel_proxyconn_read_start(tun);
		return 0;
	} else if (tun->ob != NULL) free(tun->ob);
__error:
	tunnel_proxyconn_write(tun, badgateway, sizeof(badgateway) - 1);
	tunnel_close(tun);
	return -1;
}

int tunnel_proxyconn_http_parse_header(_tunnel *tun)
{
	static char *method_string[] = {
		"CONNECT ",			// see draft-luotonen-web-proxy-tunneling-01
		"GET ", "POST ", "PUT ", "HEAD ", "OPTIONS ", "DELETE ", "TRACE "
	};
	#define N_METHODS	(sizeof(method_string)/sizeof(method_string[0]))

	char *pstr = &exbuf_peekat(&tun->eb, 0);
	int method;
	for(method=0; method<N_METHODS; ++method) {
		// check the current method is the prefix of HTTP header
		char *pstr2 = method_string[method];
		for(; *pstr2 != '\0'; ++pstr2, ++pstr)
			if (*pstr2 != *pstr) break;

		if (*pstr2 == '\0') {
			#define __dport		tun->dport
			#define __dst		tun->dst
			char *host, *port;
			/* http://tools.ietf.org/html/rfc2616#section-5.2 */

			if (*pstr != '/') {
				for(host = port = pstr; *pstr != '\0'; ++pstr) {
					if (*pstr == ':') {
						if (pstr[1] == '/' && pstr[2] == '/')
							host = port = (pstr += 3);
						else port = ++pstr;
					} else if (*pstr == '/' || *pstr == ' ')
						goto __connecthttpserver;
				}
			}

			if ((pstr = strstr(pstr, "Host:")) != NULL) {
				pstr += 5;			// Skip 'Host:'
				while(*pstr == ' ') ++pstr;
				for(host = port = pstr; *pstr != '\0'; ++pstr) {
					if (*pstr == '\r' && pstr[1] == '\n') {
__connecthttpserver:	if (host != port && port != pstr) {
							__dport = atoi(port);
							strncpy(__dst, host, port - host - 1);
						} else {
							__dport = 80;
							strncpy(__dst, host, pstr - host);
						}

						if (method) {
							int siz = exbuf_length(&tun->eb);
							tun->ob = (char *)malloc(siz + 4);
							*((u32 *)tun->ob) = siz;
							memcpy(tun->ob + 4, &exbuf_peekat(&tun->eb, 0), siz);
						}
						exbuf_clear(&tun->eb);
						tunnel_proxyconn_read_stop(tun);
						return 0;
					} else if (*pstr == ':') port = ++pstr;
				}
			}
			break;
		}
	}
	return -1;
}
