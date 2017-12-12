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
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define true (1 == 1)
#define false (!true)

/* el código de error devuelto por ciertas llamadas */
extern int errno;

/* puerto de acceso al servidor */
int port;

struct Options{
	char A[30];
	char B[30];
	char C[30];
	char D[30];
};

struct Round{
	char question[100];
	struct Options options;
	char correct_answer;
};

// struct ResultRound{
// 	char correct_answer[50];
// 	int points;
// };

void printRound(struct Round ronda){
	printf("%s\n", ronda.question);
	printf("A. %s\tB. %s\nC. %s\tD. %s\n", ronda.options.A, ronda.options.B, ronda.options.C, ronda.options.D);
	printf("Which option is the right answer, A, B, C or D?\n");
}

int printResult(struct Round ronda, int answer){
	int points_obtained;
	printf("The right answer was: %s\n", &ronda.correct_answer);
	
	if(answer == true){
		printf("You get 10 points in this round\n");
		points_obtained = 10;
	}
	else{
		printf("You get no points in this round\n");
		points_obtained = 0;
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

	/* ¿Están todos los argumentos en la línea de comandos? */
	if(argc != 3){
      printf("Sintax: %s <server_name> <port>\n", argv[0]);
      return -1;
    }

	/* armamos el puerto */
	port = atoi(argv[2]);

	printf("Welcome to Quiz Game.\n");
	printf("What is your name? (It will be used to identify you during the game): ");
	fflush(stdout);
	read(0, &username, 100);

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
	if(write (sd, &username, sizeof(username)) <= 0){
		perror("[client]Error writing on the server.\n");
		return errno;
    }

	/* The game itself, questions and answers */
	while(finish != -1){
		if(read(sd, &ronda, sizeof(ronda)) < 0){
			perror ("[client]Error in read() from server.\n");
			return errno;
		}

		printRound(ronda);
		read(0, &answer, 1);

		// if(write (sd, &answer, 1) <= 0){
		// 	perror("[client]Error al escribir en el servidor.\n");
		// 	return errno;
		// }

		// if(read(sd, &result_round, sizeof(result_round)) < 0){
		// 	perror ("[client]Error en read() del servidor.\n");
		// 	return errno;
		// }

		points_obtained_in_round = printResult(ronda, strcmp(&answer, &ronda.correct_answer));

		if(write(sd, &points_obtained_in_round, sizeof(int)) <= 0){
			perror("[client]Error in write() to server.\n");
			return errno;
		}

		if(read(sd, &finish, sizeof(finish)) < 0){
			perror ("[client]Error in read() from server.\n");
			return errno;
		}
	}

	/* The game is over, now we receive the scores and finish the game */

	if(read(sd, &result, sizeof(result)) < 0){
		perror("[client]Error in read() from server.\n");
		return errno;
	}

	printf("Final clasification:\n");
	printf("%s", result);

	close(sd);
}