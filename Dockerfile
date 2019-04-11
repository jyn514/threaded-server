FROM buildpack-deps
RUN apt update
# to build
RUN apt install -y make gcc libmagic-dev
# to test
RUN apt install -y apache2-utils curl bats clang-tidy cppcheck valgrind
