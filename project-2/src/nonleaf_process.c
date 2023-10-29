#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>

//Increased buffer size to account for storing file paths and hashes
#define BUFFER_SIZE 16384

// Note: 10 chosen since max number of files in assumption is 10
#define MAX_READ_ENDS 10

#define FD_MAX 10

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Usage: ./nonleaf_process <directory_path> <pipe_write_end> \n");
        return 1;
    }
    //TODO(overview): fork the child processes(non-leaf process or leaf process) each associated with items under <directory_path>

    //TODO(step1): get <file_path> <pipe_write_end> from argv[]
    char *dir_path = argv[1];
    int pipe_write_end = atoi(argv[2]);

    //TODO(step2): malloc buffer for gathering all data transferred from child process as in root_process.c
    char *sub_filepath_hashvalue = (char *)malloc(BUFFER_SIZE * sizeof(char));
    memset(sub_filepath_hashvalue, '\0', BUFFER_SIZE * sizeof(char));

    //TODO(step3): open directory
    DIR *current_directory = opendir(dir_path);
    if (current_directory == NULL)
    {
        perror("Failed to open directory");
        exit(-1);
    }

    //TODO(step4): traverse directory and fork child process
    // Hints: Maintain an array to keep track of each read end pipe of child process
    int pipe_read_ends_array[MAX_READ_ENDS];

    struct dirent *dir_entry;

    //Construct the pipes connecting current process to all children
    int pipe_read_ends_array_size = 0;
    while ((dir_entry = readdir(current_directory)) != NULL)
    {
        char *entry_name = dir_entry->d_name;
        if (strcmp(entry_name, ".") == 0 || strcmp(entry_name, "..") == 0)
        {
            continue;
        }

        //Create a new pipe
        int fd[2];
        if (pipe(fd) == -1)
        {
            perror("Failed to create pipe");
            exit(-1);
        }

        pipe_read_ends_array[pipe_read_ends_array_size] = fd[0];

        int ret_id = fork();
        if (ret_id == -1) {
            perror("Failed to fork at root");
            exit(-1);
        }
        else if (ret_id == 0)
        {
            if (close(fd[0]) == -1)
            {
                perror("Child failed to close read end of pipe");
                exit(-1);
            }

            //Construct file path
            char location[PATH_MAX];
            memset(location, '\0', PATH_MAX * sizeof(char));
            strcpy(location, dir_path);
            strcat(location, "/");
            strcat(location, dir_entry->d_name);

            //Make the file descriptor integer a string
            char fd_name[FD_MAX];
            memset(fd_name, '\0', FD_MAX * sizeof(char));
            sprintf(fd_name, "%d", fd[1]);

            //Execute the next process & error check
            if (dir_entry->d_type == DT_DIR)
            {
                execl("./nonleaf_process", "./nonleaf_process", location, fd_name, NULL);
                perror("Failed to execute next nonleaf process");
                exit(-1);
            }
            else if (dir_entry->d_type == DT_REG || dir_entry->d_type == DT_LNK)
            {
                execl("./leaf_process", "./leaf_process", location, fd_name, NULL);
                perror("Failed to execute next leaf process");
                exit(-1);
            }
            else
            {
                perror("Weird file type");
                exit(-1);
            }

            if (close(fd[1]) == -1)
            {
                perror("Child failed to close write end");
                exit(-1);
            }

            perror("Child processes failed to execute");
            exit(-1);
        }
        else
        {
            //Parent sets up the read ends of pipe structures
            if (close(fd[1]) == -1)
            {
                perror("Failed to close write end");
                exit(-1);
            }

            pipe_read_ends_array[pipe_read_ends_array_size] = fd[0];
            ++pipe_read_ends_array_size;
        }
    }

    //TODO(step5): read from pipe constructed for child process
    for (int i = 0; i < pipe_read_ends_array_size; ++i)
    {
        ssize_t nbytes;
        char buffer[BUFFER_SIZE];
        memset(buffer, '\0', BUFFER_SIZE * sizeof(char));
        while ((nbytes = read(pipe_read_ends_array[i], buffer, BUFFER_SIZE)) != 0)
        {
            if (nbytes == -1)
            {
                perror("Failed to read from pipe");
                exit(-1);
            }
            
            strcat(sub_filepath_hashvalue, buffer);
        }

        if (close(pipe_read_ends_array[i]) == -1)
        {
            perror("Failed to close read end to child pipe");
            exit(-1);
        }
    }

    //Write to parent pipe
    ssize_t strlen_sub_filepath_hashvalue = strlen(sub_filepath_hashvalue);
    if (write(pipe_write_end, sub_filepath_hashvalue, strlen_sub_filepath_hashvalue) < strlen_sub_filepath_hashvalue)
    {
        perror("Failed to write to parent pipe");
        exit(-1);
    }

    if (close(pipe_write_end) == -1)
    {
        perror("Failed to close write end of parent pipe");
        exit(-1);
    }

    free(sub_filepath_hashvalue);
    sub_filepath_hashvalue = NULL;

    while (wait(NULL) != -1);
}
