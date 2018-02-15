#!/bin/sh
cd
if [ -d lutok/.git ]; then
    cd lutok
    git pull
else
    git clone https://github.com/jmmv/lutok.git
    cd lutok
    autoreconf -i -s
    ./configure --prefix=/usr
fi

make
sudo make install