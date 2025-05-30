#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <string.h>
#include <ctype.h>
#include <sys/epoll.h>
#include <sys/errno.h>
#include <arpa/inet.h>
#include "../server/sort_csv.h"

#define MAX_USER_INPUT 8 //max amount of chars to be read from stdin
#define LINE_LEN 27
#define PACK_SIZE 40 //amount of vlan_id-mac pairs in one 'sendto' call

char *help = "Launch: {IPv4} {server_port}\n"
			 "\t - Start with server IP and port;\n"
			 "\t - Enter '127.0.0.1' as IP address if server and client share same host;\n"
			 "Usage:\n"
			 "\t - Enter {N} number of VLAN_ID-MAC pairs to receive from server;\n"
			 "\t - Enter q to exit;\n";

int isValidIp(char *ip){
	struct sockaddr_in sa;
	int res = inet_pton(AF_INET, ip, &(sa.sin_addr));
	return res != 0;
}

int isValidPort(char *port) {
	int port_dec = strtol(port, NULL, 10);
	int isValidRange = (port_dec >= 1024 && port_dec <= 65535) ? 1 : 0;
    return isValidRange;
}

int isValidRequest(char *buff, uint32_t *rec_pairs_num){
	//read char by char up to MAX_USER_INPUT
	for(int i = 0; i < MAX_USER_INPUT; i++){
		buff[i] = fgetc(stdin);
		if (buff[i] == '\n'){
			//return \n to stdin to avoid need of pressing Enter two times to submit input
			ungetc('\n', stdin);
			break;
		}
	}
	//clear stdin from leftovers
	char c;
	while((c = fgetc(stdin)) != '\n' && c != EOF);
	
	//ret 0 if input is valid number greater than zero, otherwise 1 
	return (sscanf(buff, "%d", rec_pairs_num) == 1 && *(int *)rec_pairs_num > 0) ? 0 : 1;
}

int isQuitOpt(){
	char c;  
	if((c = getc(stdin)) == 'q'){
		ungetc('\n', stdin);
		return 0;
	} else {
		ungetc(c, stdin);
		return 1;
	}

}

int main(int argc, char *argv[]){

	//show help if started without options
	if(argc != 3){
		printf("%s", help);
		exit(EXIT_SUCCESS);
	}

	int serv_port = 0;
	char *serv_ip = "0";

	//validate server IP
	if(!isValidIp(argv[1])){
		printf("Invalid server IP address, expected like: 198.123.0.100 or 127.0.0.1 for localhost\n");
		exit(EXIT_FAILURE);
	}
	serv_ip = argv[1];

	//validate server port
	if(!isValidPort(argv[2])){
		printf("Invalid server port, expected integer value in range [1024:65535]\n");
		exit(EXIT_FAILURE);
	} 
	serv_port = strtol(argv[2], NULL, 10);

	int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock_fd == -1){
		perror("[ERROR] create server UDP socket:");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(serv_port);
	inet_pton(AF_INET, serv_ip, &(serv_addr.sin_addr.s_addr));
	
	uint32_t rec_pairs_num = 0;
	char buff[MAX_USER_INPUT] = {0};

	struct timeval timeout = {.tv_sec = 5, .tv_usec = 0};
	setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

	while(1){
		printf("\nEnter amount of VLAN_ID/MAC pairs to recieve: ");
		if(!isQuitOpt()){
			close(sock_fd);
			exit(EXIT_SUCCESS);
		}
		if (!isValidRequest(buff, &rec_pairs_num)) {
			socklen_t serv_len = sizeof(serv_addr);
			uint32_t net_rec_pairs_num = htonl(rec_pairs_num);
			int res = sendto(sock_fd, &net_rec_pairs_num, sizeof(&net_rec_pairs_num),
			 								0, (struct sockaddr *)&serv_addr, serv_len);
			char rec_buff[PACK_SIZE * LINE_LEN];
			for(int i = 0; i < rec_pairs_num; i++){
				int recv_ret = recvfrom(sock_fd, rec_buff, sizeof(rec_buff),
									0, (struct sockaddr *)&serv_addr, &serv_len);
				if (recv_ret == -1) {
					if (errno == EAGAIN || errno == EWOULDBLOCK) {
						printf("Server timeout\n");
						break;
					} else {
						perror("[ERROR]: recvfrom:");
						close(sock_fd);
						exit(EXIT_FAILURE);
					}
				} else {
					printf("%s", rec_buff);
					memset(rec_buff, 0, sizeof(rec_buff));			
				}
			}
		}
	}

	close(sock_fd);
	return 0;
}