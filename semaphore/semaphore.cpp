#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#define buf_size 512
#define buf_num 1
#define MAX_RETRIES 10

enum semaphore_num {
    mutex = 0,
    full  = 1,
    empty = 2,
    ping  = 3,
    nsems
};

union semun {
    int val;
    struct semid_ds *buf;
    ushort *array;
};

int semid;
char *shared;

void SemOpMul (int semid, int semnum, int value, int flag) {
    sembuf ops;
    ops.sem_num = semnum;
    ops.sem_op = value;
    ops.sem_flg = flag;
    if ((semop (semid, &ops, 1)) == -1) {
        fprintf (stderr,"\nTerminating\n");
        exit(EXIT_FAILURE);
    }
}

void SemOp (const char type, int semid, int semnum, int flag) {
    sembuf ops;
    ops.sem_num = semnum;
    if (type == 'P') {
        ops.sem_op = -1;
    }
    else {
        ops.sem_op = 1;
    }
    ops.sem_flg = IPC_NOWAIT;
    
    int tmp = 0;
    unsigned short *ptr = (unsigned short *) 
                            calloc (nsems, sizeof (unsigned short));
    
    while (tmp == 0) {
        if ((semop (semid, &ops, 1)) == -1) {
            if (errno != EAGAIN) {
                fprintf (stderr,"\nTerminating\n");
                exit(EXIT_FAILURE);
            }
            else {
                semctl (semid, ping, GETALL, ptr);
                if (ptr [ping] != 2) {
                    fprintf (stderr,"\nPartner was killed\n");
                    exit (EXIT_SUCCESS);
                }
            }
        }
        else {
            tmp = 1;
        }
    }
}

int main (int argc, char *argv []) {
    key_t key = ftok (argv [0], 0);
        
    semid = semget (key, nsems, 0644 | IPC_CREAT | IPC_EXCL);
    if (semid >= 0) { //got it first
        if (((semctl (semid, mutex, SETVAL, 1)) == -1) ||
                ((semctl (semid, full,  SETVAL, 0)) == -1) ||
                ((semctl (semid, empty, SETVAL, buf_num)) == -1)) {
                perror("\nFailed to set value for the semaphore.");
                exit(EXIT_FAILURE);
            }
        SemOpMul (semid, ping, 1, SEM_UNDO);
    }
    else if (errno == EEXIST) {  //someone else got it first
        
        semid = semget (key, nsems, 0);
        if (semid < 0) {
            fprintf (stderr, "Terminating\n");
            exit(EXIT_FAILURE);
        }
        
        SemOpMul (semid, ping, 1, SEM_UNDO);
        
        unsigned short *ptr = (unsigned short *) 
                            calloc (nsems, sizeof (unsigned short));
        int ready = 0;
        for (int i = 0; i < MAX_RETRIES && !ready; i++){
            semctl (semid, ping, GETALL, ptr);
            if (ptr [ping] == 2) {
                ready = 1;
            }
            else {
                sleep (1);
            }
        }
        
        if (!ready) {
            perror ("Time is out\n");
            exit (EXIT_FAILURE);
        }
    }
    else {
            semctl (semid, 0, IPC_RMID);
            shmdt (shared);
            perror("\nFailed to create semaphore.");
            exit(EXIT_FAILURE);
    }
    
    int shmid = shmget (key, buf_num * buf_size, 0644 | IPC_CREAT);
    if (shmid == -1) {
        perror("\nFailed to allocate shared memory.");
        exit(EXIT_FAILURE);
    }
    shared = (char*) shmat (shmid, NULL, 0);
        
    char buffer [buf_size + 1] = {};
    buffer [buf_size] = '\0';        
    
    if (argc > 1) {
        //writer (Producer)
        int src = open (argv [1], O_RDONLY, 0);
        int ret_num = 1;
        while (ret_num > 0) {
            ret_num = read (src, buffer, buf_size);
            SemOp ('P', semid, empty, 0);
            SemOp ('P', semid, mutex, 0);
            strncpy (shared, buffer, ret_num);
            strncpy (buffer, shared, ret_num);
            fflush (stdout);
            SemOp ('V', semid, mutex, 0);
            SemOp ('V', semid, full,  0);
        }
    } 
    else {
        //reader (Consumer)
        while (1) {
            SemOp ('P', semid, full,  0);
            SemOp ('P', semid, mutex, 0);
            strncpy (buffer, shared, buf_size);
            printf("%s", buffer);
            memset (shared, 0, buf_size);
            fflush (stdout);
            SemOp ('V', semid, mutex, 0);
            SemOp ('V', semid, empty, 0);
        }
    }
    return 0;
}
