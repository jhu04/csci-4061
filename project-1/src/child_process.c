#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include "hash.h"

#define PATH_MAX 1024

int main(int argc, char* argv[]) {
    if (argc != 5) {
        printf("Usage: ./child_process <blocks_folder> <hashes_folder> <N> <child_id>\n");
        return 1;
    }

    char* blocks_folder = argv[1];
    char* hashes_folder = argv[2];
    int N = atoi(argv[3]); // number of blocks
    int cur_id = atoi(argv[4]);
    
    // TODO: If the current process is a leaf process, read in the associated block file 
    // and compute the hash of the block.

    if(cur_id >= N-1 && cur_id <= 2*N-2){
        // Open file to read leaf data from
        char block_name[PATH_MAX];
        sprintf(block_name, "%s/%d.txt", blocks_folder, cur_id-N+1);

        // Compute hash and store it in a buffer
        char hash_buf[SHA256_BLOCK_SIZE];
        hash_data_block(hash_buf, block_name);

        // Open file to write hash into
        char hash_name[PATH_MAX];
        sprintf(hash_name, "%s/%d.txt", hashes_folder, cur_id);

        // Write computed hash into the file
	//TODO: Error check on fopen & fwrite
        FILE* fp = fopen(hash_name, "w");
        fwrite(hash_buf, 1, SHA256_BLOCK_SIZE, fp);     
    }
    // TODO: If the current process is not a leaf process, spawn two child processes using  
    // exec() and ./child_process. 
    else{
        pid_t pid;
        for(int i=0; i<2; i++){
            pid = fork();
            // TODO: Restart fork on error
            if(pid == -1){
                //
            }
            if(pid == 0){
                char child_id[5];
                sprintf(child_id, "%d", 2*cur_id+i+1);
                execl("./child_process", blocks_folder, hashes_folder, N, child_id, NULL);
                exit(-1);
            }
        }
        // TODO: Wait for the two child processes to finish 
        for(int j=0; j<2; j++){
            //TODO: Error Checking
            int ret = wait(NULL);
        }

        // TODO: Retrieve the two hashes from the two child processes from output/hashes/
        // and compute and output the hash of the concatenation of the two hashes.
        char child1_name[PATH_MAX];
        sprintf(child1_name, "%s/%d.out", hashes_folder, 2*cur_id+1);

	//Error checking needed
        FILE* child1_fp = fopen(child1_name, "r");
        char hash1[SHA256_BLOCK_SIZE];
        //TODO : Error Checking
        fread(child1_fp, sizeof(char), SHA256_BLOCK_SIZE, hash1);

	fclose(child1_fp);

        char child2_name[PATH_MAX];
        sprintf(child2_name, "%s/%d.out", hashes_folder, 2*cur_id+2);

	//TODO: Error checking
        FILE* child2_fp = fopen(child2_name, "r");
        char hash2[SHA256_BLOCK_SIZE];
        //TODO : Error Checking
        fread(child2_fp, sizeof(char), SHA256_BLOCK_SIZE, hash2);

	fclose(child2_fp);

	char parent_hash[SHA256_BLOCK_SIZE];
	compute_dual_hash(parent_hash, hash1, hash2);

	char parent_name[PATH_MAX];
        sprintf(parent_name, "%s/%d.out", hashes_folder, cur_id);

	//TODO: For both fopen & fwrite: error check
	FILE *parent_p = fopen(parent_name, "w");
	fwrite(parent_hash, sizeof(char), SHA256_BLOCK_SIZE, parent_p);

	//Error check needed ??? ask TA
	fclose(parent_p);
    }
}

