all: 
	gcc source/server.c -o bin/server -Wall 
	gcc source/client.c -o bin/client -Wall 
	gcc resource/cgi-bin/user.c -o resource/cgi-bin/user -Wall 

clean:
	rm bin/server bin/client
