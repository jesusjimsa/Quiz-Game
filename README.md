# Quiz Game

Online quiz game made for the class of Computer Networks in the UAIC as final project.  

The game consists of a server that handles the questions and the players, and an infinite number of clients (the players). The server has two types of threads, the thread that keeps waiting for players and the thread that handles the game itself.  
The player connects to the server and sends it its name, then the server sends 10 questions, one after another, and the client have to answer each one in less than 5 seconds. If the answer is correct, he/she gets 10 points but if the answer is wrong or later than 5 seconds, the player loses two points.  
After 10 rounds the server sends the global score to every player and then the game ends for the player.  
![Execution](https://i.imgur.com/cN8EBuI.png "Execution")
