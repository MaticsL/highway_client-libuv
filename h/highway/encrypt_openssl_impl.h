/* Copyright (C) 2015, Hsiang Kao (e0e1e) <0xe0e1e@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef __HIGHWAY_ENCRYPT_OPENSSL_IMPL_H
#define __HIGHWAY_ENCRYPT_OPENSSL_IMPL_H

#include "encrypt.h"

int encrypt_openssl_key_length(_methodinfo *);
_cipher *encrypt_openssl_cipher_init(_methodinfo *, char *, char *, int);
int encrypt_openssl_cipher_update(_cipher *, char *, int, char *);
int encrypt_openssl_cipher_final(_cipher *, char *);
void encrypt_openssl_cipher_exit(_cipher *);

#endif
