#include "server.h"

#define PORT 5570
#define MAX_CLIENTS 5
#define BUFFER_SIZE 1024

void *clientHandler(void *socket)
{
    int conn_fd = *(int *)socket;

    //This outer while loop iterates for every image the client sends to the be processed
    while (true)
    {
        //make initial recv call to get the first packet
        //Basically, obtain metadata about incoming image data
        char recvdata[sizeof(packet_t)];
        memset(recvdata, '\0', sizeof(packet_t));

        if (recv(conn_fd, recvdata, sizeof(packet_t), 0) == -1) // receive first packet from client
        {
            perror("recv error");
            pthread_exit(NULL);
        }

        // Determine the packet operation and flags
        packet_t *recvpacket = deserializeData(recvdata);
        int operation = recvpacket->operation;
        int flags = recvpacket->flags;
        long int size = ntohl(recvpacket->size);

        if (operation == IMG_OP_EXIT)
        {
            break;
        }

        //Nested loop is receiving packets for incoming image data. This data is combined into the img_data buffer
        char temp_filename[BUFFER_SIZE];
        memset(temp_filename, '\0', BUFFER_SIZE * sizeof(char));
        sprintf(temp_filename, "%lu.png", pthread_self());

        FILE *temp = fopen(temp_filename, "w");
        if (temp == NULL)
        {
            perror("Failed to open file");
            exit(-1);
        }

        int i = 0;
        char img_data_buf[BUFFER_SIZE];

        SHA256_CTX ctx;
        sha256_init(&ctx);

        while (i < size)
        {
            //Receiving individual chunks of the image data
            memset(img_data_buf, '\0', BUFFER_SIZE);
            int bytes_added = recv(conn_fd, img_data_buf, BUFFER_SIZE, 0);
            if (bytes_added == -1)
            {
                perror("recv error");
                pthread_exit(NULL);
            }

            if ((flags & IMG_FLAG_ENCRYPTED) != 0) {
                dec(img_data_buf, bytes_added);
            }

            //concat chunks into buffer
            int written = fwrite(img_data_buf, sizeof(char), bytes_added, temp);
            if (written != bytes_added)
            {
                perror("Failed to write all bytes to file");
                exit(-1);
            }
            sha256_update(&ctx, img_data_buf, bytes_added);

            i += bytes_added;
        }

        if (fclose(temp) != 0)
        {
            perror("Failed to close file");
            exit(-1);
        }

        // checksum
        BYTE hash[SHA256_BLOCK_SIZE];
        if ((flags & IMG_FLAG_CHECKSUM) != 0)
        {
            sha256_final(&ctx, hash);

            for (int j = 0; j < SHA256_BLOCK_SIZE; ++j)
            {
                if (hash[j] != recvpacket->checksum[j])
                {
                    fprintf(stderr, "Failed checksum\n");
                    break;
                }
            }
        }

        free(recvpacket);

        // Image processing

        /*
            Stbi_load takes:
                A file name, int pointer for width, height, and bpp
        */

        int width = 0;
        int height = 0;
        int bpp = 0;

        uint8_t *image_result = stbi_load(temp_filename, &width, &height, &bpp, CHANNEL_NUM); //stbi_load_from_memory(img_data, size, &width, &height, &bpp, CHANNEL_NUM);
        if (image_result == NULL)
        {
            packet_t NAKpkt = {.operation = operation, .flags = flags, .size = htonl(size)};
            memcpy(NAKpkt.checksum, recvpacket->checksum, SHA256_BLOCK_SIZE);

            char *serializedNAK = serializePacket(&NAKpkt);
            if (send(conn_fd, serializedNAK, sizeof(packet_t), 0) == -1)
            {
                perror("send error");
                exit(-1);
            }

            free(serializedNAK);
            pthread_exit(NULL);
        }

        uint8_t **result_matrix = (uint8_t **)malloc(sizeof(uint8_t *) * width);
        uint8_t **img_matrix = (uint8_t **)malloc(sizeof(uint8_t *) * width);
        for (int i = 0; i < width; i++)
        {
            result_matrix[i] = (uint8_t *)malloc(sizeof(uint8_t) * height);
            img_matrix[i] = (uint8_t *)malloc(sizeof(uint8_t) * height);
        }

        /*
            linear_to_image takes:
                The image_result matrix from stbi_load
                An image matrix
                Width and height that were passed into stbi_load

        */
        linear_to_image(image_result, img_matrix, width, height);

        //You should be ready to call flip_left_to_right or flip_upside_down depends on the angle(Should just be 180 or 270)
        //both take image matrix from linear_to_image, and result_matrix to store data, and width and height.
        //Hint figure out which function you will call.
        if ((flags & IMG_FLAG_ROTATE_180) != 0)
        {
            flip_left_to_right(img_matrix, result_matrix, width, height);
        }
        else
        {
            flip_upside_down(img_matrix, result_matrix, width, height);
        }

        uint8_t *img_array = malloc(sizeof(uint8_t) * width * height); ///Hint malloc using sizeof(uint8_t) * width * height
        memset(img_array, 0, width * height * sizeof(uint8_t));

        //You should be ready to call flatten_mat function, using result_matrix
        //img_arry and width and height;
        //Flattening image to 1-dimensional data structure
        flatten_mat(result_matrix, img_array, width, height);

        //New path to where you want to save the file,
        //Width
        //height
        //img_array
        //width*CHANNEL_NUM
        stbi_write_png(temp_filename,
                       width,
                       height,
                       CHANNEL_NUM,
                       img_array,
                       width * CHANNEL_NUM);

        // Acknowledge the request (send ACK packet)
        temp = fopen(temp_filename, "r");
        if (temp == NULL)
        {
            perror("Failed to open file");
            exit(-1);
        }

        // Find the file size
        if (fseek(temp, 0, SEEK_END) == -1)
        { // Error check fseek
            perror("Failed to move file offset to the end");
            exit(-1);
        }

        long int file_size = ftell(temp);
        if (file_size == -1)
        { // Error check ftell
            perror("Failed to determine position of file offset");
            exit(-1);
        }

        if (fclose(temp) != 0)
        {
            perror("Failed to close file");
            exit(-1);
        }

        temp = fopen(temp_filename, "r");
        if (temp == NULL)
        {
            perror("Failed to open file");
            exit(-1);
        }
        
        sha256_init(&ctx);

        char img_data[file_size];
        memset(img_data, '\0', file_size * sizeof(char));

        int bytes;
        while ((bytes = fread(img_data, sizeof(char), file_size, temp)) != 0)
        {
            if (bytes == -1)
            {
                perror("Failed to read file");
                exit(-1);
            }

            sha256_update(&ctx, img_data, bytes);
            memset(img_data, '\0', bytes * sizeof(char));
        }

        if (fclose(temp) != 0)
        {
            perror("Failed to close file");
            exit(-1);
        }

        sha256_final(&ctx, hash);

        packet_t packet = {.operation = IMG_OP_ACK, .flags = flags | IMG_FLAG_ENCRYPTED | IMG_FLAG_CHECKSUM, .size = htonl(file_size)};
        memcpy(packet.checksum, hash, SHA256_BLOCK_SIZE);

        char *serializedData = serializePacket(&packet);
        if (send(conn_fd, serializedData, sizeof(packet_t), 0) == -1) // send message to client
        {
            perror("send error");
            pthread_exit(NULL);
        }

        free(serializedData);

        //send image data back to client
        //so another set of nested while loop to send back as byte streams
        //use img_array as the data to send back to the client
        //Send the file data
        memset(img_data_buf, '\0', BUFFER_SIZE);

        temp = fopen(temp_filename, "r");
        if (temp == NULL)
        {
            perror("Failed to open file");
            exit(-1);
        }

        while ((bytes = fread(img_data_buf, sizeof(char), BUFFER_SIZE, temp)) != 0)
        {
            if (bytes == -1)
            {
                perror("Failed to read image file into buffer");
                exit(-1);
            }

            // encrypt
            enc(img_data_buf, bytes);

            if (send(conn_fd, img_data_buf, bytes, 0) == -1)
            {
                perror("send error");
                exit(-1);
            }
            memset(img_data_buf, '\0', bytes * sizeof(char));
        }

        if (fclose(temp) != 0)
        {
            perror("Failed to close file");
            exit(-1);
        }

        if (remove(temp_filename) == -1)
        {
            perror("Failed to remove file");
            exit(-1);
        };
    }

    if (close(conn_fd) == -1)
    {
        perror("Failed to close conn_fd");
        exit(-1);
    }
}

int main(int argc, char *argv[])
{
    // Creating socket file descriptor
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0); // create listening socket
    if (listen_fd == -1)
    {
        perror("socket error");
        exit(-1);
    }

    // Bind the socket to the port
    struct sockaddr_in servaddr;
    memset(&servaddr, '\0', sizeof(servaddr));
    servaddr.sin_family = AF_INET;                // IPv4
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // Listen to any of the network interface (INADDR_ANY)
    servaddr.sin_port = htons(PORT);              // Port number

    if (bind(listen_fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1) // bind address, port to socket
    {
        perror("bind error");
        exit(-1);
    }

    // Listen on the socket (does this run endlessly?)
    if (listen(listen_fd, MAX_CLIENTS) == -1) // listen on the listen_fd
    {
        perror("listen error");
        exit(-1);
    }

    // Accept connections and create the client handling threads
    pthread_t thread;

    int conn_fd[1024];
    int i = 0;

    while (true)
    {
        struct sockaddr_in clientaddr;
        socklen_t clientaddr_len = sizeof(clientaddr);
        conn_fd[i] = accept(listen_fd, (struct sockaddr *)&clientaddr, &clientaddr_len); // accept a request from a client
        if (conn_fd[i] == -1)
        {
            perror("accept error");
            continue;
        }

        if (pthread_create(&thread, NULL, clientHandler, &conn_fd[i]) != 0)
        {
            perror("Error creating client handler thread");
            continue;
        }

        if (pthread_detach(thread) != 0)
        {
            perror("Error detaching client handler thread");
        }

        ++i;
    }

    // Release any resources
    return 0;
}
