/* Copyright (C) 2015, Hsiang Kao (e0e1e) <0xe0e1e@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#define HIGHWAY_CUSTOMIZED_USER_AGENT		\
	"Mozilla/5.0 (X11; Linux x86_64; rv:12.0) Gecko/20100101 Firefox/21.0"

#include <string.h>

static inline void __bintohex(char *buf, int len)
{
	const char hexstring[] = "0123456789abcdef";
	char *p = buf + (len<<1);
	int i = len;
	*p = '\0';
	while(i) {
		char ch = buf[--i];
		*--p = hexstring[ch & 15];
		*--p = hexstring[(ch >> 4) & 15];
	}
}

#include "highway/tun_impl.h"
#include "highway/envir_impl.h"
#include "dbg.h"

_record(postdata)
	_list_node_d list;
	CURL *easy;
	_tunnel *tun;
	void (*libcurl_postconn_complete_cb)(void *, CURL *, CURLMsg *);
_end_record(postdata)

void tunnel_libcurl_close(_tunnel *tun)
{
	CURLM *curlm = tun->env->curl.curlm;
	_list_node_d *p;
	// 如果有，关闭所有post连接
	while((p = doubly_linked_list_the_tail(&tun->post)) !=
		doubly_linked_list_the_end(&tun->post)) {
		_postdata *post = container_of(p, _postdata, list);
		curl_multi_remove_handle(curlm, post->easy);
		curl_easy_cleanup(post->easy);
		doubly_linked_list_remove(p);
		free(post);
	}
	
	// 如果有，关闭recvconn
	CURL *easy = tun->recvconn;
	if (easy != NULL) {
		curl_multi_remove_handle(curlm, easy);
		curl_easy_cleanup(easy);
		tun->recvconn = NULL;
	}
}

#include "highway/tun_pub.h"

static
void tunnel_libcurl_recvconn_complete_cb(void *p, CURL *easy, CURLMsg *m)
{
	_tunnel *tun = container_of(p, _tunnel, libcurl_socket_complete_cb);
	if (easy != tun->recvconn)
		log(__log_error, "%s: e != tun->recvconn", __FUNCTION__);
	else {
		long http_code;
		curl_easy_getinfo (easy, CURLINFO_RESPONSE_CODE, &http_code);
		if (http_code == 0) {
			switch(m->data.result) {
				case CURLE_COULDNT_CONNECT:
				case CURLE_OPERATION_TIMEDOUT:
					if (tun->remaining_retries) {
						--tun->remaining_retries;
						curl_multi_add_handle(tun->env->curl.curlm, easy);
						return;
					}
				default: (void)1;
			}
			log(__log_error, "%s: No response from the url: %s [status=%d]",
				__FUNCTION__, tun->info->baseurl, m->data.result);
		} else if (m->data.result != CURLE_OK || http_code != 200)
			log(__log_error, "Connection error %d [status: %ld]",
				m->data.result, http_code);

		// (only works on connection failed)
		if (!tunnel_proxyconn_connect_try_reply(tun, 0))

			// fully close the tunnel
			tunnel_close(tun);
	}
}

#include "config.h"

static
void tunnel_libcurl_exitconn_complete_cb(void *p, CURL *easy, CURLMsg *m)
{
	_postdata *exit = container_of(p, _postdata, libcurl_postconn_complete_cb);
	_tunnel *tun = exit->tun;

	if (m->data.result == CURLE_OPERATION_TIMEDOUT &&
		tun->remaining_retries) {
		--tun->remaining_retries;
		curl_multi_add_handle(tun->env->curl.curlm, easy);
	} else {
		doubly_linked_list_remove(&exit->list);
		free(exit), curl_easy_cleanup(easy);

		// fully close the tunnel
		tunnel_close(tun);
	}
}

#include "highway/libcurl_pub.h"

int tunnel_libcurl_proxyconn_exit(_tunnel *tun)
{
	_postdata *exit = (_postdata *)malloc(sizeof(_postdata));
	if (exit != NULL) {
		CURL *easy = curl_easy_init();
		if (easy != NULL) {
			exit->easy = easy, exit->tun = tun;

			char url[strlen(tun->info->baseurl) + sizeof("?x")];
			sprintf(url, "%s?x", tun->info->baseurl);
			curl_easy_setopt(easy, CURLOPT_URL, url);
			curl_easy_setopt(easy, CURLOPT_USERAGENT,
				HIGHWAY_CUSTOMIZED_USER_AGENT);
			curl_easy_setopt(easy, CURLOPT_COOKIE, tun->cookies);
			curl_easy_setopt(easy, CURLOPT_CONNECTTIMEOUT,
				REQUEST_CONN_TIMEOUT);
			exit->libcurl_postconn_complete_cb =
				tunnel_libcurl_exitconn_complete_cb;
			curl_easy_setopt(easy, CURLOPT_PRIVATE, 
				&exit->libcurl_postconn_complete_cb);
			curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION,
					libcurl_easy_noop_write_cb);

			if (doubly_linked_list_the_head(&tun->post) ==
				doubly_linked_list_the_end(&tun->post))
				curl_multi_add_handle(tun->env->curl.curlm, easy);

			doubly_linked_list_insert_before(&tun->post, &exit->list);
			return 0;
		} else log(__log_error, "%s: Could not init easy", __FUNCTION__);
	} else log(__log_error, "%s: Out of memory", __FUNCTION__);
	return -1;
}

static
void tunnel_libcurl_postconn_complete_cb(void *p, CURL *easy, CURLMsg *m)
{
	_postdata *post = container_of(p, _postdata, libcurl_postconn_complete_cb);
	_tunnel *tun = post->tun;

	long http_code;
	curl_easy_getinfo (easy, CURLINFO_RESPONSE_CODE, &http_code);

	if (m->data.result != CURLE_OK && http_code == 0 &&
		tun->remaining_retries) {
		--tun->remaining_retries;
		curl_multi_add_handle(tun->env->curl.curlm, easy);
	} else if (http_code != 200 || m->data.result != CURLE_OK) {
		if (http_code != 200)
			log(__log_error, "%s: Internal error %d [status: %ld]",
				__FUNCTION__, m->data.result, http_code);
		else log(__log_error, "%s: Connection broken [status=%d]",
				__FUNCTION__, m->data.result);
				doubly_linked_list_remove(&post->list);
		free(post), curl_easy_cleanup(easy);
		tunnel_close(tun);		// if error, fully close
	} else {
		tun->remaining_retries = REQUEST_RETRIES;
	
		_list_node_d *cp = doubly_linked_list_go(&post->list);
		doubly_linked_list_remove(&post->list);
		if (cp != doubly_linked_list_the_end(&tun->post)) {
			_postdata *coming = container_of(cp, _postdata, list);
			curl_multi_add_handle(tun->env->curl.curlm, coming->easy);
		}
		free(post), curl_easy_cleanup(easy);
	}
}

#include "highway/encrypt_pub.h"

#ifdef __GNUC__
__attribute__ ((__noinline__))
#endif
int tunnel_libcurl_postconn_write(_tunnel *tun, size_t nread, char *plain)
{
	_encrypt *enc = encrypt_init(tun->info->method, tun->info->pw);
	if (enc != NULL) {
		const int lenh = encrypt_iv_length(enc)+4;
		
		char buf[lenh + nread];
		encrypt_update(enc, buf, 4, (char *)&nread);	// little endian
		encrypt_update(enc, buf + lenh, nread, plain);
		encrypt_free(enc);
		
		_postdata *post = (_postdata *)malloc(sizeof(_postdata));
		if (post != NULL) {
			CURL *easy = curl_easy_init();
			if (easy != NULL) {
				post->easy = easy, post->tun = tun;
				post->libcurl_postconn_complete_cb =
					tunnel_libcurl_postconn_complete_cb;
				
				curl_easy_setopt(easy, CURLOPT_URL, tun->info->baseurl);
				curl_easy_setopt(easy, CURLOPT_POST, 1L);
				curl_easy_setopt(easy, CURLOPT_USERAGENT,
					HIGHWAY_CUSTOMIZED_USER_AGENT);
				curl_easy_setopt(easy, CURLOPT_COOKIE, tun->cookies);
				curl_easy_setopt(easy, CURLOPT_POSTFIELDSIZE,
					(long)(lenh + nread));
				curl_easy_setopt(easy, CURLOPT_COPYPOSTFIELDS, buf);
				curl_easy_setopt(easy, CURLOPT_PRIVATE,
					&post->libcurl_postconn_complete_cb);
				curl_easy_setopt(easy, CURLOPT_CONNECTTIMEOUT,
					REQUEST_CONN_TIMEOUT);
				
				
				if (doubly_linked_list_the_head(&tun->post) ==
					doubly_linked_list_the_end(&tun->post))
					curl_multi_add_handle(tun->env->curl.curlm, easy);
				
				doubly_linked_list_insert_before(&tun->post, &post->list);
				return 0;
			} else log(__log_error, "%s: Could not init easy", __FUNCTION__);
		} else log(__log_error, "%s: Out of memory", __FUNCTION__);
	} else
		log(__log_error, "%s: Failed to initialize encrypt", __FUNCTION__);
	return -1;
}

static
size_t tunnel_libcurl_recvconn_header_cb(char *buffer, size_t size,
	size_t nitems, void *userdata)
{
	_tunnel *tun = (_tunnel *)userdata;
	int realsiz = size * nitems;
	if (realsiz > 11 && !strncmp(buffer, "Set-Cookie:", 11)) {
		char buf[realsiz + 1 - 11], *p = buf;

		buffer += 11;	// skip 'Set-Cookie:'

		while(*buffer == ' ') ++buffer;
		while(*buffer != '\0' && *buffer != ';') {
			if (*buffer == '\r' || *buffer == '\n') break;
			*p++ = *buffer++;
		}
		*p = '\0';

		if (1 + tun->cookieslength + (p-buf) + 2 < sizeof(tun->cookies)) {
			if (tun->cookieslength) {
				tun->cookies[tun->cookieslength] = ';';
				tun->cookies[tun->cookieslength+1] = ' ';
				tun->cookieslength += 2;
			}
			strcpy(tun->cookies + tun->cookieslength, buf);
			tun->cookieslength += p-buf;
		}
	} else if (realsiz == 0)
		tun->remaining_retries = REQUEST_RETRIES;
	return size * nitems;
}

static
int tunnel_libcurl_recvconn_handle_hello(_tunnel *tun)
{
	_decrypt *de = decrypt_init(tun->info->method, tun->info->pw);
	if (de != NULL) {
		int total, lenh;
		lenh = 4 + decrypt_iv_length(de);
		if ((total = exbuf_length(&tun->eb)) >= lenh) {
			unsigned bodysiz;
			decrypt_update(de, (void *)&bodysiz, lenh,
				&exbuf_peekat(&tun->eb, 0));
			
			decrypt_free(de);
			
			if (total >= bodysiz+lenh) {
				if (total > bodysiz+lenh) {
					log(__log_error,
						"the received hello are corrupted[bodysiz: %d]",
						bodysiz);
					return -1;
				}
				exbuf_pop(&tun->eb, total);			// should be no more data
				tunnel_proxyconn_connect_try_reply(tun, 1);	// connect OK
			}
		}
		return 0;
	}
	log(__log_error, "%s: Failed to initialize decrypt", __FUNCTION__);
	return -1;
}

static
int tunnel_libcurl_recvconn_write_tcpstream(_tunnel *tun)
{
	_decrypt *de = decrypt_init(tun->info->method, tun->info->pw);
	if (de != NULL) {
		int total, lenh;
		lenh = 4 + decrypt_iv_length(de);
		while ((total = exbuf_length(&tun->eb)) >= lenh) {
			unsigned bodysiz;
			decrypt_update(de, (void *)&bodysiz, lenh,
				&exbuf_peekat(&tun->eb, 0));

			if (bodysiz > EXBUF_DATASIZ/2) {
				log(__log_error,
					"the received data are corrupted[bodysiz: %d]",
					bodysiz);
				return -1;
			} else if (total < bodysiz+lenh) break;
			
			char buf[bodysiz];
			decrypt_update(de, buf, bodysiz, &exbuf_peekat(&tun->eb, lenh));
			decrypt_final(de, buf);
			
			if (tunnel_proxyconn_write(tun, buf, bodysiz) < 0) {
				decrypt_free(de);
				return -1;
			}
			exbuf_pop(&tun->eb, lenh+bodysiz);
		}
		decrypt_free(de);
		return 0;
	}
	log(__log_error, "%s: Failed to initialize decrypt", __FUNCTION__);
	return -1;	
}

size_t tunnel_libcurl_recvconn_write_cb(char *ptr, size_t size,
	size_t nmemb, void *userdata)
{
	_tunnel *tun = (_tunnel *)userdata;
	int realsiz = size*nmemb;
	exbuf_push(&tun->eb, ptr, realsiz);
	static int (*const func[])() = {
		tunnel_libcurl_recvconn_handle_hello,
		tunnel_libcurl_recvconn_write_tcpstream
	};
	if (func[tunnel_proxyconn_is_tcp_stream(tun)](tun))
		return 0;	// abort connection
	return realsiz;
}

int base64_encode(int, char *, int, const char *);

#include <openssl/sha.h>
#define SHA_DIGEST_HEX_LENGTH		(SHA_DIGEST_LENGTH<<1)

int libcurlmgr_tunnel_connecthttpserver(_tunnel *tun)
{
	int plainsiz = 2 + strlen(tun->dst);	// port + address

	char plain[plainsiz + 1];
	*(u16 *)plain = tun->dport;				// 2-byte litten endian dport
	strcpy(plain + 2, tun->dst);			// n-byte dst

	_encrypt *enc = encrypt_init("rc4-md5_8", tun->info->pw);
	if (enc != NULL) {
		// generate ?e
		char e[plainsiz + encrypt_iv_length(enc)];
		int esiz = encrypt_update(enc, e, plainsiz, plain);
		encrypt_free(enc);
		
		char e_base64[((esiz+2)/3)*4+1];
		esiz = base64_encode(sizeof(e_base64), e_base64, esiz, e);

		// generate ?s
		char s_hex[SHA_DIGEST_HEX_LENGTH + 1];
		SHA_CTX sha1;
		SHA1_Init(&sha1);
		SHA1_Update(&sha1, plain, plainsiz);
		SHA1_Update(&sha1, e_base64, esiz);
		SHA1_Update(&sha1, tun->info->pw, strlen(tun->info->pw));
		SHA1_Final(s_hex, &sha1);
		__bintohex(s_hex, SHA_DIGEST_LENGTH);
		
		CURL *easy = curl_easy_init();
		if (easy != NULL) {
			char *e_urlencoded = curl_easy_escape(easy, e_base64, esiz);
			if (e_urlencoded != NULL) {

				char *baseurl = tun->info->baseurl;
				char url[strlen(baseurl) +
					sizeof("?e=&s=") + SHA_DIGEST_HEX_LENGTH + 
					strlen(e_urlencoded)];
				
				sprintf(url, "%s?s=%s&e=%s", baseurl, s_hex, e_urlencoded);
				curl_free(e_urlencoded);

				tun->recvconn = easy;
				curl_easy_setopt(easy, CURLOPT_URL, url);

				tun->libcurl_socket_complete_cb =
					tunnel_libcurl_recvconn_complete_cb;
				curl_easy_setopt(easy, CURLOPT_PRIVATE,
					&tun->libcurl_socket_complete_cb);

				curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION,
					tunnel_libcurl_recvconn_write_cb);
				curl_easy_setopt(easy, CURLOPT_WRITEDATA, (void *)tun);

				curl_easy_setopt(easy, CURLOPT_HEADERFUNCTION,
					tunnel_libcurl_recvconn_header_cb);
				curl_easy_setopt(easy, CURLOPT_HEADERDATA, (void *)tun);

				curl_easy_setopt(easy, CURLOPT_USERAGENT,
					HIGHWAY_CUSTOMIZED_USER_AGENT);

				// set to 1 to mark this as a new cookie "session"
				curl_easy_setopt(easy, CURLOPT_COOKIESESSION, 1L);

				curl_easy_setopt(easy, CURLOPT_CONNECTTIMEOUT,
					REQUEST_CONN_TIMEOUT);

				// forces the HTTP request to get back to using GET.
				// curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
				curl_multi_add_handle(tun->env->curl.curlm, easy);

				log(__log_info, "Connect to %s:%d", tun->dst, tun->dport);
				return 0;
			} else log(__log_error, "%s: Failed to urlencode", __FUNCTION__);
			curl_easy_cleanup(easy);
		} else log(__log_error, "%s: Could not initialize easy", __FUNCTION__);
	} else log(__log_error, "%s: Failed to initialize encrypt", __FUNCTION__);
	return -1;
}
