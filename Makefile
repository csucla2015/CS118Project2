CC=g++

all: server client

server: server.cpp
	$(CC) server.cpp -o server

client: client.cpp
	$(CC) client.cpp -o client

.PHONY: clean

clean:
	rm client server
