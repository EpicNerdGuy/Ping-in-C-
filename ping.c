#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <signal.h>
#include <netinet/ip.h> 

volatile sig_atomic_t stop = 0;
int sent = 0, received = 0;

struct icmp_header {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint16_t id;   
    uint16_t seq;  
};

void handle_sigint(int sig) {
    stop = 1;
    printf("\n--- Ping statistics ---\n");
    if (sent > 0) {
        printf("%d packets transmitted, %d received, %.1f%% packet loss\n",
               sent, received, ((sent - received) * 100.0) / sent);
    } else {
        printf("No packets were sent\n");
    }
    exit(0);
}

uint16_t calc_checksum(void *buffer, int len) {
    uint16_t *data = buffer;
    unsigned long sum = 0;

    while (len > 1) {
        sum += *data++;
        len -= 2;
    }
    if (len == 1) {
        uint16_t leftover = 0;
        *((uint8_t*)&leftover) = *(uint8_t *)data;
        sum += leftover;
    }
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    return (uint16_t)(~sum);
}

int main() {
    char ip_addr[16];
    struct icmp_header header;
    struct icmp_header *icmp = &header; 
    signal(SIGINT, handle_sigint);

    printf("Enter IP address to send packets: ");
    scanf("%15s", ip_addr);

    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0) {
        perror("SOCKET NOT CREATED");
        exit(EXIT_FAILURE);
    }

    int ttl_value = 64;
    if (setsockopt(sockfd, IPPROTO_IP, IP_TTL, &ttl_value, sizeof(ttl_value)) < 0) {
        perror("Failed to set socket TTL");
        exit(EXIT_FAILURE);
    }

    uint16_t sequence_number = 0;

    while (!stop) {
        struct timeval start_time, end_time;
        double rtt_microseconds;

        // ICMP Echo Request
        icmp->type = 8;
        icmp->code = 0;
        icmp->id = getpid() & 0xFFFF; 
        icmp->seq = sequence_number++;
        icmp->checksum = 0;
        icmp->checksum = calc_checksum(icmp, sizeof(*icmp));

        gettimeofday(&start_time, NULL);

        struct sockaddr_in dest;
        memset(&dest, 0, sizeof(dest));
        dest.sin_family = AF_INET;
        inet_pton(AF_INET, ip_addr, &dest.sin_addr);

        if (sendto(sockfd, icmp, sizeof(*icmp), 0, (struct sockaddr*)&dest, sizeof(dest)) < 0) {
            perror("failed to send data");
        }
        sent++;

        char buffer[1024];
        ssize_t response = recvfrom(sockfd, buffer, sizeof(buffer), 0, NULL, NULL);

        if (response > 0) {
            struct iphdr *ip = (struct iphdr*)buffer;
            struct icmp_header *icmp_reply = (struct icmp_header*)(buffer + (ip->ihl * 4));

            if (icmp_reply->type == 0 && icmp_reply->id == (getpid() & 0xFFFF)) { // Echo reply
                received++;
                gettimeofday(&end_time, NULL);
                rtt_microseconds =
                    (double)(end_time.tv_sec - start_time.tv_sec) * 1000000.0 +
                    (double)(end_time.tv_usec - start_time.tv_usec);

                printf("Reply from %s: RTT = %.2f µs\n", ip_addr, rtt_microseconds);
            }
        }

        sleep(1);
    }

    close(sockfd);
    return 0;
}

