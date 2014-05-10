import sys 
import socket 
import threading

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
            self.socket.listen (100)
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
        msg = self.socket.recv (1024)
        print msg
        self.socket.sendall (msg)
        msg = self.socket.recv (1024)
        print msg
        

HOST, PORT = "127.0.0.1", 10345

server = Server (HOST, PORT)
server.serve()
server.socket.close()
