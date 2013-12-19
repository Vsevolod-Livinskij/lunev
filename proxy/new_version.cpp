#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

static inline int MAX(int lhs, int rhs) {
    if(lhs > rhs)
        return lhs;
    else
        return rhs;
}

struct child_io {
    int ch_fd [2], par_fd [2], size, pos;
    char *buf;
    bool eof;
};

#define MAX_BUF_SIZE 1021*128
#define BUF_SIZE_NUM 128

void Child_Start (child_io child);

int main (int argc, char* argv [0]) {
    if (argc != 3){
		perror ("Wrong number of arguments\n");
		exit (EXIT_FAILURE);
	}
    int N = 0;
	if (sscanf(argv[1], "%u", &N) == 0){
		perror("Second argument is not number\n");
		exit (EXIT_FAILURE);
	}
    
    int fd_tmp [2], fd_max = 0;
    
    child_io child [N];
    
    for (int i = 0; i < N; i++) {
        if (i != 0) {
            pipe (fd_tmp);
            child[i].ch_fd [0] = fd_tmp [0];
            fd_max  = MAX(fd_max, fd_tmp [0]);
            
            child[i - 1].par_fd [1] = fd_tmp [1];
            fcntl(child[i - 1].par_fd [1], F_SETFL, O_NONBLOCK);
            fd_max  = MAX(fd_max, fd_tmp [1]);
        }
        
        pipe (fd_tmp);
        child[i].ch_fd [1] = fd_tmp [1];
        fd_max  = MAX(fd_max, fd_tmp [1]);
        
        child[i].par_fd [0] = fd_tmp [0];
        fcntl(child[i].par_fd [0], F_SETFL, O_NONBLOCK);
        fd_max  = MAX(fd_max, fd_tmp [0]);
        
        child[i].size = 3 * 1021 * (N - i + 1);
        if (child[i].size > MAX_BUF_SIZE)
            child[i].size = MAX_BUF_SIZE -1;
        child[i].buf= (char*) malloc(child[i].size);    
        child[i].eof = false;
        child[i].pos = 0;
            
        if (i == 0) 
                child[i].ch_fd [0] = open(argv [2], O_RDONLY);    
        
        int pid = fork ();
        if (pid == 0) {
            for (int j = 0; j < i; j++) {
                close (child[j].par_fd [0]);
                close (child[j].par_fd [1]);
            }
            close (child[i].par_fd [0]);
            
            if (i == 0) 
                child[i].ch_fd [0] = open(argv [2], O_RDONLY);
                
            Child_Start(child [i]);
        }
        
        close(child[i].ch_fd[0]);
        close(child[i].ch_fd[1]);
    }
    
    child[N - 1].par_fd [1] = 1;
    fd_max++;
    
    fd_set read_fds, write_fds;
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
        
    for (int i = 0; i < N; i++)
    {
        FD_SET (child[i].par_fd[0], &read_fds);
    }
    
    fd_set readfds_prev, writefds_prev;
    
    while (1) {
        readfds_prev = read_fds;
        writefds_prev = write_fds;
        select (fd_max, &readfds_prev, &writefds_prev, NULL, NULL);
        
        bool flag = false;
        int fd_num = 0;
        for (int i = 0; i < N; i++) {
            if (FD_ISSET(child[i].par_fd [0], &readfds_prev)) {
                flag = true;
                fd_num = i;
                break;
            }
        }
        if (flag) {
            ssize_t nread = read (child[fd_num].par_fd[0],
                                     child[fd_num].buf + child[fd_num].pos, 
                                     child[fd_num].size - child[fd_num].pos);
            if (nread == 0) {
                child[fd_num].eof = true;
                FD_CLR (child[fd_num].par_fd [0], &read_fds);
                if (child[fd_num].pos == 0) {
                    close(child[fd_num].par_fd [0]);
					if (fd_num != N-1)
						close(child[fd_num].par_fd [1]);
					else
						break;
				}
			}
            else if ((child[fd_num].pos += nread) == child[fd_num].size)
                FD_CLR(child[fd_num].par_fd [0], &read_fds);
            if (child[fd_num].pos != 0)
                FD_SET(child[fd_num].par_fd [1], &write_fds);
        }
        
        flag = false;
        for (int i = 0; i < N; i++) {
            if (FD_ISSET(child[i].par_fd [1], &writefds_prev)) {
                flag = true;
                fd_num = i;
                break;
            }
        }
        if (flag) {
            ssize_t nwritten = write (child[fd_num].par_fd[1],
                                      child[fd_num].buf, 
                                      child[fd_num].pos);
            if (nwritten < child[fd_num].pos) {
                //memcpy (&child[fd_num].buf [0], &child[fd_num].buf [nwritten + 1],
                //        sizeof (char) * (child[fd_num].size - child[fd_num].pos));
                for (int j= 0; j < (child[fd_num].pos-nwritten); j++)
					child[fd_num].buf[j]= child[fd_num].buf[j+nwritten];
				child[fd_num].pos -= nwritten;
			}
			else {
				child[fd_num].pos = 0;
				FD_CLR (child[fd_num].par_fd [1], &write_fds);
				if (child[fd_num].eof) {
					close (child[fd_num].par_fd [0]);
					if (fd_num != N-1)
						close (child[fd_num].par_fd [1]);
					else
						break;
				}
				else
					FD_SET(child[fd_num].par_fd [0], &read_fds);
			}
        }        
    }
    
    for (int i = 0; i < N; i++)
		wait(NULL);
    exit (EXIT_SUCCESS);    
}

void Child_Start (child_io child) {
    char *buf = (char*) calloc(BUF_SIZE_NUM, sizeof (char));
    ssize_t ret_num = 1;
    while (1) {
        ret_num = read (child.ch_fd [0], buf, BUF_SIZE_NUM * sizeof (char));
        if (ret_num == 0) 
            break;
       
        while (ret_num > 0) {
            //ret_num = write (child.ch_fd [1], buf, ret_num);
            int n = write (child.ch_fd [1], buf, ret_num);
            for (int i = 0; i < (ret_num - n); i++)
				buf [i]= buf [i + n];
			ret_num -= n; 
            //memcpy (&buf [0], &buf [ret_num + 1],
            //        sizeof (char) * (BUF_SIZE_NUM - ret_num));
        }
    }
    close (child.ch_fd [0]);
    close (child.ch_fd [1]);
    exit (EXIT_SUCCESS);
}
