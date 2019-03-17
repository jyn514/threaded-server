#include <iostream>
#include <string>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <arpa/inet.h>
#include "response.h"

// 2**16 - 1
#define MAX_PORT 65535

using std::string;

static int sockfd;
volatile static sig_atomic_t interrupted = 0;
string current_dir;

void *respond(void *arg) {
  int client_sock = (long)arg;
  int bytes;
  if (ioctl(client_sock, FIONREAD, &bytes)) {
    perror("Could not get number of bytes");
    return NULL;
  }
  std::cout << "got new socket connection: " << client_sock
   << " with num bytes " << bytes << '\n';
  string BUF(8192, 0);
  // fails miserably if the socket doesn't contain an entire HTTP request
  int received = recv(client_sock, &BUF[0], 8192, 0);
  if (received < 0) {
    perror("Receive failed");
    return NULL;
  }
  BUF.resize(received);
  struct response result = handle_request(BUF);
  send(client_sock, &result.status[0], result.status.size(), 0);
  send(client_sock, &result.headers[0], result.headers.size(), 0);
  send(client_sock, result.body, result.length, 0);
  close(client_sock);
  if (result.is_mmapped) munmap(result.body, result.length);
  else free(result.body);
  return NULL;
}

// we can't pass arguments to interrupt handlers
void cleanup(int ignored) {
  close(sockfd);
  interrupted = 1;
  // cout is not interrupt safe
  const char *message = "Interrupted: preventing further connections\n";
  write(STDERR_FILENO, message, strlen(message));
  pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
  if (argc > 3 || (argc == 2 &&
      (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0))) {
    std::cerr << "usage: " << argv[0] << " [<port>] [<host>]\n";
    exit(1);
  }
  // by pure chance, strtol returns 0 if the entire string is invalid
  // since 0 is an invalid port anyway, we don't need to handle this specially
  // this does mean that strings starting with a valid port number then garbage
  // will be accepted
  const int port = argc > 1 ? strtol(argv[1], NULL, 0) : 80;
  if (port < 1 || port > MAX_PORT) {
    std::cerr << "invalid port number: port must be between 1 and " << MAX_PORT << '\n';
    exit(1);
  }
  const char *addr = argc > 2 ? argv[2] : "0.0.0.0";

  /* set current_dir */
  char temp[PATH_MAX];
  if (!getcwd(temp, sizeof(temp))) {
    perror("Could not getcwd");
    exit(1);
  }
  current_dir = temp + '/';

  /* initialize socket */
  struct sockaddr_in addrport;
  addrport.sin_family = AF_INET;
  addrport.sin_port = htons(port);
  if (inet_aton(addr, &addrport.sin_addr) == 0) {
    std::cerr << "Invalid hostname " << addr << '\n';
    exit(1);
  }

  sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (bind(sockfd, (struct sockaddr *) &addrport, sizeof(addrport)) != 0) {
    perror("Failed to bind to socket, quitting");
    exit(1);
  }

  /* register interrupt handler to close the socket */
  struct sigaction handler, old_handler;
  handler.sa_handler = &cleanup;
  if (sigaction(SIGINT, &handler, &old_handler) != 0) {
    perror("Failed to register SIGINT handler, quitting");
    exit(1);
  }
  if (sigaction(SIGTERM, &handler, &old_handler) != 0) {
    perror("Failed to register SIGTERM handler, quitting");
    exit(1);
  }

  /* open the socket */
  if (listen(sockfd, 10) != 0) {
    perror("Failed to listen to socket, quitting");
    exit(1);
  }
  /* main event loop.
   * get responses out of the way ASAP so we can listen to more connections */
  while (true) {
    long client_sock = accept(sockfd, NULL, NULL);
    // branches are expensive,
    // but making a thread for a failed connection is more expensive
    // also, accept will reset perror, so this is only chance to find out
    // why we have an error
    if (client_sock < 0) {
      if (!interrupted) perror("Failed to receive socket connection, ignoring");
    } else {
      pthread_t current;
      pthread_create(&current, NULL, &respond, (void*)client_sock);
      pthread_detach(current);
    }
  }
}
