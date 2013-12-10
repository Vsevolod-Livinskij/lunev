#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <time.h>

#define BUF_SIZE 1024

enum semaphore_num {
    empty = 0,
    full = 1,
    reader = 2,
    writer = 3,
    //mutex = 4,
    num_sem
};

struct in_data {
    char buffer [BUF_SIZE];
    int size;
};

union semun {
    int val;
    struct semid_ds *buf;
    ushort *array;
};

int SEMID;
in_data *SHMID;

int init_sem (int num_sem, key_t key);
in_data *init_shm (key_t key);
void exist (int semid, int sem);
void wait_sem (int semid, int sem);
void set_val (int semid);
void consume (int semid, in_data *shared);
void produce (int semid, in_data *shared, int src);
void P (int semid, int sem, int sem_mod, int sem_exist);
void V (int semid, int sem, int sem_mod, int sem_exist);

int main (int argc, char *argv []) {
    key_t key = ftok (argv [0], 0);
    int semid = init_sem (num_sem, key);
    in_data *shared = init_shm (key);
    SEMID = semid;
    SHMID = shared;
        
    if (argc > 1) {
        //printf ("writer");
        //printf("%d\n", semctl(semid, writer, GETVAL, 0));
        exist (semid, writer);
        set_val (semid);
        wait_sem (semid, reader);
        int src = open (argv[1], O_RDONLY);
        produce (semid, shared, src);
    }
    else {
        //printf ("reader");
        //printf("%d\n", semctl(semid, reader, GETVAL, 0));
        exist (semid, reader);
        wait_sem (semid, writer);
        consume (semid, shared);
    }
    return 0;
}

int init_sem (int num_sem, key_t key) {
    int semid = semget (key, num_sem, 0644 | IPC_CREAT);
    if (semid < 0) {
        perror ("Failed to create semaphore");
        semctl (SEMID, 0, IPC_RMID, NULL);
        shmdt (SHMID);
        exit (EXIT_FAILURE);
    }
    return semid;
}

void exist (int semid, int sem) {
    sembuf ops [2];

    ops [0].sem_num = sem;
    ops [0].sem_op = 0;
    ops [0].sem_flg = IPC_NOWAIT;

    ops [1].sem_num = sem;
    ops [1].sem_op = 1;
    ops [1].sem_flg = SEM_UNDO;

    if(semop(semid, ops, 2) < 0){
        semctl (SEMID, 0, IPC_RMID, NULL);
        shmdt (SHMID);
        exit(EXIT_FAILURE);
    }
}

void wait_sem (int semid, int sem) {
    sembuf ops [2];

    ops [0].sem_num = sem;
    ops [0].sem_op = -1;
    ops [0].sem_flg = 0;

    ops [1].sem_num = sem;
    ops [1].sem_op = 1;
    ops [1].sem_flg = 0;

    if(semop(semid, ops, 2) < 0) {
        semctl (SEMID, 0, IPC_RMID, NULL);
        shmdt (SHMID);
        exit (EXIT_FAILURE);  
    }
}

void set_val (int semid) {
    union semun semopts [num_sem];
    semopts [full  ].val = 0;
    semopts [empty ].val = 1;
    //semopts [mutex ].val = 1;

    if (((semctl (semid, full,   SETVAL, semopts [full ])) == -1) ||
        //((semctl (semid, mutex,   SETVAL, semopts [mutex ])) == -1) ||
        ((semctl (semid, empty,  SETVAL, semopts [empty])) == -1)) {
        semctl (SEMID, 0, IPC_RMID, NULL);
        shmdt (SHMID);
        exit (EXIT_FAILURE);
    }
}

in_data* init_shm (key_t key) {
    int shmid = shmget (key, sizeof (in_data), 0644 | IPC_CREAT);
    if (shmid < 0) {
        perror ("Failed to create memory");
        semctl (SEMID, 0, IPC_RMID, NULL);
        shmdt (SHMID);
        exit (EXIT_FAILURE);
    }

    in_data *shared = (in_data*) shmat (shmid, NULL, 0); 
    return shared;
}

void produce (int semid, in_data *shared, int src) {
    while(1)
    {
        P (semid, empty, full, reader);
        //printf("%d\n", semctl(semid, mutex, GETVAL, 0));
        //fflush(stdout);
        //P (semid, mutex, reader);
        
        shared -> size = read(src, shared -> buffer, BUF_SIZE);
        //V (semid, mutex, reader);
        V (semid, full, empty, reader);
    }
}

void consume (int semid, in_data *shared) {
    unsigned char buf [BUF_SIZE + 1];
    memset(buf, '\0', BUF_SIZE + 1 );

    int size = 1;
    while (size > 0)
    {
        P (semid, full, empty, writer);
        //printf("%d\n", semctl(semid, mutex, GETVAL, 0));
        //fflush(stdout);
        //P (semid, mutex, writer);
        
        write (1, shared -> buffer, shared -> size);
        size = shared -> size;
        fflush (stdout);
        //V (semid, mutex, writer);
        V (semid, empty, full, writer);
    }
}

void P (int semid, int sem, int sem_mod, int sem_exist) {
    sembuf ops [3];

    ops [0].sem_num = sem_exist;
    ops [0].sem_op = -1;
    ops [0].sem_flg = IPC_NOWAIT;

    ops [2].sem_num = sem_mod;
    ops [2].sem_op = -1;
    ops [2].sem_flg = IPC_NOWAIT;

    ops [1].sem_num = sem;
    ops [1].sem_op = -1;
    ops [1].sem_flg = SEM_UNDO;

    ops [2].sem_num = sem_mod;
    ops [2].sem_op  = 1;
    ops [2].sem_flg = SEM_UNDO;

    ops [2].sem_num = sem_exist;
    ops [2].sem_op = 1;
    ops [2].sem_flg = 0;

    if(semop(semid, ops, 3) < 0) {
        semctl (SEMID, 0, IPC_RMID, NULL);
        shmdt (SHMID);
        exit(EXIT_FAILURE);    
    }
}

void V (int semid, int sem, int sem_mod, int sem_exist) {
    sembuf ops [3];

    ops [0].sem_num = sem_exist;
    ops [0].sem_op = -1;
    ops [0].sem_flg = IPC_NOWAIT;

    ops [2].sem_num = sem_mod;
    ops [2].sem_op  = 1;
    ops [2].sem_flg = SEM_UNDO;

    ops [1].sem_num = sem;
    ops [1].sem_op = 1;
    ops [1].sem_flg = SEM_UNDO;

    ops [2].sem_num = sem_mod;
    ops [2].sem_op = -1;
    ops [2].sem_flg = IPC_NOWAIT;

    ops [2].sem_num = sem_exist;
    ops [2].sem_op = 1;
    ops [2].sem_flg = 0;

    if(semop(semid, ops, 3) < 0) {
        semctl (SEMID, 0, IPC_RMID, NULL);
        shmdt (SHMID);
        exit(EXIT_FAILURE);    
    }
}
