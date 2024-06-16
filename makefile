# TODO Tirar Valgrind e colocar flags de compilação

CC = gcc
CFLAGS = 
# CFLAGS = -Wall -Werror
# valgrind = 
valgrind = valgrind --leak-check=full --show-leak-kinds=all

ms1:
	gnome-terminal -- bash -c "echo Hello, World!; exec bash"
	gnome-terminal -- bash -c "echo Hello!; exec bash"
	gnome-terminal -- bash -c "pwd; exec bash"

ms2:
	make server
	make client
	gnome-terminal -- bash -c "make runServer; exec bash"
	gnome-terminal -- bash -c "make runClient; exec bash"

espaco a:
	echo "Hello World"

server: server.c 
	$(CC) $^ -o $@ $(CFLAGS)
	
client: client.c 
	$(CC) $^ -o $@ $(CFLAGS)

all: server client

clean:
	rm server client

runServer: server
	$(valgrind) ./$^
	
runClient: client
	$(valgrind) ./$^
