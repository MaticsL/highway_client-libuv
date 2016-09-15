/* Copyright (C) 2015, Hsiang Kao (e0e1e) <0xe0e1e@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "highway/tun_impl.h"
#include "dbg.h"

#include "config.h"

#include "highway/tun_pub.h"

#include <stdlib.h>

void tunnel_proxyconn_write_cb(uv_write_t *req, int status)
{
	if (status < 0)
		log(__log_error, "%s: error %s",
			__FUNCTION__, uv_strerror(status));
	_tunnel *tun = container_of((uv_tcp_t *)req->handle, _tunnel, conn);
	free(req);
	if (status == UV_EPIPE) tunnel_close(tun);
}

#include <string.h>

int
tunnel_proxyconn_write(_tunnel *tun, char *buf, int siz)
{

#if defined(_WIN32)
	uv_buf_t a = uv_buf_init(buf, siz);
	uv_write_t *req = (uv_write_t *)malloc(sizeof(uv_write_t));
#else
	uv_write_t *req = (uv_write_t *)malloc(sizeof(uv_write_t) + siz);
	memcpy(&req[1], buf, siz);
	uv_buf_t a = uv_buf_init((char *)&req[1], siz);
#endif

	int ret = uv_write(req, (uv_stream_t *)&tun->conn,
		&a, 1, tunnel_proxyconn_write_cb);
	if (ret < 0) {
		free(req);
		log(__log_error, "%s: uv_write error %s",
			__FUNCTION__, uv_strerror(ret));
	}
	return ret;
}

static
void
tunnel_proxyconn_close_cb(uv_handle_t *handle)
{
	_tunnel *tun = container_of((uv_tcp_t *)handle, _tunnel, conn);
	free(tun);
}

#include "highway/tun_libcurl_pub.h"

//
// HIGHWAY的设计里是允许客户端半关的，即若成功发出了EXIT
// 信号会等待recvconn关闭连接（即对方关闭）后再关闭。
// 但不允许服务器半关，因为如果recvconn关闭后，sessionid
// 已经失去作用，（php版本由recvconn维护socket）也不可能
// 再给远程socket数据。【所以我们只设计客户端半关的情况】
//
enum {
	TS0_EMPTY = 0,
	TSx_END,

	TSx_TCP_STREAM = 0x10,
	TSx_TCP_EXIT,

	TS1_SOCKS4_VER = 0x40,		// socks4
	TS1_SOCKS4_CONNECT,
	TS1_SOCKS4_CONNECT_DPORT,
	TS1_SOCKS4_CONNECT_DST,
	TS1_SOCKS4_CONNECT_USRID,
	TS2_SOCKS4_CONNECT_HTTPSERVER,
	TS2_SOCKS4_CONNECT_WAIT_HELLO,

	TS1_SOCKS5_VER = 0x50,		// socks5
	TS1_SOCKS5_METHOD,
	TS1_SOCKS5_VER2,
	TS1_SOCKS5_CONNECT_CMD,
	TS1_SOCKS5_CONNECT_ATYP_0_IPV4,
	TS1_SOCKS5_CONNECT_ATYP_0_DOMAINNAME,
	TS1_SOCKS5_CONNECT_ATYP_1,
	TS2_SOCKS5_CONNECT_HTTPSERVER,
	TS2_SOCKS5_CONNECT_WAIT_HELLO,

	TS1_HTTP_PARSE_HEADER = 0x80,
	TS1_HTTP_CONNECT_HTTPSERVER,
	TS2_HTTP_HEADER_WAIT_HELLO
};

static inline
void tunnel_proxyconn_shutdown_cb(uv_shutdown_t *req, int status)
{
	_tunnel *tun = container_of(req, _tunnel, shutdown);

	// If the server has closed the connection already, we
	//	 get ENOTCONN on Linux, and ECONNRESET on BSD.
	if (status < 0 &&
		status != UV_ENOTCONN && status != UV_ECONNRESET)
		log(__log_error, "%s: error %s",
			__FUNCTION__, uv_strerror(status));

	uv_close((uv_handle_t *)&tun->conn, tunnel_proxyconn_close_cb);
}

// 挂断tunnel，目前有两个触发时机:
// 一个是EXIT消息错误，另一个是recvconn complete
void tunnel_close(_tunnel *tun)
{
	if (tun->ev != TSx_END) {
		tun->ev = TSx_END;

		tunnel_libcurl_close(tun);
		if (uv_shutdown(&tun->shutdown,
			(uv_stream_t *)&tun->conn, tunnel_proxyconn_shutdown_cb) < 0)
			tunnel_proxyconn_shutdown_cb(&tun->shutdown, 0);
	}
}

void tunnel_proxyconn_exit(_tunnel *tun)
{
	if (!tunnel_libcurl_proxyconn_exit(tun))
		tun->ev = TSx_TCP_EXIT;
	else tunnel_close(tun);
}

static int
tunnel_proxyconn_socks4_connect_reply(_tunnel *tun, int ok)
{
	char buf[] = {0, 0x5A, 0, 0, 0, 0, 0, 0};
	
	if (ok) {
		struct sockaddr_storage sockaddr;
		int len = sizeof(sockaddr);

		uv_tcp_getsockname(&tun->conn, (struct sockaddr *)&sockaddr, &len);
		*(short *)(buf+2) =
			htons(((struct sockaddr_in *)&sockaddr)->sin_port);
		struct in_addr in_addr = ((struct sockaddr_in *)&sockaddr)->sin_addr;
		*(long *)(buf+4) = htonl(*(long *)&in_addr);
	} else buf[1] = 0x5B;

	// FIXME! 暂时先这样处理，主要还是由于少一个状态
	if (!ok || tunnel_proxyconn_write(tun, buf, sizeof(buf))<0) {
		tunnel_close(tun);
		return -1;
	}
	tun->ev = TSx_TCP_STREAM;
	return 0;
}

static int
tunnel_proxyconn_socks5_connect_reply(_tunnel *tun, int ok)
{
	char buf[] ={5, 0, 0, 1, 0, 0, 0, 0, 0, 0};
	if (ok) {
		struct sockaddr_storage sockaddr;
		int len = sizeof(sockaddr);
		
		uv_tcp_getsockname(&tun->conn, (struct sockaddr *)&sockaddr, &len);
		struct in_addr in_addr = ((struct sockaddr_in *)&sockaddr)->sin_addr;
		*(long *)(buf+4) = htonl(*(long *)&in_addr);
		*(short *)(buf+8) =
			htons(((struct sockaddr_in *)&sockaddr)->sin_port);
	} else buf[1] = 4;
	
	if (!ok || tunnel_proxyconn_write(tun, buf, sizeof(buf))<0) {
		tunnel_close(tun);
		return -1;
	}
	tun->ev = TSx_TCP_STREAM;
	return 0;
}

static inline void tunnel_proxyconn_read_start(_tunnel *);
#define tunnel_proxyconn_read_stop(x)	uv_read_stop((uv_stream_t *)&(x)->conn)
#include "tun_httpimpl.h"

int tunnel_proxyconn_connect_try_reply(_tunnel *tun, int ok)
{
	if (tun->ev == TS2_SOCKS4_CONNECT_WAIT_HELLO)
		return tunnel_proxyconn_socks4_connect_reply(tun, ok);
	else if (tun->ev == TS2_SOCKS5_CONNECT_WAIT_HELLO)
		return tunnel_proxyconn_socks5_connect_reply(tun, ok);
	else if (tun->ev == TS2_HTTP_HEADER_WAIT_HELLO)
		return tunnel_proxyconn_http_connect_reply(tun, ok);
	else if (tun->ev != TSx_TCP_STREAM && tun->ev != TSx_TCP_EXIT) {
		log(__log_error, "%s: unknown ev %d", __FUNCTION__, tun->ev);
		tunnel_close(tun);
		return -1;
	}
	return 0;
}

int tunnel_proxyconn_is_tcp_stream(_tunnel *tun) {
	return tun->ev == TSx_TCP_STREAM || tun->ev == TSx_TCP_EXIT;
}

void
tunnel_proxyconn_read_cb(uv_stream_t *s,
	ssize_t nread, const uv_buf_t *buf)
{
	_tunnel *tun = container_of((uv_tcp_t *)s, _tunnel, conn);

	if (nread < 0) {
		if (nread != UV_EOF)
			log(__log_error, "%s: Read error %s",
				__FUNCTION__, uv_err_name(nread));

		// TCP收到eof，则让客户端半关，送EXIT信号到proxyconn写队列
		if (tun->ev == TSx_TCP_STREAM) tunnel_proxyconn_exit(tun);
		else {
			log(__log_error, "%s: EOF recvd when tun->ev == %xh (!= TSx_TCP_STREAM)",
				__FUNCTION__, tun->ev);
			tunnel_close(tun);
		}
	} else if (nread != 0) {
		if (tun->ev == TSx_TCP_STREAM) {
			if (tunnel_libcurl_postconn_write(tun, nread, buf->base))
				tunnel_close(tun);
		} else {
			exbuf_push(&tun->eb, buf->base, nread);
			while(1) {
				switch(tun->ev) {
					#define __length	exbuf_length(&tun->eb)
					#define __peekat(x) exbuf_peekat(&tun->eb, x)
					#define __peek2at(x) exbuf_peek2at(&tun->eb, x)
					#define __peek4at(x) exbuf_peek4at(&tun->eb, x)
					#define __pop(x) exbuf_pop(&tun->eb, x)
					#define __swi(x) ({tun->ev = x; continue;})
					#define __swi_end(x) ({tun->ev = x;})
					#define __throw		({goto __ev_error;})
					#define __dport		tun->dport
					#define __dst		tun->dst
					case TS0_EMPTY:
						switch (__peekat(0)) {
							case 4:
								__pop(1); __swi(TS1_SOCKS4_VER);	// SOCKS 4
							case 5:
								__pop(1); __swi(TS1_SOCKS5_VER);	// SOCKS 5

							/* http://tools.ietf.org/html/rfc2616#section-5.1.1 */
							case 'C':
							case 'G': case 'P': case 'H':
							case 'O': case 'D': case 'T':
								__swi(TS1_HTTP_PARSE_HEADER);

							default:
								log(__log_error, "%s: Unknown protocol [%xh]",
									__FUNCTION__, __peekat(0));
								__throw;
						}
					#include "tun_socks4.h"
					#include "tun_socks5.h"
					#include "tun_http.h"
					default:			// 这不应该发生，所以此时RST
__ev_error:				tunnel_close(tun);
				}
				break;
			}
		}
	}
	if (buf->base != NULL) free(buf->base);
}

static void
alloc_buffer(uv_handle_t *handle,
	size_t suggested_size, uv_buf_t *buf)
{
    *buf = uv_buf_init((char *)malloc(suggested_size), suggested_size);
}

static inline
void tunnel_proxyconn_read_start(_tunnel *tun)
{
	uv_read_start((uv_stream_t *)&tun->conn,
		alloc_buffer, tunnel_proxyconn_read_cb);
}

#include "highway/envir_impl.h"
#include "die.h"

_record(tunnelmgr)
	uv_tcp_t hub;
	_envir *env;
	_tunnelinfo *info;
_end_record(tunnelmgr)

void tunnelmgr_connection_cb(uv_stream_t *h, int status)
{
	if (status != 0)
		log(__log_error, "%s: New connection error %s",
			__FUNCTION__, uv_strerror(status));
	else {
		_tunnelmgr *m = container_of((uv_tcp_t *)h, _tunnelmgr, hub);
		// and set to zero
		_tunnel *tun = (_tunnel *)calloc(1, sizeof(_tunnel));
		if (tun == NULL) die("%s: Out of memory", __FUNCTION__);
		else {
			_envir *e = m->env;
			uv_tcp_init(e->loop, &tun->conn);
			if (uv_accept(h, (uv_stream_t *)&tun->conn) == 0) {
				tun->env = e, tun->info = m->info;
				tun->ev = TS0_EMPTY;
				exbuf_clear(&tun->eb);
				tun->remaining_retries = REQUEST_RETRIES;
				tun->post = 
					(_list_node_d)doubly_linked_list_pre_initialise(tun->post);
				tunnel_proxyconn_read_start(tun);
			} else
				uv_close((uv_handle_t *)&tun->conn, tunnel_proxyconn_close_cb);
		}
	}
}

int tunnelmgr(_envir *e, char *ip, u16 port, _tunnelinfo *tuninfo)
{
	if (e != NULL) {
#ifdef _POSIX_C_SOURCE
#include <signal.h>
		signal(SIGPIPE, SIG_IGN);			// ignore 'EPIPE'
#endif

		#define hub		(mgr.hub)
		#define loop	(e->loop)
		_tunnelmgr mgr = {.env = e, .info = tuninfo};
		uv_tcp_init(loop, &hub);
		
		struct sockaddr_in sockaddr;
		uv_ip4_addr(ip, port, &sockaddr);
		uv_tcp_bind(&hub, (const struct sockaddr*)&sockaddr, 0);
		
		int ret;
		ret = uv_listen((uv_stream_t*)&hub, BACKLOG, tunnelmgr_connection_cb);
		
		if (ret) log(__log_error, "Listen error %s", uv_strerror(ret));
		else {
			log(__log_info, "Highway started at %s:%u", ip, port); 
			uv_run(loop, UV_RUN_DEFAULT);
		}
	}
	return -1;
}
