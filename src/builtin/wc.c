#include <stdio.h>

int main(int argc, char **argv)
{
    char buf[1024];
    fread(buf, 1, sizeof(buf), stdin);

    unsigned int count = 0;
    for (size_t i = 0; i < sizeof(buf); i++) {
        if (buf[i++] == '\n')
            count++;
    }

    printf("%d %s\n", count, (count > 1) ? "lines" : "line");

    return 0;
}