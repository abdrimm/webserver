all: bin bin/server bin/client
	
bin:
	mkdir bin

bin/server: src/server.c
	gcc server.c -o bin/server -Wall -lm -fsanitize=undefined -fsanitize=address

bin/client: src/client.c
	gcc client.c -o bin/client -Wall -lm -fsanitize=undefined -fsanitize=address

clean:
	rm bin/server bin/client
	rmdir bin
