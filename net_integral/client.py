import socket
import sys

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
 
    def send (self, msg):
        self.socket.sendall (msg)
        print self.socket.recv(1024)
 
 
HOST, PORT = "127.0.0.1", 10345
client = Client (HOST, PORT)
client.send("haha")
client.socket.close()
