#ifndef LOGGING_H
#define LOGGING_H

#include <stdio.h>

#define SUC(fmt, ...) printf("\x1b[35m" "[_INFO] " fmt "\x1b[0m\n", ##__VA_ARGS__)
#define INF(fmt, ...) printf("\x1b[34m" "[_INFO] " fmt "\x1b[0m\n", ##__VA_ARGS__)
#define WRN(fmt, ...) printf("\x1b[33m" "[_WARN] " fmt "\x1b[0m\n", ##__VA_ARGS__)
#define ERR(fmt, ...) printf("\x1b[31m" "[ERROR] " fmt "\x1b[0m\n", ##__VA_ARGS__)

#ifdef NDEBUG
	#define DBG(fmt, ...) ((void)0)
	#define VER(fmt, ...) ((void)0)
#else
	#define DBG(fmt, ...) VER("[DEBUG] " fmt "\n", ##__VA_ARGS__)
	#define VER(fmt, ...) VER(fmt, ##__VA_ARGS__)
#endif

#endif