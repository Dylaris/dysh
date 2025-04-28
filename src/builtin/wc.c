#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#define BUF_LEN (1024 * 10)

int main(void)
{
    char *buf = malloc(sizeof(char) * BUF_LEN);
    if (!buf) {
        fprintf(stderr, "out of memory\n");
        return 1;
    }
    memset(buf, 0, BUF_LEN);

    size_t bytes = fread(buf, 1, BUF_LEN, stdin);
    buf[bytes] = '\0';

    unsigned int lines = 0;
    for (int i = 0; i < BUF_LEN; i++) {
        if (buf[i] == '\n') lines++;
    }

    printf("total %s: %zu\n", (bytes > 1) ? "bytes" : "byte", bytes);
    printf("total %s: %u\n",  (lines > 1) ? "lines" : "line", lines);

    free(buf);
    return 0;
}
