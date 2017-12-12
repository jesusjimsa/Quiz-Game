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

/**
 * @todo Parse xml file and save it in Round structure, parse the entire file at the
 * beginning of the execution and save it in an array of rounds
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
	int score;		// Score obtained during the game
	char username[50];	// Username
};

struct Options{
	char A[50];
	char B[50];
	char C[50];
	char D[50];
};

struct Round{
	char question[100];
	struct Options options;
	char correct_answer;
};

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

/*************************** https://stackoverflow.com/a/3536261/7071193 ***************************/
typedef struct{
	struct Round *array;
	size_t used;
	size_t size;
} ArrayRound;

void initArrayRound(ArrayRound *a, size_t initialSize){
	a->array = (struct Round *)malloc(initialSize * sizeof(struct Round));
	a->used = 0;
	a->size = initialSize;
}

void insertArrayRound(ArrayRound *a, struct Round element){
	// a->used is the number of used entries, because a->array[a->used++] updates a->used only *after* the array has been accessed.
	// Therefore a->used can go up to a->size
	if(a->used == a->size){
		a->size *= 2;
		a->array = (struct Round *)realloc(a->array, a->size * sizeof(struct Round));
	}

	a->array[a->used++] = element;
}

void freeArrayRound(ArrayRound *a){
	free(a->array);
	a->array = NULL;
	a->used = a->size = 0;
}
/*****************************************************************************************************/

/* global variables to be able to catch players all the time */
struct sockaddr_in server;	// la estructura utilizada por el servidor
struct sockaddr_in from;
int sd;			//descriptor de socket
socklen_t length = sizeof(from);
Array players;
ArrayRound rounds;

void waitForClients(){
	struct Player aux;

	aux.score = 0;

	while(true){
		aux.id = accept(sd, (struct sockaddr *) &from, &length)
		
		/* error al aceptar una conexión de un cliente */
		if(aux.id < 0){
			perror("[server]Error in accept().\n");
			continue;
		}
		
		if(read(aux.id, &aux.username, sizeof(aux.username)) <= 0){
			perror("[server]Error in read() from client.\n");
			close(client);	/* cerramos la conexión con el cliente */
			continue;		/* seguimos escuchando */
		}

		insertArray(&players, aux);
	}
}

void XMLParser(FILE *XML_questions){
	char line[256];
	char lineQ[13];
	char lineOA[13];
	char lineOB[13];
	char lineOC[13];
	char lineOD[13];
	char lineAns[11];
	struct Round ronda;
	int i;

 	while(fgets(line, sizeof(line), XML_questions)) {
		//This is done to choose detect the labels
		for(i = 0; i < 12; i++){
			lineQ[i] = line[i];
			lineOA[i] = line[i];
			lineOB[i] = line[i];
			lineOC[i] = line[i];
			lineOD[i] = line[i];

			if(i < 10){
				lineAns[i] = line[i];
			}
		}

		lineQ[12] = '\0';
		lineOA[12] = '\0';
		lineOB[12] = '\0';
		lineOC[12] = '\0';
		lineOD[12] = '\0';
		lineAns[10] = '\0';

		if(!strcmp("<quiz>\n", line)){
			continue;
		}
		else{
			if(!strcmp("\t<round>\n", line)){
				continue;
			}
			else{
				if(!strcmp("\t\t<question>", lineQ)){
					int j = 12;

					while(line[j] != '<' && line[j + 1] != '/'){
						ronda.question[j - 12] = line[j];

						j++;
					}

					ronda.question[j - 12] = '\0';
				}
				else{
					if(!strcmp("\t\t<options>\n", line)){
						continue;
					}
					else{
						if(!strcmp("\t\t\t<optionA>", lineOA)){
							int j = 12;

							while(line[j] != '<' && line[j + 1] != '/'){
								ronda.options.A[j - 12] = line[j];

								j++;
							}

							ronda.options.A[j - 12] = '\0';
						}
						else{
							if(!strcmp("\t\t\t<optionB>", lineOB)){
								int j = 12;

								while(line[j] != '<' && line[j + 1] != '/'){
									ronda.options.B[j - 12] = line[j];

									j++;
								}

								ronda.options.B[j - 12] = '\0';
							}
							else{
								if(!strcmp("\t\t\t<optionC>", lineOC)){
									int j = 12;

									while(line[j] != '<' && line[j + 1] != '/'){
										ronda.options.C[j - 12] = line[j];

										j++;
									}

									ronda.options.C[j - 12] = '\0';
								}
								else{
									if(!strcmp("\t\t\t<optionD>", lineOD)){
										int j = 12;

										while(line[j] != '<' && line[j + 1] != '/'){
											ronda.options.D[j - 12] = line[j];

											j++;
										}

										ronda.options.D[j - 12] = '\0';
									}
									else{
										if(!strcmp("\t\t</options>\n", line)){
											continue;
										}
										else{
											if(!strcmp("\t\t<answer>", lineAns)){
												ronda.correct_answer = line[10];
											}
											else{
												if(!strcmp("\t</round>\n", line)){
													insertArrayRound(&rounds, ronda);
												}
												else{
													if(!strcmp("\n", line)){
														continue;
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
    }
}

int main(){
	int i = 0;
	FILE *XML_questions;

	initArray(&players, 2);
	initArrayRound(&rounds, 2);

	/* Opening the file with the questions */
	if(!(XML_questions = fopen("Q&A.xml", "r"))){
		perror("[server]Error opening file with questions\n");
		return errno;
	}

	/* creando un socket */
	if((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1){
		perror("[server]Error in socket().\n");
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
		perror("[server]Error in bind().\n");
		return errno;
	}

	/* le pedimos al servidor que escuche si los clientes vienen a conectarse */
	if (listen(sd, 5) == -1){
		perror ("[server]Error in listen().\n");
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