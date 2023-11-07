#include "image_rotation.h"

//Global integer to indicate the length of the queue??
//Global integer to indicate the number of worker threads
//Global file pointer for writing to log file in worker??
FILE *logfile;

//Might be helpful to track the ID's of your threads in a global array
pthread_t processing_thread;
pthread_t *worker_threads;


//Requests Queue initialization + Queue Functions
request_t requests_queue;

void enqueue(request_entry_t entry){
    int size = requests_queue.size;
    requests_queue.requests[size] = entry;//{.filename = entry.filename, .rotation_angle = entry.rotation_angle};
    requests_queue.size++;
}

void dequeue(){
    int size = requests_queue.size;

    for(int i=0; i<size-1; i++){
        requests_queue.requests[i] = requests_queue.requests[i+1];
    }

    requests_queue.size--;
}

//What kind of locks will you need to make everything thread safe? [Hint you need multiple]
//What kind of CVs will you need  (i.e. queue full, queue empty) [Hint you need multiple]
//How will you track the requests globally between threads? How will you ensure this is thread safe?
//How will you track which index in the request queue to remove next?
//How will you update and utilize the current number of requests in the request queue?
//How will you track the p_thread's that you create for workers?
//How will you know where to insert the next request received into the request queue?


/*
    The Function takes:
    to_write: A file pointer of where to write the logs. 
    requestNumber: the request number that the thread just finished.
    file_name: the name of the file that just got processed. 

    The function output: 
    it should output the threadId, requestNumber, file_name into the logfile and stdout.
*/
void log_pretty_print(FILE *to_write,
                      int threadId,
                      int requestNumber,
                      char *file_name) {

}

/*

    1: The processing function takes a void* argument called args. It is expected to be a pointer to a structure processing_args_t 
    that contains information necessary for processing.

    2: The processing thread need to traverse a given dictionary and add its files into the shared queue while maintaining synchronization using lock and unlock. 

    3: The processing thread should pthread_cond_signal/broadcast once it finish the traversing to wake the worker up from their wait.

    4: The processing thread will block(pthread_cond_wait) for a condition variable until the workers are done with the processing of the requests and the queue is empty.

    5: The processing thread will cross check if the condition from step 4 is met and it will signal to the worker to exit and it will exit.

*/

void *processing(void *args) {
    // Obtain image directory from args
    processing_args_t *p_args = (processing_args_t *)args;
    char *image_directory = p_args->image_directory;
    int num_worker_threads = p_args->num_worker_threads;
    int rotation_angle = p_args->rotation_angle;

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
        if(entry->d_type == DT_REG){
            // TODO: make sure to free this
            char *path_buf = malloc(BUFF_SIZE * sizeof(char));
            memset(path_buf, '\0', BUFF_SIZE * sizeof(char));
            strcpy(path_buf, image_directory);
            strcat(path_buf, "/");
            strcat(path_buf, entry->d_name);

            request_entry_t request_entry = {.filename = path_buf, .rotation_angle = rotation_angle};
            enqueue(request_entry);

            // prints queue for debugging purposes
            for (int i = 0; i < requests_queue.size; ++i) {
                printf("requests_queue.requests[%d].filename = %s\n", i, requests_queue.requests[i].filename);
            }
            printf("\n");
        }

    }
    if(closedir(directory) == -1){
        perror("Failed to close directory");
        exit(-1);
    }
}

/*
    1: The worker threads takes an int ID as a parameter

    2: The Worker thread will block(pthread_cond_wait) for a condition variable that there is a requests in the queue. 

    3: The Worker threads will also block(pthread_cond_wait) once the queue is empty and wait for a signal to either exit or do work.

    4: The Worker thread will processes request from the queue while maintaining synchronization using lock and unlock. 

    5: The worker thread will write the data back to the given output dir as passed in main. 

    6:  The Worker thread will log the request from the queue while maintaining synchronization using lock and unlock.  

    8: Hint the worker thread should be in a While(1) loop since a worker thread can process multiple requests and It will have two while loops in total
        that is just a recommendation feel free to implement it your way :)
    9: You may need different lock depending on the job.  

*/


void *worker(void *args) {


    /*
        Stbi_load takes:
            A file name, int pointer for width, height, and bpp

    */
    // For intermediate submission, print thread ID and exit:
    printf("Thread ID is : %d\n", *((int *)args));
    fflush(stdout);
    // exit(0);
    // uint8_t* image_result = stbi_load("??????","?????", "?????", "???????",  CHANNEL_NUM);

//Need to uncomment after intermediate
/*
    uint8_t **result_matrix = (uint8_t **) malloc(sizeof(uint8_t *) * width);
    uint8_t **img_matrix = (uint8_t **) malloc(sizeof(uint8_t *) * width);
    for (int i = 0; i < width; i++) {
        result_matrix[i] = (uint8_t *) malloc(sizeof(uint8_t) * height);
        img_matrix[i] = (uint8_t *) malloc(sizeof(uint8_t) * height);
    }
*/

    /*
    linear_to_image takes:
        The image_result matrix from stbi_load
        An image matrix
        Width and height that were passed into stbi_load

    */
    //linear_to_image("??????", "????", "????", "????");


    ////TODO: you should be ready to call flip_left_to_right or flip_upside_down depends on the angle(Should just be 180 or 270)
    //both take image matrix from linear_to_image, and result_matrix to store data, and width and height.
    //Hint figure out which function you will call.
    //flip_left_to_right(img_matrix, result_matrix, width, height); or flip_upside_down(img_matrix, result_matrix ,width, height);




    //uint8_t* img_array = NULL; ///Hint malloc using sizeof(uint8_t) * width * height


    ///TODO: you should be ready to call flatten_mat function, using result_matrix
    //img_arry and width and height;
    //flatten_mat("??????", "??????", "????", "???????");


    ///TODO: You should be ready to call stbi_write_png using:
    //New path to where you wanna save the file,
    //Width
    //height
    //img_array
    //width*CHANNEL_NUM
    // stbi_write_png("??????", "?????", "??????", CHANNEL_NUM, "??????", "?????"*CHANNEL_NUM);


}

/*
    Main:
        Get the data you need from the command line argument 
        Open the logfile
        Create the threads needed
        Join on the created threads
        Clean any data if needed. 


*/

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr,
                "Usage: File Path to image dirctory, File path to output dirctory, number of worker thread, and Rotation angle\n");
    }

    // TODO:
    requests_queue.requests = malloc(MAX_QUEUE_LEN*(sizeof(request_entry_t)));
    requests_queue.size = 0;

    char *image_directory = argv[1];
    char *output_directory = argv[2];
    int num_worker_threads = atoi(argv[3]);
    int rotation_angle = atoi(argv[4]);

    logfile = fopen(LOG_FILE_NAME, "w");

    processing_args_t processing_args;
    processing_args.image_directory = image_directory;
    processing_args.num_worker_threads = num_worker_threads;
    processing_args.rotation_angle = rotation_angle;

    if( pthread_create(&processing_thread,
                   NULL,
                   (void *) processing,
                   (void *) &processing_args)    != 0){
        fprintf(stderr, "Error creating processing thread\n");
    }

    worker_threads = malloc(num_worker_threads * sizeof(pthread_t));
    int args[num_worker_threads];

    for (int i = 0; i < num_worker_threads; ++i) {
        args[i] = i;
        if( pthread_create(&worker_threads[i],
                       NULL,
                       (void *) worker,
                       (void *) &args[i])    != 0){
            fprintf(stderr, "Error creating worker thread #%d\n", i);
        }
    }

    // TODO: pthread_join
    for(int j=0; j<num_worker_threads; j++){
        pthread_join(worker_threads[j], NULL);
    }
    pthread_join(processing_thread, NULL);
    fclose(logfile);
    free(worker_threads);
    free(requests_queue.requests);
}
