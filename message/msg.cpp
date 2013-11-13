#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

struct msg_t {
    long msgtype;
};

int main (int argc, char** argv) {

	if (argc != 2) 
	{
		printf ("Number not specified\n");
		exit (EXIT_FAILURE);
	}

    int msgid = msgget (IPC_PRIVATE, 0666 | IPC_CREAT);
    msg_t msg;
    msg.msgtype = 1;
    msgsnd (msgid, & msg, sizeof (msg_t) - sizeof (msg.msgtype), 0);

    int quant = atoi (argv [1]);
    for (int i = 2; i < quant + 2; i++) {
    int cpid = fork ();

	    if (cpid == 0) 
	    {  
            msg_t msg_r;
            msgrcv (msgid, & msg_r, sizeof (msg_t) - sizeof (long), i - 1, 0);
            printf("%d ", i - 1);
            fflush (stdout);
            msg_r.msgtype = i;
            msgsnd (msgid, & msg_r, sizeof (msg_t) - sizeof (long), 0);
            exit(EXIT_SUCCESS);
        }
    }
    
    msgrcv (msgid, & msg, sizeof (msg_t) - sizeof (long), quant + 1, 0); 
    msgctl (msgid, IPC_RMID, NULL);
    return 0;
}

//while /bin/true ; do ./out 30 ; echo ; done

