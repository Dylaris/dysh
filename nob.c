#define NOB_IMPLEMENTATION
#include "nob.h"

int main(void)
{
    Nob_Cmd cmd = {0};
    nob_cmd_append(&cmd, "cc", "-Wall", "-Wextra", "-o", "dysh", "src/main.c", "src/parse.c");
    if (!nob_cmd_run_sync(cmd)) return 1;
    return 0;
}