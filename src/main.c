/* Copyright 2019 Joshua Nelson
 * This program is licensed under the BSD 3-Clause License.
 * See LICENSE.txt or https://opensource.org/licenses/BSD-3-Clause for details.
 *
 * Main program. Opens sockets, launches threads, and performs initialization.
 */
#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include <errno.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
// bsd doesn't include tcp with sys/socket
#ifndef IPPROTO_TCP
#include <netinet/tcp.h>
#include <netinet/in.h>
#endif
/* mmap */
#include <sys/mman.h>
/* INET_ADDR */
#include <arpa/inet.h>
/* poll */
#include <poll.h>

#include "response.h"
#include "parse.h"

// 2**16 - 1
#define MAX_PORT 65535
#define SOCKET_BUF_SIZE 8192
// milliseconds
#define TIMEOUT 5000

static int sockfd;
static volatile sig_atomic_t interrupted = 0;
char current_dir[PATH_MAX];
DICT mimetypes;

static void cleanup(int);
static void *respond(void *);

int main(int argc, char *argv[]) {
  if (argc > 3 || (argc == 2 &&
      (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0))) {
    fprintf(stderr, "usage: %s [<port>] [<host>]\n", argv[0]);
    exit(1);
  }
  // by pure chance, strtol returns 0 if the entire string is invalid
  // since 0 is an invalid port anyway, we don't need to handle this specially
  // this does mean that strings starting with a valid port number then garbage
  // will be accepted
  const long port = argc > 1 ? strtol(argv[1], NULL, 0) : 80;
  if (port < 1 || port > MAX_PORT) {
    fprintf(stderr,
            "invalid port number: port must be between 1 and %d\n", MAX_PORT);
    exit(1);
  }
  const char *const addr = argc > 2 ? argv[2] : "0.0.0.0";

  /* set current_dir */
  if (!getcwd(current_dir, PATH_MAX)) {
    perror("Could not getcwd");
    exit(1);
  }

  /* read mimetypes */
  mimetypes = get_all_mimetypes();

  /* initialize socket */
  struct sockaddr_in addrport;
  addrport.sin_family = AF_INET;
  addrport.sin_port = htons(port);
  if (inet_aton(addr, &addrport.sin_addr) == 0) {
    fprintf(stderr, "Invalid hostname '%s'\n", addr);
    exit(1);
  }

  sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (bind(sockfd, (struct sockaddr *) &addrport, sizeof(addrport)) != 0) {
    perror("Failed to bind to socket, quitting");
    exit(1);
  }

  /* register interrupt handler to close the socket */
  struct sigaction handler;
  sigset_t set;
  sigemptyset(&set);
  handler.sa_handler = &cleanup;
  handler.sa_flags = 0;
  handler.sa_mask = set;
  if (sigaction(SIGINT, &handler, NULL) != 0
    || sigaction(SIGTERM, &handler, NULL) != 0) {
    perror("Failed to register interrupt handler, quitting");
    exit(1);
  }

  /* ignore SIGPIPE when client closes a connection */
  handler.sa_handler = SIG_IGN;
  if (sigaction(SIGPIPE, &handler, NULL) != 0) {
    perror("Failed to register SIGPIPE handler, quitting");
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
    int client_sock = accept(sockfd, NULL, NULL);
    // branches are expensive,
    // but making a thread for a failed connection is more expensive
    // also, accept will reset perror, so this is only chance to find out
    // why we have an error
    if (client_sock < 0) {
      if (!interrupted) perror("Failed to receive socket connection, ignoring");
    } else {
      // TODO: actually keep track of which threads are active (:
      pthread_t current;
      pthread_create(&current, NULL, &respond, (void*)(long)client_sock);
      pthread_detach(current);
    }
  }
}

void *respond(void *arg) {
  int client_sock = (long)arg;
  char BUF[SOCKET_BUF_SIZE];
  struct pollfd fds = {client_sock, POLLIN, 0};

  while (poll(&fds, 1, TIMEOUT) > 0) {
    // fails miserably if the socket doesn't contain an entire HTTP request
    ssize_t received = recv(client_sock, &BUF[0], SOCKET_BUF_SIZE, 0);
    if (received < 0) {
      perror("Receive failed");
      break;
    } else if (received == 0) {  // connection closed
      break;
    }
    struct response result = handle_request(BUF);
    if (send(client_sock, result.status, strlen(result.status), 0) < 0
        || send(client_sock, result.headers, strlen(result.headers), 0) < 0
        || send(client_sock, result.body, result.length, 0) < 0) {
      if (errno != EPIPE) perror("Failed to send data through socket");
    }
    free(result.status);
    free(result.headers);
    if (result.is_mmapped)
        munmap(result.body, result.length);
    else
        free(result.body);

    if (interrupted || !result.persist_connection) break;
  }

  close(client_sock);
  return NULL;
}

// we can't pass arguments to interrupt handlers, this ignored argument
// is which signal we got
void cleanup(int _) {
  close(sockfd);
  interrupted = 1;
  // cout is not interrupt safe
  const char message[] = "Interrupted: preventing further connections\n";
  write(STDERR_FILENO, message, sizeof(message));
  pthread_exit(NULL);
}
