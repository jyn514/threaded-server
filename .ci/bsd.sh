#!/bin/sh
# running
pkg install -y gmake
# testing
pkg install -y valgrind bats-core apache24 curl sudo
# test setup
mkdir /var/www
chown www:www /var/www
chmod a+r test.bats
