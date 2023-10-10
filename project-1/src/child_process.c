#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include "hash.h"

#define PATH_MAX 1024
#define ERR_MESS 2048
#define N_LEN_MAX 5

int main(int argc, char* argv[]) {
    if (argc != 5) {
        printf("Usage: ./child_process <blocks_folder> <hashes_folder> <N> <child_id>\n");
        return 1;
    }

    char* blocks_folder = argv[1];
    char* hashes_folder = argv[2];
    char *N_string = argv[3];
    int N = atoi(N_string); // number of blocks
    int cur_id = atoi(argv[4]);
    
    // TODO: If the current process is a leaf process, read in the associated block file 
    // and compute the hash of the block.

    if(cur_id >= N-1 && cur_id <= 2*N-2){
        // Open file to read leaf data from
        char block_name[PATH_MAX];
        sprintf(block_name, "%s/%d.txt", blocks_folder, cur_id-N+1);

        // Compute hash and store it in a buffer
        char hash_buf[2*SHA256_BLOCK_SIZE+1];
        hash_data_block(hash_buf, block_name);

        // Open file to write hash into
        char hash_name[PATH_MAX];
        sprintf(hash_name, "%s/%d.out", hashes_folder, cur_id);

        FILE* fp = fopen(hash_name, "w");
        if(fp == NULL){
            char err_message[ERR_MESS];
            sprintf(err_message, "Failed to open file: %s", hash_name);
            perror(err_message);
        }

	// Write computed hash into the file
        if(fwrite(hash_buf, 1, 2*SHA256_BLOCK_SIZE+1, fp) < 2*SHA256_BLOCK_SIZE+1){
	    perror("Failed to fully write hash to file");
	    exit(-1);
	}
    }
    // TODO: If the current process is not a leaf process, spawn two child processes using  
    // exec() and ./child_process. 
    else{
        pid_t pid;
        for(int i=0; i<2; i++){
            pid = fork();

            if(pid == -1){
		//Forking failure
                perror("Failed to fork a child process");
                exit(-1);
            }else if(pid == 0){
		//Spawn a child process and execute ./child_process for newly created process
                char child_id[N_LEN_MAX];
                sprintf(child_id, "%d", 2*cur_id+i+1);
                
                execl("child_process", "./child_process", blocks_folder, hashes_folder, N_string, child_id, NULL);
                perror("Child process failed to execute into ./child_process");
                exit(-1);
            } else {
		//As the parent, wait for children to complete
                wait(NULL);
            }
        }
        // TODO: Wait for the two child processes to finish 
        // for(int j=0; j<2; j++){
        //     //TODO: Error Checking
        //     int ret = wait(NULL);
        //     if(ret == -1){
        //         char waiting_failed[ERR_MESS];
        //         sprintf(waiting_failed, "Parent %d failed to wait for child", cur_id);
        //         perror(waiting_failed);
        //     }
        // }

        // Retrieve the two hashes from the two child processes from output/hashes/
        // and compute and output the hash of the concatenation of the two hashes.

	//Store the name of the 1st child's hash file in a string
	char child1_name[PATH_MAX];
        sprintf(child1_name, "%s/%d.out", hashes_folder, 2*cur_id+1);

        //Open the hash file for 1st child
        FILE* child1_fp = fopen(child1_name, "r");

        if(child1_fp == NULL){
            char failed_to_open_child1[ERR_MESS];
            sprintf(failed_to_open_child1, "Failed to open %s", child1_name);
            perror(failed_to_open_child1);
        }

        char hash1[2*SHA256_BLOCK_SIZE+1];

	//Attempt to read the hash of the 1st child
	if(fread(hash1, sizeof(char), 2*SHA256_BLOCK_SIZE+1, child1_fp) < 2*SHA256_BLOCK_SIZE+1){
	    perror("Failed to read entire block of hash data for child 1");
	    exit(-1);
	}

	//Close 1st child's hash file
        fclose(child1_fp);

	//Store the name of the 2nd child's hash file in a string
        char child2_name[PATH_MAX];
        sprintf(child2_name, "%s/%d.out", hashes_folder, 2*cur_id+2);

        //Open the hash file for the 2nd child
        FILE* child2_fp = fopen(child2_name, "r");

        if(child2_fp == NULL){
            char failed_to_open_child2[ERR_MESS];
            sprintf(failed_to_open_child2, "Failed to open %s", child2_name);
            perror(failed_to_open_child2);
        }

        char hash2[2*SHA256_BLOCK_SIZE+1];
        
	//Attempt to read the hash of the 2nd child
        if(fread(hash2, sizeof(char), 2*SHA256_BLOCK_SIZE+1, child2_fp) < 2*SHA256_BLOCK_SIZE+1){
	    perror("Failed to read entire block of hash data for child 2");
	    exit(-1);
	}

	//Close 2nd child's hash file
        fclose(child2_fp);

	//Compute the parent's hash
        char parent_hash[2*SHA256_BLOCK_SIZE+1];
        compute_dual_hash(parent_hash, hash1, hash2);

	//Store the name of the parent file in a string
        char parent_name[PATH_MAX];
        sprintf(parent_name, "%s/%d.out", hashes_folder, cur_id);

        //Open the parent hash file
        FILE *parent_p = fopen(parent_name, "w");

        if(parent_p == NULL){
            char failed_to_open_parent[ERR_MESS];
            sprintf(failed_to_open_parent, "Failed to open %s", parent_name);
            perror(failed_to_open_parent);
        }

	//Write parent hash into the parent hash file
	if(fwrite(parent_hash, sizeof(char), 2*SHA256_BLOCK_SIZE+1, parent_p) < 2*SHA256_BLOCK_SIZE+1){
	    perror("Failed to fully write parent hash");
	    exit(-1);
	}

        //Close the parent hash file
        fclose(parent_p);
    }
}

