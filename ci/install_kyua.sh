#!/bin/sh
git clone https://github.com/jmmv/kyua.git
cd kyua
autoreconf -i -s
./configure --prefix=/usr
make -j 4 CXXFLAGS=-Wno-error
make install
