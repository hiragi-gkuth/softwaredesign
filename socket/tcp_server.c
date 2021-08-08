#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFSIZE 1024

int main() {
    // create socket
    int sock0 = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;

    // setting socket
    addr.sin_family = AF_INET;
    addr.sin_port = htons(54321);
    addr.sin_addr.s_addr = INADDR_ANY; // listen from all network interface

    if (bind(sock0, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        perror("Server: bind failed");
        return EXIT_FAILURE;
    }

    // listen...
    listen(sock0, 5);

    // handle connection request
    char addrstr[INET_ADDRSTRLEN];
    struct sockaddr_in client;
    socklen_t len = sizeof(client);

    int sock = accept(sock0, (struct sockaddr *)&client, &len);
    inet_ntop(AF_INET, &client.sin_addr, addrstr, sizeof(addrstr));
    printf("Server: connection from: %s, port=%d\n", addrstr, ntohs(client.sin_port));

    // read, and write
    int n;
    char buf[BUFSIZE];
    memset(buf, 0, sizeof(buf));
    n = read(sock, buf, sizeof(buf));
    printf("Server: n = %d\nmessage: %s\n", n, buf);

    snprintf(buf, sizeof(buf), "Server: message from IPv4 server");
    n = write(sock, buf, strnlen(buf, sizeof(buf)));

    // close sockets
    close(sock);
    close(sock0);

    return EXIT_SUCCESS;
}