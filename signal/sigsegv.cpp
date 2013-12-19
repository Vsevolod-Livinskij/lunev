#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/fcntl.h>
#include <iostream>

void segv (int trash) {
    exit(0);
}

int main(int argc, char *argv []) {
    struct sigaction act;
    memset (&act, 0, sizeof (act));
    act.sa_handler = segv;
    //sigaction (SIGSEGV, &(act), NULL);
    int *a = NULL;
    *a = 10;
    return 0;
}
