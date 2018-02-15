#!/bin/sh
cd
if [ -d atf/.git ]; then
    cd atf
    git pull
else
    git clone https://github.com/jmmv/atf.git
    cd atf
    autoreconf -i -s
    ./configure --prefix=/usr
fi

make
sudo make install
