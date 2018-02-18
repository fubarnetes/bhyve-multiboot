#!/bin/bash

CFLAGS="-I/usr/include/freebsd/ -ftest-coverage -fprofile-arcs"
LIBELF=-lelf-freebsd
LIBATF_C=/usr/lib/libatf-c.a

bmake CFLAGS="$CFLAGS" LIBELF=$LIBELF LIBATF_C=$LIBATF_C $@ all
bmake CFLAGS="$CFLAGS" LIBELF=$LIBELF LIBATF_C=$LIBATF_C $@ check
