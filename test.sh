#!/usr/bin/env bash

[ -z "$BUILD_DIR" ] && export BUILD_DIR=tmp
export PORT=$(( $RANDOM + 1001 ))

if [ -n "$1" ]; then MAKE="$1"
elif [ -x "$(which gmake)" ]; then MAKE=gmake
else MAKE=make
fi

$MAKE || exit 1

cd $BUILD_DIR
./main $PORT >/dev/null &
bats $OLDPWD/test.bats
STATUS=$?
kill $!
exit $STATUS
