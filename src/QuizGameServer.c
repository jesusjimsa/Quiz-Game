/*
	Implement a multithreading server that supports any customer.
	The server will coordinate the clients who respond to a set of questions in turn,
	in the order in which they were registered. Each customer is asked a question and
	has a number of seconds to answer the question. The server checks the customer
	response and if it is correct it will retain the score for that client. The server
	also synchronises all clients with each other and gives each one for n seconds to
	respond. Communication between the server and the client will be done using sockets.
	All the logic will be done on the server, the client just answers the questions.
	Questions with response variants will be stored either in XML files or in a SQLite
	database. The server will manage the situations in which one of the players leaves
	the game so that the game continues smoothly.
*/

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#define true (1 == 1)
#define false (!true)

/* el puerto usado */
#define PORT 2024

/* el código de error devuelto por ciertas llamadas */
extern int errno;

/* Threads */
pthread_t tid[2];

/* struct for the players */
struct Player{
	int id;				// ID to connect to the player
	int score = 0;		// Score obtained during the game
	char username[50];	// Username
};

/* global variables to be able to catch players all the time */
struct sockaddr_in server;	// la estructura utilizada por el servidor
struct sockaddr_in from;
int sd;			//descriptor de socket
socklen_t length = sizeof(from);
Array players;

/*************************** https://stackoverflow.com/a/3536261/7071193 ***************************/
typedef struct{
	struct Player *array;
	size_t used;
	size_t size;
} Array;

void initArray(Array *a, size_t initialSize){
	a->array = (struct Player *)malloc(initialSize * sizeof(struct Player));
	a->used = 0;
	a->size = initialSize;
}

void insertArray(Array *a, struct Player element){
	// a->used is the number of used entries, because a->array[a->used++] updates a->used only *after* the array has been accessed.
	// Therefore a->used can go up to a->size
	if(a->used == a->size){
		a->size *= 2;
		a->array = (struct Player *)realloc(a->array, a->size * sizeof(struct Player));
	}

	a->array[a->used++] = element;
}

void freeArray(Array *a){
	free(a->array);
	a->array = NULL;
	a->used = a->size = 0;
}
/*****************************************************************************************************/

struct Options{
	char A[30];
	char B[30];
	char C[30];
	char D[30];
};

struct Round{
	char question[100];
	struct Options options;
};

struct ResultRound{
	char correct_answer[50];
	int points;
};

void waitForClients(){
	struct Player aux;

	while(true){
		aux.id = accept(sd, (struct sockaddr *) &from, &length)
		
		/* error al aceptar una conexión de un cliente */
		if(aux.id < 0){
			perror("[server]Error en accept().\n");
			continue;
		}
		
		if(read(aux.id, &aux.username, sizeof(aux.username)) <= 0){
			perror("[server]Error en read() del client.\n");
			close(client);	/* cerramos la conexión con el cliente */
			continue;		/* seguimos escuchando */
		}

		insertArray(&players, aux);
	}
}

int main(){
	int i = 0;
	FILE *XML_questions;

	initArray(&players, 2);

	/* Opening the file with the questions */
	if(!(XML_questions = fopen("Q&A.xml", "r"))){
		perror("[server]Error al abrir el archivo de preguntas\n");
		return errno;
	}

	/* creando un socket */
	if((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1){
		perror("[server]Error de socket().\n");
		return errno;
	}

	/* preparación de estructuras de datos */
	bzero(&server, sizeof(server));
	bzero(&from, sizeof(from));

	/* llene la estructura utilizada por el servidor */
	/* estableciendo la familia de sockets */
	server.sin_family = AF_INET;

	/* aceptamos cualquier dirección */
	server.sin_addr.s_addr = htonl(INADDR_ANY);

	/* usamos un puerto de usuario */
	server.sin_port = htons(PORT);

	/* adjuntamos el socket */
	if (bind(sd, (struct sockaddr *) &server, sizeof(struct sockaddr)) == -1){
		perror("[server]Error en bind().\n");
		return errno;
	}

	/* le pedimos al servidor que escuche si los clientes vienen a conectarse */
	if (listen(sd, 5) == -1){
		perror ("[server]Error de listen().\n");
		return errno;
	}

	/* Creation of a thread that will keep creating players */
	int err;
	err = pthread_create(&(tid[0]), NULL, &waitForClients, NULL);
	
	if (err != 0){
		printf("\nCan't create thread :[%s]", strerror(err));
	}
	else{
		printf("\n Thread created successfully\n");
	}

	while(players.used < 2);	// The server will wait until there is at least two players
}