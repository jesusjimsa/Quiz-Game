BIN = bin
SRC = src
CFLAGS = gcc -O3 -g -Wall -pthread

all: client server

client: $(SRC)/QuizGameClient.c
	$(CFLAGS) $(SRC)/QuizGameClient.c -o $(BIN)/client

server: $(SRC)/QuizGameServer.c
	$(CFLAGS) $(SRC)/QuizGameServer.c -o $(BIN)/server

clean:
	echo "Limpiando..."
	rm -r $(BIN)/*