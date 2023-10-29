#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../include/hash.h"
#include "../include/utils.h"

#define SHA256_STRING_SIZE (2 * SHA256_BLOCK_SIZE + 1)

char *output_file_folder = "output/final_submission/";

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Usage: Inter Submission --> ./leaf_process <file_path> 0\n");
        printf("Usage: Final Submission --> ./leaf_process <file_path> <pipe_write_end>\n");
        return -1;
    }
    //TODO(): get <file_path> <pipe_write_end> from argv[]
    char *file_path = argv[1];
    int pipe_write_end = atoi(argv[2]);

    //TODO(): create the hash of given file
    char hash[SHA256_STRING_SIZE];
    memset(hash, '\0', SHA256_STRING_SIZE * sizeof(char));
    hash_data_block(hash, file_path);

    //TODO(): construct string write to pipe. The format is "<file_path>|<hash_value>|"
    char to_write[PATH_MAX + SHA256_STRING_SIZE + 2];
    memset(to_write, '\0', PATH_MAX * sizeof(char));
    strcpy(to_write, file_path);
    strcat(to_write, "|");
    strcat(to_write, hash);
    strcat(to_write, "|");

    if (pipe_write_end == 0)
    {
        //TODO(inter submission)
        //TODO(overview): create a file in output_file_folder("output/inter_submission/root*") and write the constructed string to the file
        //TODO(step1): extract the file_name from file_path using extract_filename() in utils.c
        char *file_name = extract_filename(file_path);

        //TODO(step2): extract the root directory(e.g. root1 or root2 or root3) from file_path using extract_root_directory() in utils.c
        char *root_dir = extract_root_directory(file_path);

        //TODO(step3): get the location of the new file (e.g. "output/inter_submission/root1" or "output/inter_submission/root2" or "output/inter_submission/root3")
        char location[PATH_MAX];
        memset(location, '\0', PATH_MAX * sizeof(char));
        strcpy(location, output_file_folder);
        strcat(location, root_dir);
        strcat(location, file_name);

        //create file
        FILE *fp = fopen(location, "w");
        if (fp == NULL)
        {
            perror("Failed to open file");
            exit(-1);
        }

        //write to file
        size_t strlen_to_write = strlen(to_write);
        if (fwrite(to_write, sizeof(char), strlen_to_write, fp) < strlen_to_write)
        {
            fclose(fp);

            perror("Failed to fully write to file");
            exit(-1);
        }

        //close file
        fclose(fp);

        //TODO(step5): free any arrays that are allocated using malloc!! Free the string returned from extract_root_directory()!! It is allocated using malloc in extract_root_directory()
        free(root_dir);
    }
    else
    {
        //TODO(final submission): write the string to pipe & error check
        if (write(pipe_write_end, to_write, strlen(to_write)) == -1)
        {
            perror("Failed to write to pipe");
            exit(-1);
        }

        if (close(pipe_write_end) == -1)
        {
            perror("Failed to close pipe write end");
            exit(-1);
        }

        exit(0);
    }

    exit(0);
}
