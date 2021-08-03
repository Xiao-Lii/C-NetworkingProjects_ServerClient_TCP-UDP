/**********************************************************/
/* This program is a 'pass and play' version of tictactoe */
/* Two users, player 1 and player 2, pass the game back   */
/* and forth, on a single computer                        */
/**********************************************************/

/* 
Name: Lee Phonthongsy       
Assignment: Assignment / Lab 5 (Server Side - Player 1)
*/

/* include files go here */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <time.h>

/* #define Section */
#define ROWS 3
#define COLUMNS 3
#define VERSION 0x03
#define NEWGAME 0x00
#define CONTINUEGAME 0x01
#define TIMETOWAIT 120
#define MAXGAMES 10
#define BUFFERLENGTH 40


struct game {
  char board[ROWS][COLUMNS];
  struct sockaddr_in clientAddress;
  char clientVersion, clientCommand, clientMove, status;
  int clientLength, gameID;
  time_t time; 
}; 


/* C language requires that you predefine all the routines you are writing */
int checkwin(char board[ROWS][COLUMNS]);
void print_board(char board[ROWS][COLUMNS]);
int tictactoe();
int initSharedState(char board[ROWS][COLUMNS]);
int checkArguments(int argc);
int checkConnection(int sock, char msg[]);
int checkBoard(int choice, char board[ROWS][COLUMNS]);
int sendToClient(int gameSocket, char *buffer, struct game game);
int waitForMove(int gameSocket, char *clientBuffer, struct sockaddr_in serverAddress);
int randomMove(int choice);
void waitForClient(int gameSocket, int gameNum, struct game *gameData);
int createServerSocket(int port, int *gameSocket);
int findNextGame(struct game *gameData);


/* Lab 6 Sequence Protocol Details
 - If client sends back a previous packet or a duplicate, we ignore all of them and resend our last packet 
*/


int main(int argc, char *argv[]){
  int rc, port, gameSocket, tempClientLength;
  int currentGameNum = 0; 
  char board[ROWS][COLUMNS];
  char msg[20], ipAddress[15], buffer[40];
  struct sockaddr_in tempClientAddress;
  struct game gameData[MAXGAMES]; 


  // Initialize Stats for Empty Games 
  for (int i = 0; i < (MAXGAMES - 1); i++){
    gameData[i].gameID = -1;              // Set GameID = -1 for empty game
    gameData[i].status = 0;               // Set GameStatus = 0, not in progress 
    initSharedState(gameData[i].board);   // Initialize each tictactoe board
  }
  
  // Confirms Correct # of Parameters
  checkArguments(argc);
  port = atoi(argv[1]);

  rc = createServerSocket(port, &gameSocket);
  if (rc < 0 ){
    printf("[SERVER] Error: Couldn't create server socket.\n");
    exit(1);
  }

  while (1){

  memset(buffer, 0, 40);
    /*  Receive 5-byte header from client 
      1. Version check - Exit if not same
      2. Command Check
          If 0, check if gameData array is full/empty. We can check to see if any index in gameData is null or send server occupied msg
            If all good: start new game obj, init board, and send 1st move
          If 1, receive move by init game's board in gameData array
      3. Move check - Done in tic tac toe function
      4. Game Number - If command check was 1, index must be within range of gameData array 
  */

  if (currentGameNum > MAXGAMES){
    printf("[SERVER] Error: The server cannot support any more games at this moment.\n");
    exit(1);
  }
    
  waitForClient(gameSocket, currentGameNum, gameData); 
  }

  return 0;
}


int createServerSocket(int port, int *gameSocket){
  struct sockaddr_in serverAddress; 
  int length; 

  // Initialize the Server Socket
  *gameSocket = socket(AF_INET, SOCK_DGRAM, 0);
  if (*gameSocket < 0) {     // If connection failed
    printf("[SERVER] Socket creation failed. Please try again\n");
    exit(1);
  }

  // Create the Server Socket
  serverAddress.sin_family = AF_INET;               // IPv4
  serverAddress.sin_port = htons(port);             // Saving Port # from Command Line
  serverAddress.sin_addr.s_addr = INADDR_ANY;       // Saving any available IP address to Server's socket
  memset(serverAddress.sin_zero, '\0', sizeof(serverAddress.sin_zero));

  // Checks if Port is available to bind Server Socket to 
  if (bind(*gameSocket, (const struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {     // If connection failed
    printf("[SERVER] Bind failed. Please try again\n");
    exit(1);
  }

  return 1;
}



void waitForClient(int gameSocket, int gameNum, struct game *gameData){
  int rc, currentGameIndex, tempClientLength;
  struct sockaddr_in tempClientAddress; 
  //struct game newGame;
  char buffer[40], clientBuffer[40];
  unsigned int choice;      // used for keeping track of choice user makes

  /* Receive 5-byte header from client 
      1. Version check - Exit if not same
      2. Command Check
          If 0, check if gameData array is full/empty. We can check to see if any index in gameData is null or just iterate
            If all good: start new game obj, init board, and send 1st move
          If 1, receive move by init game's board in gameData array
      3. Move check - Done in tic tac toe function
      4. Game Number - If command check was 1, index must be within range of gameData array, if not send server occupied msg 
  */
  
  memset(buffer, 0, 40);
  memset(clientBuffer, 0, 40);
  
  tempClientLength = sizeof(tempClientAddress);
  rc = recvfrom(gameSocket, (char *)clientBuffer, 40, MSG_WAITALL, (struct sockaddr *)&tempClientAddress, &tempClientLength);
  /*if (rc < 0){
    printf("[SERVER] Error: Missing data from Player 2\n");
    exit(1);
  }*/
  printf("\nReceived from client\n Version: %x\n Command: %x\n Move: %x\n Game #: %x\n", clientBuffer[0], clientBuffer[1], clientBuffer[2], clientBuffer[3]);

  // Checking Version # between Server & Client 
  if (clientBuffer[0] != VERSION){
    perror("[SERVER] Error: Version compatibility issues. Please ensure client is running the same version as the server.\n");
    exit(1);
  }
  
  // If Command = 0 / NEWGAME: Initialize/save game data to Server's gameData
  if (clientBuffer[1] == NEWGAME){
    gameNum = findNextGame(&gameData[0]); 
    if (rc == -1){
      gameData[gameNum].clientVersion = VERSION; 
      gameData[gameNum].clientCommand = CONTINUEGAME;      
      gameData[gameNum].clientAddress = tempClientAddress;    
      gameData[gameNum].clientLength = tempClientLength;     
    }
    // Save All of the Client's info into an index of Game Data: Version, Command, Time, Address, & Length 
    gameData[gameNum].clientVersion = clientBuffer[0];      
    gameData[gameNum].clientCommand = clientBuffer[1];      
    gameData[gameNum].time = clock();                             
    gameData[gameNum].clientAddress = tempClientAddress;    
    gameData[gameNum].clientLength = tempClientLength;      

    /*
    rc = initSharedState(newGame.board);         // Initialize the 'game' board
    // Error Handling - Check if Board Initialized Properly
    if (rc != 0){
      printf("Error: Board wasn't initialized properly.\n");
      exit(1);
    }*/

    int i = 0;

    do {
      if (gameData[i].gameID == -1){
        gameData[gameNum].gameID = i;                       // Save current GameID to game Struct
        break;
      }
      else 
        i += 1;
    } while (i < MAXGAMES);

    rc = tictactoe(gameData[gameNum].board, gameSocket, gameData[gameNum].clientAddress, gameData[gameNum].gameID, gameData);

  }
  // If Command = 1 / CONTINUEGAME: Prepare to receive from
  else if (clientBuffer[1] == CONTINUEGAME){
    currentGameIndex = clientBuffer[3];

    rc = tictactoe(gameData[currentGameIndex].board, gameSocket, gameData[currentGameIndex].clientAddress, gameData[currentGameIndex].gameID, gameData);
    printf("Back from tictactoe board. Rc = %d", rc); 
    /*
    if (i == 1){                  // means a player won!! congratulate them
    printf("==>\aPlayer %d wins\n ", --player);
  }
  else
    printf("==>\aGame draw");   // ran out of squares, it is a draw */

    /*
    //rc = tictactoe(gameData[currentGameIndex].board, choice, 2, gameSocket, gameData[currentGameIndex].gameID, gameData); 
    // Error Handling - Check if Game Ended Properly
    if (rc != 0){/
      printf("Error: Game didn't end properly.\n");
      exit(1);
    } */

  }
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
  if (connection < 0) {     // If connection failed
    printf("[SERVER] %s failed. Please try again\n", msg);
    exit(1);
  }
  else {                    // If connection successful
    printf("[SERVER] %s successful.\n", msg);
    return connection;
  }
}


int tictactoe(char board[ROWS][COLUMNS], int gameSocket, struct sockaddr_in clientAddress, int gameNum, struct game *gameData){

  /* this is the meat of the game, you'll look here for how to change it up */
  int player = 1;         // keep track of whose turn it is
  unsigned int choice;    // used for keeping track of choice user makes
  int i, row, column, returnCode, clientLength, n;
  char mark;              // either an 'x' or an 'o'
  int serverMove;
  char buffer[40], clientBuffer[40];

  /* loop, first print the board, then ask player 'n' to make a move */
  //do {
    print_board(board);            // call function to print the board on the screen
    player = (player % 2) ? 1 : 2; // Mod math to figure out who the player is

    
    if (player == 1){
      do {
        //printf("Player %d, enter a number: ", player); // print out player so you can pass game
        //returnCode = scanf("%x", &choice);           // using scanf to get the choice
        choice = randomMove(choice);            // Generate a random number for Player 1's move

      } while ((choice < 0x01) || (choice > 0x09) || (checkBoard(choice, board)));

      // Save Variables to Buffer to send to Player 2
      buffer[0] = VERSION;
      buffer[1] = CONTINUEGAME;
      buffer[2] = choice; 
      buffer[3] = gameNum;
      
      printf("Sending to Player 2:\n Version: %x\n Command: %x\n Move: %x\n Game #: %x\n", buffer[0], buffer[1], buffer[2], buffer[3]);

      returnCode = sendto(gameSocket, &buffer, 4, 0, (struct sockaddr *)&clientAddress, sizeof(clientAddress)); 
      // Error Handling - Send / Receive between players
      if (returnCode != 4){
        printf("[SERVER] Error: Too much or too little data was sent to Player 2\n");
        exit(1);
      }
    }
    else {
      // Retrieve Player 2's Move, save to choice 
      choice = waitForMove(gameSocket, clientBuffer, clientAddress);
      gameNum = clientBuffer[3];
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
    i = checkwin(gameData[gameNum].board);

    player++;

    
  //} while (i == -1);            // -1 means no one won

  /* print out the board again */
  print_board(board);

  /*

  if (i == 1){                  // means a player won!! congratulate them
    printf("==>\aPlayer %d wins\n ", --player);
  }
  else
    printf("==>\aGame draw");   // ran out of squares, it is a draw

  return 0;*/

  return i; 
}


int randomMove(int choice){
  /******************************************************************/
  /* We'll generate a random move between 1 - 9 as the server's     */
  /* move.                                                          */
  /******************************************************************/
  choice = (rand() % 9) + 1;
  return choice;
}


int waitForMove(int gameSocket, char *clientBuffer, struct sockaddr_in clientAddress){
  /******************************************************************/
  /* We'll pass the game socket to recv Player 2's 3 byte header.   */
  /* We'll check to see Player 2's command & version is valid &     */
  /* compatible.                                                    */
  /******************************************************************/
  char clientVersion, clientCommand, clientMove; 
  int returnCode, clientLength, move, clientGameNum; 
  struct timeval tv; 
  tv.tv_sec = TIMETOWAIT;
  tv.tv_usec = 0; 

  memset(clientBuffer, 0, 40);

  do {
    setsockopt(gameSocket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)); 

    clientLength = sizeof(clientAddress);
    returnCode = recvfrom(gameSocket, clientBuffer, 40, 0, (struct sockaddr *)&clientAddress, &clientLength);
  } while (returnCode <= 0);

  // Save Indexes in clientBuffer to Variables
  clientVersion = clientBuffer[0];
  clientCommand = clientBuffer[1];
  clientMove = clientBuffer[2];
  clientGameNum = clientBuffer[3];
  move = (int)clientMove;

  // Check Player 2's version compatibility 
  if (clientVersion != VERSION){
    printf("[SERVER] Error: Game versions aren't compatible with each other.\n");
    exit(1);
  }
  
  return move;
}



int sendToClient(int gameSocket, char *buffer, struct game game){
  unsigned int choice;
  int returnCode; 

  do {
      //printf("Player %d, enter a number: ", player); // print out player so you can pass game
      //returnCode = scanf("%x", &choice);             // using scanf to get the choice
      choice = randomMove(choice);            // Generate a random number for Player 1's move
      /*
      if (returnCode == 0){
        getchar();
      }*/
    } while ((choice < 0x01) || (choice > 0x09) || (checkBoard(choice, game.board)));

    // Save Variables to Buffer to send to Player 2
    buffer[0] = VERSION;
    buffer[1] = CONTINUEGAME;
    buffer[2] = choice;
    buffer[3] = game.gameID;
    
    printf("Sending to Player 2:\n Version: %x\n Command: %x\n Move: %x \n Game ID: %x\n", buffer[0], buffer[1], buffer[2], buffer[3]);

    returnCode = sendto(gameSocket, &buffer, 4, 0, (struct sockaddr *)&game.clientAddress, game.clientLength); 
    // Error Handling - Send / Receive between players
    if (returnCode != 4){
      printf("[SERVER] Error: Too much or too little data was sent to Player 2\n");
      exit(1);
    }

    return choice; 
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


int findNextGame(struct game *gameData){
  int i;
  printf("[SERVER] Searching for a new game.\n");

  for (i = 0; i < MAXGAMES; i++)
    if (gameData[i].status == 0){
      gameData[i].status = 1;
      printf("[SERVER] Success! The server will start game #: %d\n", i);
      return i;
    }
  return -1;
}