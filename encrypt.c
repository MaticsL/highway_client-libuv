/* Copyright (C) 2015, Hsiang Kao (e0e1e) <0xe0e1e@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "highway/encrypt_pub.h"
#include "highway/encrypt_impl.h"

#define ENCRYPT_MAX_KEY_LENGTH              32
#define ENCRYPT_MAX_IV_LENGTH               16

_record(encrypt)
	_methodinfo *method;
	int keysiz;
	char keyiv[ENCRYPT_MAX_KEY_LENGTH + ENCRYPT_MAX_IV_LENGTH];
	_cipher *cipher;
_end_record(encrypt)

_record(methodinfo)
	_methodinfo_0 *_0;
_end_record(methodinfo)

int
encrypt_iv_length(_encrypt *crypt)
{
	_methodinfo *m = crypt->method;
	return m->_0->iv_length(m);
}

#include <string.h>


void
encrypt_generatekey(_encrypt *crypt, char *pw)
{
	_methodinfo *m = crypt->method;
	int keysiz = m->_0->key_length(m);
	if (keysiz)	// 若keysiz != 0，则使用标准算法生成
		encrypt_EVP_BytesToKey(pw, crypt->keyiv, crypt->keysiz = keysiz, 0);
	else {		// 若keysiz == 0, 此时key就是pw
		int i;
		for(i=0; pw[i] != '\0'; ++i)
			crypt->keyiv[i] = pw[i];
		crypt->keysiz = i;
	}
}

#include <stdlib.h>


_methodinfo *encrypt_openssl_get_methodbyname(char *);
_methodinfo *encrypt_rc4_md5_get_methodbyname(char *);

static
_methodinfo *(*encrypt_get_methodbyname[])(char *) = {
	encrypt_openssl_get_methodbyname,
	encrypt_rc4_md5_get_methodbyname,
	NULL
};

_decrypt *decrypt_init(char *method, char *pw)
{
	int i;
	for(i=0; encrypt_get_methodbyname[i] != NULL; ++i) {
		_methodinfo *m = encrypt_get_methodbyname[i](method);
		if (m != NULL) {
			_decrypt *crypt = (_encrypt *)malloc(sizeof(_decrypt));
			if (crypt != NULL) {
				crypt->method = m;
				crypt->cipher = NULL;
				if (pw != NULL) encrypt_generatekey(crypt, pw);
				return crypt;
			}
		}
	}
	return NULL;
}

_encrypt *encrypt_init(char *method, char *pw)
{
	_encrypt *crypt = decrypt_init(method, pw);
	if (crypt != NULL) {
		_methodinfo *m = crypt->method;
		encrypt_rand_bytes(m->_0->iv_length(m), crypt->keyiv + crypt->keysiz);
	}
	return crypt;
}

int encrypt_update(_encrypt *crypt, char *dst, unsigned bufsiz, char *buf)
{
	_methodinfo *m = crypt->method;
	if (m != NULL) {
		int ivsiz;
		if (crypt->cipher == NULL) {
			char *iv = crypt->keyiv + crypt->keysiz;
			// 1 for encryption, 0 for decryption
			crypt->cipher = m->_0->cipher_init(m, crypt->keyiv, iv, 1);
			memcpy(dst, iv, ivsiz = m->_0->iv_length(m));
			dst += ivsiz;
		} else ivsiz = 0;
		return m->_0->cipher_update(crypt->cipher, dst, bufsiz, buf) + ivsiz;
	}
	return -1;
}

int decrypt_update(_decrypt *crypt, char *dst, unsigned bufsiz, char *buf)
{
	_methodinfo *m = crypt->method;
	if (m != NULL) {
		if (crypt->cipher == NULL) {
			int ivsiz = m->_0->iv_length(m);
			if (bufsiz >= ivsiz) {
				char *iv = crypt->keyiv + crypt->keysiz;
				memcpy(iv, buf, ivsiz);
				// 1 for encryption, 0 for decryption
				crypt->cipher = m->_0->cipher_init(m, crypt->keyiv, iv, 0);
				buf += ivsiz, bufsiz -= ivsiz;
			} else return -1;
		}
		if (bufsiz) return m->_0->cipher_update(crypt->cipher, dst, bufsiz, buf);
		else return 0;
	}
	return -1;
}

int encrypt_final(_encrypt *crypt, char *dst)
{
	_methodinfo *m = crypt->method;
	if (m != NULL) {
		int ret = m->_0->cipher_final(crypt->cipher, dst);
		if (ret >= 0) {
			m->_0->cipher_exit(crypt->cipher);
			crypt->cipher = NULL;
		}
		return ret;
	}
	return -1;
}

void encrypt_free(_encrypt *crypt)
{
	_methodinfo *m = crypt->method;
	if (m != NULL)
		if (crypt->cipher != NULL)
			m->_0->cipher_exit(crypt->cipher);
	free(crypt);
}

void encryptor_openssl_init(void);
void encryptor_rc4_md5_init(void);

void encryptor_init(void)
{
	encryptor_openssl_init();

}

 