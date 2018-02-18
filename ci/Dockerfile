From ubuntu:artful

RUN apt-get update -qq
RUN apt-get install -qq build-essential autoconf automake libtool-bin

# kyua, atf, lutok
RUN apt-get install -qq liblua5.2-dev libsqlite3-dev git
RUN mkdir /ci

ADD ./ci/install_atf.sh /ci/install_atf.sh
RUN sh /ci/install_atf.sh

ADD ./ci/install_lutok.sh /ci/install_lutok.sh
RUN sh /ci/install_lutok.sh

ADD ./ci/install_kyua.sh /ci/install_kyua.sh
RUN sh /ci/install_kyua.sh

RUN apt-get install -qq freebsd-buildutils bmake libelf-freebsd-dev
RUN apt-get install -qq gcc-7 clang-5.0
RUN apt-get install -qq curl

ADD . /build
WORKDIR /build

ADD ./ci/build.sh /bin/build
ADD ./ci/coverage.sh /bin/coverage
