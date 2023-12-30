#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>


int main() {
    int clientSocket = socket(AF_INET,SOCK_STREAM, 0);
    if (clientSocket == -1) {
        perror("Socket creation failed!\n");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.0");
    serverAddress.sin_port = htons(8088);
    
    int c = connect(clientSocket,(struct sockaddr*)&serverAddress,sizeof(serverAddress));
    if (c == -1) {
        perror("Connection failed!\n");
        close(clientSocket);
        exit(EXIT_FAILURE);
    }

    printf("Connected to server!\n");

    char message[] = "Hi, Server!";
    ssize_t sentBytes = send(clientSocket, message, sizeof(message),0);
    if (sentBytes == -1) perror("Error sending message\n");
    else printf("Message sent to server\n");

    close(clientSocket);
    return 0;
}