#include "image_rotation.h"

//Global integer to indicate the length of the queue??
//Global integer to indicate the number of worker threads
//Global file pointer for writing to log file in worker??
FILE *logfile;

//Might be helpful to track the ID's of your threads in a global array
pthread_t processing_thread;
pthread_t *worker_threads;

//Array to keep track of number of requests processed by each thread
int *processed_count_array;

//Requests Queue initialization + Queue Functions
request_t requests_queue;

//Global variable to check if all images have been processed
bool requests_complete = false;
bool *will_term;
bool ready_for_term = false;

//Number of requests that have been processed so far
//int num_requests_processed = 0;

//Output directory
char *output_directory = "";

//What kind of locks will you need to make everything thread safe? [Hint you need multiple]
pthread_mutex_t queue_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t num_requests_lock = PTHREAD_MUTEX_INITIALIZER;
//What kind of CVs will you need  (i.e. queue full, queue empty) [Hint you need multiple]
pthread_cond_t prod_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t cons_cond = PTHREAD_COND_INITIALIZER;

pthread_mutex_t *worker_done_lock;
pthread_cond_t *worker_done_cv;

pthread_mutex_t term_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t terminate = PTHREAD_COND_INITIALIZER;

pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;

void enqueue(request_entry_t entry) {
    int size = requests_queue.size;
    requests_queue.requests[size] = entry;//{.filename = entry.filename, .rotation_angle = entry.rotation_angle};

    printf("Added QueueEntry#%d, Name:%s, Angle:%d\n", requests_queue.size, requests_queue.requests[size].filename, requests_queue.requests[size].rotation_angle);
    fflush(stdout);

    requests_queue.size++;
}

request_entry_t dequeue() {
    int size = requests_queue.size;
    request_entry_t res = requests_queue.requests[0];

    printf("Removed size=%d, Name:%s, Angle:%d\n", requests_queue.size, requests_queue.requests[size].filename, requests_queue.requests[size].rotation_angle);
    fflush(stdout);

    for (int i = 0; i < requests_queue.size - 1; i++) {
        requests_queue.requests[i] = requests_queue.requests[i + 1];
    }

    requests_queue.size--;

    return res;
}

// TODO : to consider in depth
//How will you track the requests globally between threads? How will you ensure this is thread safe?
//How will you track which index in the request queue to remove next?
//How will you update and utilize the current number of requests in the request queue? --> 
//How will you track the p_thread's that you create for workers? -->  TODO : do we even need this? we use proc_count_arr to keep track of thread_id's via index with count so why do we need worker_threads?
//How will you know where to insert the next request received into the request queue? --> uses queue size to track current position ot be filled


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
    fprintf(to_write, "[%d][%d][%s]\n", threadId, requestNumber, file_name);
    fflush(to_write);
    fprintf(stdout, "[%d][%d][%s]\n", threadId, requestNumber, file_name);
    fflush(stdout);
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
    // TODO: processing
    // TODO: mutex locks for fprintf

    // Obtain image directory from args
    processing_args_t *p_args = (processing_args_t *) args;
    char *image_directory = p_args->image_directory;
    int num_worker_threads = p_args->num_worker_threads;
    int rotation_angle = p_args->rotation_angle;

    DIR *directory = opendir(image_directory);
    if (directory == NULL) {
        perror("Failed to open directory");
        exit(1);
    }
    struct dirent *entry;

    int num_files = 0;
    while ((entry = readdir(directory)) != NULL) {
        char *entry_name = entry->d_name;
        if (strcmp(entry_name, ".") == 0 || strcmp(entry_name, "..") == 0) {
            continue;
        }
        if (entry->d_type == DT_REG) {
            // TODO: make sure to free this (in worker after done processing)
            char *path_buf = malloc(BUFF_SIZE * sizeof(char));
            memset(path_buf, '\0', BUFF_SIZE * sizeof(char));
            //strcpy(path_buf, image_directory);
            //strcat(path_buf, "/");
            strcat(path_buf, entry->d_name);

            request_entry_t request_entry = {.filename = path_buf, .rotation_angle = rotation_angle};

            pthread_mutex_lock(&queue_lock);

            // TODO: debug
            printf("Producer added following entry to queue\n");

            while (requests_queue.size == MAX_QUEUE_LEN) {
                pthread_cond_wait(&prod_cond, &queue_lock);
            }

            enqueue(request_entry);

	    
            pthread_cond_signal(&cons_cond);
            pthread_mutex_unlock(&queue_lock);

            ++num_files;

            // prints queue for debugging purposes
//            for (int i = 0; i < requests_queue.size; ++i) {
//                printf("requests_queue.requests[%d].filename = %s\n",
//                       i,
//                       requests_queue.requests[i].filename);
//            }
//            printf("\n");
        }
    }

    if (closedir(directory) == -1) {
        perror("Failed to close directory");
        exit(-1);
    }

    //what if worker thread aren't done yet?
    //No longer putting things on the queue
    //TODO: check if broadcasting correct: (request_complete)
    printf("Requests (directory traversal) complete\n"); // TODO: debug --> we need to wait for all workers 1st, compare #processes files, then send broadcast
    requests_complete = true;
    pthread_cond_broadcast(&cons_cond);

    printf("Producer broadcasted and computing total\n"); // TODO: debug
    int total = 0;
    for (int i = 0; i < num_worker_threads; ++i) {
        // TODO: why need lock? -Just Jeffrey being confused
        printf("Thread%d is ready to terminate\n", i); // TODO: debug

	//use a while !will_terminate[i] (signalling b4 waiting)
        pthread_mutex_lock(&worker_done_lock[i]);

	while(!will_term[i]){
            pthread_cond_wait(&worker_done_cv[i], &worker_done_lock[i]);
	}

        pthread_mutex_unlock(&worker_done_lock[i]);

        total += processed_count_array[i];
    }

    if (num_files != total) {
        // TODO: what to do if verification fails?
        exit(1);
    }

    printf("Processor will send termination signal\n");
    pthread_mutex_lock(&term_lock);

    ready_for_term = true;
    pthread_cond_broadcast(&terminate);

    pthread_mutex_unlock(&term_lock);
    printf("Processor will terminate\n");
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

//TODO : consider making a worker_args_t struct instead of using a global variable for output directory and the like
void *worker(void *args) {
    // Intermediate Submission :
    // For intermediate submission, print thread ID and exit:
    // printf("Thread ID is : %d\n", *((int *)args));
    // fflush(stdout);
    // exit(0);
    int threadId = *((int *) args);
    request_entry_t cur_request;

    //int processed_count = 0;
    while (true) {
        //processed_count = processed_count+1;
        pthread_mutex_lock(&queue_lock);
        printf("Thread%d locked queue_lock\n", threadId); // TODO: debug
        while (requests_queue.size == 0) {
            if (requests_complete) {
                pthread_mutex_unlock(&queue_lock);

                // TODO: why need lock?
                pthread_mutex_lock(&worker_done_lock[threadId]);
                //processed_count_array[threadId] = processed_count;
		will_term[threadId] = true;

		//Signal to processor thread that this worker is waiting to terminate
                printf("Consumer signalled done\n"); // TODO: debug
                pthread_cond_signal(&worker_done_cv[threadId]);

		//Wait for termination signal --> bool for termination sent
		while(!ready_for_term){
                    pthread_cond_wait(&terminate, &worker_done_lock[threadId]);
		}
                pthread_mutex_unlock(&worker_done_lock[threadId]);
                printf("Consumer exiting\n"); // TODO: debug
                pthread_exit(NULL);
            }
            printf("Consumer waiting on cons_cond\n"); // TODO: debug
            pthread_cond_wait(&cons_cond, &queue_lock);
        }

        //#####################CONSUMER CODE#######################################

        //Take an element off of the queue & signal to the processing thread about a new empty slot before unlocking
        //processed_count++;
	//printf("Thread%d | Processed count = %d\n", threadId, processed_count);
	cur_request = dequeue();
        pthread_cond_signal(&prod_cond);
        printf("Thread%d unlocked queue_lock\n", threadId); // TODO: debug
        pthread_mutex_unlock(&queue_lock);

	//pthread_mutex_lock(&worker_done_lock[threadId]);
	processed_count_array[threadId]++;
	//pthread_mutex_unlock(&worker_done_lock[threadId]);

        /*
        Stbi_load takes:
            A file name, int pointer for width, height, and bpp
        */

        //TODO : The buf size is arbitrary
        int width = 0;
        int height = 0;
        int bpp = 0;
        uint8_t *image_result =
            stbi_load(cur_request.filename, &width, &height, &bpp, CHANNEL_NUM);
        uint8_t
            **result_matrix = (uint8_t **) malloc(sizeof(uint8_t *) * width);
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


        ////TODO: you should be ready to call flip_left_to_right or flip_upside_down depends on the angle(Should just be 180 or 270)
        //both take image matrix from linear_to_image, and result_matrix to store data, and width and height.
        //Hint figure out which function you will call.
        if (cur_request.rotation_angle == 180) {
            flip_left_to_right(img_matrix, result_matrix, width, height);
        } else {
            flip_upside_down(img_matrix, result_matrix, width, height);
        }

        uint8_t *img_array = malloc(sizeof(uint8_t) * width
                                        * height); ///Hint malloc using sizeof(uint8_t) * width * height


        ///TODO: you should be ready to call flatten_mat function, using result_matrix
        //img_arry and width and height;
        //Flattening image to 1-dimensional data structure
        flatten_mat(result_matrix, img_array, width, height);


        ///TODO: You should be ready to call stbi_write_png using:
        //New path to where you wanna save the file,
        //TODO : make 2048 a constant
        char *path_buf = malloc(BUFF_SIZE * sizeof(char));
        memset(path_buf, '\0', BUFF_SIZE * sizeof(char));
        strcpy(path_buf, output_directory);
        strcat(path_buf, "/");
        strcat(path_buf, cur_request.filename);
        //Width
        //height
        //img_array
        //width*CHANNEL_NUM
        stbi_write_png(path_buf,
                       width,
                       height,
                       CHANNEL_NUM,
                       img_array,
                       width * CHANNEL_NUM);

        //Update the processed_count_array
        //Don't need to lock this since individual threads access their own parts of the array
        //processed_count++;
        // TODO: delete
        //Update number of processed images
        //num_requests is a shared global variable: needs to be locked
//        pthread_mutex_lock(&num_requests_lock);
//        num_requests_processed += 1;
//        pthread_mutex_unlock(&num_requests_lock);

        //TODO : is locking even needed? (protect logFile from raise conditions)
        // TODO: lock logfile

        pthread_mutex_lock(&log_lock);
        log_pretty_print(logfile,
                         threadId,
                         processed_count_array[threadId],
                         path_buf);
	pthread_mutex_unlock(&log_lock);
    }
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

    requests_queue.requests = malloc(MAX_QUEUE_LEN * (sizeof(request_entry_t)));
    requests_queue.size = 0;

    char *image_directory = argv[1];
    output_directory = argv[2];
    int num_worker_threads = atoi(argv[3]);
    int rotation_angle = atoi(argv[4]);

    //intialise the array that keeps track of number of requests processed by each worker thread
    processed_count_array = (int *)malloc(sizeof(int) * num_worker_threads);
    memset(processed_count_array, 0, sizeof(int) * num_worker_threads);

    worker_done_lock = (pthread_mutex_t *)malloc(num_worker_threads * sizeof(pthread_mutex_t));
    for (int i = 0; i < num_worker_threads; ++i) {
        // TODO: error check
        pthread_mutex_init(&worker_done_lock[i], NULL);
    }

    worker_done_cv = (pthread_cond_t *)malloc(num_worker_threads * sizeof(pthread_cond_t));
    for (int i = 0; i < num_worker_threads; ++i) {
        // TODO: error check
        pthread_cond_init(&worker_done_cv[i], NULL);
    }

    will_term = (bool *)malloc(num_worker_threads * sizeof(bool));
    memset(will_term, false, num_worker_threads * sizeof(bool));

    logfile = fopen(LOG_FILE_NAME, "w");

    processing_args_t processing_args;
    processing_args.image_directory = image_directory;
    processing_args.num_worker_threads = num_worker_threads;
    processing_args.rotation_angle = rotation_angle;

    if (pthread_create(&processing_thread,
                       NULL,
                       (void *) processing,
                       (void *) &processing_args) != 0) {
        fprintf(stderr, "Error creating processing thread\n");
    }

    worker_threads = malloc(num_worker_threads * sizeof(pthread_t));
    int args[num_worker_threads];

    for (int i = 0; i < num_worker_threads; ++i) {
        args[i] = i;
        if (pthread_create(&worker_threads[i],
                           NULL,
                           (void *) worker,
                           (void *) &args[i]) != 0) {
            fprintf(stderr, "Error creating worker thread #%d\n", i);
        }
    }

    // TODO: pthread_join
    for (int j = 0; j < num_worker_threads; j++) {
        pthread_join(worker_threads[j], NULL);
    }
    pthread_join(processing_thread, NULL);
    fclose(logfile);
    free(worker_threads);
    free(requests_queue.requests);
}
