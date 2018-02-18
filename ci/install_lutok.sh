#!/bin/sh
git clone https://github.com/jmmv/lutok.git
cd lutok
autoreconf -i -s
./configure --prefix=/usr
make
make install
