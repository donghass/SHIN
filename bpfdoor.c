#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <net/ethernet.h>
#include <sys/socket.h>
#include <linux/if_ether.h>
#include <linux/filter.h>

void apply_bpf_filter(int sd);
bool is_loopback_traffic(const struct iphdr *ip_pkt);
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

    if ((sd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0) {
        perror("error creating socket");
        free(buf);
        exit(1);
    }

    apply_bpf_filter(sd);
    printf("Listening for loopback UDP/53 trigger packets in log-only mode.\n");

    while (1) {
        struct iphdr *ip_pkt;
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

        if ((size_t)pkt_size < sizeof(struct ether_header) + sizeof(struct iphdr) + 8) {
            continue;
        }

        ip_pkt = (struct iphdr *)(buf + sizeof(struct ether_header));
        if (!is_loopback_traffic(ip_pkt)) {
            continue;
        }

        data_offset = sizeof(struct ether_header) + sizeof(struct iphdr) + 8;
        data = buf + data_offset;
        data_len = (size_t)pkt_size - data_offset;

        if (data_len == 0 || data[0] != 'X') {
            continue;
        }

        if (!extract_trigger_host(data, data_len, host, sizeof(host))) {
            printf("Trigger detected on loopback UDP/53, but payload parsing failed.\n");
            continue;
        }

        log_trigger_event(ip_pkt, host, data_len);
    }

    close(sd);
    free(buf);
    return 0;
}

void apply_bpf_filter(int sd) {
    // Generated from: tcpdump udp and dst port 53 -dd
    struct sock_filter filter[] = {
        { 0x28, 0, 0, 0x0000000c },
        { 0x15, 0, 4, 0x000086dd },
        { 0x30, 0, 0, 0x00000014 },
        { 0x15, 0, 11, 0x00000011 },
        { 0x28, 0, 0, 0x00000038 },
        { 0x15, 8, 9, 0x00000035 },
        { 0x15, 0, 8, 0x00000800 },
        { 0x30, 0, 0, 0x00000017 },
        { 0x15, 0, 6, 0x00000011 },
        { 0x28, 0, 0, 0x00000014 },
        { 0x45, 4, 0, 0x00001fff },
        { 0xb1, 0, 0, 0x0000000e },
        { 0x48, 0, 0, 0x00000010 },
        { 0x15, 0, 1, 0x00000035 },
        { 0x6, 0, 0, 0x00040000 },
        { 0x6, 0, 0, 0x00000000 },
    };
    size_t filter_size = sizeof(filter) / sizeof(struct sock_filter);
    struct sock_fprog bpf = {
        .len = filter_size,
        .filter = filter,
    };

    if ((setsockopt(sd, SOL_SOCKET, SO_ATTACH_FILTER, &bpf, sizeof(bpf))) < 0) {
        perror("error attaching BPF filter");
        exit(1);
    }
}

bool is_loopback_traffic(const struct iphdr *ip_pkt) {
    unsigned char src_first_octet = (unsigned char)(ntohl(ip_pkt->saddr) >> 24);
    unsigned char dst_first_octet = (unsigned char)(ntohl(ip_pkt->daddr) >> 24);
    return src_first_octet == 127 && dst_first_octet == 127;
}

bool extract_trigger_host(const char *data, size_t data_len, char *host, size_t host_size) {
    size_t i;

    if (data_len < 2 || host_size < 2 || data[0] != 'X') {
        return false;
    }

    for (i = 1; i < data_len && (i - 1) < host_size - 1; i++) {
        if (data[i] == ':' || data[i] == '\0') {
            break;
        }
        host[i - 1] = data[i];
    }

    if (i == 1) {
        return false;
    }

    host[i - 1] = '\0';
    return true;
}

void log_trigger_event(const struct iphdr *ip_pkt, const char *host, size_t payload_len) {
    struct in_addr src_addr;
    struct in_addr dst_addr;

    src_addr.s_addr = ip_pkt->saddr;
    dst_addr.s_addr = ip_pkt->daddr;

    printf("Trigger matched on loopback UDP/53.\n");
    printf("source=%s\n", inet_ntoa(src_addr));
    printf("destination=%s\n", inet_ntoa(dst_addr));
    printf("parsed_host=%s\n", host);
    printf("payload_length=%zu\n", payload_len);
    printf("action=log_only (remote shell disabled)\n");
}
