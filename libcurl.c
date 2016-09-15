/* Copyright (C) 2015, Hsiang Kao (e0e1e) <0xe0e1e@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "highway/envir_impl.h"

// 给每个需要监视的curl_socket_t绑定一个libcurl_socket
_record(libcurl_socket)
	uv_poll_t pollhandle;
	curl_socket_t s;
	_envir *e;
_end_record(libcurl_socket)

#include "dbg.h"
#include "die.h"

static
void libcurl_socket_close_cb(uv_handle_t *handle)
{
	_libcurl_socket *p = container_of((uv_poll_t *)handle,
		_libcurl_socket, pollhandle);
	free(p);
}

#define libcurl_socket_destory(x)	\
	uv_close((uv_handle_t *)&(x)->pollhandle, libcurl_socket_close_cb)

static
_libcurl_socket *libcurl_socket_allocate(_envir *e, curl_socket_t s)
{
	_libcurl_socket *p = (_libcurl_socket *)malloc(sizeof(*p));
	if (p == NULL) die("libcurl_socket_allocate: Out of memory");
	p->e = e, p->s = s;
	uv_poll_init_socket(e->loop, &p->pollhandle, s);
	curl_multi_assign(e->curl.curlm, s, p);
	return p;
}

static
void libcurl_process_multi_info(CURLM *curlm)
{
	int msgq = 0;
	CURLMsg *m;
	while(m = curl_multi_info_read(curlm, &msgq)) {
		if (m->msg == CURLMSG_DONE) {
			CURL *e = m->easy_handle;
			curl_multi_remove_handle(curlm, e);
			
			void (**libcurl_socket_complete_cb)();
			
			curl_easy_getinfo(e, CURLINFO_PRIVATE,
				(void *)&libcurl_socket_complete_cb);
			
			if (libcurl_socket_complete_cb != NULL)
				// here is a trick
				(*libcurl_socket_complete_cb)(libcurl_socket_complete_cb, e, m);
			else curl_easy_cleanup(e);
		}
	}

}

static
void libcurl_socket_poll_cb(uv_poll_t *handle, int status, int events)
{
	if (status < 0)
		log(__log_error, "libcurl_socket_poll_cb error %s",
			uv_strerror(status));
	else {
		_libcurl_socket *p = container_of(handle, _libcurl_socket, pollhandle);
		_libcurlmgr *curl = &p->e->curl;

		// libcurl里的例程针对timeout四处矛盾，以下根据libuv例程写的，因为按
		// 自己理解有了READ、WRITE之类的事件TIMEOUT就可以结束了。
		//uv_timer_stop(&curl->timeout);
		
		int ev_bitmask = 0;
		if (events & UV_READABLE) ev_bitmask |= CURL_CSELECT_IN;
		if (events & UV_WRITABLE) ev_bitmask |= CURL_CSELECT_OUT;
		
		int running_handles;
		curl_multi_socket_action(curl->curlm, p->s,
			ev_bitmask, &running_handles);
		
		// After curl_multi_socket_action, we loop through and check if
		// there are any transfers that have completed
		libcurl_process_multi_info(curl->curlm);
		
		if (running_handles <= 0)
			uv_timer_stop(&p->e->curl.timeout);
	}
}

static
int libcurlmgr_socket_callback(CURL *easy,	curl_socket_t s,
	int action,	void *userp, void *socketp)
{
	_libcurl_socket *p = (_libcurl_socket *)socketp;
	_envir *e = (_envir *)userp;
	if (action == CURL_POLL_REMOVE) {
		if (p != NULL) {
			uv_poll_stop(&p->pollhandle);
			libcurl_socket_destory(p);
			curl_multi_assign(e->curl.curlm, s, NULL);
		}
	} else if (action != CURL_POLL_NONE) {
		// since CURL_POLL_INOUT == 3
		int events = 0;
		if (action & CURL_POLL_IN) events |= UV_READABLE;
		if (action & CURL_POLL_OUT) events |= UV_WRITABLE;

		if (p == NULL) p = libcurl_socket_allocate(e, s);
		uv_poll_start(&p->pollhandle, events, libcurl_socket_poll_cb);
	}
	return 0;
}

static
void libcurlmgr_uv_timer_cb(uv_timer_t *handle)
{
	_libcurlmgr *curl = container_of(handle, _libcurlmgr, timeout);
	int running;
    curl_multi_socket_action(curl->curlm, CURL_SOCKET_TIMEOUT, 0, &running);
    
    libcurl_process_multi_info(curl->curlm);
}

static
int libcurlmgr_timer_callback(CURLM *multi,
	long timeout_ms, void *userp)
{
	_envir *e = (_envir *)userp;
	if (timeout_ms < 0)
		uv_timer_stop(&e->curl.timeout);
	else {
		if (timeout_ms == 0) timeout_ms = 1;
		uv_timer_start(&e->curl.timeout, libcurlmgr_uv_timer_cb, timeout_ms, 0);
	}
	return 0;
}

size_t libcurl_easy_noop_write_cb(char *ptr, size_t size,
	size_t nmemb, void *userdata)
{
	return size * nmemb;
}

int __libcurlmgr_init(_envir *e)
{
	if(curl_global_init(CURL_GLOBAL_ALL) == 0) {
		uv_timer_init(e->loop, &e->curl.timeout);
		e->curl.curlm = curl_multi_init();
		curl_multi_setopt(e->curl.curlm, CURLMOPT_SOCKETDATA, (void *)e);
		curl_multi_setopt(e->curl.curlm, CURLMOPT_SOCKETFUNCTION,
			libcurlmgr_socket_callback);
		curl_multi_setopt(e->curl.curlm, CURLMOPT_TIMERDATA, (void *)e);
		curl_multi_setopt(e->curl.curlm, CURLMOPT_TIMERFUNCTION,
			libcurlmgr_timer_callback);
		return 0;
	} else log(__log_error, "Could not init cURL");
	return -1;
}

