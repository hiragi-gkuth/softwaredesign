#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFSIZE 1024

int main() {
  // create socket
  int sock0 = socket(AF_INET6, SOCK_STREAM, 0);
  struct sockaddr_in6 addr;

  // setting socket
  addr.sin6_family = AF_INET6;
  addr.sin6_port = htons(54321);
  addr.sin6_addr = in6addr_any;

  if (bind(sock0, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
    perror("Server: bind failed");
    return EXIT_FAILURE;
  }

  // listen...
  listen(sock0, 5);

  // handle connection request
  char addrstr[INET6_ADDRSTRLEN];
  struct sockaddr_in6 client;
  socklen_t len = sizeof(client);

  for (;;) {
    int sock = accept(sock0, (struct sockaddr *)&client, &len);
    inet_ntop(AF_INET6, &client.sin6_addr, addrstr, sizeof(addrstr));
    printf("Server: connection from: %s, port=%d\n", addrstr,
           ntohs(client.sin6_port));

    // read, and write
    int n;
    char buf[BUFSIZE];
    memset(buf, 0, sizeof(buf));
    n = read(sock, buf, sizeof(buf));
    printf("Server: n = %d\nmessage: %s\n", n, buf);

    snprintf(buf, sizeof(buf), "Server: message from IPv6 server");
    n = write(sock, buf, strnlen(buf, sizeof(buf)));
    
    close(sock);
  }

  // close sockets

  close(sock0);

  return EXIT_SUCCESS;
}