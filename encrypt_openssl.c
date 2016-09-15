/* Copyright (C) 2015, Hsiang Kao (e0e1e) <0xe0e1e@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <openssl/evp.h>
#include "highway/encrypt.h"

_record(methodinfo)
	_methodinfo_0 *_0;
	char *name;
	const EVP_CIPHER *cipher;
_end_record(methodinfo)

int encrypt_openssl_key_length(_methodinfo *method)
{
	return EVP_CIPHER_key_length(method->cipher);
}

int encrypt_openssl_iv_length(_methodinfo *method)
{
	return EVP_CIPHER_iv_length(method->cipher);
}

extern _methodinfo_0 methodinfo_0_openssl;



static
_methodinfo method_supported[] = {
	{&methodinfo_0_openssl, "aes-128-cfb", NULL},
	{&methodinfo_0_openssl, "aes-192-cfb", NULL},
	{&methodinfo_0_openssl, "aes-256-cfb", NULL},
	{&methodinfo_0_openssl, "aes-128-ofb", NULL},
	{&methodinfo_0_openssl, "aes-192-ofb", NULL},
	{&methodinfo_0_openssl, "aes-256-ofb", NULL},
	{&methodinfo_0_openssl, "aes-128-ctr", NULL},
	{&methodinfo_0_openssl, "aes-192-ctr", NULL},
	{&methodinfo_0_openssl, "aes-256-ctr", NULL},
	{&methodinfo_0_openssl, "aes-128-cfb8", NULL},
	{&methodinfo_0_openssl, "aes-192-cfb8", NULL},
	{&methodinfo_0_openssl, "aes-256-cfb8", NULL},
	{&methodinfo_0_openssl, "aes-128-cfb1", NULL},
	{&methodinfo_0_openssl, "aes-192-cfb1", NULL},
	{&methodinfo_0_openssl, "aes-256-cfb1", NULL},
	{&methodinfo_0_openssl, "bf-cfb", NULL},
	{&methodinfo_0_openssl, "camellia-128-cfb", NULL},
	{&methodinfo_0_openssl, "camellia-192-cfb", NULL},
	{&methodinfo_0_openssl, "camellia-256-cfb", NULL},
	{&methodinfo_0_openssl, "cast5-cfb", NULL},
	{&methodinfo_0_openssl, "des-cfb", NULL},
	{&methodinfo_0_openssl, "idea-cfb", NULL},
	{&methodinfo_0_openssl, "rc2-cfb", NULL},
	{&methodinfo_0_openssl, "rc4", NULL},
	{&methodinfo_0_openssl, "seed-cfb", NULL}
};

#define N_METHODS		(sizeof(method_supported)/sizeof(_methodinfo))

#include <string.h>

#ifdef _POSIX_C_SOURCE
#define stricmp strcasecmp
#endif

_methodinfo *
encrypt_openssl_get_methodbyname(const char *name)
{
	_methodinfo *p = method_supported;
	for(;p < &method_supported[N_METHODS]; ++p) {
		if (!stricmp(name, p->name)) {
			if (p->cipher == NULL)
				p->cipher = EVP_get_cipherbyname(name);
			return p;
		}
	}
	return NULL;
}

#include "list.h"

_record(cipher)
	_methodinfo *method;
	_list_node_s list;
	EVP_CIPHER_CTX *ctx;
_end_record(cipher)

_list_node_s cipher_free_list;


#define __encrypt_openssl_alloc() ({\
	_cipher *__ciph; \
	_list_node_s *s = singly_linked_list_the_head(&cipher_free_list);	\
	if (s != NULL) { \
		__ciph = container_of(s, _cipher, list);	\
		singly_linked_list_remove_after(&cipher_free_list); \
	} else __ciph = (_cipher *)calloc(1, sizeof(_cipher)); \
__ciph;})

_cipher *encrypt_openssl_cipher_init(_methodinfo *method, char *key, char *iv, int op)
{
	_cipher *ciph = __encrypt_openssl_alloc();
	if (ciph != NULL) {
		ciph->method = method;
		if (ciph->ctx == NULL) {
			ciph->ctx = EVP_CIPHER_CTX_new();
			if (ciph->ctx != NULL) {
				if (EVP_CipherInit_ex(ciph->ctx, method->cipher, NULL, key, iv, op))
					return ciph;
			}
		}
		singly_linked_list_insert_after(&cipher_free_list, &ciph->list);
	}
	return NULL;
}

int
encrypt_openssl_cipher_update(_cipher *ciph, char *dst, int bufsiz, char *buf)
{
	if (ciph->ctx != NULL) {
		int dstsiz;
		if (EVP_CipherUpdate(ciph->ctx, dst, &dstsiz, buf, bufsiz))
			return dstsiz;
	}
	return -1;
}

int
encrypt_openssl_cipher_final(_cipher *ciph, char *dst)
{
	if (ciph->ctx != NULL) {
		int dstsiz;
		if (EVP_CipherFinal(ciph->ctx, dst, &dstsiz))
			return dstsiz;
	}
	return -1;
}

void
encrypt_openssl_cipher_exit(_cipher *ciph)
{
	if (ciph->ctx != NULL) {
		EVP_CIPHER_CTX_cleanup(ciph->ctx);
		EVP_CIPHER_CTX_free(ciph->ctx);
		ciph->ctx = NULL;
	}
	singly_linked_list_insert_after(&cipher_free_list, &ciph->list);
}

_methodinfo_0 methodinfo_0_openssl = {
	encrypt_openssl_key_length,
	encrypt_openssl_iv_length,
	encrypt_openssl_cipher_init,
	encrypt_openssl_cipher_update,
	encrypt_openssl_cipher_final,
	encrypt_openssl_cipher_exit
};

void
encrypt_openssl_cipher_freeall()
{
	_list_node_s *s;
	while((s = singly_linked_list_the_head(&cipher_free_list)) != \
		singly_linked_list_the_end(&cipher_free_list)) {
		singly_linked_list_remove_after(&cipher_free_list);
		_cipher *p = container_of(s, _cipher, list);
		free(p);
	}
}

#include <openssl/md5.h>

void encrypt_EVP_BytesToKey(char *pw, char *keyiv, int keysiz, int ivsiz)
{
	char Di[MD5_DIGEST_LENGTH + strlen(pw)];
	int Disiz = 0;
	
	for(ivsiz+=keysiz; ivsiz; ivsiz -= Disiz) {
		char *p;
		for(p = pw; *p != '\0'; p++)
			Di[Disiz++] = *p;
		MD5(Di, Disiz, Di);
		Disiz = ivsiz>MD5_DIGEST_LENGTH ? MD5_DIGEST_LENGTH : ivsiz;
		memcpy(keyiv, Di, Disiz);
		keyiv += Disiz;
	}
}

#include <openssl/rand.h>

int encrypt_rand_bytes(int len, char *buf) {
	return RAND_bytes(buf, len);
}

void encryptor_openssl_init(void) {
	OpenSSL_add_all_ciphers();
}

