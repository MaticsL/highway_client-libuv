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
	int ivsiz;
_end_record(methodinfo)

#include "highway/encrypt_openssl_impl.h"

int encrypt_rc4_md5_iv_length(_methodinfo *method)
{
	return method->ivsiz;
}

extern _methodinfo_0 methodinfo_0_rc4_md5;


static
_methodinfo method_supported[] = {
	{&methodinfo_0_rc4_md5, "rc4-md5", NULL, 16},
	{&methodinfo_0_rc4_md5, "rc4-md5_8", NULL, 8}
};

#define N_METHODS		(sizeof(method_supported)/sizeof(_methodinfo))

#include <string.h>

#ifdef _POSIX_C_SOURCE
#define stricmp strcasecmp
#endif

_methodinfo *
encrypt_rc4_md5_get_methodbyname(const char *name)
{
	_methodinfo *p = method_supported;
	for(;p < &method_supported[N_METHODS]; ++p) {
		if (!stricmp(name, p->name)) {
			if (p->cipher == NULL)
				p->cipher = EVP_rc4();
			return p;
		}
	}
	return NULL;
}

#include <string.h>
#include <openssl/md5.h>

_cipher *
encrypt_rc4_md5_cipher_init(_methodinfo *method, char *key, char *iv, int op)
{
	char md5_key[EVP_MAX_KEY_LENGTH + EVP_MAX_IV_LENGTH];
	int keysiz = encrypt_openssl_key_length(method);
	memcpy(md5_key, key, keysiz); memcpy(md5_key + keysiz, iv, method->ivsiz);
	MD5(md5_key, keysiz+method->ivsiz, md5_key);
	return encrypt_openssl_cipher_init(method, md5_key, NULL, op);
}

_methodinfo_0 methodinfo_0_rc4_md5 = {
	encrypt_openssl_key_length,
	encrypt_rc4_md5_iv_length,
	encrypt_rc4_md5_cipher_init,
	encrypt_openssl_cipher_update,
	encrypt_openssl_cipher_final,
	encrypt_openssl_cipher_exit
};

void encryptor_rc4_md5_init(void)
{
}
