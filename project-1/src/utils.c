#include "utils.h"

// Create N files and distribute the data from the input file evenly among them
// See section 3.1 of the project writeup for important implementation details
void partition_file_data(char *input_file, int n, char *blocks_folder) {
    // Hint: Use fseek() and ftell() to determine the size of the file
    FILE *fp = fopen(input_file, "r");
    fseek(fp, 0, SEEK_END);
    long int input_file_size = ftell(fp);

    long int output_file_size = input_file_size/n;
    long int last_out_file_size = input_file_size/n + input_file_size%n;

    //rewind file position to the beginning
    fseek(fp, 0, SEEK_SET);

    for(int i=0; i<n; i++){
	//open new file & follow naming convention
	char fout_name[50];
	sprintf(fout_name, "%s/%d.txt", blocks_folder, i);

	//read input file
	int fout_size = 0;
	if(i<n-1){fout_size = output_file_size;}
	else{fout_size = last_out_file_size;}

	char fout_contents[fout_size];

	//needs some error checking
	fread(fout_contents, 1, fout_size, fp);

	//write these contents into newly opened file
	FILE *foutp = fopen(fout_name, "w");

	//needs some error checking
	fwrite(fout_contents, 1, fout_size, foutp);

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
