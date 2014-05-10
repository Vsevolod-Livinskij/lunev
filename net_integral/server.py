import sys 
import socket 
import threading

bufferSize = 1024
max_connections = 100

class Server():
    def __init__ (self, host, port):
        try:
            self.socket = socket.socket (socket.AF_INET, socket.SOCK_STREAM)
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
            
    def serve (self):
        client_socket, client_addr = self.socket.accept()
        ClientHandler (client_socket)
        

class ClientHandler (threading.Thread):
    def __init__ (self, client_socket):
        self.socket = client_socket
        super(ClientHandler, self).__init__()
        self.start()
        
    def run (self):
        msg = self.socket.recv (bufferSize)
        print msg
        self.socket.sendall (msg)
        msg = self.socket.recv (bufferSize)
        print msg
        
########################################################################
HOST = "192.168.1.45"
PORT_UDP = 10345
PORT_TCP = 11345

dest = ('<broadcast>', PORT_UDP)
s = socket.socket (socket.AF_INET, socket.SOCK_DGRAM)
s.bind (('', 0))
s.setsockopt (socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s.setsockopt (socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
s.sendto (HOST, dest)

server = Server (HOST, PORT_TCP)
server.serve ()
server.socket.close ()
