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
#include <netdb.h>
#include <sys/time.h>
#include <signal.h>

volatile sig_atomic_t stop = 0;
int sent = 0, received = 0;

struct icmp_header{
	uint8_t type;
	uint8_t code;
	uint16_t checksum;
	uint8_t seq;
};

void handle_sigint(int sig) {
    stop = 1;
    printf("\n--- Ping statistics ---\n");
    printf("%d packets transmitted, %d received, %.1f%% packet loss\n",
           sent, received, ((sent - received) * 100.0) / sent);
    exit(0);
}

uint16_t calc_checksum(void *buffer,int len){
	uint16_t *data=buffer;
	unsigned long sum=0;
	// divide into packets and then add up
	while(len>1){
		sum+=*data++;
		len-=2;
	}
	// for leftover bytes
	if(len==1){
		uint16_t leftover=0;
		*((uint8_t)& leftover)=*(uint8_t *)data;
		sum+=leftover;
	}

	// add carry bits
	while(sum >> 16){
		sum=(sum & 0xFFFF) + (sum >> 16);
	}

	return (uint16_t)(~sum); // one's complement


}

int main(){
	char ip_addr[16];
	struct icmp_header header;
	struct icmp_header* icmp=&header; 
	signal(SIGINT,handle_sigint);
	printf("Enter IP address to send packets: ");
	scanf("%s\n",ip_addr);
	int count=0;
	for(int i=0;i<strlen(ip_addr);i++){
		if(ip_addr[i]=='.'){
			count++;
		}
	}
	if(count!=4){
		printf("IP address not correct\n");
	}

	//1. Create a raw socket
	int sockfd=socket(AF_INET,SOCK_RAW,IPPROTO_ICMP);
	if(sockfd<1){
		perror("SOCKET NOT CREATED\n");
		exit(EXIT_FAILURE);
	}

	//2. set socket opt
	int ttl_value = 69; // for chaos
	if(setsockopt(sockfd, IPPROTO_IP, IP_TTL, &ttl_value, sizeof(ttl_value))<0){
		perror("Failed to set socket TTL\n");
		exit(EXIT_FAILURE);
	}

	while(!stop){
	struct timeval start_time, end_time, received_start_time;
    double rtt_microseconds;
	//3. ICMP Echo request packet
		icmp->type=8;
		icmp->code=0;
		icmp->seq=0;
		icmp->checksum=0;

		//4. calculate checksum
		icmp->checksum=calc_checksum(icmp,sizeof(*icmp));


		gettimeofday(&start_time, NULL); 
		//5. send packet (target IP)
		struct sockaddr_in dest;
		memset(&dest,0,sizeof(dest));
		dest.sin_family=AF_INET;
		inet_pton(AF_INET,ip_addr,&dest.sin_addr);
		ssize_t bytes_sent=sendto(sockfd,icmp,sizeof(*icmp),0,(struct sockaddr*)&dest,sizeof(dest));
		if(bytes_sent<0){
			perror("failed to send data\n");
		}
		//6. recvfrom() 
		ssize_t response=recvfrom(sockfd,icmp,sizeof(*icmp),0,NULL,NULL);
		if(response<0){
			perror("failed to recieve response\n");
		}
		//7. parse the reply
		icmp->type=0;
		icmp->seq++;
		gettimeofday(&end_time, NULL);
		rtt_microseconds = (double)(end_time.tv_sec - received_start_time.tv_sec) * 1000000.0 +
	                       (double)(end_time.tv_usec - received_start_time.tv_usec);
		printf("Round Trip Time: %.2f\n",rtt_microseconds);

		//8. sleep to set delay
		sleep(1000);
	}

	close(sockfd);
	return 0;       

}