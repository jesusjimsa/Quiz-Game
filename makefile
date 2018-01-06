BIN = bin
SRC = src
CFLAGS = gcc -O3 -g -Wall

all: client server

client: $(SRC)/QuizGameClient.c
	$(CFLAGS) $(SRC)/QuizGameClient.c -o $(BIN)/client

server: $(SRC)/QuizGameServer.c
	$(CFLAGS) $(SRC)/QuizGameServer.c -o $(BIN)/server

prueba: 
	$(CFLAGS) $(SRC)/prueba.c -o $(BIN)/prueba
	
clean:
	echo "Limpiando..."
	rm $(BIN)/*