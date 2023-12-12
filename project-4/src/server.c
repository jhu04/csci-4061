#include "server.h"

#define PORT 5570
#define MAX_CLIENTS 5
#define BUFFER_SIZE 1024

void *clientHandler(void *socket) {
    int conn_fd = *(int *)socket;

    //This outer while loop iterates for every image the client sends to the be processed
    while(1){
        //make initial recv call to get the first packet
        //Basically, obtain metadata about incoming image data
        char recvdata[sizeof(packet_t)];
        memset(recvdata, 0, sizeof(packet_t));

        int ret = recv(conn_fd, recvdata, sizeof(packet_t), 0); // receive first packet from client

        if (ret == -1) {
            perror("recv error");
            pthread_exit(NULL);
        }

        // Determine the packet operation and flags
        packet_t *recvpacket = deserializeData(recvdata);
        int operation = recvpacket->operation;
        int flags = recvpacket->flags;
        long int size = ntohl(recvpacket->size);
        //char img_data[size]; //TODO: Delete

        //memset(img_data, '\0', size*sizeof(char)); //TODO: Delete
        free(recvpacket);

        if (operation == IMG_OP_EXIT) {
            fprintf(stdout, "Server received IMG_OP_EXIT packet\n");
            break;
        }
        fprintf(stdout, "Server received IMG_OP_ROTATE packet w/ operation %d with size %ld from client\n", operation, size);

        //Nested loop is receiving packets for incoming image data. This data is combined into the img_data buffer
        char temp_filename[BUFFER_SIZE];
        memset(temp_filename, '\0', BUFFER_SIZE * sizeof(char));
        sprintf(temp_filename, "SERVER_%lu.png", pthread_self());

        FILE *temp = fopen(temp_filename, "a");

        int i=0;
        char img_data_buf[BUFFER_SIZE];

        while (i<size) {
            //Receiving individual chunks of the image data
            fprintf(stdout, "Received packet#%d of image data\n", i);
            memset(img_data_buf, '\0', BUFFER_SIZE);
            int bytes_added = recv(conn_fd, img_data_buf, BUFFER_SIZE, 0);    
            
            if(ret == -1){
                perror("recv error");
                pthread_exit(NULL);
            }

            //concat chunks into buffer
	    fwrite(img_data_buf, sizeof(char), bytes_added, temp);
            //strcat(img_data, img_data_buf); //TODO: Delete
	    i+=bytes_added;
        }

	fprintf(stdout, "Received all image data packets\n");

	fclose(temp);
        //do img_processing
        
        /*
            TODO:
            Need to send IMG_OP_NAK packet if smth wrong occurs in any of these functions below:
                stbi_load_from_memory
                linear_to_image
                flip_left_to_right
                flip_upside_down
                flatten_mat
        */


        /*
            Stbi_load takes:
                A file name, int pointer for width, height, and bpp
        */
        
        int width = 0;
        int height = 0;
        int bpp = 0;

        //TODO: fill image_result w/ image buffer??
        uint8_t *image_result = stbi_load(temp_filename, &width, &height, &bpp, CHANNEL_NUM); //stbi_load_from_memory(img_data, size, &width, &height, &bpp, CHANNEL_NUM);
        uint8_t **result_matrix = (uint8_t **) malloc(sizeof(uint8_t *) * width);
        uint8_t **img_matrix = (uint8_t **) malloc(sizeof(uint8_t *) * width);
        for (int i = 0; i < width; i++) {
            result_matrix[i] = (uint8_t *) malloc(sizeof(uint8_t) * height);
            img_matrix[i] = (uint8_t *) malloc(sizeof(uint8_t) * height);
        }


        /*
            linear_to_image takes:
                The image_result matrix from stbi_load
                An image matrix
                Width and height that were passed into stbi_load

        */
        linear_to_image(image_result, img_matrix, width, height);

        fprintf(stdout, "=======================================IMAGE MATRIX=========================================\n");

	for(int i=0; i<width; i++){
	    for(int j=0; j<height; j++){
		fprintf(stdout, "%d ", img_matrix[i][j]);
	    }
	    fprintf(stdout, "\n");
	}

        //You should be ready to call flip_left_to_right or flip_upside_down depends on the angle(Should just be 180 or 270)
        //both take image matrix from linear_to_image, and result_matrix to store data, and width and height.
        //Hint figure out which function you will call.
        if (flags == IMG_FLAG_ROTATE_180) {
            flip_left_to_right(img_matrix, result_matrix, width, height);
        } else {
            flip_upside_down(img_matrix, result_matrix, width, height);
        }

	fprintf(stdout, "=======================================RESULT MATRIX=========================================\n");

        for(int i=0; i<width; i++){
            for(int j=0; j<height; j++){
                fprintf(stdout, "%d ", result_matrix[i][j]);
            }
            fprintf(stdout, "\n");
        }


        uint8_t *img_array = malloc(sizeof(uint8_t) * width * height); ///Hint malloc using sizeof(uint8_t) * width * height
        memset(img_array, 0, width * height * sizeof(uint8_t));


        //You should be ready to call flatten_mat function, using result_matrix
        //img_arry and width and height;
        //Flattening image to 1-dimensional data structure
        flatten_mat(result_matrix, img_array, width, height);


        fprintf(stdout, "=======================================IMAGE ARRAY=========================================\n");

        for(int i=0; i<sizeof(uint8_t) * width * height; i++){
            fprintf(stdout, "%d ", img_array[i]);
        }
	fprintf(stdout, "\n");

        // Acknowledge the request (send ACK packet)
        // fflush(stdout);
        
        fprintf(stdout, "Sending Ack Packet\n");
        packet_t packet = {IMG_OP_ACK, flags, htonl(sizeof(uint8_t) * width * height), NULL}; //TODO: need a case when image processing didn't work, send IMG_OP_NAK pkt
        char *serializedData = serializePacket(&packet);
        ret = send(conn_fd, serializedData, sizeof(packet_t), 0); // send message to client
        if (ret == -1) {
	    fprintf(stdout, "Ack packet sending failed...\n");
            perror("send error");
            pthread_exit(NULL);
        }

        free(serializedData);

        //send image data back to client
        //so another set of nested while loop to send back as byte streams
        //use img_array as the data to send back to the client
        //Send the file data
        //char img_data[BUFFER_SIZE];
        //bzero(img_data, BUFFER_SIZE);
    
	//fopen(temp, "r");
        fprintf(stdout, "Image Array Material\n");
	fprintf(stdout, "%s\n", img_array);

        fprintf(stderr, "w : %d, h : %d\n", width, height);
        for(int i=0; i<width * height * sizeof(uint8_t); i+=BUFFER_SIZE){
            fprintf(stdout, "Send packet#%d to client\n", i);

	    //memset(img_data_buf, '\0', BUFFER_SIZE);
	    //int bytes_read = fread(img_data_buf, sizeof(char), BUFFER_SIZE, temp);

            int ret = send(conn_fd, img_array+i, BUFFER_SIZE, 0);
            if(ret == -1){
                perror("send error");
                pthread_exit(NULL);
            }
        }

    }


    close(conn_fd);
}

int main(int argc, char *argv[]) {
    // Creating socket file descriptor
    int listen_fd, conn_fd;

    listen_fd = socket(AF_INET, SOCK_STREAM, 0); // create listening socket
    if (listen_fd == -1) {
        perror("socket error");
        exit(-1);
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
        exit(-1);
    }

    // Listen on the socket (does this run endlessly?)
    ret = listen(listen_fd, MAX_CLIENTS); // listen on the listen_fd
    if (ret == -1) {
        perror("listen error");
        exit(-1);
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

        if (pthread_create(&thread, NULL, clientHandler, &conn_fd) != 0) {
            perror("Error creating client handler thread");
            continue;
        }

        if (pthread_detach(thread) != 0) {
            perror("Error detaching client handler thread");
        }
    }

    // Release any resources
    return 0;
}
