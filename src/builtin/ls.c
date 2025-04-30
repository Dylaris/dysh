#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "err.h"

int main(int argc, char **argv)
{
    if (argc > 3) {
        fprintf(stderr, "Usage: ls [-a] [directory|file]\n");
        return 1; 
    }

    int opt;
    const char *optfmt = "a";
    opterr = 0;
    int print_hidden_file = 0;

    while ((opt = getopt(argc, argv, optfmt)) != -1) {
        switch (opt) {
        case 'a': print_hidden_file = 1; break;
        case '?': break;
        default:
            fprintf(stderr, "Usage: ls [-a] [directory|file]\n");
            return 1; 
        }
    }

    const char *path = (argv[optind] != NULL) ? argv[optind] : ".";
    char filename_buf[512];

    if (CHECK_FILE_TYPE(path, FILE_TYPE_R) == TRUE) {
        printf("\t%sFILE:%s   %s%s%s\n", WHITE, RESET, WHITE, path, RESET);
    } else {
        struct dirent *entry;
        DIR *dp = opendir(path);
        EXIT_IF(dp == NULL, "ls error: open path");
        while ((entry = readdir(dp)) != NULL) {
            if (entry->d_name[0] == '.' && print_hidden_file == 0) continue;
            snprintf(filename_buf, sizeof(filename_buf), "%s/%s", path, entry->d_name);
            if (CHECK_FILE_TYPE(filename_buf, FILE_TYPE_R) == TRUE) {
                printf("\t%sFILE:%s   %s%s%s\n", WHITE, RESET, WHITE, entry->d_name, RESET);
            } else if (CHECK_FILE_TYPE(filename_buf, FILE_TYPE_D) == TRUE) {
                printf("\t%sDIR:%s    %s%s%s\n", CYAN, RESET, WHITE, entry->d_name, RESET);
            } else {
                printf("\t%sUNKW:%s   %s%s%s\n", RED, RESET, WHITE, entry->d_name, RESET);
            }
        }
    }

    return 0;
}
