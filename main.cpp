#include <iostream>
#include <string>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include "response.h"

#define MAX_THREADS 20

using std::string;

volatile sig_atomic_t interrupted = 0;

void *respond(void *arg) {
  int client_sock = (long)arg;
  if (client_sock < 0) {
    if (!interrupted) perror("Failed to receive socket connection, ignoring");
    return NULL;
  }
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
  string result = handle_request(BUF);
  send(client_sock, &result[0], result.size(), 0);
  close(client_sock);
  return NULL;
}

// we can't pass arguments to interrupt handlers
void cleanup(int ignored) {
  interrupted = 1;
}

int main(void) {

  /* initialize socket */
  struct sockaddr_in addrport;
  addrport.sin_family = AF_INET;
  addrport.sin_port = htons(8080);
  addrport.sin_addr.s_addr = htonl(INADDR_ANY);

  int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (bind(sockfd, (struct sockaddr *) &addrport, sizeof(addrport)) != 0) {
    perror("Failed to bind to socket 8080, quitting");
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
  if (listen(sockfd, MAX_THREADS) != 0) {
    perror("Failed to listen to socket, quitting");
    exit(1);
  }
  /* main event loop.
   * get responses out of the way ASAP so we can listen to more connections */
  pthread_t threads[MAX_THREADS];
  int num_threads = 0;
  while (true) {
    long client_sock = accept(sockfd, NULL, NULL);
    pthread_create(&threads[num_threads++], NULL, &respond, (void*)client_sock);
    if (interrupted) {
      close(sockfd);
      std::cout << "Interrupted: preventing further connections\n";
      for (int i = 0; i < num_threads; ++i) {
        pthread_join(threads[i], NULL);
      }
      exit(130);
    }
  }
}
