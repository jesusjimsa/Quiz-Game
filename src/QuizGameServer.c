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
 * @todo Just the first player receives question, and just one time
 * the rest of the players and rounds don't work
*/

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define true (1 == 1)
#define false (!true)

/* el puerto usado */
#define PORT 2024

/* el código de error devuelto por ciertas llamadas */
extern int errno;

/* Threads */
pthread_t tid[5];		// Max 50 simultaneous games (with infinite players each)

/* struct for the players */
struct Player{
	int id;				// ID to connect to the player
	int score;			// Score obtained during the game
	char username[50];	// Username
};

struct Round{
	char question[100];
	char A[50];
	char B[50];
	char C[50];
	char D[50];
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
/***************************************************************************************************/

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
/***************************************************************************************************/

/* global variables to be able to catch players all the time */
struct sockaddr_in server;	// la estructura utilizada por el servidor
struct sockaddr_in from;
int sd;			//descriptor de socket
int semaphore = -1;
const char EMPTY = '\0';
socklen_t length = sizeof(from);
Array players[50];
ArrayRound rounds;

void waitForClients(int *current_game){
	struct Player aux;

	aux.score = 0;

	while(true){
		aux.id = accept(sd, (struct sockaddr *) &from, &length);

		/* error al aceptar una conexión de un cliente */
		if(aux.id < 0){
			perror("[server]Error in accept().\n");
			continue;
		}

		if(recv(aux.id, &aux.username, sizeof(aux.username), 0) <= 0){
			perror("[server]Error in recv() from client.\n");
			close(aux.id);	/* cerramos la conexión con el cliente */
			continue;		/* seguimos escuchando */
		}

		insertArray(&players[*current_game], aux);
		printf("Player connected: %s\n", aux.username);
		memset(aux.username, 0, sizeof(aux.username));
	}
}

void game(int *current_game){
	pthread_t searching;
	int i, j;
	int add_points;
	int current = *current_game;
	const int numberOfRounds = 2;
	char *result = "\0";
	char *resultUntilNow = "\0";
	char *newResult = "\0";
	
	printf("Waiting for at least two players\n");

	/* Creation of a thread that will keep creating players */
	int err;
	err = pthread_create(&(searching), NULL, &waitForClients, &(*current_game));

	if(err != 0){
		printf("\nCan't create thread :[%s]", strerror(err));
	}

	// The server will wait until there is at least two players
	while(players[*current_game].used < 2){
		/*
			For some reason, if it doesn't prints anything, it doesn't works.
			To fix this, I print something, but it is the empty character '\0',
			then, nothing is actually printed, but it works.
		*/
		printf("%c", EMPTY);
	}

	printf("Now, we will wait ten seconds to wait for more players\n");
	sleep(5);	// After two players have joined, we give 10 seconds to the rest of the players to join in time
	printf("Five more seconds to begin\n");
	sleep(5);
	printf("Let's go!\n");

	pthread_cancel(searching);	// We won't wait any more players in this game

	semaphore = 1;	// The next game can begin

	// Players that are here since the beginning of the game will participate in 10 rounds
	for(i = 0; i < numberOfRounds; i++){
		for(j = 0; j < players[current].used; j++){
			// We send a random question to the player
			if(send(players[current].array[j].id, &rounds.array[rand() % rounds.used], sizeof(rounds.array[rand() % rounds.used]), 0) <= 0){
				perror("[server]Error in send() to client.\n");
				break;
			}			
		}

		for(j = 0; j < players[current].used; j++){
			// We receive the points obtained with this question
			if(recv(players[current].array[j].id, &add_points, sizeof(int), 0) <= 0){
				perror("[server]Error in recv() from client.\n");
				break;
			}

			players[current].array[j].score += add_points;

			if(i < (numberOfRounds - 1)){
				int not_finish = 1;

				// We send the signal of not finishing the game
				if(send(players[current].array[j].id, &not_finish, sizeof(int), 0) <= 0){
					perror("[server]Error in send() to client.\n");
					break;
				}
			}
			else{
				int finish = -1;

				// The game finishes
				if(send(players[current].array[j].id, &finish, sizeof(int), 0) <= 0){
					perror("[server]Error in send() to client.\n");
					break;
				}
			}
		}
	}

	for(i = 0; i < players[current].used; i++){
		if((newResult = malloc((strlen(players[current].array[i].username) + 25) * 2)) != NULL){
			newResult[0] = '\0';

			sprintf(newResult, "%s –– %d points\n", i + 1, players[current].array[i].username, players[current].array[i].score);
		}
		else{
			perror("Malloc failed!\n");
			exit(0);
		}
		
		if ((result = malloc((strlen(resultUntilNow) + strlen(newResult) + 1) * 2)) != NULL){
			result[0] = '\0';	// Ensures the memory is an empty string
			
			strcat(result, resultUntilNow);
			strcat(result, newResult);

			if((resultUntilNow = malloc((strlen(result) + 1) * 2)) != NULL){
				resultUntilNow[0] = '\0';

				strcpy(resultUntilNow, result);
			}
			else{
				perror("Malloc failed!\n");
				exit(0);
			}
			
		}
		else{
			perror("Malloc failed!\n");
			exit(0);
		}
	}

	for(i = 0; i < players[current].used; i++){
		if(send(players[current].array[i].id, result, strlen(result), 0) <= 0){
			perror("[client]Error in send() to server.\n");
			break;
		}
	}

	free(result);
	free(resultUntilNow);
	free(newResult);
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
								ronda.A[j - 12] = line[j];

								j++;
							}

							ronda.A[j - 12] = '\0';
						}
						else{
							if(!strcmp("\t\t\t<optionB>", lineOB)){
								int j = 12;

								while(line[j] != '<' && line[j + 1] != '/'){
									ronda.B[j - 12] = line[j];

									j++;
								}

								ronda.B[j - 12] = '\0';
							}
							else{
								if(!strcmp("\t\t\t<optionC>", lineOC)){
									int j = 12;

									while(line[j] != '<' && line[j + 1] != '/'){
										ronda.C[j - 12] = line[j];

										j++;
									}

									ronda.C[j - 12] = '\0';
								}
								else{
									if(!strcmp("\t\t\t<optionD>", lineOD)){
										int j = 12;

										while(line[j] != '<' && line[j + 1] != '/'){
											ronda.D[j - 12] = line[j];

											j++;
										}

										ronda.D[j - 12] = '\0';
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
	int i, j;
	int current_game = 0;
	FILE *XML_questions;

	for(i = 0; i < 50; i++){
		initArray(&players[i], 2);
	}

	initArrayRound(&rounds, 2);

	srand(time(NULL));

	/* Opening the file with the questions */
	if(!(XML_questions = fopen("/Users/jesusjimsa/Dropbox/Documentos/Universidad/3 - Primer cuatrimestre/Computer Networks/Teoría/Ejercicios/Quiz-Game/data/Q&A.xml", "r"))){
		perror("[server]Error opening file with questions\n");
		return errno;
	}

	XMLParser(XML_questions);	// We parse the file with the questions
	fclose(XML_questions);		// We don't need this file anymore, so we can close it

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
	if(bind(sd, (struct sockaddr *) &server, sizeof(struct sockaddr)) == -1){
		perror("[server]Error in bind().\n");
		return errno;
	}

	/* le pedimos al servidor que escuche si los clientes vienen a conectarse */
	if(listen(sd, 5) == -1){
		perror ("[server]Error in listen().\n");
		return errno;
	}

	/* Creation of the first thread that will handle the game */
	int errG[50];
	
	for(i = 0; i < 50; i++){
		current_game = i;

		errG[i] = pthread_create(&(tid[i]), NULL, &game, &current_game);

		if(errG[i] != 0){
			printf("\nCan't create thread :[%s]", strerror(errG[i]));
		}

		/*
			Waiting for the signal that last game has begun,
			next one can begin looking for players
		*/
		while(semaphore == -1){
			printf("%c", EMPTY);
		}

		sleep(2);

		semaphore = -1;
	}

	for(j = 0; j < 50; j++){
		for(i = 0; i < players[j].used; i++){
			close(players[j].array[i].id);
		}
	}

	for(i = 0; i < 50; i++){
		freeArray(&players[i]);
	}

	freeArrayRound(&rounds);
	close(sd);
}