#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>

#define SYNC_FIFO_NAME "sync.fifo"
#define NAME_LENGTH 32
#define BUFFER_SIZE 1024

int CreateAndOpenFifo (const char *name, int open_flag);
int OpenFifo (const char *name, int flag);
void ExcludeNONBLOCK(int fifo);

int main (int argc, char *argv []) {
    if (!strcmp (argv [1], "-t")) { 
////////////////////////////////////////////////////////////////////////////////
// Talker
        int src = open (argv [2], O_RDONLY);
        if (src < 0) {
            perror ("Unable to open source file.\n");
            exit(EXIT_FAILURE);
        }
        
        int pid = getpid();
        char name_buffer [NAME_LENGTH];
        sprintf(name_buffer, "%d", pid);
        for (int i = strlen (name_buffer); i < NAME_LENGTH - 1; i++)
            name_buffer [i] = '!';
        name_buffer [NAME_LENGTH - 1] = '\0';
        
        int tmp = CreateAndOpenFifo(name_buffer, O_RDONLY | O_NONBLOCK); 
        int data_fifo = OpenFifo(name_buffer, O_WRONLY | O_NONBLOCK);
        close(tmp);     
        
        int sync_fifo = CreateAndOpenFifo(SYNC_FIFO_NAME, O_WRONLY);
        if (write(sync_fifo, name_buffer, NAME_LENGTH) < NAME_LENGTH) {
            perror ("Unable to write pid to sync.fifo\n");
            exit(EXIT_FAILURE);
        }
        close(sync_fifo);
        
        sleep(1);
        
        ExcludeNONBLOCK(data_fifo);
                
        char buffer [BUFFER_SIZE] = {}; 
        int ret_num = 1;   
        while(ret_num) {
            ret_num = read(src, buffer, BUFFER_SIZE);
            ret_num = write(data_fifo, buffer, ret_num);
        }
        
        close(data_fifo);
        close(src);
        unlink(name_buffer);
    }
    else { 
////////////////////////////////////////////////////////////////////////////////
// Listener
        int sync_fifo = CreateAndOpenFifo(SYNC_FIFO_NAME, O_RDONLY);
        
        char name_buffer [NAME_LENGTH] = {};
        int ret_num = 0;
        while (ret_num < NAME_LENGTH) {
            ret_num = read(sync_fifo, name_buffer, NAME_LENGTH);
            if (ret_num < 0) {
                perror ("Listener: unable to read from sync.fifo\n");
                exit(EXIT_FAILURE);
            }
        }
        close(sync_fifo);
        
        int data_fifo = OpenFifo (name_buffer, O_RDONLY | O_NONBLOCK);
        ExcludeNONBLOCK(data_fifo);
        
        char buffer [BUFFER_SIZE] = {};
        ret_num = 1;
        while (ret_num) {
            ret_num = read(data_fifo, buffer, BUFFER_SIZE);
            ret_num = write(1, buffer, ret_num);
        }
        
        close(data_fifo);    
    }
    
    exit (EXIT_SUCCESS);
}

int CreateAndOpenFifo (const char *name, int open_flag) {
    if ((mkfifo (name, 0644)) && (errno != EEXIST))
	{ 			
        perror ("Can't create FIFO\n");
		exit (EXIT_FAILURE);
	}
    
    int fifo_id = open (name, open_flag);
    if (fifo_id < 0) {
        perror("Can't open FIFO\n");
        exit(EXIT_FAILURE); 
    }
    
    return fifo_id;
}

int OpenFifo (const char *name, int open_flag) {
    int fifo_id = open (name, open_flag);
    if (fifo_id < 0) {
        perror("Can't open FIFO\n");
        exit(EXIT_FAILURE); 
    }
    
    return fifo_id;
}

void ExcludeNONBLOCK(int fifo) {
    int flag = fcntl (fifo, F_GETFL, 0);
    flag = flag & ~O_NONBLOCK;
    fcntl(fifo, F_SETFL, flag);
}
