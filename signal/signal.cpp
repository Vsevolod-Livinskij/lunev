#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/fcntl.h>
#include <iostream>

int ping;
int zero;
int one;

void usr1 (int trash) {
    zero = 1;
}  

void usr2 (int trash) {
    one = 1;
}

void pipe (int trash) {
    ping = 1;
    alarm (100);
}

void alarm (int trash) {
    exit (0);
}

void die (int trash) {
    exit (0);
}

int main(int argc, char *argv []) {
    sigset_t set, set_empty;
    sigemptyset (&set);
    sigemptyset (&set_empty);
    sigaddset (&set, SIGUSR1);
    sigaddset (&set, SIGUSR2);
    sigaddset (&set, SIGPIPE);
    sigaddset (&set, SIGCHLD);
    sigaddset (&set, SIGALRM);
    sigprocmask (SIG_BLOCK, &set, NULL);
    
    int ppid = getpid ();
    int pid = fork ();
    
    if (pid == 0) {
        ping = 0;
        
        sigset_t pipe_set, alarm_set;
        sigemptyset (&pipe_set);
        sigemptyset (&alarm_set);
        sigaddset (&pipe_set, SIGPIPE);
        sigaddset (&alarm_set, SIGALRM);
        sigprocmask (SIG_BLOCK, &pipe_set, NULL);
        sigprocmask (SIG_UNBLOCK, &alarm_set, NULL);
        
        struct sigaction pipe_act, alarm_act;
        memset (&pipe_act, 0, sizeof (pipe_act));
        memset (&alarm_act, 0, sizeof (alarm_act));
        pipe_act.sa_handler = pipe;
        alarm_act.sa_handler = alarm;
        sigaction (SIGPIPE, &(pipe_act), NULL);
        sigaction (SIGALRM, &(alarm_act), NULL);
        
        int src = open ("in.txt", O_RDONLY);
        int ret_num = 1;
        char buffer [1] = {};
        
        while (ret_num > 0) {
            ret_num = read (src, buffer, 1);
            
            for (int i = 0; i < 8; i++) {
                ping = 0;
                
                sigprocmask (SIG_UNBLOCK, &pipe_set, NULL);
                if (!ping)
                    sigsuspend (&set_empty);
                sigprocmask (SIG_BLOCK, &pipe_set, NULL);

                unsigned int bit = (buffer [0] & (1 << i)) >> i;
                if (bit == 0) {
                    kill (ppid, SIGUSR1);
                } else {
                    kill (ppid, SIGUSR2);
                }
            }
        }
        kill (ppid, SIGCHLD);
    }
    else {
        zero = 0;
        one  = 0;
        
        sigset_t usr_set, chld_set;
        sigemptyset (&usr_set);
        sigemptyset (&chld_set);
        sigaddset (&usr_set, SIGUSR1);
        sigaddset (&usr_set, SIGUSR2);
        sigaddset (&chld_set, SIGCHLD);
        
        struct sigaction usr1_act, usr2_act, die_act;
        memset (&usr1_act, 0, sizeof (usr1_act));
        memset (&usr2_act, 0, sizeof (usr2_act));
        memset (&die_act, 0, sizeof (die_act));
        usr1_act.sa_handler = usr1;
        usr2_act.sa_handler = usr2;
        die_act.sa_handler = die;
        
        sigaction (SIGUSR1, &(usr1_act), NULL);
        sigaction (SIGUSR2, &(usr2_act), NULL);
        sigaction (SIGCHLD, &(die_act), NULL);
        
        sigprocmask (SIG_UNBLOCK, &chld_set, NULL);
        
        kill (pid, SIGPIPE);
        
        while (1) {
            char data = 0;
                
            for (int i = 0; i < 8; i++) {
                zero = 0;
                one  = 0;
                
                sigprocmask (SIG_UNBLOCK, &usr_set, NULL);
                if (!zero && !one)
                    sigsuspend (&set_empty);
                sigprocmask (SIG_BLOCK, &usr_set, NULL);
                
                if (one == 1) {
                    data = data + (1 << i);
                }
                
                kill (pid, SIGPIPE);
            }
            printf ("%c", data);
            fflush (stdout);
        }  
    }

    wait (NULL);
    return 0;
}
