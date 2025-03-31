#ifndef HEADER_RUBIK_COMMON_H
#define HEADER_RUBIK_COMMON_H

#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>

#define UNUSED __attribute__((unused))

#define POSIXOK(e) \
	do { \
		UNUSED int _tmp_posixok = (e); \
		assert(!e); \
	} while (0)

#endif /* !HEADER_RUBIK_COMMON */
