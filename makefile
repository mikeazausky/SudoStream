all: server client

server: ./src/server.c
	g++ -Wall ./src/server.c -o ./bin/server


client: ./src/client.c
	g++ -Wall ./src/client.c -o ./bin/client