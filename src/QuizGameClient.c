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
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define true (1 == 1)
#define false (!true)

/* Normalmente el puerto es el 2024 y la dirección es 127.0.0.1 */

/* el código de error devuelto por ciertas llamadas */
extern int errno;

/* puerto de acceso al servidor */
int port;

struct Round{
	char question[100];
	char A[50];
	char B[50];
	char C[50];
	char D[50];
	char correct_answer;
};

void printRound(struct Round ronda){
	printf("%s\n", ronda.question);
	printf("A. %s\tB. %s\nC. %s\tD. %s\n", ronda.A, ronda.B, ronda.C, ronda.D);
	printf("Which option is the right answer, A, B, C or D?\n");
}

int printResult(struct Round ronda, int answer){
	int points_obtained;
	printf("The right answer was: %s\n", &ronda.correct_answer);
	
	if(answer == true){
		printf("\nYou get 10 points in this round\n");
		printf("\n–––––––––––––––––––––––––––––––––––––––––––––––\n\n");
		points_obtained = 10;
	}
	else{
		printf("\nYou lose 2 points in this round\n");
		printf("\n–––––––––––––––––––––––––––––––––––––––––––––––\n\n");
		points_obtained = -2;
	}

	return points_obtained;
}

int main(int argc, char *argv[]){
	int sd;			// descriptor de socket
	struct sockaddr_in server;	// la estructura utilizada para conectar
	char username[100];
	char result[51200];
	char answer;
	struct Round ronda;
	//struct ResultRound result_round;
	int points_obtained_in_round;
	int finish = 1;	// When the server sends a -1 value to this variable, the game finishes
	int i, username_size;
	const int TIME_TO_ANSWER = 5;
	time_t timeBefore, timeAfter;

	result[0] = '\0';

	/* ¿Están todos los argumentos en la línea de comandos? */
	if(argc != 3){
      printf("Sintax: %s <server_name> <port>\n", argv[0]);
      return -1;
    }

	/* armamos el puerto */
	port = atoi(argv[2]);

	for(i = 0; i < 100; i++){
		username[i] = '#';
	}

	printf("Welcome to Quiz Game.\n");
	printf("What is your name? (It will be used to identify you during the game): ");
	fflush(stdout);
	read(0, &username, 100);
	printf("You have 5 seconds to answer each question.\nBe fast!\n");
	printf("\n–––––––––––––––––––––––––––––––––––––––––––––––\n\n");

	for(i = 0; username[i] != '#' && i < 100; i++);

	username_size = i - 1;

	/* creación del socket */
	if((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		perror("Error in socket().\n");
		return errno;
    }

	/* llenamos la estructura utilizada para conectar al servidor */
	/* la familia de socket */
	server.sin_family = AF_INET;
	
	/* dirección IP del servidor */
	server.sin_addr.s_addr = inet_addr(argv[1]);
	
	/* el puerto de conexión */
	server.sin_port = htons(port);

	/* nos conectamos al servidor */
	if(connect(sd, (struct sockaddr *) &server, sizeof(struct sockaddr)) == -1){
		perror("[client]Error in connect().\n");
		return errno;
    }

	/* enviamos el nombre de usuario al servidor para ser añadidos a la partida */
	if(send(sd, username, username_size, 0) <= 0){
		perror("[client]Error writing on the server.\n");
		return errno;
    }

	/* The game itself, questions and answers */
	while(finish != -1){
		if(recv(sd, &ronda, sizeof(ronda), 0) < 0){
			perror ("[client]Error in recv() from server.\n");
			return errno;
		}

		printRound(ronda);

		time(&timeBefore);
		scanf("%s", &answer);
		time(&timeAfter);

		if(difftime(timeBefore, timeAfter) > TIME_TO_ANSWER){
			answer = 'X';	// Not a valid answer
			printf("Too slow answering");
		}

		switch(answer){
			case 'a':
				answer = 'A';
				break;
			case 'b':
				answer = 'B';
				break;
			case 'c':
				answer = 'C';
				break;
			case 'd':
				answer = 'D';
				break;
		}

		points_obtained_in_round = printResult(ronda, (answer == ronda.correct_answer));

		if(send(sd, &points_obtained_in_round, sizeof(int), 0) <= 0){
			perror("[client]Error in send() to server.\n");
			return errno;
		}

		if(recv(sd, &finish, sizeof(finish), 0) < 0){
			perror ("[client]Error in recv() from server.\n");
			return errno;
		}
	}

	/* The game is over, now we receive the scores and finish the game */

	if(recv(sd, &result, sizeof(result), 0) < 0){
		perror("[client]Error in recv() from server.\n");
		return errno;
	}

	printf("Final clasification:\n");
	printf("%s", result);

	close(sd);
}