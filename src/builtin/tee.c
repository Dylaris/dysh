#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    if (argc > 3) {
        fprintf(stderr, "Usage: tee [-a] file\n");
        exit(EXIT_FAILURE);
    }

    char *open_flags = "w"; // O_WRONLY | O_CREAT | O_TRUNC

    int opt;
    const char *opt_fmt = "a";

    while ((opt = getopt(argc, argv, opt_fmt)) != -1) {
        switch (opt) {
        case 'a':
            open_flags = "a"; // O_CREAT | O_WRONLY | O_APPEND
            break;
        default:
            fprintf(stderr, "Usage: tee [-a] file\n");
            exit(EXIT_FAILURE);
        }
    }

    const char *filename = argv[optind];
    FILE *fp = fopen(filename, open_flags); // default permision: rw-rw-rw-
    if (!fp) {
        fprintf(stderr, "tee error: open file\n");
        exit(EXIT_FAILURE);
    }

    size_t rbytes = 0;
    const unsigned int BUF_SIZE = 1024;
    char buf[BUF_SIZE];

    while ((rbytes = fread(buf, sizeof(char), sizeof(buf), stdin)) > 0) {
        fwrite(buf, sizeof(char), rbytes, stdout);
        fwrite(buf, sizeof(char), rbytes, fp);
        if (feof(stdin)) break; // avoid empty read
    }

    // printf("just for debug\n");
    fclose(fp);

    return 0;
}
