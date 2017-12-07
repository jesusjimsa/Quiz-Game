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

/* el puerto usado */
#define PORT 2024

/* el código de error devuelto por ciertas llamadas */
extern int errno;

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

int main(){
	struct sockaddr_in server;	// la estructura utilizada por el servidor
	struct sockaddr_in from;
	int sd;			//descriptor de socket
	FILE *XML_questions;

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
}