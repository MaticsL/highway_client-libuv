/* Copyright (C) 2015, Hsiang Kao (e0e1e) <0xe0e1e@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef __HIGHWAY_ENCRYPT_IMPL_H
#define __HIGHWAY_ENCRYPT_IMPL_H

void encrypt_EVP_BytesToKey(char *, char *, int, int);
int encrypt_rand_bytes(int, char *);

#endif
