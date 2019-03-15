CXXFLAGS = -g -Wall -Wextra -Wpedantic -Wshadow -fsanitize=address -fsanitize-address-use-after-scope -pthread

main: main.o response.o parse.o
	$(CXX) $(CXXFLAGS) $^ -o $@
main.o: response.h
response.o: response.h parse.h
parse.o: parse.h

clean:
	$(RM) *.o main
