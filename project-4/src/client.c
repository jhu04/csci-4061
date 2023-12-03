#include "client.h"

#define PORT 8080
#define BUFFER_SIZE 1024 

int send_file(int socket, const char *filename) {
    // Open the file

    // Set up the request packet for the server and send it

    // Send the file data
    return 0;
}

int receive_file(int socket, const char *filename) {
    // Open the file

    // Receive response packet

    // Receive the file data

    // Write the data to the file
    return 0;
}

int main(int argc, char* argv[]) {
    if(argc != 4){
        fprintf(stderr, "Usage: ./client File_Path_to_images File_Path_to_output_dir Rotation_angle. \n");
        return 1;
    }
    char *image_directory = argv[1];
    char *output_directory = argv[2];
    int rotation_angle = atoi(argv[3]);

    request_t queue[BUFFER_SIZE];
    int queue_size = 0;

    // Set up socket

    // Connect the socket

    // Read the directory for all the images to rotate
    DIR *directory = opendir(image_directory);
    if(directory == NULL){
        perror("Failed to open directory");
        exit(1);
    }
    struct dirent *entry;

    while((entry = readdir(directory)) != NULL){
        char* entry_name = entry->d_name;
        if(strcmp(entry_name, ".") == 0 || strcmp(entry_name, "..") == 0){
            continue;
        }
        //if(entry->d_type == DT_REG){
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
        //}

    }
    if(closedir(directory) == -1){
        perror("Failed to close directory");
        exit(-1);
    }


    // Send the image data to the server

    // Check that the request was acknowledged

    // Receive the processed image and save it in the output dir

    // Terminate the connection once all images have been processed

    // Release any resources
    return 0;
}
