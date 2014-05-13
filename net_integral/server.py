import argparse
import sys 
import socket 
import threading
import time

bufferSize = 1024
max_connections = 100
client_list = []
delay = 10

class Server():
    def __init__ (self, host, port, a, b, N):
        try:
            self.socket = socket.socket (socket.AF_INET, socket.SOCK_STREAM)
            self.a, self.b, self.N = a, b, N
        except socket.error as e:
            print "Can't create server"
        self.bind (host, port)
        
    def bind (self, host, port):
        try:
            self.socket.bind ((host, port))
            self.socket.listen (max_connections)
        except socket.error as e:
            print "Can't bind server"
            self.socket.close()
            sys.exit (1)
            
    def get_client (self):
        time_start = time.time ()
        while (time.time () - time_start < delay):
            try:
                self.socket.setblocking(0)
                client_socket, client_addr = self.socket.accept()
                GetClient (client_socket)
            except socket.error as e:
                pass
    
    def serve (self):
        dec = (self.b - self.a) / (len (client_list) * 1.0)
        prev = self.a
        task = []
        for i in range (len (client_list)):
            task.append (prev)
            prev += dec 
        task.append (prev)
        for i in range (len (client_list)):
            ServeClient (client_list [i], task [i], task [i+1], self.N / len (client_list))
        
class GetClient (threading.Thread):
    def __init__ (self, client_socket):
        self.socket = client_socket
        super(GetClient, self).__init__()
        self.start()
        
    def run (self):
        client_list.append (self.socket)

class ServeClient (threading.Thread):
    def __init__ (self, client_socket, a, b, N):
        self.socket = client_socket
        self.a, self.b, self.N = a, b, N
        super(ServeClient, self).__init__()
        self.start()
        
    def run (self):
        msg = str (self.a) + " " + str (self.b) + " " + str (self.N)
        self.socket.sendall (msg)
        print self.socket.recv(bufferSize)
        
        
########################################################################
HOST = "127.0.0.1"
PORT_UDP = 10345
PORT_TCP = 11345

parser = argparse.ArgumentParser (description = "Integral with Simpson's method spread to net")
parser.add_argument("a", help = "start value", type = int)
parser.add_argument("b", help = "end value", type = int)
parser.add_argument("N", help = "step number", type = int)
args = parser.parse_args()

#dest = ('<broadcast>', PORT_UDP)
#s = socket.socket (socket.AF_INET, socket.SOCK_DGRAM)
#s.bind (('', 12345))
#s.setsockopt (socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
#s.setsockopt (socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
#msg = HOST
#s.sendto (msg, dest)

server = Server (HOST, PORT_TCP, args.a, args.b, args.N)
server.get_client ()
print client_list
server.serve ()
server.socket.close ()
