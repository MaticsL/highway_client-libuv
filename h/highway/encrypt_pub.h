/* Copyright (C) 2015, Hsiang Kao (e0e1e) <0xe0e1e@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef __HIGHWAY_ENCRYPT_PUB_H
#define __HIGHWAY_ENCRYPT_PUB_H

#include "encrypt.h"

_declrec(encrypt)
typedef _encrypt _decrypt;

int encrypt_iv_length(_encrypt *);
#define decrypt_iv_length		encrypt_iv_length

void encrypt_generatekey(_encrypt *, char *);
_decrypt *decrypt_init(char *, char *);
_encrypt *encrypt_init(char *, char *);
int encrypt_update(_encrypt *, char *, unsigned, char *);
int decrypt_update(_decrypt *, char *, unsigned, char *);
int encrypt_final(_encrypt *, char *);
#define decrypt_final encrypt_final
void encrypt_free(_encrypt *);
#define decrypt_free encrypt_free

void encryptor_init(void);


#endif
