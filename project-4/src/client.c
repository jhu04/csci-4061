#include "client.h"

#define PORT 5570
#define BUFFER_SIZE 1024

int send_file(int socket, const char *filename)
{
    // Send the file data
    char img_data[BUFFER_SIZE];
    memset(img_data, '\0', BUFFER_SIZE * sizeof(char));

    FILE *fp = fopen(filename, "r");
    if (fp == NULL)
    {
        perror("Failed to open file");
        exit(-1);
    }

    int bytes;
    while ((bytes = fread(img_data, sizeof(char), BUFFER_SIZE, fp)) != 0)
    {
        if (bytes == -1)
        {
            perror("Failed to read image file into buffer");
            exit(-1);
        }

        // encrypt
        enc(img_data, bytes);

        int sent = send(socket, img_data, bytes, 0);
        if (sent == -1 || sent != bytes)
        {
            perror("Failed to send bytes");
            exit(-1);
        }
    }

    return 0;
}

int receive_file(int socket, const char *filename)
{
    // Buffer to store processed image data
    char img_data_buf[BUFFER_SIZE];
    memset(img_data_buf, '\0', BUFFER_SIZE * sizeof(char));

    char recvdata[sizeof(packet_t)];
    memset(recvdata, '\0', sizeof(packet_t));

    if (recv(socket, recvdata, sizeof(packet_t), 0) == -1) // receive data from server
    {
        perror("recv error");
        exit(-1);
    }

    // Deserialize the received data, check common.h and sample/client.c
    packet_t *ackpacket = deserializeData(recvdata);
    int operation = ackpacket->operation;
    int flags = ackpacket->flags;
    long int size = ntohl(ackpacket->size);

    //Received NAK Packet, no further action taken
    if (operation == IMG_OP_NAK)
    {
        perror("Received a NAK packet");
        free(ackpacket);
        return -1;
    }

    FILE *fp = fopen(filename, "w");
    if (fp == NULL)
    {
        perror("Failed to open file");
        exit(-1);
    }

    SHA256_CTX ctx;
    sha256_init(&ctx);

    for (int i = 0; i < size; )
    {
        memset(img_data_buf, '\0', BUFFER_SIZE);

        int bytes_added = recv(socket, img_data_buf, BUFFER_SIZE, 0);
        if (bytes_added == -1)
        {
            perror("recv error on img_data packets");
            exit(-1);
        }

        if ((flags & IMG_FLAG_ENCRYPTED) != 0) {
            dec(img_data_buf, bytes_added);
        }

        int bytes_written = fwrite(img_data_buf, sizeof(char), bytes_added, fp);
        if (bytes_written != bytes_added)
        {
            perror("Failed to write all bytes to file");
            exit(-1);
        }

        sha256_update(&ctx, img_data_buf, bytes_added);

        i += bytes_added;
    }

    if (fclose(fp) != 0)
    {
        perror("Failed to close file");
        exit(-1);
    }

    // checksum
    if ((flags & IMG_FLAG_CHECKSUM) != 0)
    {
        BYTE hash[SHA256_BLOCK_SIZE];
        sha256_final(&ctx, hash);

        for (int j = 0; j < SHA256_BLOCK_SIZE; ++j)
        {
            if (hash[j] != ackpacket->checksum[j])
            {
                fprintf(stderr, "Failed checksum\n");
                break;
            }
        }
    }

    free(ackpacket);

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
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
    if (sockfd == -1)
    {
        perror("socket error");
        exit(-1);
    }

    // Connect the socket
    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(PORT);

    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)
    {
        perror("connect error");
        exit(-1);
    }

    // Read the directory for all the images to rotate
    DIR *directory = opendir(image_directory);
    if (directory == NULL)
    {
        perror("Failed to open directory");
        exit(-1);
    }

    //Directory traversal to add image files to queue
    struct dirent *entry;
    while ((entry = readdir(directory)) != NULL)
    {
        char *entry_name = entry->d_name;

        if (strcmp(entry_name, ".") == 0 || strcmp(entry_name, "..") == 0)
        {
            continue;
        }

        if (entry->d_type == DT_REG)
        {
            char *path_buf = malloc(BUFFER_SIZE * sizeof(char));
            memset(path_buf, '\0', BUFFER_SIZE * sizeof(char));
            strcpy(path_buf, image_directory);
            strcat(path_buf, "/");
            strcat(path_buf, entry->d_name);

            queue[queue_size] = (request_t){.rotation_angle = rotation_angle, .file_name = path_buf};
            queue_size++;
        }
    }

    if (closedir(directory) == -1)
    {
        perror("Failed to close directory");
        exit(-1);
    }

    char output_path_buf[BUFFER_SIZE];

    //Send each image file from the queue to server for processing, and then receive results --> put into output directory
    for (int i = 0; i < queue_size; i++)
    {
        // Open the file using filepath from filename
        FILE *fp = fopen(queue[i].file_name, "r");
        if (fp == NULL)
        {
            perror("Failed to open file");
            exit(-1);
        }

        // Find the file size
        if (fseek(fp, 0, SEEK_END) == -1)
        { // Error check fseek
            perror("Failed to move file offset to the end");
            exit(-1);
        }

        long int input_file_size = ftell(fp);
        if (input_file_size == -1)
        { // Error check ftell
            perror("Failed to determine position of file offset");
            exit(-1);
        }

        if (fclose(fp) != 0)
        {
            perror("Failed to close file");
            exit(-1);
        }

        fp = fopen(queue[i].file_name, "r");
        if (fp == NULL)
        {
            perror("Failed to open file");
            exit(-1);
        }

        SHA256_CTX ctx;
        sha256_init(&ctx);

        char img_data[input_file_size];
        memset(img_data, '\0', input_file_size * sizeof(char));

        int bytes; // TODO: error check fread
        while ((bytes = fread(img_data, sizeof(char), input_file_size, fp)) != 0)
        {
            if (bytes == -1)
            {
                perror("Failed to read file");
                exit(-1);
            }

            sha256_update(&ctx, img_data, bytes);
            memset(img_data, '\0', bytes * sizeof(char));
        }

        if (fclose(fp) != 0)
        {
            perror("Failed to close file");
            exit(-1);
        }

        BYTE hash[SHA256_BLOCK_SIZE];
        sha256_final(&ctx, hash);

        // Set up the request packet for the server and send it
        packet_t packet = {.operation = IMG_OP_ROTATE, .flags = (rotation_angle == 180 ? IMG_FLAG_ROTATE_180 : IMG_FLAG_ROTATE_270) | IMG_FLAG_ENCRYPTED | IMG_FLAG_CHECKSUM, .size = htonl(input_file_size)};
        memcpy(packet.checksum, hash, SHA256_BLOCK_SIZE);

        char *serializedData = serializePacket(&packet);

        if (send(sockfd, serializedData, sizeof(packet_t), 0) == -1)
        {
            perror("send error");
            exit(-1);
        }

        // Send the image data to the server
        if (send_file(sockfd, queue[i].file_name) != 0)
        {
            perror("Failed call to send_file");
            exit(-1);
        }

        // Receive the processed image and save it in the output dir
        memset(output_path_buf, '\0', BUFFER_SIZE * sizeof(char));
        strcpy(output_path_buf, output_directory);
        strcat(output_path_buf, "/");
        strcat(output_path_buf, get_filename_from_path(queue[i].file_name));

        if (receive_file(sockfd, output_path_buf) != 0)
        {
            perror("Failed call to receive_file");
            exit(-1);
        }

        free(queue[i].file_name);
        free(serializedData);
    }

    packet_t packet = {.operation = IMG_OP_EXIT, .flags = 0, .size = 0};

    char *serializedData = serializePacket(&packet);
    if (send(sockfd, serializedData, sizeof(packet_t), 0) == -1)
    {
        perror("send error");
        exit(-1);
    }

    // Terminate the connection once all images have been processed
    if (close(sockfd) == -1)
    {
        perror("Failed to close the socket");
        exit(-1);
    }

    // Release any resources
    free(serializedData);

    return 0;
}
