#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../include/hash.h"
#include "../include/utils.h"

#define PATH_MAX 1024

char *output_file_folder = "output/inter_submission/";

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Usage: Inter Submission --> ./leaf_process <file_path> 0\n");
        printf("Usage: Final Submission --> ./leaf_process <file_path> <pipe_write_end>\n");
        return -1;
    }
    //TODO(): get <file_path> <pipe_write_end> from argv[]
    char *file_path = argv[1];
    int pipe_write_end = atoi(argv[2]);

    //TODO(): create the hash of given file
    char hash[2*SHA256_BLOCK_SIZE+1];
    memset(hash, '\0', 2*SHA256_BLOCK_SIZE+1);
    hash_data_block(hash, file_path);

    //TODO(): construct string write to pipe. The format is "<file_path>|<hash_value>"
    char to_write[PATH_MAX];
    memset(to_write, '\0', PATH_MAX);
    strcpy(to_write, file_path);
    strcat(to_write, "|");
    strcat(to_write, hash);
    strcat(to_write, "|");

    if(pipe_write_end == 0){
        //TODO(inter submission)
        //TODO(overview): create a file in output_file_folder("output/inter_submission/root*") and write the constructed string to the file
        //TODO(step1): extract the file_name from file_path using extract_filename() in utils.c
	char *file_name = extract_filename(file_path);
	//printf("Filename: %s\n", file_name);

        //TODO(step2): extract the root directory(e.g. root1 or root2 or root3) from file_path using extract_root_directory() in utils.c
	char *root_dir = extract_root_directory(file_path);
	//printf("Rootdir: %s\n", root_dir);

        //TODO(step3): get the location of the new file (e.g. "output/inter_submission/root1" or "output/inter_submission/root2" or "output/inter_submission/root3")
	char location[PATH_MAX];
	strcpy(location, output_file_folder);
	strcat(location, root_dir);
	strcat(location, file_name);

	printf("Loc: %s\n", location);
        //TODO(step4): create and write to file, and then close file
	FILE *fp = fopen("test.txt", "w+");
	fwrite(to_write, sizeof(char), strlen(to_write), fp);
	fclose(fp);

        //TODO(step5): free any arrays that are allocated using malloc!! Free the string returned from extract_root_directory()!! It is allocated using malloc in extract_root_directory()
	free(root_dir);

    }else{
        //TODO(final submission): write the string to pipe

        exit(0);

    }

    exit(0);
}