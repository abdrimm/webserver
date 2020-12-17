all: 
	gcc source/server.c -o bin/server
	gcc source/client.c -o bin/client
	gcc resource/cgi-bin/user.c -o resource/cgi-bin/user

clean:
	rm bin/server bin/client
