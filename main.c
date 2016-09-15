/* Copyright (C) 2015, Hsiang Kao (e0e1e) <0xe0e1e@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "log.h"

void usage(void)
{
	log(__log_info, "Usage: highway [OPTIONS...]\n"
		"The highway tunnel proxy client that helps you bypass firewalls.\n"
		"\t-s SERVER_BASEURL      server baseurl\n"
		"\t-p PASSWORD            password\n"
		"\t-m METHOD              encryption method, default: aes-256-cfb\n"
		"\t-b LOCAL_ADDR          local binding address, default: 0.0.0.0\n"
		"\t-l LOCAL_PORT          local port, default: 1080\n"
		"\t-h                     Display this information");
}

#include <stdio.h>
#include "getopt.h"
#include "die.h"

#include "highway/encrypt_pub.h"
#include "highway/envir_pub.h"
#include "highway/tun_pub.h"

int main(int argc, char *argv[])
{
	setvbuf(stderr, NULL, _IONBF, 0);
#ifdef WIN32
	setvbuf(stdout, NULL, _IONBF, 0);
#else
	setvbuf(stdout, NULL, _IOLBF, BUFSIZ);
#endif

	_tunnelinfo info = {
#if	0
		.baseurl = "http://127.0.0.1:8080/tun.php",
		.pw = "xxxxxx",
#else
		.baseurl = NULL,
		.pw = NULL,
#endif
		.method = "aes-256-cfb"
	};
	char *local_address = "0.0.0.0";
	int local_port = 1080;
	
	int opt;
	extern char *optarg;
	while((opt= getopt(argc, argv, "s:p:m:b:l:h")) != -1) {
		switch(opt) {
			case 's': info.baseurl = optarg; break;
			case 'p': info.pw = optarg; break;
			case 'm': info.method = optarg; break;
			case 'b': local_address = optarg; break;
			case 'l': local_port = atoi(optarg); break;
			case 'h': usage(); return 0;
		}
	}
	if (info.baseurl == NULL)
		die("Highway: baseurl is not specified. Type -h for help");
	else if (info.pw == NULL)
		die("Highway: password is not specified. Type -h for help");
	else if (local_port >= 0x10000)
		die("Highway: invaild local port. Type -h for help");
	encryptor_init();
	return tunnelmgr(envir_default(), local_address, local_port, &info);
}
