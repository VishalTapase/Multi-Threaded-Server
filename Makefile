CC = g++ -Wno-write-strings
SERVER_FILE = server.cpp
HTTP_SERVER_FILE = http_server.cpp

all: server client

server: $(SERVER_FILE) $(HTTP_SERVER_FILE)
	$(CC) $(SERVER_FILE) $(HTTP_SERVER_FILE) -o server

load_gen: load_gen.c
	gcc load_gen.c -o load_gen

clean:
	rm -f server load_gen