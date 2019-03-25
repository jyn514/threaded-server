setup () {
  PORT=$(( $RANDOM + 1000 ))
  mkdir -p test
  make BUILD_DIR=test >/dev/null 2>&1
  # this is ugly but required:
  # https://github.com/bats-core/bats-core#file-descriptor-3-read-this-if-bats-hangs
  cd test && ./main $PORT >/dev/null 2>&1 3>&- &
  PID=$!
}

# utilities

debug () {
  echo "$@" >&3
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

# tests

@test "server compiles" {
  make BUILD_DIR=test
}

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
  [ "$(content_size "$(curl_status blah)")" = 0 ];
}

teardown () {
  # TODO: this is really hacky
  killall main
}
