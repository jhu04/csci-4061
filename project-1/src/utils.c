#include "utils.h"

// Create N files and distribute the data from the input file evenly among them
// See section 3.1 of the project writeup for important implementation details
void partition_file_data(char *input_file, int n, char *blocks_folder) {
    // Hint: Use fseek() and ftell() to determine the size of the file
    FILE *fp = fopen(input_file, "r");
    if (fp == NULL) { // error checking
        perror("Failed to open input file.\n");
        exit(-1);
    }

    fseek(fp, 0, SEEK_END);
    long int input_file_size = ftell(fp);

    long int output_file_size = input_file_size/n;
    long int last_out_file_size = input_file_size/n + input_file_size%n;

    //rewind file position to the beginning
    fseek(fp, 0, SEEK_SET);

    for(int i=0; i<n; i++){
        //open new file & follow naming convention
        char fout_name[PATH_MAX];
        sprintf(fout_name, "%s/%d.txt", blocks_folder, i);

        //read input file
        int fout_size = 0;
        if(i<n-1){fout_size = output_file_size;}
        else{fout_size = last_out_file_size;}

        int fout_contents_size = fout_size + 1; // add 1 to account for null terminator
        char fout_contents[fout_contents_size];
        memset(fout_contents, '\0', fout_contents_size);

        char buf[fout_contents_size];
        memset(buf, '\0', fout_contents_size);
        for (size_t blocks_read = fread(buf, 1, fout_size, fp), total_blocks_read = blocks_read; blocks_read != 0; blocks_read = fread(buf, 1, fout_size - total_blocks_read, fp), total_blocks_read += blocks_read) { // error checking
            strcat(fout_contents, buf);
        }

        //write these contents into newly opened file
        FILE *foutp = fopen(fout_name, "w");
        if (foutp == NULL) { // error checking
            fclose(fp);

            perror("Failed to open output file.\n");
            exit(-1);
        }

        if (fwrite(fout_contents, 1, fout_size, foutp) < fout_size) { // error checking
            fclose(foutp);
            fclose(fp);
            
            perror("Failed to write entire block to output file.\n");
            exit(-1);
        }

        fclose(foutp);
    }

    fclose(fp);
}


// ##### DO NOT MODIFY THIS FUNCTION #####
void setup_output_directory(char *block_folder, char *hash_folder) {
    // Remove output directory
    pid_t pid = fork();
    if (pid == 0) {
        char *argv[] = {"rm", "-rf", "./output/", NULL};
        if (execvp(*argv, argv) < 0) {
            printf("ERROR: exec failed\n");
            exit(1);
        }
        exit(0);
    } else {
        wait(NULL);
    }

    sleep(1);

    // Creating output directory
    if (mkdir("output", 0777) < 0) {
        printf("ERROR: mkdir output failed\n");
        exit(1);
    }

    sleep(1);

    // Creating blocks directory
    if (mkdir(block_folder, 0777) < 0) {
        printf("ERROR: mkdir output/blocks failed\n");
        exit(1);
    }

    // Creating hashes directory
    if (mkdir(hash_folder, 0777) < 0) {
        printf("ERROR: mkdir output/hashes failed\n");
        exit(1);
    }
}
