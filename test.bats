# http://redsymbol.net/articles/unofficial-bash-strict-mode/
# requires bats >= 1.0.0 (https://github.com/bats-core/bats-core)
set -euo pipefail

# utilities

debug () {
  sed 's/^/# /' >&3
}

debug_cmd () {
  stderr "$@" | debug
}

stderr () {
  "$@" 2>&1 >/dev/null
}

curl () {
  URL=$1; shift
  command curl -m 1 -s "$@" "localhost:$PORT/$URL"
}

curl_status () {
  URL=$1; shift
  curl "$URL" -o /dev/null -w "%{http_code}" "$@"
}

content_size () {
  grep "Content-Length: " <<< "$@" |
  sed 's/Content-Length: //; s/\s//g'
}

# runs before each test

setup () {
  PORT=$(( $RANDOM + 1000 ))
  # this is ugly but required:
  # https://github.com/bats-core/bats-core#file-descriptor-3-read-this-if-bats-hangs
  cd $BUILD_DIR && ./main $PORT >/dev/null &
  PID=$!
}

# tests

@test "serves files" {
  for file in index.html hey how are you; do
    echo hi > $BUILD_DIR/$file
    [ "$(curl $file)" = "hi" ]
  done
}

@test "resolves directories" {
  echo hi > $BUILD_DIR/index.html
  [ "$(curl /)" = "hi" ]
}

@test "prevents path traversal" {
  [ "$(curl_status ../../../../../../../../../etc/passwd)" = 404 ]
}

@test "returns 404 if not found" {
  rm -f $BUILD_DIR/tmp
  [ "$(curl_status tmp "$@")" = 404 ]
}

@test "returns 403 if permission denied" {
  touch $BUILD_DIR/not_allowed
  chmod 000 $BUILD_DIR/not_allowed
  [ "$(curl_status not_allowed "$@")" = 403 ]
}

@test "supports HEAD" {
  echo hi > $BUILD_DIR/index.html
  run curl / -I
  [ "${lines[0]}" = "$(echo -e 'HTTP/1.1 200 OK\r')" ]
  [ "$(content_size "$output")" = 3 ]
}

@test "Content-Length is accurate" {
  echo hi > $BUILD_DIR/blah
  [ "$(content_size "$(curl /blah -i)")" = 3 ]
  > $BUILD_DIR/empty
  [ "$(content_size "$(curl /empty -i)")" = 0 ]
}

@test "HEAD returns 403 if permission denied" {
  test_returns_403_if_permission_denied -I
}

@test "HEAD returns 404 if not found" {
  test_returns_404_if_not_found -I
}

@test "Content-Length for HEAD is accurate" {
  > $BUILD_DIR/blah
  run curl /blah -I
  [ "$(content_size "$output")" = 0 ]
  echo hi > $BUILD_DIR/hey
  [ "$(content_size "$(curl /hey -I)")" = 3 ]
}

@test "Resolves subdirectories" {
  mkdir -p $BUILD_DIR/subdir
  echo hi > $BUILD_DIR/subdir/blah
  run curl /subdir/blah -I
  [ "$(content_size "$output")" = 3 ];
}

@test "Closes connection for HTTP/1.0" {
  ab -s 1 localhost:$PORT/
}

@test "Maintains connection for HTTP/1.1+" {
  curl / localhost:$PORT -v 2>&1 | grep "Re-using existing connection"
}

teardown () {
  # TODO: this is really hacky
  killall main
}
