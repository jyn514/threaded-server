#!/usr/bin/env bash

export BUILD_DIR=tmp
export PORT=$(( $RANDOM + 1001 ))

if [ -n "$1" ]; then MAKE="$1"
elif [ -x "$(which gmake)" ]; then MAKE=gmake
else MAKE=make
fi

$MAKE

cd tmp
./main $PORT >/dev/null &
bats ../test.bats
STATUS=$?
kill $!
exit $STATUS
