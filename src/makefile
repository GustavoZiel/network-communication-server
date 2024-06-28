CC = gcc
CFLAGS = -Wall -Werror

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
