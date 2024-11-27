#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main(int argc, char const *argv[]) {
	int sock;
	struct sockaddr_in server_addr;
	char buffer[1024] = {0};

	if(argc < 4) {
		perror("usage: ./client <stop> <ip> <port>\n");
		return -1;
	}
	
	const char* stop 	= argv[1];
	const char* ip 		= "127.0.0.1";
	int port 		= atoi(argv[3]);
	
	// Create Socket	
	if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Socket create error\n");
		return -1;
	}
	
	server_addr.sin_family 	= AF_INET;
	server_addr.sin_port	= htons(port);

	// Convert IPv4 address to binary
	if(inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
		perror("Convert address error\n");
		return -1;
	}

	// Connect to server
	if(connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		perror("Connect error\n");
		return -1;
	}

	send(sock, stop, strlen(stop), 0);
	read(sock, buffer, 1024);

	close(sock);

	printf("SUCCESS\n");

	return 0;
}
