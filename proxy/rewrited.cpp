#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_BUF_SIZE 123541 // 121*1021
#define CHILD_BUF 20482
#define BUF_SIZE 1021

typedef int buf_t; 

enum I_O_Descriptor {
    in = 0,
    out = 1,
    max_direc
};

class Child {
public:
    int num;
    buf_t size, position;
    char *buffer;
    int fd [max_direc];
    bool full, eof, empty;
    
    Child () {}
    
    ~Child () {
        delete (buffer);
        close (fd [in]);
        close (fd [out]);
    }

    void set_value (int _num, buf_t _size);
    void receive ();
    void send ();
};

void Child::set_value (int _num, buf_t _size) {
        num = _num;
        size = _size;
        position = 0;
        buffer = new char [size];
        full = false;
        empty = true;
        eof = false;
}

void Child::receive () {
    int ret_num = read (fd [in], &(buffer [position]), size - position);
    
    /*
    fprintf (stderr, "\n!!Parent #%d read %d bytes\n", num, ret_num);
    //fflush (stderr);*/
    
    if (ret_num <= 0) 
        eof = true; 
        
    if (ret_num > 0)
        position += ret_num;
    if (position == size) 
        full = true;   
    /*
    if (position == 0)
        empty = true;*/
};

void Child::send () {
    int ret_num = write (fd [out], buffer, position);
    /*
    fprintf (stderr, "\n!!Parent #%d write %d bytes\n", num, ret_num);
    fflush (stderr);*/
    
    if (ret_num > 0) {
        full = false;
        memcpy (buffer, &(buffer [ret_num]), position - ret_num);
        /*
        for (buf_t i = 0; i < position - ret_num; i++)
            buffer[i] = buffer[i + ret_num];*/
        position -= ret_num;
    }
    else position = 0;
    
    if (position == 0)
        empty = true;
}

class Receiver {
public:
    int num;
    buf_t size, position;
    int fd [max_direc];
    char buffer [CHILD_BUF];
    
    Receiver () {}
    
    ~Receiver () {
        close (fd [in]);
        close (fd [out]);
        exit (EXIT_SUCCESS);
    }
    
    void commit ();
    void set_value (int _num);
    void receive ();
    void send ();
};

void Receiver::commit () {
    while (1) {
        receive ();
        send ();
    }
}

void Receiver::set_value (int _num) {
    num = _num;
    size = CHILD_BUF;
    position = 0;
}

void Receiver::receive () {
    position = read (fd [in], buffer, size);

    if (position <= 0) {
        close (fd [in]);
        close (fd [out]);
        exit (EXIT_SUCCESS);
    }
    
    /*
    if (num ==  && position != 0) {
        fprintf (stderr, "\n!!Child #%d read %d bytes\n", num, position);
        fflush (stderr);
    }*/
}

void Receiver::send () {
    int tmp_num = position;
    while (tmp_num > 0) {
        int ret_num = write (fd [out], buffer, position);
        /*
        fprintf (stderr, "\n!!Child #%d write %d bytes\n", num, ret_num);
        fflush (stderr);*/
        memcpy (&(buffer [0]), &(buffer [ret_num]), size - ret_num);
        /*
        for (buf_t i = 0; i < size - ret_num; i++)
            buffer[i]= buffer[i + ret_num];*/
        tmp_num -= ret_num;
    }
}



int main (int argc, char *argv []) {
    
    if (argc != 3){
		perror ("Wrong number of arguments\n");
		exit (EXIT_FAILURE);
	}
    
    int child_num = 0;
	if (sscanf(argv[1], "%u", &child_num) == 0){
		perror("Second argument is not number\n");
		exit (EXIT_FAILURE);
	}
    
    Child *child = new Child [child_num];
    Receiver *receivers = new Receiver [child_num];
    
    int fd_prev [max_direc], fd_tmp [max_direc];
    
    for (int i = 0; i < child_num; i++) {
        receivers[i].set_value (i);
        int size = BUF_SIZE * 3 * (child_num - i + 1);
            if (size > MAX_BUF_SIZE)
                size = MAX_BUF_SIZE;
        child[i].set_value (i, CHILD_BUF);
        
        if (i == 0) {
            receivers[i].fd [in] = open(argv [2], O_RDONLY);
        }
        else {
            receivers[i].fd [in] = fd_prev [in];
        }
        
        pipe (fd_tmp);
        fcntl (fd_tmp [in], F_SETFL, O_NONBLOCK);
        child[i].fd [in] = fd_tmp [in];
        receivers[i].fd [out] = fd_tmp [out];
        
        pipe (fd_prev);
        fcntl (fd_prev [out], F_SETFL, O_NONBLOCK);
        child[i].fd [out] = fd_prev [out];
  
        int pid = fork ();
        if (pid == 0) {
            for (int j = 0; j <= i; j++) {
                close (child[j].fd [in]);
                close (child[j].fd [out]);
            }
            /*
            close (child[i].fd [out]);
            close (child[i].fd [in]);
            */
            receivers[i].commit ();
        }
        
        close (receivers[i].fd [in]);
        close (receivers[i].fd [out]);
    }
        
    child[child_num - 1].fd [out] = 1;
    
    fd_set read_fds, write_fds;
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
        
    int fd_max = 0;
    for (int i = 0; i < child_num; i++) {
        if (child[i].fd [in] > fd_max)
            fd_max = child[i].fd [in];
        if (child[i].fd [out] > fd_max)
            fd_max = child[i].fd [out];
            
        FD_SET (child[i].fd [in], &read_fds);
    }
    
    fd_set readfds_prev, writefds_prev;
    
    int control_sum = child_num; 
    while(control_sum) {
        readfds_prev = read_fds;
        writefds_prev = write_fds;
        /*
        bool flag = true;
        for (int i = 0; i < child_num; i++) {
            if (FD_ISSET(child[i].fd [in], &readfds_prev) ||
                FD_ISSET(child[i].fd [out], &writefds_prev))
                flag = false;
        }
        if (flag)
            break;*/
        select (fd_max + 1, &readfds_prev, &writefds_prev, NULL, NULL);
        
        
        for (int i = 0; i < child_num; i++) {
            if (FD_ISSET(child[i].fd [in], &readfds_prev)) {
                child[i].receive();
                if (child[i].eof)                           // Unable to read
                    FD_CLR(child[i].fd [in], &read_fds);
                if (child[i].full) 
                    FD_CLR(child[i].fd [in], &read_fds);
                
                FD_SET(child[i].fd [out], &write_fds);
            }
                
            if (FD_ISSET(child[i].fd [out], &writefds_prev)) {
                child[i].send();
                if (child[i].eof && child[i].empty) {       // End of transmission
                    control_sum--;
                    FD_CLR (child[i].fd [out], &write_fds);
                    FD_CLR(child[i].fd [in], &read_fds);
                    close (child[i].fd [in]);
                    if (i != child_num - 1)
                        close (child[i].fd [out]);
                    continue;
                }                    
                if (child[i].empty)
                    FD_CLR (child[i].fd [out], &write_fds); // Unable to write
                    
                FD_SET(child[i].fd [in], &read_fds);
            }
        }
    }
    
    for (int i = 0; i < child_num; i++)
        wait(NULL);
    
    exit (EXIT_SUCCESS);
}
