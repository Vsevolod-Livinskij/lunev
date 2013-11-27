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

struct in_data {
    char buffer [buf_size];
    int num;
};

enum semaphore_num {
    mutex = 0,
    full  = 1,
    empty = 2,
    ping1 = 3,
    ping2 = 4,
    init1 = 5,
    init2 = 6,
    nsems
};

union semun {
    int val;
    struct semid_ds *buf;
    ushort *array;
};

int semid;
in_data *shared;

void SemOpMul (int semid, int semnum, int value, int flag) {
    sembuf ops;
    ops.sem_num = semnum;
    ops.sem_op = value;
    ops.sem_flg = flag;
    if ((semop (semid, &ops, 1)) == -1) {
        fprintf (stderr,"\nTerminating\n");
        semctl (semid, 0, IPC_RMID, NULL);
        shmdt (shared);
        exit(EXIT_FAILURE);
    }
}

void SemOp (const char type, int semid, int semnum, int flag, int ping_num) {
    sembuf ops [3];
    
    ops [0].sem_num = ping_num;
    ops [0].sem_op  = -2;
    ops [0].sem_flg = IPC_NOWAIT | SEM_UNDO;
    
    ops [1].sem_num = semnum;
    if (type == 'P') {
        ops [1].sem_op = -1;
    }
    else {
        ops [1].sem_op = 1;
    }
    ops [1].sem_flg = SEM_UNDO;
        
    ops [2].sem_num = ping_num;
    ops [2].sem_op  = 2;
    ops [2].sem_flg = IPC_NOWAIT | SEM_UNDO;
    
    if ((semop (semid, ops, 3)) == -1) {
        if (errno != EAGAIN) {
            fprintf (stderr,"\nTerminating\n");
            semctl (semid, 0, IPC_RMID, NULL);
            shmdt (shared);
            exit(EXIT_FAILURE);
        }
        else {
            unsigned short *ptr = (unsigned short *) 
                            calloc (nsems, sizeof (unsigned short));
            semctl (semid, ping_num, GETALL, ptr);
            if (ptr [ping_num] != 2) {
                fprintf (stderr,"\nPartner was killed\n");
                semctl (semid, 0, IPC_RMID, NULL);
                shmdt (shared);
                exit (EXIT_SUCCESS);
            }
            else {
                fprintf (stderr,"\nSome bugs\n");
                semctl (semid, 0, IPC_RMID, NULL);
                shmdt (shared);
                exit (EXIT_SUCCESS);
            }
        }
    }
}

void SemInit () {
    union semun semopts [nsems];    

    semopts [mutex].val = 1;
    semopts [full ].val = 0;
    semopts [empty].val = buf_num;
    semopts [ping1].val = 0;
    semopts [ping2].val = 0;
    semopts [init1].val = 1;
    semopts [init2].val = 1;
    
    if (((semctl (semid, mutex, SETVAL, semopts [mutex])) == -1) ||
        ((semctl (semid, full,  SETVAL, semopts [full ])) == -1) ||
        ((semctl (semid, empty, SETVAL, semopts [empty])) == -1) ||
        ((semctl (semid, ping1, SETVAL, semopts [ping1])) == -1) ||
        ((semctl (semid, ping2, SETVAL, semopts [ping2])) == -1) ||
        ((semctl (semid, init1, SETVAL, semopts [init1])) == -1) ||
        ((semctl (semid, init2, SETVAL, semopts [init2])) == -1)) {
            perror("\nFailed to set value for the semaphore.");
            semctl (semid, 0, IPC_RMID, NULL);
            shmdt (shared);
            exit(EXIT_FAILURE);
    }
     
}

int main (int argc, char *argv []) {
    key_t key = ftok (argv [0], 0);
    semid = semget (key, nsems, 0644 | IPC_CREAT | IPC_EXCL);
    
    if (semid >= 0) { //got it first
        SemInit();
    }
    else if (errno == EEXIST) {  //someone else got it first
        
        semid = semget (key, nsems, 0);
        unsigned short *ptr = (unsigned short *) 
                            calloc (nsems, sizeof (unsigned short));
        semctl (semid, mutex, GETALL, ptr);
        if (ptr [mutex] != 1 || ptr [full] != 0 || ptr [empty] != buf_num) {
            SemInit();
        }
        if (semid < 0) {
            fprintf (stderr, "Failed to open semaphore\n");
            semctl (semid, 0, IPC_RMID, NULL);
            shmdt (shared);
            exit(EXIT_FAILURE);
        }
    }
    else {
            semctl (semid, 0, IPC_RMID, NULL);
            shmdt (shared);
            perror("\nFailed to create semaphore.");
            semctl (semid, 0, IPC_RMID, NULL);
            shmdt (shared);
            exit(EXIT_FAILURE);
    }
    
    int shmid = shmget (key, sizeof (in_data), 0644 | IPC_CREAT);
    if (shmid == -1) {
        perror("\nFailed to allocate shared memory.");
        semctl (semid, 0, IPC_RMID, NULL);
        shmdt (shared);
        exit(EXIT_FAILURE);
    }
    shared = (in_data*) shmat (shmid, NULL, 0);
    
    if (argc > 1) {
        SemOpMul (semid, ping1,  1, SEM_UNDO);
        SemOpMul (semid, ping2,  1, SEM_UNDO);
        SemOpMul (semid, init1, -1, SEM_UNDO);
        SemOpMul (semid, init2,  0, 0);
        //writer (Producer)
        int src = open (argv [1], O_RDONLY, 0);
        int ret_num = 1;
        while (ret_num > 0) {
            SemOp ('P', semid, empty, 0, ping1);
            SemOp ('P', semid, mutex, 0, ping1);
            ret_num = read (src, shared -> buffer, buf_size);
            shared -> num = ret_num;
            SemOp ('V', semid, mutex, 0, ping1);
            SemOp ('V', semid, full,  0, ping1);
        }
    } 
    else {
        //reader (Consumer)
        SemOpMul (semid, ping2, 1, SEM_UNDO);
        SemOpMul (semid, ping1, 1, SEM_UNDO);
        SemOpMul (semid, init2, -1, SEM_UNDO);
        SemOpMul (semid, init1,  0, 0);
        while (1) {
            SemOp ('P', semid, full,  0, ping2);
            SemOp ('P', semid, mutex, 0, ping2);
            write (1, shared -> buffer, shared -> num);
            fflush (stdout);
            SemOp ('V', semid, mutex, 0, ping2);
            SemOp ('V', semid, empty, 0, ping2);
            
        }
    }
    semctl (semid, 0, IPC_RMID, NULL);
    shmdt (shared);
    return 0;
}
