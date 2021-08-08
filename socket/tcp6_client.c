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
    // AF_INET6 = IPv6
    // SOCK_STREAM = TCP
    int soc = socket(AF_INET6, SOCK_STREAM, 0);
    struct sockaddr_in6 dest;

    memset(&dest, 0, sizeof(dest));
    dest.sin6_family = AF_INET6;
    dest.sin6_port = htons(54321);

    // dest.sin6_addr = in6addr_loopback;
    inet_pton(AF_INET6, "::0", &dest.sin6_addr);

    if (connect(soc, (struct sockaddr *)&dest, sizeof(dest)) != 0) {
        perror("Client: connection failed");
        return EXIT_FAILURE;
    }

    char buf[1024];
    snprintf(buf, sizeof(buf), "Client: message from IPv6 client");

    int n;
    n = write(soc, buf, strnlen(buf, sizeof(buf)));

    memset(buf, 0, sizeof(buf));
    n = read(soc, buf, sizeof(buf));

    printf("Client: n = %d, %s\n", n, buf);

    close(soc);

    return EXIT_SUCCESS;
}