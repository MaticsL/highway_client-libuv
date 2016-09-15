#ifndef __DIE_H
#define __DIE_H

#include "log.h"
#include <stdlib.h>

#define die(...)	do{ \
	log(__log_error, __VA_ARGS__);	\
	exit(-1);	\
} while(0)

#endif
