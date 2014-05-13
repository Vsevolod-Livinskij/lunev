import socket
import sys
import select
import hello

bufferSize = 1024

def foo(task):
    res = hello.count(task [0], task [1], task [2])
    return res


class Client():
    def __init__ (self, host, port):
        try:
            self.socket = socket.socket (socket.AF_INET, socket.SOCK_STREAM)
        except socket.error as e:
            print "Can't create client"
 
        self.connect (host, port)

    def connect (self, host, port):
        try:
            self.socket.connect ((host, port))
        except socket.error as e:
            print "Can't connect"
            self.socket.close()
            sys.exit (1)
 
    def send (self):
        msg = self.socket.recv(bufferSize)
        data = msg.split ()
        data [0], data [1] = float (data [0]), float (data [1])
        data [2] = int (data [2])
        print msg
        print data
        self.socket.sendall (str (foo ((data [0], data [1], data [2]))))
 
########################################################################
 
HOST = "127.0.0.1"
PORT_UDP = 10345
PORT_TCP = 11345

#s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
#s.bind(("", PORT_UDP))
##result = select.select([s],[],[])
##msg = result[0][0].recv(bufferSize) 
#msg, addr = s.recvfrom (bufferSize)
#print "Server IP: " + msg

client = Client (HOST, PORT_TCP)
client.send()
client.socket.close()
