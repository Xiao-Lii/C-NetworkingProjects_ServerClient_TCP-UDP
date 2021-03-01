/**********************************************************/
/* This program is a 'pass and play' version of tictactoe */
/* Two users, player 1 and player 2, pass the game back   */
/* and forth, on a single computer                        */
/**********************************************************/

/* 
Name: Lee Phonthongsy       
Assignment: Assignment / Lab 4 (Server Side - Player 1)
*/

/* include files go here */
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/time.h>
#include <errno.h>

/* #define section, for now we will define the number of rows and columns */
#define ROWS 3
#define COLUMNS 3
#define BACKLOG 10
#define VERSION 0x02
#define NEWGAME 0x00
#define CONTINUEGAME 0x01
#define TIMETOWAIT 10

/* C language requires that you predefine all the routines you are writing */
int checkwin(char board[ROWS][COLUMNS]);
void print_board(char board[ROWS][COLUMNS]);
int tictactoe();
int initSharedState(char board[ROWS][COLUMNS]);
int checkArguments(int argc);
int checkConnection(int sock, char msg[]);
int checkBoard(int choice, char board[ROWS][COLUMNS]);
void sendToClient(int gameSocket, char *buffer, struct sockaddr_in server_sock);
int waitForMove(int gameSocket, char *clientBuffer, struct sockaddr_in serverAddress);


int main(int argc, char *argv[]){
  int rc, client, port;
  char board[ROWS][COLUMNS];
  char msg[20];

  // Confirms Correct # of Parameters
  checkArguments(argc);
  port = atoi(argv[1]); 

  rc = initSharedState(board); // Initialize the 'game' board
  // Error Handling - Check if Board Initialized Properly
  if (rc != 0){
    printf("Error: Board wasn't initialized properly.\n");
    exit(1);
  }

  rc = tictactoe(board, client, port); // call the 'game'
  // Error Handling - Check if Game Ended Properly
  if (rc != 0){
    printf("Error: Game didn't end properly.\n");
    exit(1);
  }

  return 0;
}


int checkArguments(int argc){
  /************************************************************************/
  /* Checks to ensure the correct number of arguments have been placed    */
  /* in command line for server-side / player 1                           */
  /************************************************************************/
  // <Port #>
  if (argc < 2){
    printf("Error: Incorrect # of parameters, run format should be:\n"
           "./tictactoeOriginal <Port-#> \n");
    exit(1);
  }
  //printf("Correct # of parameters in run command line.\n");
  return argc;
}


int checkConnection(int connection, char msg[]){
  /************************************************************************/
  /* Checks to ensure there are no issues with creating, binding,         */
  /* listening, and accepting the connection between the server & client  */
  /************************************************************************/
  // ERROR-HANDLING - INITIALIZING THE SOCKET
  if (connection < 0) { // If connection failed
    printf("[SERVER] %s failed. Please try again\n", msg);
    exit(1);
  }
  else { // If connection successful
    printf("[SERVER] %s successful.\n", msg);
    return connection;
  }
}


int tictactoe(char board[ROWS][COLUMNS], int client, int port){
  /* this is the meat of the game, you'll look here for how to change it up */
  int player = 1;         // keep track of whose turn it is
  unsigned int choice;    // used for keeping track of choice user makes
  int i, row, column, returnCode, gameSocket, clientLength, n;
  char mark;              // either an 'x' or an 'o'
  char serverCommand = '1', serverVersion = '2', serverMove, clientVersion, clientCommand, clientMove;
  char buffer[3], clientBuffer[3];
  struct sockaddr_in serverAddress, clientAddress;

  memset(buffer, 0, 3);
  memset(clientBuffer, 0, 3);

  // Initialize the Socket
  checkConnection(gameSocket = socket(AF_INET, SOCK_DGRAM, 0), "Socket creation");

  // Create the Socket
  serverAddress.sin_family = AF_INET;               // IPv4
  serverAddress.sin_port = htons(port);             // Saving Port # from Command Line
  serverAddress.sin_addr.s_addr = INADDR_ANY;       // Saving any available IP address to Server's socket
  memset(serverAddress.sin_zero, '\0', sizeof(serverAddress.sin_zero));

  // Checks Each Step of Establishing the Connection with the Client
  checkConnection(bind(gameSocket, (const struct sockaddr *)&serverAddress, sizeof(serverAddress)), "Bind");

  clientLength = sizeof(clientAddress);
  returnCode = recvfrom(gameSocket, (char *)clientBuffer, 2, MSG_WAITALL, (struct sockaddr *)&clientAddress, &clientLength);

  if (!returnCode){
    printf("[SERVER] Error: Connection could not be established with Player 2\n");
    exit(1);
  }

  clientVersion = clientBuffer[0];
  clientCommand = clientBuffer[1];

  printf("Received from client\n Version: %x\n Command: %x", clientVersion, clientCommand);
  if (clientCommand != NEWGAME){
    perror("[SERVER] Error: Cannot execute a move without starting a game.\n");
    exit(1);
  }

  /* loop, first print the board, then ask player 'n' to make a move */
  do {
    print_board(board);            // call function to print the board on the screen
    player = (player % 2) ? 1 : 2; // Mod math to figure out who the player is

    if (player == 1){
      do {
        printf("Player %d, enter a number: ", player); // print out player so you can pass game
        returnCode = scanf("%x", &choice);             // using scanf to get the choice

        if (returnCode == 0){
          getchar();
        }
      } while ((choice < 0x01) || (choice > 0x09) || (checkBoard(choice, board)));

      // Save Variables to Buffer to send to Player 2
      buffer[0] = VERSION;
      buffer[1] = CONTINUEGAME;
      buffer[2] = choice; 
      
      printf("Sending to Player 2:\n Version: %x\n Command: %x\n Move: %x\n", buffer[0], buffer[1], buffer[2]);

      returnCode = sendto(gameSocket, &buffer, 3, 0, (struct sockaddr *)&clientAddress, sizeof(clientAddress)); 
      // Error Handling - Send / Receive between players
      if (returnCode != 3){
        printf("[SERVER] Error: Too much or too little data was sent to Player 2\n");
        exit(1);
      }
      
    }
    else {
      // Retrieve Player 2's Move, save to choice 
      choice = waitForMove(gameSocket, clientBuffer, clientAddress); 
    }

    mark = (player == 1) ? 'X' : 'O'; // depending on who the player is, either us x or o

    /******************************************************************/
    /** little math here. you know the squares are numbered 1-9, but  */
    /* the program is using 3 rows and 3 columns. We have to do some  */
    /* simple math to conver a 1-9 to the right row/column            */
    /******************************************************************/

    row = (int)((choice - 1) / ROWS);
    column = (choice - 1) % COLUMNS;

    if (board[row][column] == (choice + '0'))
      board[row][column] = mark;

    /* after a move, check to see if someone won! (or if there is a draw */
    i = checkwin(board);

    player++;
  } while (i == -1);            // -1 means no one won

  /* print out the board again */
  print_board(board);

  if (i == 1)                   // means a player won!! congratulate them
    printf("==>\aPlayer %d wins\n ", --player);
  else
    printf("==>\aGame draw");   // ran out of squares, it is a draw

  return 0;
}


int waitForMove(int gameSocket, char *clientBuffer, struct sockaddr_in clientAddress){
  /******************************************************************/
  /* We'll pass the game socket to recv Player 2's 3 byte header.   */
  /* We'll check to see Player 2's command & version is valid &     */
  /* compatible.                                                    */
  /******************************************************************/
  char clientVersion, clientCommand, clientMove; 
  int returnCode, clientLength, move; 
  struct timeval tv; 
  tv.tv_sec = TIMETOWAIT;
  tv.tv_usec = 0; 

  memset(clientBuffer, 0, 3); 

  do {
    setsockopt(gameSocket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)); 

    clientLength = sizeof(clientAddress);
    returnCode = recvfrom(gameSocket, clientBuffer, 3, 0, (struct sockaddr *)&clientAddress, &clientLength);
    if (returnCode != 3){
      printf("[SERVER] Timeout from Player 2.\n");
      exit(1);
    }
  } while (returnCode <= 0);

  // Save Indexes in clientBuffer to Variables
  clientVersion = clientBuffer[0];
  clientCommand = clientBuffer[1];
  clientMove = clientBuffer[2];
  move = (int)clientMove;

  // Check Player 2's version compatibility 
  if (clientVersion != VERSION){
    printf("[SERVER] Error: Game versions aren't compatible with each other.\n");
    exit(1);
  }
  else
    printf("Player 2 Version: %x\n", clientVersion);

  // Check Player 2's Command
  if (clientCommand != CONTINUEGAME){
    printf("[SERVER] Player 2 is trying to start a new game. The game will now close.\n");
    exit(1);
  }
  else
    printf("Player 2 Command: %x\n", clientCommand);
  
  return move;
}



void sendToClient(int gameSocket, char *buffer, struct sockaddr_in server_sock){
  int returnCode; 

  printf("Sending to Player 2:\n Version: %x\n Command: %x\n Move: %x\n", buffer[0], buffer[1], buffer[2]);
  returnCode = sendto(gameSocket, buffer, 3, 0, (struct sockaddr *)&server_sock, sizeof(server_sock)); 

  // Error Handling - Send / Receive between players
  if (!returnCode){
    printf("[SERVER] Error: Connection was lost with player 2\n");
    exit(1);
  }
}


int checkBoard(int choice, char board[ROWS][COLUMNS]){
  /******************************************************************/
  /* We'll pass in the player's choice to check if it's already     */
  /* occupied on our tictactoe board. If so, it will return a -1.   */
  /* If the spot is found to be empty it will return a 0.           */
  /******************************************************************/
  int row, column;

  row = (int)((choice - 1) / ROWS);
  column = (choice - 1) % COLUMNS;

  /* first check to see if the row/column chosen is has a digit in it, if it */
  /* square 8 has and '8' then it is a valid choice                          */

  if (board[row][column] == (choice + '0'))
    return 0;   // Returns 0 if empty
  else
    return -1;  // Returns -1 if spot is already occupied
}

int checkwin(char board[ROWS][COLUMNS]){
  /************************************************************************/
  /* brute force check to see if someone won, or if there is a draw       */
  /* return a 0 if the game is 'over' and return -1 if game should go on  */
  /************************************************************************/
  if (board[0][0] == board[0][1] && board[0][1] == board[0][2]) // row matches
    return 1;

  else if (board[1][0] == board[1][1] && board[1][1] == board[1][2]) // row matches
    return 1;

  else if (board[2][0] == board[2][1] && board[2][1] == board[2][2]) // row matches
    return 1;

  else if (board[0][0] == board[1][0] && board[1][0] == board[2][0]) // column
    return 1;

  else if (board[0][1] == board[1][1] && board[1][1] == board[2][1]) // column
    return 1;

  else if (board[0][2] == board[1][2] && board[1][2] == board[2][2]) // column
    return 1;

  else if (board[0][0] == board[1][1] && board[1][1] == board[2][2]) // diagonal
    return 1;

  else if (board[2][0] == board[1][1] && board[1][1] == board[0][2]) // diagonal
    return 1;

  else if (board[0][0] != '1' && board[0][1] != '2' && board[0][2] != '3' &&
           board[1][0] != '4' && board[1][1] != '5' && board[1][2] != '6' &&
           board[2][0] != '7' && board[2][1] != '8' && board[2][2] != '9')

    return 0;   // Return of 0 means game over
  else
    return -1;  // return of -1 means keep playing
}

void print_board(char board[ROWS][COLUMNS]){
  /*****************************************************************/
  /* brute force print out the board and all the squares/values    */
  /*****************************************************************/

  printf("\n\n\n\tCurrent TicTacToe Game\n\n");

  printf("Player 1 (X)  -  Player 2 (O)\n\n\n");

  printf("     |     |     \n");
  printf("  %c  |  %c  |  %c \n", board[0][0], board[0][1], board[0][2]);

  printf("_____|_____|_____\n");
  printf("     |     |     \n");

  printf("  %c  |  %c  |  %c \n", board[1][0], board[1][1], board[1][2]);

  printf("_____|_____|_____\n");
  printf("     |     |     \n");

  printf("  %c  |  %c  |  %c \n", board[2][0], board[2][1], board[2][2]);

  printf("     |     |     \n\n");
}

int initSharedState(char board[ROWS][COLUMNS]){
  /* this just initializing the shared state aka the board */
  int i, j, count = 1;
  printf("in sharedstate area\n");
  for (i = 0; i < 3; i++)
    for (j = 0; j < 3; j++)
    {
      board[i][j] = count + '0';
      count++;
    }
  return 0;
}