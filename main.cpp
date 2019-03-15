#include <iostream>
#include <string>
#include "response.h"

using std::string;

static inline string read_everything(void) {
  char BUF[1000];
  string everything;
  while (fread(BUF, 1, 1000, stdin) > 0) everything += BUF;
  return everything;
}

int main(void) {
  string everything = read_everything();
  std::cout << handle_request(everything);
}
