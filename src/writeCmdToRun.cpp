#include <stdio.h>
void writeCmdToRun(const char *cmd)
{
    FILE *f = fopen("/tmp/cmd_to_run.sh", "w+");
    if (f != NULL) {
        fputs(cmd, f);
        fclose(f);
    }
}
