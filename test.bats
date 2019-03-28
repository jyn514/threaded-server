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
  command curl -s "$@" "localhost:$PORT/$URL"
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
    echo hi > test/$file
    [ "$(curl $file)" = "hi" ]
  done
}

@test "resolves directories" {
  echo hi > test/index.html
  [ "$(curl /)" = "hi" ]
}

@test "prevents path traversal" {
  [ "$(curl_status ../../../../../../../../../etc/passwd)" = 404 ]
}

@test "returns 404 if not found" {
  rm -f test/tmp
  [ "$(curl_status tmp "$@")" = 404 ]
}

@test "returns 403 if permission denied" {
  touch test/not_allowed
  chmod 000 test/not_allowed
  [ "$(curl_status not_allowed "$@")" = 403 ]
}

@test "supports HEAD" {
  echo hi > test/index.html
  run curl / -I
  [ "${lines[0]}" = "$(echo -e 'HTTP/1.1 200 OK\r')" ]
  [ "$(content_size "$output")" = 3 ]
}

@test "HEAD returns 403 if permission denied" {
  test_returns_403_if_permission_denied -I
}

@test "HEAD returns 404 if not found" {
  test_returns_404_if_not_found -I
}

@test "Content-Length for HEAD is accurate" {
  > test/blah
  run curl /blah -I
  [ "$(content_size "$output")" = 0 ];
}

@test "Resolves subdirectories" {
  mkdir -p test/subdir
  echo hi > test/subdir/blah
  run curl /subdir/blah -I
  [ "$(content_size "$output")" = 3 ];
}

teardown () {
  # TODO: this is really hacky
  killall main
}
