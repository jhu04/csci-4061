#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "../include/utils.h"

#define WRITE (O_WRONLY | O_CREAT | O_TRUNC)
#define PERM (S_IRUSR | S_IWUSR)

#define BUFFER_SIZE 16384
#define LIST_LIM 128
// TODO: combine this FD_MAX and FD_MAX in nonleaf_process.c in a header file?
#define FD_MAX 10

char *output_file_folder = "output/final_submission/";

void redirection(char **dup_list, int size, char* root_dir){
    // TODO(overview): redirect standard output to an output file in output_file_folder("output/final_submission/")
    // TODO(step1): determine the filename based on root_dir. e.g. if root_dir is "./root_directories/root1", the output file's name should be "root1.txt"

    char output_file_name[BUFFER_SIZE];
    memset(output_file_name, '\0', BUFFER_SIZE * sizeof(char));
    strcpy(output_file_name, extract_filename(root_dir)); // TODO: null check
    strcat(output_file_name, ".txt");

    char out_location[BUFFER_SIZE];
    strcpy(out_location, output_file_folder);
    strcat(out_location, output_file_name);

    //TODO(step2): redirect standard output to output file (output/final_submission/root*.txt)
    int fd;
    if( (fd = open(out_location, WRITE, PERM)) == -1){
        perror("Failed to open output file folder");
        exit(-1);
    }
    int TEMP_STDOUT_FILENO = dup(STDOUT_FILENO);
    if(dup2(fd, STDOUT_FILENO) == -1){
        perror("Failed to create temporary duplicate for stdout fileno");
        exit(-1);
    }

    //TODO(step3): read the content each symbolic link in dup_list, write the path as well as the content of symbolic link to output file(as shown in expected)
    for(int i=0; i<size; i++){
        //printf(dup_list[i]);
        char buffer[BUFFER_SIZE];
        memset(buffer, '\0', BUFFER_SIZE * sizeof(char));
        if(readlink(dup_list[i], buffer, BUFFER_SIZE) == -1){
            perror("Failed to read symbolic link contents");
            exit(-1);
        }

        //fprintf()
        //printf("Size: %d\n", size);
        //fflush(stdout);
        printf("[<path of symbolic link> --> <path of retained file>] : [%s --> %s]\n", dup_list[i], buffer);
        fflush(stdout);
    }

    close(fd);

    if(dup2(TEMP_STDOUT_FILENO, STDOUT_FILENO) == -1){
        close(TEMP_STDOUT_FILENO);
        perror("Failed to restore stdout fileno");
        exit(-1);
    }

    close(TEMP_STDOUT_FILENO);
}

void create_symlinks(char **dup_list, char **retain_list, int size) {
    //TODO(): create symbolic link at the location of deleted duplicate file
    //TODO(): dup_list[i] will be the symbolic link for retain_list[i]
    for(int i=0; i<size; i++){
        if(symlink(retain_list[i], dup_list[i]) == -1){
            perror("Failed to symlink a duplicate file");
            exit(-1);
        }
    }

}

void delete_duplicate_files(char **dup_list, int size) {
    //TODO(): delete duplicate files, each element in dup_list is the path of the duplicate file
    for(int i=0; i<size; i++){
        if(unlink(dup_list[i]) == -1){
            perror("Failed to remove a duplicate file");
            exit(-1);
        }
    }
}

// ./root_directories <directory>
int main(int argc, char* argv[]) {
    if (argc != 2) {
        // dir is the root_directories directory to start with
        // e.g. ./root_directories/root1
        printf("Usage: ./root <dir> \n");
        return 1;
    }

    //TODO(overview): fork the first non_leaf process associated with root directory("./root_directories/root*")
    char* root_directory = argv[1];
    char all_filepath_hashvalue[BUFFER_SIZE]; //buffer for gathering all data transferred from child process
    memset(all_filepath_hashvalue, 0, BUFFER_SIZE * sizeof(char)); // clean the buffer

    //TODO(step1): construct pipe
    int fd[2];
    if(pipe(fd) == -1){
        perror("Failed to create pipe for root process");
        exit(-1);
    }

    //TODO(step2): fork() child process & read data from pipe to all_filepath_hashvalue
    int pid = fork();

    // TODO: error handling
    if (pid == 0) {
        close(fd[0]);

        char fd_name[FD_MAX];
        memset(fd_name, '\0', FD_MAX * sizeof(char));
        sprintf(fd_name, "%d", fd[1]);

        //check to see if we malloc'd anything here to free
        execl("./nonleaf_process", "./nonleaf_process", root_directory, fd_name, NULL);

        // TODO: error handling
        close(fd[1]);
        perror("Failed to execute first nonleaf process");
        exit(-1);
    } else if (pid > 0) {
        close(fd[1]);
        //ssize_t nbytes;
        char buffer[BUFFER_SIZE];
        memset(buffer, '\0', BUFFER_SIZE * sizeof(char));

        while(read(fd[0], buffer, BUFFER_SIZE) != 0){
            strcat(all_filepath_hashvalue, buffer);
        }
        close(fd[0]);
        // TODO: read data from pipe
    } else {
        perror("Failed to fork at root");
        exit(-1);
    }


    //TODO(step3): malloc dup_list and retain list & use parse_hash() in utils.c to parse all_filepath_hashvalue
    // dup_list: list of paths of duplicate files. We need to delete the files and create symbolic links at the location
    // retain_list: list of paths of unique files. We will create symbolic links for those files
    char **dup_list    = (char **) malloc(sizeof(char *) * LIST_LIM);
    char **retain_list = (char **) malloc(sizeof(char *) * LIST_LIM);
    memset(dup_list, 0, LIST_LIM * sizeof(char *));
    memset(retain_list, 0, LIST_LIM * sizeof(char *));

    int size = parse_hash(all_filepath_hashvalue, dup_list, retain_list);

    //TODO(step4): implement the functions

    delete_duplicate_files(dup_list,size);
    create_symlinks(dup_list, retain_list, size);
    redirection(dup_list, size, argv[1]);

    //TODO(step5): free any arrays that are allocated using malloc!!
    int dup_list_freed[size];
    memset(dup_list_freed, 0, size * sizeof(int));

    int retain_list_freed[size];
    memset(retain_list_freed, 0, size * sizeof(int));

    for (int i = 0; i < size; ++i) {
        int free_dup_list_i = 1;

        for (int j = 0; j < size; ++j) {
            if (dup_list[i] == dup_list[j] && dup_list_freed[j]) {
                free_dup_list_i = 0;
                break;
            }
        }

        if (free_dup_list_i) {
            free(dup_list[i]);
            dup_list_freed[i] = 1;
        }

        int free_retain_list_i = 1;

        for (int j = 0; j < size; ++j) {
            if (retain_list[i] == retain_list[j] && retain_list_freed[j]) {
                free_retain_list_i = 0;
                break;
            }
        }

        if (free_retain_list_i) {
            free(retain_list[i]);
            retain_list_freed[i] = 1;
        }
    }

    free(dup_list);
    dup_list = NULL;
    free(retain_list);
    retain_list = NULL;
}
