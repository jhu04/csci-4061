#include "client.h"

#define PORT 5570
#define BUFFER_SIZE 1024

//TODO: needs error checking
int send_file(int socket, const char *filename) {
    // Send the file data
    char img_data[BUFFER_SIZE];
    bzero(img_data, BUFFER_SIZE);

    FILE *fp = fopen(filename, "r");

    int i=0;
    while(fread(img_data, sizeof(char), BUFFER_SIZE, fp) != 0){
        //send img_data over network
	fprintf(stdout, "Sending packet#%d to server\n", i);
	send(socket, img_data, BUFFER_SIZE, 0);
	i++;
    }

    return 0;
}

//TODO: needs error checking
int receive_file(int socket, const char *filename) {
    // Buffer to store processed image data
    char buffer[BUFFER_SIZE];
    bzero(buffer, BUFFER_SIZE);

    char recvdata[sizeof(packet_t)];
    memset(recvdata, '\0', sizeof(packet_t));

    unsigned int img_size;

    int ret = recv(socket, recvdata, sizeof(packet_t), 0); // receive data from server
    if(ret == -1) {
        perror("recv error");
        exit(-1);
    }

    // Deserialize the received data, check common.h and sample/client.c
    packet_t *ackpacket = deserializeData(recvdata);
    fprintf(stdout, "Received ackpacket with flag %d and image size %u\n", ackpacket->flags, ackpacket->size);
    img_size = ackpacket->size;

    ///////////////////////////////////////////////////////////////////////////
    char img_data[img_size];
    bzero(img_data, img_size);

    unsigned int bytes_counted = 0;

    while(bytes_counted < img_size){
    	int ret = recv(socket, buffer, BUFFER_SIZE, 0);
	if(ret == -1) {
	    perror("recv error on img_data packets");
	    exit(-1);
	}
        fprintf(stdout, "Received img_data packet#%d of size %d\n", ret/2, ret);

	strcat(img_data, buffer);
	bytes_counted += ret;
    }

    free(ackpacket);

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
        exit(-1);
    }

    // Connect the socket
    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(PORT);

    int ret = connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
    if (ret == -1) {
        perror("connect error");
        exit(-1);
    }

    // Read the directory for all the images to rotate
    DIR *directory = opendir(image_directory);
    if (directory == NULL) {
        perror("Failed to open directory");
        exit(-1);
    }

    struct dirent *entry;
    while ((entry = readdir(directory)) != NULL) {
        char *entry_name = entry->d_name;

        if (strcmp(entry_name, ".") == 0 || strcmp(entry_name, "..") == 0) {
            continue;
        }

        if (entry->d_type == DT_REG) {
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
        if (fp == NULL) {
            perror("Failed to open file");
            exit(-1);
        }

        // Find the file size
        if (fseek(fp, 0, SEEK_END) == -1){ // Error check fseek
            perror("Failed to move file offset to the end");
            exit(-1);
        }

        long int input_file_size = ftell(fp);
        if(input_file_size == -1){ // Error check ftell
            perror("Failed to determine position of file offset");
            exit(-1);
        }

        if (fclose(fp) != 0) {
            perror("Failed to close file");
            exit(-1);
        }

        // Setup checksum placeholder
        unsigned char checksum_placeholder[SHA256_BLOCK_SIZE];
        memset(checksum_placeholder, '\0', SHA256_BLOCK_SIZE * sizeof(char));

        // TODO: checksum_placeholder

        // Set up the request packet for the server and send it
        packet_t packet = {IMG_OP_ROTATE, rotation_angle == 180 ? IMG_FLAG_ROTATE_180 : IMG_FLAG_ROTATE_270, htonl(input_file_size), NULL};
        char *serializedData = serializePacket(&packet);

        int ret = send(sockfd, serializedData, sizeof(packet_t), 0);
        if (ret == -1) {
            perror("send error");
            exit(-1);
        }

        // Send the image data to the server
        int send_ret = send_file(sockfd, queue[i].file_name);

        //TODO : Check that the packet was acknowledged IMG_OP_ACK or IMG_OP_NAK
	//TODO: Code clean-up: Receiving the ack/nak packet already happens in receive_file
        char recvdata[sizeof(packet_t)];
        memset(recvdata, '\0', sizeof(packet_t));
        ret = recv(sockfd, recvdata, sizeof(packet_t), 0); // receive data from server
        if(ret == -1) {
            perror("recv error");
            exit(-1);
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
        int recv = receive_file(sockfd, output_path_buf);


        free(queue[i].file_name);
        free(serializedData);
    }

    packet_t packet = {IMG_OP_EXIT, rotation_angle == 180 ? IMG_FLAG_ROTATE_180 : IMG_FLAG_ROTATE_270, 0, NULL};

    char *serializedData = serializePacket(&packet);

    ret = send(sockfd, serializedData, sizeof(packet_t), 0);
    if (ret == -1) {
        perror("send error");
        exit(-1);
    }

    // Terminate the connection once all images have been processed
    close(sockfd);

    // Release any resources
    free(output_path_buf);
    free(serializedData);
    
    return 0;
}
