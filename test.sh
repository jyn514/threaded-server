#!/usr/bin/env bash

[ -z "$BUILD_DIR" ] && export BUILD_DIR=tmp
export PORT=$(( RANDOM + 1001 ))

if [ -n "$1" ]; then MAKE="$1"
elif [ -x "$(which gmake)" ]; then MAKE=gmake
else MAKE=make
fi

MAIN=./main
if [ "$2" = "--valgrind" ] || [ "$2" = '-v' ]; then
	MAIN="valgrind --leak-check=full $MAIN"
fi

$MAKE || exit 1

cd $BUILD_DIR || exit 2
$MAIN $PORT >/dev/null &
sleep 1
bats "$OLDPWD/test.bats"
STATUS=$?
kill $!
exit $STATUS
