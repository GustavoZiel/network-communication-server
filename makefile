# TODO Tirar Valgrind e colocar flags de compilação

CC = gcc
CFLAGS = 
CFLAGS = -Wall -Werror
valgrind = 
# valgrind = valgrind --leak-check=full --show-leak-kinds=all

server: server.c 
	$(CC) $^ -o $@ $(CFLAGS)
	
client: client.c 
	$(CC) $^ -o $@ $(CFLAGS)

clean:
	rm server client

runServer: server
	$(valgrind) ./$^ $(ARGS)
	
runClient: client
	$(valgrind) ./$^ $(ARGS)
