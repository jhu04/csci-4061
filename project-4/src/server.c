#include "server.h"

#define PORT 8080
#define MAX_CLIENTS 5
#define BUFFER_SIZE 1024

void *clientHandler(void *socket) {
    // Receive packets from the client


    // Determine the packet operation and flags

    // Receive the image data using the size

    // Process the image data based on the set of flags

    // Acknowledge the request and return the processed image data
}

int main(int argc, char *argv[]) {
    // Creating socket file descriptor
    int listen_fd, conn_fd;

    listen_fd = socket(AF_INET, SOCK_STREAM, 0); // create listening socket
    if (listen_fd == -1) {
        perror("socket error");
    }

    // Bind the socket to the port
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET; // IPv4
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // Listen to any of the network interface (INADDR_ANY)
    servaddr.sin_port = htons(PORT); // Port number

    int ret = bind(listen_fd, (struct sockaddr *) &servaddr, sizeof(servaddr)); // bind address, port to socket
    if (ret == -1) {
        perror("bind error");
    }

    // Listen on the socket
    ret = listen(listen_fd, MAX_CLIENTS); // listen on the listen_fd
    if (ret == -1) {
        perror("listen error");
    }

    // Accept connections and create the client handling threads
    // TODO: how to handle multiple connections?
    // Array of connections?
    pthread_t thread;
//    while (true) {
    // Can we just put connection in here?
    struct sockaddr_in clientaddr;
    socklen_t clientaddr_len = sizeof(clientaddr);
    conn_fd = accept(listen_fd, (struct sockaddr *) &clientaddr, &clientaddr_len); // accept a request from a client
    if (conn_fd == -1) {
        perror("accept error");
    }

    pthread_create(&thread, NULL, clientHandler, &conn_fd);

    // TODO: may want to replace pthread_join with something else, but do note that if the server exits, the pthreads will
    // not finish without pthread_join
    pthread_join(thread, NULL);
//    }

    // Release any resources
    return 0;
}
