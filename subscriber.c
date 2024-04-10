#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "helpers.h"
#include <netinet/tcp.h>

int power10(int n) {
	int power = 1;
	for(int i = 1; i <= n; i++) {
		power *= 10;
	}
	return power;
}

void message_show(char* addr_udp, uint16_t port_udp, struct message_udp* message) {
	switch(message->type){
		case 0: {
			if(message->content[0] == 1) {
				printf("%s:%d - %s - INT - -%d\n", addr_udp, ntohs(port_udp), 
				message->name_topic, (ntohl(*((uint32_t *)&message->content[1]))));
			}
			else if(message->content[0] == 0) {
				printf("%s:%d - %s - INT - %d\n", addr_udp, ntohs(port_udp), 
				message->name_topic, (ntohl(*((uint32_t *)&message->content[1]))));
			}
			break;
		}
		case 1: {
			printf("%s:%d - %s - SHORT_REAL - %.2f\n", addr_udp, ntohs(port_udp), 
				message->name_topic, (float) (ntohs(*((uint16_t *) message->content))) / 100);
			break;
		}
		case 2: {
			uint8_t power = message->content[5];
			double nr = (double)(ntohl(*((uint32_t *)&message->content[1])));
			if(message->content[0] == 1) {
				printf("%s:%d - %s - FLOAT - -%.*f\n", addr_udp, ntohs(port_udp), 
				message->name_topic, power, (float)(nr/power10(power)));
			}
			else if(message->content[0] == 0) {
				
				printf("%s:%d - %s - FLOAT - %.*f\n", addr_udp, ntohs(port_udp), 
				message->name_topic, power, (float)(nr/power10(power)));
			}
			break;
		}
		case 3: {
			printf("%s:%d - %s - STRING - %s\n", addr_udp, ntohs(port_udp), 
				message->name_topic, message->content);
			break;
		}
		default: printf("NU e bine");
	}


}
void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_address server_port\n", file);
	exit(0);
}

int main(int argc, char *argv[])
{
	int sockfd, n, ret;
	struct sockaddr_in serv_addr;
	char buffer[BUFLEN];
	fd_set read_fds, tmp_fds;
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	if (argc < 3) {
		usage(argv[0]);
	}

	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "inet_aton");
	// dezactivare aalgoritm Nagle
	int flag = 1;
	int result = setsockopt(sockfd,            /* socket affected */
                        IPPROTO_TCP,     /* set option at TCP level */
                        TCP_NODELAY,     /* name of option */
                        (char *) &flag,  /* the cast is historical cruft */
                        sizeof(int));    /* length of option value */
	DIE(result < 0, "nagle");

	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "connect");

	//trimit catre server id ul clientului tcp
	n = send(sockfd, argv[1], strlen(argv[1]), 0);
	DIE(n < 0, "send");
	FD_SET(STDIN_FILENO, &read_fds);
	FD_SET(sockfd, &read_fds);

	while (1) {
		tmp_fds = read_fds; 
		
		ret = select(sockfd + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

		if (FD_ISSET(STDIN_FILENO, &tmp_fds)) {
			// se citeste de la tastatura
			memset(buffer, 0, BUFLEN);
			read(STDIN_FILENO, buffer, 70);
			if (strncmp(buffer, "exit", 4) == 0) {
				break;
			}

			// se trimite mesaj la server
			n = send(sockfd, buffer, strlen(buffer), 0);
			DIE(n < 0, "send");

			if(buffer[0] == 's') {
				printf("Subscribed to topic.\n");
			}
			else if(buffer[0] == 'u') {
				printf("Unsubscribed from topic.\n");
			}
			else {
				fprintf(stderr, "invalid command\n");
			}
		}

		if (FD_ISSET(sockfd, &tmp_fds)) {
			memset(buffer, 0, BUFLEN);
			n = recv(sockfd, buffer, sizeof(message_udp), 0);
			DIE(n < 0, "recv_message");
			// conexiunea s-a inchis
			if (n==0) {
				break;
			}
			struct message_udp* message = (struct message_udp*) buffer;
			struct in_addr addr_udp;
			n = recv(sockfd, &addr_udp.s_addr, sizeof(uint32_t), 0);
			DIE(n < 0, "recv_addr");
			uint16_t port_udp;
			n = recv(sockfd, &port_udp, sizeof(uint16_t), 0);
			DIE(n < 0, "recv_addr");
	
			message_show(inet_ntoa(addr_udp), ntohs(port_udp), message);
		}
	}

	close(sockfd);

	return 0;
}
