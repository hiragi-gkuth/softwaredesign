#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFSIZE 1024


int main() {
    // AF_INET = IPv4
    // SOCK_STREAM = TCP
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in dest;
    dest.sin_family = AF_INET;
    dest.sin_port = htons(54321);

    inet_pton(AF_INET, "127.0.0.1", &dest.sin_addr.s_addr);

    if (connect(sock, (struct sockaddr *)&dest, sizeof(dest)) != 0) {
        perror("Client: connection failed");
        return EXIT_FAILURE;
    }

    char buf[1024];
    snprintf(buf, sizeof(buf), "Client: message from IPv4 client");

    int n;
    n = write(sock, buf, strnlen(buf, sizeof(buf)));

    memset(buf, 0, sizeof(buf));
    n = read(sock, buf, sizeof(buf));

    printf("Client: n = %d, %s\n", n, buf);

    close(sock);
    return EXIT_SUCCESS;
}