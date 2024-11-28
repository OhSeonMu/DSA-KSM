#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

int main(int argc, char const *argv[]) {
    int sock = 0, port = 9709;
    struct sockaddr_in serv_addr;
    char* buffer = (char*)std::malloc(1024);
    char* buffer_read = (char*)std::malloc(1024);
    const char* tmp = " ";

    if (argc < 3) {
	std::cerr << "usage: ./client <stop> <num_trials> <ip> [port]" << std::endl;
	return -1;
    }

    const char* stop = argv[1];
    const char* num_trials = argv[2];
    const char* ip = argv[3];

    if (argc == 5)
	    port = atoi(argv[4]);

    ip = "127.0.0.1";

    // Create socket file descriptor
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation error" << std::endl;
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(AF_INET, ip, &serv_addr.sin_addr)<=0) {
        std::cerr << "Invalid address/ Address not supported" << std::endl;
        return -1;
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection Failed" << std::endl;
        return -1;
    }

    // Send message to the server
    strcpy(buffer, stop); 
    strcat(buffer, tmp); 
    strcat(buffer, num_trials); 
    
    send(sock, buffer, strlen(buffer), 0);
    read(sock, buffer_read, 1024);
    printf("SUCCESS\n");
    printf("%s\n", buffer);
    printf("%s\n", buffer_read);
    free(buffer);
    free(buffer_read);

    return 0;
}
