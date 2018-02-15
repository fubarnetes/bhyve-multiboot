#!/bin/sh
cd
if [ -d kyua/.git ]; then
    cd kyua
    git pull
else
    git clone https://github.com/jmmv/kyua.git
    cd kyua
    autoreconf -i -s
    ./configure --prefix=/usr
fi

make
sudo make install