#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/socket.h>
#include <bits/sigaction.h>
#include <asm-generic/signal-defs.h>
#include <bits/types/sigset_t.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <errno.h>

//Declare signal handler
volatile sig_atomic_t wasSigHup = 0;
void sigHupHandler(int r) {
    wasSigHup = 1;
}

int main() {
    //Register signal handler
    struct sigaction sa;
    sigaction(SIGHUP, NULL, &sa);
    sa.sa_handler = sigHupHandler;
    sa.sa_flags |= SA_RESTART;
    sigaction(SIGHUP, &sa, NULL);

    //Block signal handler
    sigset_t blockedMask, originalMask;
    sigemptyset(&blockedMask);
    sigemptyset(&originalMask);
    sigaddset(&blockedMask, SIGHUP);
    sigprocmask(SIG_BLOCK, &blockedMask, &originalMask);

    //Supersocket
    int superSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (superSocket == -1) {
        perror("Socket creation failed! :( ");
        exit(EXIT_FAILURE);
    }

    //Server address
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(8088);

    //Supersocket and address bind
    int b = bind(superSocket,(struct sockaddr*)&serverAddress, sizeof(serverAddress));
    if (b == -1) {
        perror("Bind failed!");
        close(superSocket);
        exit(EXIT_FAILURE);
    }

    //Supersocket starts listening
    int listening = listen(superSocket,10); // N = 10 queue size
    if (listening == -1) {
        perror("Listen failed!");
        close(superSocket);
        exit(EXIT_FAILURE);
    }
    
    printf("Server listening on port %d...\n",8088);

    char buffer[1024] = { 0 };
    fd_set fds;
    int maxfd, sock = 0, addrLen = sizeof(serverAddress);

    //Main cycle
    while (1) {
        FD_ZERO(&fds);
        FD_SET(superSocket,&fds);

        if (sock > 0) {
            FD_SET(sock, &fds);
        }
        maxfd = (sock > superSocket) ? sock : superSocket;

        
        int psel = pselect(maxfd + 1, &fds, NULL, NULL, NULL, &originalMask);
        if (psel == -1) {
            if (errno == EINTR && wasSigHup) {
                printf("Recieved SIGHUP\n");
                wasSigHup = 0;  
            }
            else {
                perror("pselect error!");
                exit(EXIT_FAILURE);
            }
        }

        if (FD_ISSET(sock, &fds) && sock > 0) {
            int readBytes = read(socket, buffer, 1024);
            if(readBytes > 0)  {
                printf("Recieved data: %d bytes\n", readBytes);
                printf("Message from client: %s\n", buffer);
            }
            else if (readBytes == 0) {
                printf("Closing connection\n");
                close(sock);
                sock = 0;
            }
            else {
                perror("read error!\n");
            }
        }
        if (FD_ISSET(sock, &fds)) {
            sock = accept(superSocket, (struct sockaddr*)&serverAddress,(socklen_t*)&addrLen);
            if (sock < 0) {
                perror("Accept error\n");
                exit(EXIT_FAILURE);
            }
            printf("New connection!\n");
        }
    }
    close(superSocket);
    return 0;
}