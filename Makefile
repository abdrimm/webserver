all: bin bin/server bin/client
	
bin:
	mkdir bin
	mkdir resource/cgi-bin

bin/server: source/server.c
	gcc server.c -o bin/server -Wall -lm -fsanitize=undefined -fsanitize=address

bin/client: source/client.c
	gcc client.c -o bin/client -Wall -lm -fsanitize=undefined -fsanitize=address

resource/cgi-bin/hello-world: resource/cgi-source/hello-world.c
	gcc hello-world.c -o resource/cgi-bin/hello-world

clean:
	rm bin/server bin/client
	rmdir bin
