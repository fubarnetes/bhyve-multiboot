#pragma once
#include <userboot.h>

#define CALLBACK(fn, args...) (callbacks->fn(callbacks_arg , ##args))

extern struct loader_callbacks *callbacks;
extern void *callbacks_arg;
