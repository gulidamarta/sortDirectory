#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#include <linux/limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>

#define BUFFER_SIZE 4096
#define PATH_LEN 65535
#define  ARGS_COUNT 4

typedef enum {
    BY_NAME = 0,
    BY_SIZE = 1
} sortType;


typedef struct {
    char path[PATH_LEN];
    char filename[NAME_MAX];
    off_t file_size;
} FileInfo;

char* module_name;

void printError(const char *module_name, const char *error_msg, const char *file_name) {
    fprintf(stderr, "%s: %s %s\n", module_name,
            error_msg, file_name ? file_name : "");
}

int cmpFilename(const void *fst, const void *snd)
{
    return strcmp( ((FileInfo*)fst)->filename, ((FileInfo*)snd)->filename );
}

int cmpSize(const void *fst, const void *snd)
{
    return ((FileInfo*)fst)->file_size > ((FileInfo*)snd)->file_size;
}

char* basename(char *filename)
{
    char *p = strrchr(filename, '/');
    return p ? p + 1 : filename;
}

void filesInDir(const char *src_dir, FileInfo **files, size_t *files_count)
{
    struct dirent *info;
    DIR *curr = opendir(src_dir);

    if (!curr) {
        printError(module_name, "Cannot open dir: ", src_dir);
        return;
    }

    char *full_path = (char*)malloc(PATH_LEN * sizeof (char));
    if (!full_path) {
        printError(module_name, "Cannot alloc memory for dir: ", src_dir);
        return;
    }
    if (!realpath(src_dir, full_path)) {
        printError(module_name, "Cannot get full path of file: ", src_dir);
        free(full_path);
        closedir(curr);
        return;
    }

    strcat(full_path, "/");
    size_t base_dir_len = strlen(full_path);
    while ((info = readdir(curr))) {

        full_path[base_dir_len] = 0;
        strcat(full_path, info->d_name);
        if (info->d_type == DT_DIR && strcmp(info->d_name, ".") && strcmp(info->d_name, "..")) {
            filesInDir(full_path, files, files_count);
        }

        if (info->d_type == DT_REG) {

            struct stat file_stat;
            if (stat(full_path, &file_stat) == -1) {
                printError(module_name,"Cannot get stat's from file: ", full_path);
                continue;
            }

            FileInfo* old_files = *files;
            *files = (FileInfo*)realloc(*files, ++(*files_count) * sizeof (FileInfo));
            if (!*files) {

                printError(module_name, "Cannot alloc memory for file: ", full_path);
                *files = old_files;
                --(*files_count);
                continue;
            }

            FileInfo *files_arr = *files;
            size_t pos = *files_count - 1;
            strcpy(files_arr[pos].path, full_path);
            strcpy(files_arr[pos].filename, info->d_name);
            files_arr[pos].file_size = file_stat.st_size;
        }
    }
    free(full_path);

    if (closedir(curr) == -1)
    {
        printError(module_name, "Error with closing directory. ", NULL);
    }
}

void writeFiles(FileInfo *files, size_t files_count, const char* out_dir)
{
    char* out_path = malloc(PATH_LEN * sizeof (char));
    if (!realpath(out_dir, out_path)) {
        printError(module_name, "Cannot get full path of file: ", out_dir);
        return;
    }

    strcat(out_path, "/");
    size_t out_path_len = strlen(out_path);
    for (size_t i = 0; i < files_count; ++i) {
        out_path[out_path_len] = 0;
        strcat(out_path, files[i].filename);
        while (access(out_path, F_OK) != -1) {
            size_t len = strlen(out_path);
            if (len > 3 && out_path[len - 1] == ']' && isdigit(out_path[len - 2])) {
                char *brc_o = strrchr(out_path, '[');
                if (brc_o && brc_o < out_path + len - 2) {
                    out_path[len - 1] = 0;
                    int digit = atoi(brc_o + 1);
                    if (digit) {
                        sprintf(++brc_o, "%d]", ++digit);
                    }
                }
            } else
                strcat(out_path, "[1]");
        }
        FILE *in = fopen(files[i].path, "r");
        if (!in) {
            printError(module_name, strerror(errno), files[i].path);
            continue;
        }
        FILE *out = fopen(out_path, "w+");
        if (!out) {
            printError(module_name, "Cannot open for write file: ", out_path);
            fclose(in);
            continue;
        }

        char buff[BUFFER_SIZE];
        size_t bytes_read;
        while ((bytes_read = fread(buff, 1, BUFFER_SIZE, in))) {
            if (fwrite(buff, 1, bytes_read, out) != bytes_read) {
                printError(module_name, "Error writing file: ", out_path);
            }
        }
        fclose(in);
        fclose(out);
        fflush(out);

    }
    free(out_path);
}

int getSortType(const char *param_str) {
    if (strcmp(param_str, "1") == 0) {
        return BY_SIZE;
    }

    if (strcmp(param_str, "2") == 0) {
        return BY_NAME;
    }

    return -1;
}

int main(int argc, char *argv[])
{
    module_name = basename(argv[0]);
    if (argc != ARGS_COUNT) {
        printError(module_name, "Wrong number of parameters.", NULL);
        return 1;
    }

    char *src_dir = argv[1];
    char *dest_dir = argv[3];


    sortType sort_type;
    if ((sort_type = (sortType)getSortType(argv[2])) == -1) {
        printError(module_name, "Invalid sort type was entered.", NULL);
        return 1;
    }


    FileInfo *files = NULL;
    size_t files_count = 0;

    filesInDir(src_dir, &files, &files_count);
    qsort(files, files_count, sizeof (*files), sort_type == BY_NAME ? cmpFilename : cmpSize);
    writeFiles(files, files_count, dest_dir);

    free(files);
    return 0;
}
