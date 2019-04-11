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
  sed 's/Content-Length: //; s/\s//g' | tr -d '\r'
}

# tests

@test "serves files" {
  for file in index.html hey how are you; do
    echo hi > $file
    [ "$(curl $file)" = "hi" ]
  done
}

@test "resolves directories" {
  echo hi > index.html
  [ "$(curl /)" = "hi" ]
}

@test "prevents path traversal" {
  [ "$(curl_status ../../../../../../../../../etc/passwd)" -eq 404 ]
}

@test "returns 404 if not found" {
  rm -f not_found
  [ "$(curl_status not_found "$@")" -eq 404 ]
}

@test "returns 403 if permission denied" {
  touch not_allowed
  chmod 000 not_allowed
  [ "$(curl_status not_allowed "$@")" -eq 403 ]
}

@test "supports HEAD" {
  echo hi > index.html
  output="$(curl / -I)"
  [ "$(echo "$output" | head -n 1)" = "$(echo -e 'HTTP/1.1 200 OK\r')" ]
  [ $(content_size "$output") -eq 3 ]
}

@test "Content-Length is accurate" {
  echo hi > blah
  [ "$(content_size "$(curl /blah -i)")" = 3 ]
  > empty
  [ "$(content_size "$(curl /empty -i)")" = 0 ]
}

@test "HEAD returns 403 if permission denied" {
  test_returns_403_if_permission_denied -I
}

@test "HEAD returns 404 if not found" {
  test_returns_404_if_not_found -I
}

@test "Content-Length for HEAD is accurate" {
  > blah
  run curl /blah -I
  [ "$(content_size "$output")" = 0 ]
  echo hi > hey
  [ "$(content_size "$(curl /hey -I)")" = 3 ]
}

@test "Resolves subdirectories" {
  mkdir -p subdir
  echo hi > subdir/blah
  [ $(content_size "$(curl /subdir/blah -I)") -eq 3 ] | debug
}

@test "Closes connection for HTTP/1.0" {
  ab -s 1 localhost:$PORT/
}

@test "Maintains connection for HTTP/1.1+" {
  curl / localhost:$PORT -v 2>&1 | grep "Re-using existing connection"
}
