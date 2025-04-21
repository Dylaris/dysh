#include <stdio.h>

int main(int argc, char **argv)
{
    char echo_content[256];

    if (argc >= 2)
        sprintf(echo_content, "%s", argv[1]);
    else
        echo_content[0] = '\0';

    puts(echo_content);

    return 0;
}
