#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>
#include "../include/hash.h"

#define FD_MAX 10

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Usage: ./nonleaf_process <directory_path> <pipe_write_end> \n");
        return 1;
    }
    //TODO(overview): fork the child processes(non-leaf process or leaf process) each associated with items under <directory_path>
    
    //TODO(step1): get <file_path> <pipe_write_end> from argv[]
    char *dir_path = argv[1];
    int pipe_write_end = atoi(argv[2]);

    //TODO(step2): malloc buffer for gathering all data transferred from child process as in root_process.c
    char *sub_filepath_hashvalue = (char *)malloc((sizeof(char)) * BUFFER_SIZE);
    memset(sub_filepath_hashvalue, 0, sizeof(sub_filepath_hashvalue));

    //TODO(step3): open directory
    DIR *current_directory = opendir(dir_path);
    if(current_directory == NULL){
        perror("Failed to open directory");
        exit(-1);
    }

    //TODO(step4): traverse directory and fork child process
    // Hints: Maintain an array to keep track of each read end pipe of child process
    int pipe_write_ends_array[10]; // Note : Chosen 10 since max number of file sin assumption is 10
    int pipe_read_ends_array[10];

    struct dirent *dir_entry;

    int i = 0;
    while ((dir_entry = readdir(current_directory))!=NULL){
        char *entry_name = dir_entry->d_name;
        if(strcmp(entry_name, ".") == 0 || strcmp(entry_name, "..") == 0){
            continue;
        }

	//Create a new pipe
        int fd[2];
        pipe(fd);
        char location[PATH_MAX];

        int ret_id = fork();
	pipe_read_ends_array[i]  = fd[0];
	pipe_write_ends_array[i] = fd[1];

        if(ret_id == 0){
            close(fd[0]);
            if(dir_entry->d_type == DT_DIR){
                memset(location, '\0', PATH_MAX);
                strcpy(location, dir_path);
		strcat(location, "/");
                strcat(location, dir_entry->d_name);
		
		char fd_name[FD_MAX];
		sprintf(fd_name, "%d", fd[1]);
                execl(location, "./nonleaf_process", dir_path, fd_name, NULL);
            }else if(dir_entry->d_type == DT_REG || dir_entry->d_type == DT_LNK){
		memset(location, '\0', PATH_MAX);
                strcpy(location, dir_path);
		strcat(location, "/");

		char fd_name[FD_MAX];
		sprintf(fd_name, "%d", fd[1]);
                strcat(location, dir_entry->d_name); // TODO : Do we need the `/ ` in /nonleaf_proc
                execl(location, "./leaf_process", dir_path, fd_name, NULL);
	    }else{
		close(fd[1]);
		perror("Weird file type");
		exit(-1);
	    }
/*
            if(dir_entry->d_type == DT_REG){
                memset(location, '\0', PATH_MAX);
                strcpy(location, dir_path);
                strcat(location, dir_entry->d_name); // TODO : Do we need the `/ ` in /nonleaf_proc
                execl(location, , "./leaf_process", dir_path, fd[1], NULL);
            }

            if(dir_entry->d_type == DT_LNK){
        	memset(location, '\0', PATH_MAX);
                strcpy(location, dir_path);
                strcat(location, dir_entry->d_name); // TODO : Do we need the `/ ` in /nonleaf_proc
                execl(location, , "./leaf_process", dir_path, fd[1], NULL);
            }
*/

	    close(fd[1]);
	    perror("Child processes failed to execute");
	    exit(-1);

            //TODO : Do we need to check for unknown file type and use lstat to handle that case?
        } else{
            close(fd[1]);
            //pipe_read_ends_array[i] = fd[0];
            i+=1;
        }
    }


    //TODO(step5): read from pipe constructed for child process and write to pipe constructed for parent process
    while(wait(NULL) != -1);






    
}
