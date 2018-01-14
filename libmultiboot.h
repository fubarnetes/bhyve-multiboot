#pragma once
#include <userboot.h>
#include <sys/types.h>

#define USERBOOT_VERSION 4
#define MALLOCSZ	(64*1024*1024)

#define CALLBACK(fn, args...) (callbacks->fn(callbacks_arg , ##args))

extern struct loader_callbacks *callbacks;
extern void *callbacks_arg;

#define ERROR(err, str) do { \
			errno = err; \
			perror(str); \
		} while(0)

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))