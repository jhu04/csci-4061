#include "client.h"

#define PORT 8080
#define BUFFER_SIZE 1024

int send_file(int socket, const char *filename) {


    // Send the file data


    return 0;
}

int receive_file(int socket, const char *filename) {
    // Open the file
    FILE *fp = fopen(filename, "w");

    // Receive response packet
    char recvdata[sizeof(packet_t)];
    memset(recvdata, 0, sizeof(packet_t));
//    int ret = recv(socket, )
    // Receive the file data

    // Write the data to the file
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: ./client File_Path_to_images File_Path_to_output_dir Rotation_angle. \n");
        return 1;
    }
    char *image_directory = argv[1];
    char *output_directory = argv[2];
    int rotation_angle = atoi(argv[3]);

    request_t queue[BUFFER_SIZE];
    int queue_size = 0;

    // Set up socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket error");
    }
    // Connect the socket
    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(PORT);

    int ret = connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
    if (ret == -1) {
        perror("connect error");
    }

    // Read the directory for all the images to rotate
    DIR *directory = opendir(image_directory);
    if (directory == NULL) {
        perror("Failed to open directory");
        exit(1);
    }
    struct dirent *entry;

    while ((entry = readdir(directory)) != NULL) {
        char *entry_name = entry->d_name;
        if (strcmp(entry_name, ".") == 0 || strcmp(entry_name, "..") == 0) {
            continue;
        }
        if (entry->d_type == DT_REG) {
            // TODO: make sure to free this (in worker after done processing)
            char *path_buf = malloc(BUFFER_SIZE * sizeof(char));
            memset(path_buf, '\0', BUFFER_SIZE * sizeof(char));
            strcpy(path_buf, image_directory);
            strcat(path_buf, "/");
            strcat(path_buf, entry->d_name);

            queue[queue_size] = (request_t) {.rotation_angle = rotation_angle, .file_name = path_buf};
            queue_size++;

            // prints queue for debugging purposes
            for (int i = 0; i < queue_size; ++i) {
                printf("queue[%d].filename = %s\n", i, queue[i].file_name);
            }
            printf("\n");
        }
    }

    if (closedir(directory) == -1) {
        perror("Failed to close directory");
        exit(-1);
    }

    char *output_path_buf = malloc(BUFFER_SIZE * sizeof(char));

    // TODO: modify queue instead of looping over
    fprintf(stdout, "Queue size %d\n", queue_size);

    for (int i = 0; i < queue_size; i++) {
        // Open the file using filepath from filename
        FILE *fp = fopen(queue[i].file_name, "r");

        // Find the file size
        // TODO : Error check
        fseek(fp, 0, SEEK_END);
        long int input_file_size = ftell(fp);

        // Setup checksum placeholder
        unsigned char checksum_placeholder[SHA256_BLOCK_SIZE];
        memset(checksum_placeholder, '\0', SHA256_BLOCK_SIZE * sizeof(char));

        // TODO: checksum_placeholder

        // Set up the request packet for the server and send it
        packet_t packet = {IMG_OP_ROTATE, rotation_angle == 180 ? IMG_FLAG_ROTATE_180 : IMG_FLAG_ROTATE_270, htonl(input_file_size), NULL};
        char *serializedData = serializePacket(&packet);

        int ret = send(sockfd, serializedData, sizeof(packet), 0);
        if (ret == -1) {
            perror("send error");
            return -1;
        }

        // Send the image data to the server
        int send_ret = send_file(sockfd, queue[i].file_name);

        //TODO : Check that the request was acknowledged
        char recvdata[sizeof(packet_t)];
        memset(recvdata, 0, sizeof(packet_t));
        ret = recv(sockfd, recvdata, sizeof(packet_t), 0); // receive data from server
        if(ret == -1) {
            perror("recv error");
            return -1;
        }

        // Deserialize the received data, check common.h and sample/client.c
        packet_t *ackpacket = deserializeData(recvdata);

        fprintf(stdout, "Received ackpacket with flag %d\n", ackpacket->flags);

        // Receive the processed image and save it in the output dir
//        memset(output_path_buf, '\0', BUFFER_SIZE * sizeof(char));
//        strcpy(output_path_buf, output_directory);
//        strcat(output_path_buf, "/");
//        strcat(output_path_buf, get_filename_from_path(queue[i].file_name));
//
//        int recv = receive_file(sockfd, output_path_buf);
    }

    packet_t packet = {IMG_OP_EXIT, rotation_angle == 180 ? IMG_FLAG_ROTATE_180 : IMG_FLAG_ROTATE_270, 0, NULL};

    char *serializedData = serializePacket(&packet);

    ret = send(sockfd, serializedData, sizeof(packet), 0);
    if (ret == -1) {
        perror("send error");
        return -1;
    }


    // Terminate the connection once all images have been processed
    close(sockfd);
    // Release any resources
//    free(path_buf);
    free(output_path_buf);
    return 0;
}
