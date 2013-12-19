#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

int MAX(int x, int y) {((x) > (y) ? (x) : (y));}

#define MAX_BUF_SIZE 1024*128
#define BUF_SIZE_NUM 128

struct child_io {
    int fd [2], size, position;
    char *buf;
    bool eof;
};

void Child_Start (child_io childs);

int main (int argc, char *argv []) {// ./a.out 15 in.txt
    int tmp_fd [2], prev_fd [2], max_fd;
    int N = atoi (argv [1]);
    child_io childs [N];
    
    for (int i = 0; i < N; i++) {
        pipe (tmp_fd);
        if (i == 0) {
            tmp_fd [0] = 
            childs[i].child_fd [0] = open(argv [2], O_RDONLY);
            max_fd = tmp_fd [1];
        }
        else {
            childs[i].child_fd [0] = tmp_fd [0];
            max_fd  = MAX(max_fd, tmp_fd [0]);
        }
        childs[i].parent_fd [1] = tmp_fd [1];
        fcntl(childs[i].parent_fd [1], F_SETFL, O_NONBLOCK);
        max_fd  = MAX(max_fd, tmp_fd [1]);
        
        pipe (tmp_fd);
        childs[i].child_fd [1] = tmp_fd [1];
        max_fd  = MAX(max_fd, tmp_fd [1]);
        childs[i].parent_fd [0] = tmp_fd [0];
        fcntl(childs[i].parent_fd [0], F_SETFL, O_NONBLOCK);
        max_fd  = MAX(max_fd, tmp_fd [0]);
        
        childs[i].size = 3 * BUF_SIZE_NUM * (N - i + 1);
        if (childs[i].size > MAX_BUF_SIZE)
            childs[i].size = MAX_BUF_SIZE -1;
        childs[i].buf= (char*) malloc(childs[i].size);    
        childs[i].eof = false;
        childs[i].position = 0;
            
        int pid = fork ();
        if (pid == 0) {
            for (int j = 0; j < i; j++) {
                close (childs[j].child_fd [0]);
                close (childs[j].child_fd [1]);
                close (childs[j].parent_fd [0]);
                close (childs[j].parent_fd [1]);
            }
            
            close (childs[i].parent_fd [0]);
            close (childs[i].parent_fd [1]);
            
            Child_Start(childs [i]);
            exit (EXIT_SUCCESS);
        }
        
        close (childs[i].child_fd [0]);
        close (childs[i].child_fd [1]);
    }
    
    max_fd++;
    childs[N-1].parent_fd [1] = 1;
    
    fd_set readfds, writefds;
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    for (int i = 0; i < N; i++)
        FD_SET(childs[i].parent_fd [0], &readfds);
    
    
    fd_set readfds_prev, writefds_prev;
    while (1) {
        fd_set readfds_prev = readfds;
        fd_set writefds_prev = writefds;
        select (max_fd, &readfds_prev, &writefds_prev, NULL, NULL);
        
        bool flag = false;
        int fd_num = 0;
        for (int i = 0; i < N; i++) {
            if (FD_ISSET(childs[i].parent_fd [1], &writefds_prev)) {
                flag = true;
                fd_num = i;
                break;
            }
        }
        if (flag) {
            ssize_t nwritten = write (childs[fd_num].parent_fd[1],
                                      childs[fd_num].buf, 
                                      childs[fd_num].position);
            if (nwritten < childs[fd_num].position) {
                memcpy (&childs[fd_num].buf [0], &childs[fd_num].buf [nwritten + 1],
                        sizeof (char) * (childs[fd_num].size - childs[fd_num].position));
				childs[fd_num].position -= nwritten;
			}
			else {
				childs[fd_num].position = 0;
				FD_CLR (childs[fd_num].parent_fd [1], &writefds);
				if (childs[fd_num].eof) {
					close (childs[fd_num].parent_fd [0]);
					if (fd_num != N-1)
						close (childs[fd_num].parent_fd [1]);
					else
						break;
				}
				else
					FD_SET(childs[fd_num].parent_fd [0], &readfds);
			}
        }
        
        
        flag = false;
        for (int i = 0; i < N; i++) {
            if (FD_ISSET(childs[i].parent_fd [0], &readfds_prev)) {
                flag = true;
                fd_num = i;
                break;
            }
        }
        if (flag) {
            ssize_t nread = read (childs[fd_num].parent_fd[0],
                                     childs[fd_num].buf + childs[fd_num].position, 
                                     childs[fd_num].size - childs[fd_num].position);
            if (nread == 0) {
                childs[fd_num].eof = true;
                FD_CLR (childs[fd_num].parent_fd [0], &readfds);
                if (childs[fd_num].position == 0) {
                    close(childs[fd_num].parent_fd [0]);
					if (fd_num != N-1)
						close(childs[fd_num].parent_fd [1]);
					else
						break;
				}
			}
            else if ((childs[fd_num].position += nread) == childs[fd_num].size)
                FD_CLR(childs[fd_num].parent_fd [0], &readfds);
            if (childs[fd_num].position != 0)
                FD_SET(childs[fd_num].parent_fd [0], &writefds);
        }        
    }
    
    for (int i = 0; i < N; i++)
		wait(NULL);
    exit (EXIT_SUCCESS);
}

void Child_Start (child_io childs) {
    char *buf = (char*) calloc(BUF_SIZE_NUM, sizeof (char));
    ssize_t ret_num = 1;
    while (1) {
        ret_num = read (childs.child_fd [0], buf, MAX_BUF_SIZE);
        sleep(10);
        if (ret_num == 0) 
            break;
       
        while (ret_num > 0) {
            ret_num = write (childs.child_fd [1], buf, ret_num); 
            memcpy (&buf [0], &buf [ret_num + 1],
                    sizeof (char) * (BUF_SIZE_NUM - ret_num));
        }
    }
    close (childs.child_fd [0]);
    close (childs.child_fd [1]);
    exit (EXIT_SUCCESS);
}
