CC = clang
CXX = $(CC)++
override CFLAGS += -g3 -Wall -Wextra -Wpedantic -Wshadow -Wcovered-switch-default -pthread -flto
override CXXFLAGS = $(CFLAGS)

main: main.o response.o parse.o lib/libmagic.so
	$(CXX) $(CXXFLAGS) $^ -o $@
main.o: response.h
response.o: response.h parse.h
parse.o: parse.h

clean:
	$(RM) *.o main
