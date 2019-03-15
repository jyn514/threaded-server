#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include "response.h"

using std::string;

void respond(int client_sock) {
  if (client_sock < 0) {
    perror("Failed to receive socket connection, ignoring");
    return;
  }
  int bytes;
  if (ioctl(client_sock, FIONREAD, &bytes)) {
    perror("Could not get number of bytes");
    return;
  }
  std::cout << "got new socket connection: " << client_sock
   << " with num bytes " << bytes << '\n';
  string BUF(8192, 0);
  int received = recv(client_sock, &BUF[0], 8192, 0);
  if (received < 0) {
    perror("Receive failed");
    return;
  }
  BUF.resize(received);
  string result = handle_request(BUF);
  send(client_sock, &result[0], result.size(), 0);
  close(client_sock);
}

int main(void) {
  struct sockaddr_in addrport;
  addrport.sin_family = AF_INET;
  addrport.sin_port = htons(8080);
  addrport.sin_addr.s_addr = htonl(INADDR_ANY);

  int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (bind(sockfd, (struct sockaddr *) &addrport, sizeof(addrport)) != 0) {
    perror("Failed to bind to socket 8080, quitting");
    exit(1);
  }

  if (listen(sockfd, 10) != 0) {
    perror("Failed to listen to socket, quitting");
    exit(1);
  }
  /* main event loop.
   * get responses out of the way ASAP so we can listen to more connections */
  while (true) {
    respond(accept(sockfd, NULL, NULL));
  }
}
