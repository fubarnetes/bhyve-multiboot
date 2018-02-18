#!/bin/sh
git clone https://github.com/jmmv/atf.git
cd atf
autoreconf -i -s
./configure --prefix=/usr
make
make install
