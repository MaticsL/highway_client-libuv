#ifndef __DBG_H
#define __DBG_H

#include "log.h"

#ifdef DEBUG
#define dbglog(...)		log(__log_info, __VA_ARGS__)
#else
#define dbglog(...)		(1)
#endif

#endif
