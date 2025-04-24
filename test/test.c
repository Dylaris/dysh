#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include "err.h"

int main(void)
{
    TEST(CHECK_FILE_MODE("test.c", FILE_MODE_E) == TRUE,  1);
    TEST(CHECK_FILE_MODE("test.c", FILE_MODE_R) == TRUE,  2);
    TEST(CHECK_FILE_MODE("test.c", FILE_MODE_W) == TRUE,  3);
    TEST(CHECK_FILE_MODE("test.c", FILE_MODE_X) == FALSE, 4);
    TEST(CHECK_FILE_MODE("tttt.c", FILE_MODE_E) == FALSE, 5);

    TEST(CHECK_FILE_TYPE("test.c", FILE_TYPE_D) == FALSE, 6);
    TEST(CHECK_FILE_TYPE("test.c", FILE_TYPE_R) == TRUE,  7);

    return 0;
}
