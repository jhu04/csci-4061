#include "server.h"

#define PORT 5570
#define MAX_CLIENTS 5
#define BUFFER_SIZE 1024

void *clientHandler(void *socket) {
    int conn_fd = *(int *)socket;

    while(1){
        // Receive packets from the client
        char recvdata[sizeof(packet_t)];
        memset(recvdata, 0, sizeof(packet_t));

        int ret = recv(conn_fd, recvdata, sizeof(packet_t), 0); // receive data from client
        if(ret == -1) {
            perror("recv error");
        }

        // Determine the packet operation and flags
        packet_t *recvpacket = deserializeData(recvdata);
        int operation = recvpacket->operation;
        int flags = recvpacket->flags;
        long int size = ntohl(recvpacket->size);
        fprintf(stdout, "Server received operation %d with size %ld from client\n", operation, size);

        free(recvpacket);
        if(operation == IMG_OP_EXIT) {
            break;
        }
        // Receive the image data using the size

        // Process the image data based on the set of flags

        // Acknowledge the request and return the processed image data
        packet_t packet = {IMG_OP_ACK, flags, htonl(size), NULL};
        char *serializedData = serializePacket(&packet);
        ret = send(conn_fd, serializedData, sizeof(packet_t), 0); // send message to client
        if(ret == -1) {
            perror("send error");
        }

        free(serializedData);
    }

    close(conn_fd);
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

    // Listen on the socket (does this run endlessly?)
    ret = listen(listen_fd, MAX_CLIENTS); // listen on the listen_fd
    if (ret == -1) {
        perror("listen error");
    }

    // Accept connections and create the client handling threads
    // TODO: how to handle multiple connections?
    // Array of connections?
    pthread_t thread;
    while (true) {
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
        //alternative is detach
        //pthread_join(thread, NULL);
        pthread_detach(thread);
    }

    // Release any resources
    return 0;
}
