#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "helpers.h"
#include <netinet/tcp.h>

#define INVALID_MESSAGE ("Format invalid pentru mesaj!")
#define FD_START 4

void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}

int find_client(int socket, struct client* clients, int nr_of_clients) {
	int i;

	for (i = 0; i < nr_of_clients; i++) {
		if(clients[i].socket == socket){
			return i;
		}
	}
	return -1;
}

int main(int argc, char *argv[])
{
	int sockfd, sockudp, newsockfd, portno;
	char buffer[BUFLEN];
	struct sockaddr_in serv_addr, cli_addr;
	int n, i, ret;
	socklen_t clilen;
	int max_clients = 100;

	// creez vectorul de clienti
	struct client* clients = malloc(100 * sizeof(struct client)); // necesita realoc pt mai multi clienti 
	int number_of_clients = 0;

	fd_set read_fds;	// multimea de citire folosita in select()
	fd_set tmp_fds;		// multime folosita temporar
	int fdmax;			// valoare maxima fd din multimea read_fds
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	if (argc < 2) {
		usage(argv[0]);
	}

	// se goleste multimea de descriptori de citire (read_fds) si multimea temporara (tmp_fds)
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	// socket-ul pentru clienti udp
	sockudp = socket(AF_INET, SOCK_DGRAM, 0);
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	portno = atoi(argv[1]);
	DIE(portno == 0, "atoi");

	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	
	// bind-ul pentru socket-ul udp
	int ret_udp = bind(sockudp, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret_udp < 0, "bind");

	// dezactivare algoritm nagle https://stackoverflow.com/questions/17842406/how-would-one-disable-nagles-algorithm-in-linux
	int flag = 1;
	int result = setsockopt(sockfd,            /* socket affected */
                        IPPROTO_TCP,     /* set option at TCP level */
                        TCP_NODELAY,     /* name of option */
                        (char *) &flag,  /* the cast is historical cruft */
                        sizeof(int));    /* length of option value */
	DIE(result < 0, "nagle");

	ret = bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind");

	ret = listen(sockfd, MAX_CLIENTS);
	DIE(ret < 0, "listen");

	// se adauga noul file descriptor (socketul pe care se asculta conexiuni) in multimea read_fds
	FD_SET(sockfd, &read_fds);
	FD_SET(sockudp, &read_fds);
	FD_SET(STDIN_FILENO, &read_fds);
	fdmax = sockfd;
	if(sockfd < sockudp) {
		fdmax= sockudp;
	}
	while (1) {
		tmp_fds = read_fds; 
		
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");
		if(FD_ISSET(STDIN_FILENO, &tmp_fds)){
			memset(buffer, 0, BUFLEN);
			fgets(buffer, BUFLEN - 1, stdin);
			// comanda exit in server
			if(strncmp(buffer, "exit", 4)==0) {
				for(int j = 0; j < number_of_clients; j++) {
					// deconectez toti clientii activi
					if(clients[j].activity == 1) {
						close(clients[j].socket);
					}
				}
				close(sockfd);
				break;
			}
			// scoatem comanda de la tastatura
			FD_CLR(STDIN_FILENO, &tmp_fds);		
		}

		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				if (i == sockfd) {
					// a venit o cerere de conexiune pe socketul inactiv (cel cu listen),
					// pe care serverul o accepta
					clilen = sizeof(cli_addr);
					newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
					DIE(newsockfd < 0, "accept");

					// primesc id-ul clientului
					memset(buffer, 0, BUFLEN);
					n = recv(newsockfd, buffer, sizeof(buffer) - 1, 0);
					DIE(n < 0, "recv");
					
					// dezactivare nagle
					int flag = 1;
					int result = setsockopt(newsockfd,            /* socket affected */
										IPPROTO_TCP,     /* set option at TCP level */
										TCP_NODELAY,     /* name of option */
										(char *) &flag,  /* the cast is historical cruft */
										sizeof(int));    /* length of option value */
					DIE(result < 0, "nagle");
					
					// verific daca id ul mai exista
					int id_unic = 1;
					int indice_id = 0;
					for(int j = 0; j < number_of_clients; j++) {
						if(strcmp(clients[j].id, buffer) == 0) {
							id_unic = 0;
							indice_id = j;
							break;
						}
					}
					if(id_unic == 1) {
						// se adauga noul socket intors de accept() la multimea descriptorilor de citire
						FD_SET(newsockfd, &read_fds);
						if (newsockfd > fdmax) { 
							fdmax = newsockfd;
						}

						printf("New client %s connected from %s:%d.\n", buffer,
								inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
						struct client new_client;
						new_client.id = malloc(11);
						strcpy(new_client.id, buffer);
						new_client.topics = NULL;
						new_client.messages = NULL;
						new_client.activity = 1;
						new_client.socket = newsockfd;
						clients[number_of_clients] = new_client;
						number_of_clients++;
						if(number_of_clients >= max_clients) {
							clients = realloc(clients, number_of_clients * 2);
							max_clients = 2 * max_clients;
						}
					}
					else{
						// clientul este conectat
						if(clients[indice_id].activity == 1){
							printf("Client %s already connected.\n", buffer);
							close(newsockfd);
						}
						// clientul este deconectat
						else{
							// se reconecteaza
							FD_SET(newsockfd, &read_fds);
							clients[indice_id].activity = 1;
							clients[indice_id].socket = newsockfd;
							printf("New client %s connected from %s:%d.\n", buffer,
								inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
							// parcurg lista de mesaje salvate in timp ce era deconectat
							// si o trimit clientului
							struct saved_message* iter = clients[indice_id].messages;
							while(iter != NULL) {
								ret = send(clients[indice_id].socket, &iter->message, sizeof(message_udp), 0);
								DIE(ret < 0, "send_msg");
								ret = send(clients[indice_id].socket, &iter->addr, sizeof(uint32_t), 0);
								DIE(ret < 0, "send_addr");
								ret = send(clients[indice_id].socket, &iter->port, sizeof(uint16_t), 0);
								DIE(ret < 0, "send_port");
								iter = iter->next;
							}
							// eliberez lista
							clients[indice_id].messages = free_list(clients[indice_id].messages);
						}
					}	
				}else if (i == sockudp) {
					//primesc mesaj de la clientul udp
					memset(buffer, 0, BUFLEN);
					// folosesc functia recvfrom pt a retine si portul si adresa clientului udp
					struct sockaddr_in addr_udp;
					memset(&addr_udp, 0, sizeof(struct sockaddr_in));
					socklen_t size = sizeof(struct sockaddr);
					n = recvfrom(i, buffer, sizeof(buffer), 0, (struct sockaddr*) &addr_udp, (socklen_t*) &size);
					DIE(n < 0, "recv");

					struct message_udp *message = (struct message_udp *) buffer;
					for(int j = 0; j < number_of_clients; j++) {
						if(find_in_list(clients[j].topics, message->name_topic) != NULL) {
							if(clients[j].activity == 1) {
								ret = send(clients[j].socket, buffer, sizeof(message_udp), 0);
								DIE(ret < 0, "send_msg");
								ret = send(clients[j].socket, &addr_udp.sin_addr.s_addr, sizeof(uint32_t), 0);
								DIE(ret < 0, "send_addr");
								ret = send(clients[j].socket, &addr_udp.sin_port, sizeof(uint16_t), 0);
								DIE(ret < 0, "send_port");
							}
							else {
								//client inactiv
								struct topic* top = find_in_list(clients[j].topics, message->name_topic);
								if (top->sf == 1)
								{
									clients[j].messages = insert_last(clients[j].messages, *message, addr_udp.sin_addr.s_addr, addr_udp.sin_port);
								}
							}
						}	
					}

				} else {
					// s-au primit date pe unul din socketii de client,
					// asa ca serverul trebuie sa le receptioneze
					memset(buffer, 0, BUFLEN);
					n = recv(i, buffer, sizeof(buffer), 0);
					DIE(n < 0, "recv");
					
					int found = find_client(i, clients, number_of_clients); 
					if (n == 0) {
						printf("Client %s disconnected.\n", clients[found].id);
						clients[found].activity = 0;
						close(i);
						FD_CLR(i, &read_fds);
						break;
						
					} else {
						// printf("S-a primit de la clientul de pe socketul %d mesajul: %s\n", i, buffer);
						char* command = strtok(buffer, " ");
						char* topic = strtok (NULL, " ");
						if(command == NULL || topic == NULL) {
							fprintf(stderr, "incomplete command\n");
							continue;
						}
						if(strcmp(command, "subscribe") == 0) {
							char* sf = strtok(NULL, " ");
							if(sf == NULL) {
								fprintf(stderr, "incomplete command\n");
								continue;
							}
							clients[found].topics = insert_first(clients[found].topics, topic, atoi(sf));	
							if (clients[found].topics == NULL)
							{
								fprintf(stderr, "error\n");
							}	
						}
						if(strcmp(command, "unsubscribe") == 0) {
							struct topic* find = delete(clients[found].topics, topic);
							if(find == NULL) {
								fprintf(stderr, "no topic found");
							}
						}
					}
				
				}
			}
		}
	}

	close(sockfd);
	close(sockudp);
	return 0;
}
