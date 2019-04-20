# crappy-server
[![Build Status](https://img.shields.io/badge/Ci-failing-red.svg)](https://cirrus-ci.com/github/jyn514/crappy-server)

Servers static files and prevents path traversal attacks.
Will probably crash if you try anything fancy.

However, it handles requests at about 10 msec/response, which is comparable to Nginx.
Try `ab -n 1000 -c 100 localhost:8080/` for a comparison.

## Usage
```
$ ./main -h
usage: ./main [<port>] [<host>]
```

## Compiling
### Dependencies
- make (or `gmake` on Mac/BSD)
- a C compiler. clang is the default, change CC=gcc in the makefile if you don't want to install it.
- libmagic (`brew install libmagic` on Mac, should be preinstalled on Linux/BSD)

## Testing
### Dependencies
- sed/grep
- [curl](https://curl.haxx.se/)
- [ab](https://httpd.apache.org/docs/2.4/programs/ab.html) (`apt install apache2-utils` on debian/ubuntu)
- [clang-tidy](https://clang.llvm.org/extra/clang-tidy/)
- [cppcheck](https://sourceforge.net/p/cppcheck/wiki/Home/)
- bash and [bats](https://github.com/bats-core/bats-core/)

### Running Tests
`make test`

### Optional
`valgrind --leak-check=full build/main 8080`

Run a few queries and see if there's any memory leaks.

## But Why?
I'm taking networking and operating systems courses where we don't write any code.
I thought it would be nice to get hands-on in system internals.

## Licensing
This project is licensed under the BSD 3-Clause license. See LICENSE.txt for details.

## TODO
- Figure out why -fsanitize=thread thinks my interrupt handler isn't safe
- Check what happens if we run into the thread limit
(currently, we spawn a thread for every request no matter how many threads we already have)
- Figure out why `sleep(10)` in `handle_request` adds a response time of 18 seconds
- Logging (?)
- Dynamic pages (??)
- HTTP/2.0 (???)
