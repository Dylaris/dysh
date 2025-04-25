#include <stdio.h>
#include <malloc.h>

int main(int argc, char **argv)
{
    if (argc < 2) return 1;

    char *filename = argv[1];
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "cat error: file %s is not exist\n", filename);
        return 1;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *buf = malloc(sizeof(char) * file_size);
    if (!buf) {
        fprintf(stderr, "out of memory");
        return 1;
    }

    fread(buf, 1, file_size, fp);
    printf("%s", buf);
    fflush(stdout);

    free(buf);

    return 0;
}
