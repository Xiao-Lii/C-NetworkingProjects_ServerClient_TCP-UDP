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
#define VERSION 4
#define NEWGAME 0
#define CONTINUEGAME 1
#define GAMEOVER 2
#define TIMEOUT 30
#define MAXGAMES 100
#define BUFFERLENGTH 40

struct info {
  char version;
  char command;
  char move;
  char game;
  char sequenceNumber;
};

struct game {
  char gameBoard[ROWS][COLUMNS];
  int turn;                     // 1 for player 1, 2 for player 2
  int gameProgress;             // 0 means not in progress
  time_t timeSinceLastMove;
  int timeoutCounter;           // A timeout counter that will reset the game once it reaches 10
  struct info lastDatagramSent; // The last datagram sent from the server to the client, in case of dropped packets
  struct info lastDatagramReceived; // Last datagram received from the client, in case of duplicate packets
  struct sockaddr_in clientAddress; 
};

/* C language requires that you predefine all the routines you are writing */
int checkwin(char board[ROWS][COLUMNS]);
void print_board(char board[ROWS][COLUMNS]);
int tictactoe(int sock, int playerNumber, char *ipAddr, int portNumber, int freeGames[], struct game *gameData);
int initSharedState(char board[ROWS][COLUMNS]);
int checkArguments(int argc);
int checkConnection(int sock, char msg[]);
int createServerSocket(int portNumber, int *sock);
int randomMove();
int recvMoveFromClient(int sock, struct sockaddr_in *myAddr, int *currentGame, int *commandInOut, int freeGames[], struct info *dataPacketIn);
int sendMove(int sock, int move, struct sockaddr_in *myAddr, int command, int gameInOut, struct info *dataPacketOut);
int findNextGame(struct game *gameData);
int findNextSquare(char squareD[ROWS][COLUMNS]);
int resetGame(struct game *gameData, int currentGame);
int isSquareOccupied(int choice, char board[ROWS][COLUMNS]);

int main(int argc, char *argv[]){
  int rc, connectedSocket, i;
  int gamesAvailable[10];
  int port, playerNumber;

  struct game gameData[MAXGAMES];

  // Confirms Correct # of Parameters
  checkArguments(argc);
  port = atoi(argv[1]);

  for (i = 0; i < MAXGAMES; i++){
    initSharedState(gameData[i].gameBoard);
    gameData[i].gameProgress = 0;       // Sets all games as available
    gameData[i].turn = 1;               // Player 1 goes first.  Client is player 2
    gameData[i].timeoutCounter = 0;     // Initialize timeout counter to zero 
  }

  rc = createServerSocket(port, &connectedSocket);
  if (rc < 0){
    printf("Error: could not create socket");
    exit(1);
  }

  while (1){
    tictactoe(connectedSocket, playerNumber, " ", port, gamesAvailable, &gameData[0]);
    printf("Games Over, start a new one\n");
  }

  return 0;
}

int checkArguments(int argc)
{
  /************************************************************************/
  /* Checks to ensure the correct number of arguments have been placed    */
  /* in command line for server-side / player 1                           */
  /************************************************************************/
  // <Port #>
  if (argc < 2){
    printf("[SERVER] Error: Incorrect # of parameters, run format should be:\n"
           "./tictactoeOriginal <Port-#> \n");
    exit(1);
  }
  return argc;
}

int tictactoe(int sock, int playerNumber, char *ipAddr, int portNumber, int freeGames[], struct game *gameData){
  /* this is the meat of the game, you'll look here for how to change it up */
  int runningTotal = 0;
  int i, move; // used for keeping track of choice user makes
  int game;
  int currentGame = 0;
  struct sockaddr_in myAddr;
  int row, column, command, sequenceNum, fromLength, rc;
  char mark;
  struct info dataPacketIn, dataPacketOut;
  int validGames = 0;
  time_t currentTime;
  double seconds;

  do {
    // Our timeout checker for all active games
    for (i = 0; i < MAXGAMES; i++){
      currentTime = time(NULL);
      seconds = currentTime - gameData[i].timeSinceLastMove;

      // Difftime will return the difference in time in secs & if it exceeds TIMETOWAIT, the game should be set to reset
      if ((gameData[i].gameProgress == 1) && (seconds > TIMEOUT)){
        // Display Timeout Error after 10 Connection attempts
        if (gameData[i].timeoutCounter >= 10){
          gameData[i].gameProgress = 0;           // Change that gameData at that index to 0 = Not in progress
          initSharedState(gameData[i].gameBoard); // Reset the Game Board
          gameData[i].turn = 1;                   // Reset to Server makes 1st Move
          printf("Timeout has occured for Game #: %d which has a time of: %f\n", i, seconds);
        }
        else {
          gameData[i].timeoutCounter++;
          printf("Game #: %d hasn't made a move in a while, attempt #: %d to reconnect to client.\n", i, gameData[i].timeoutCounter);
          //if (gameData[i].lastDatagramSent.sequenceNumber != 1)   // If it's the server's 1st move, there's no last datagram to resend from
          sendMove(sock, gameData[i].lastDatagramSent.move, &gameData[i].clientAddress, gameData[i].lastDatagramSent.command, gameData[i].lastDatagramSent.game, &gameData[i].lastDatagramSent);
        }
      }
    }

    /******************************************************************/
    /*                 Receives Data from Client                      */
    /******************************************************************/
    printf("Awaiting Data from Player 2\n\n");
    recvMoveFromClient(sock, &myAddr, &currentGame, &command, freeGames, &dataPacketIn);
    command = dataPacketIn.command;
    currentGame = dataPacketIn.game;
    sequenceNum = dataPacketIn.sequenceNumber; 

    /******************************************************************/
    /*             Checks the command received from Client.           */
    /******************************************************************/
    switch (dataPacketIn.command + 0){
      case NEWGAME:{
        game = findNextGame(&gameData[0]); // Finds a new game number to be played
        if (game == -1){
          dataPacketOut.command = (CONTINUEGAME);
          dataPacketOut.game = 0;
          dataPacketOut.version = VERSION;
          sendMove(sock, move, &myAddr, command, currentGame, &dataPacketOut);
          printf("Ran out of Games, running total is %d\n", runningTotal);
          return -1;
        }

        //if (gameData[currentGame].gameProgress == 0){
          /*
          game = findNextGame(&gameData[0]); // Finds a new game number to be played
          if (game == -1){
            dataPacketOut.command = (CONTINUEGAME);
            dataPacketOut.game = 0;
            dataPacketOut.version = VERSION;
            sendMove(sock, move, &myAddr, command, currentGame, &dataPacketOut);
            printf("Ran out of Games, running total is %d\n", runningTotal);
            return -1;
          }*/

          // Error-Handling Check, 1st Sequence Number received from Client MUST be 0
          if (dataPacketIn.sequenceNumber != 0){
            printf("[SERVER] Error: 1st sequence number received from client wasn't a 0\n");
            return -1; 
          }

          /******************************************************************/
          /*       Sets the data to be sent to Client                       */
          /******************************************************************/
          runningTotal++;

          dataPacketOut.version = VERSION;
          dataPacketOut.command = CONTINUEGAME;
          dataPacketOut.game = game;
          dataPacketOut.move = findNextSquare(gameData[game].gameBoard); /*Finds an open square to make a move. Not random*/
          gameData[game].timeSinceLastMove = time(NULL);      // Save current time of last move being made in gameData game index
          dataPacketOut.sequenceNumber = dataPacketIn.sequenceNumber + 1;
          gameData[game].lastDatagramSent = dataPacketOut;    // Save the last sent datagram to the gameData in case we need to resend
          gameData[game].clientAddress = myAddr;              // Save the Client's address in case if we need to resend datagrams
          gameData[game].gameProgress = 1;                    // Sets this game up to in-progress
          gameData[game].lastDatagramReceived = dataPacketIn; // Save the last datagram received

          // do
          // {
          //   dataPacketOut.move = randomMove();
          // } while (isSquareOccupied(dataPacketOut.move, gameData[game].gameBoard));

          mark = (gameData[game].turn = 1) ? 'X' : 'O';
          row = (int)((dataPacketOut.move - 1) / ROWS);
          column = (dataPacketOut.move - 1) % COLUMNS;
          gameData[game].gameBoard[row][column] = mark;

          if (gameData[game].turn == 1){
            gameData[game].turn = 2;
          }
          else {
            gameData[game].turn = 1;
          }

          printf("Started A New Game:\n Move: %d\n Game %d\n Command %d\n", dataPacketOut.move, dataPacketOut.game, dataPacketOut.command);
          sendMove(sock, move, &myAddr, command, currentGame, &dataPacketOut);
          print_board(gameData[game].gameBoard);

          break;
        //}
        /*
        else if (gameData[currentGame].gameProgress == 1 && (&gameData[currentGame].lastDatagramReceived == &dataPacketIn)){
          printf("Error: Duplicate packet received for a new game. Previous datagram will be resent.\n");
          sendMove(sock, gameData[currentGame].lastDatagramSent.move, &gameData[currentGame].clientAddress, gameData[currentGame].lastDatagramSent.command, gameData[currentGame].lastDatagramSent.game, &gameData[currentGame].lastDatagramSent);
        }*/
      }
      case CONTINUEGAME:{
        if (dataPacketIn.sequenceNumber == gameData[currentGame].lastDatagramSent.sequenceNumber + 1){
          move = dataPacketIn.move + 0;
          mark = (gameData[currentGame].turn == 1) ? 'X' : 'O';
          row = (int)((move - 1) / ROWS);
          column = (move - 1) % COLUMNS;

          if (gameData[currentGame].gameBoard[row][column] == (move + '0')){
            gameData[currentGame].gameBoard[row][column] = mark;
          }

          i = checkwin(gameData[currentGame].gameBoard);
          print_board(gameData[currentGame].gameBoard);

          if (i < 0)
          { // no winner)
            // change whose turn it is
            if (gameData[currentGame].turn == 1){
              gameData[currentGame].turn = 2;
            }
            else {
              gameData[currentGame].turn = 1;
            }

            printf("*************************\nContinuing Game: Current Game: %d\n*************************\n", currentGame);
            move = findNextSquare(gameData[currentGame].gameBoard);

            // do
            // {
            //   choice = randomMove();
            // } while (isSquareOccupied(choice, gameData[game].gameBoard));

            dataPacketOut.command = (CONTINUEGAME);
            dataPacketOut.move = (move);
            dataPacketOut.game = (currentGame);
            dataPacketOut.version = VERSION;
            dataPacketOut.sequenceNumber = dataPacketIn.sequenceNumber + 1;
            gameData[currentGame].timeSinceLastMove = time(NULL);  // Save current time of last move being made in gameData game index
            gameData[currentGame].lastDatagramSent = dataPacketOut;

            mark = (gameData[currentGame].turn == 1) ? 'X' : 'O';
            row = (int)((move - 1) / ROWS);
            column = (move - 1) % COLUMNS;

            if (gameData[currentGame].gameBoard[row][column] == (move + '0'))
              gameData[currentGame].gameBoard[row][column] = mark;

            i = checkwin(gameData[currentGame].gameBoard);

            sendMove(sock, move, &myAddr, command, currentGame, &dataPacketOut);
          }

          /******************************************************************/
          /*     Checks to seee who won the game of if it was a draw        */
          /******************************************************************/

          if (i >= 0){
            if (i == 1){
              printf("==>Player %d won game #: %d\nrunning total is %d\n", gameData[currentGame].turn, currentGame, runningTotal);
              
              if (gameData[currentGame].turn == 2){
                // When we get to this point, client knows we have won based on our last winning move
                // Now they're waiting for us to send back an ack of game over 
                dataPacketOut.version = VERSION;
                dataPacketOut.command = GAMEOVER;
                dataPacketOut.move = 0;
                dataPacketOut.game = (currentGame);
                dataPacketOut.sequenceNumber = dataPacketIn.sequenceNumber + 1;

                gameData[currentGame].timeSinceLastMove = 0;          // Reset time of last client move
                gameData[currentGame].gameProgress = 0;               // Set to 0 = Game not in progress 
                gameData[currentGame].timeoutCounter = 0;             // Reset timeout counter
                gameData[currentGame].turn = 1;                       // Reset to Server makes 1st Move
                initSharedState(gameData[currentGame].gameBoard);     // Reset tictactoe board

                printf("Sending back GAME OVER ack to game #: %d\n", currentGame); 
                sendMove(sock, move, &gameData[currentGame].clientAddress, GAMEOVER, currentGame, &dataPacketOut);
              }
              else{
                printf("Waiting for GAME OVER ack from game #: %d\n", currentGame); 
                //gameData[currentGame].gameProgress = 0;               // Set to 0 = Game not in progress 
                gameData[currentGame].timeoutCounter = 0;             // Reset timeout counter
              }

            }
            else {
              printf("==>\aGame draw\n"); // ran out of squares, it is a draw
              //resetGame(gameData, currentGame);
              //printf("board reset\n");
            }
          }

          else if (gameData[currentGame].turn == 1){
            gameData[currentGame].turn = 2;
          }
          else {
            gameData[currentGame].turn = 1;
          }
        }
        else if (dataPacketIn.sequenceNumber == gameData[currentGame].lastDatagramSent.sequenceNumber - 1){
          printf("[SERVER] Received a duplicate packet. Attempting to resend last datagram sent to Game #: %d\n", currentGame);
          sendMove(sock, gameData[currentGame].lastDatagramSent.move, &gameData[currentGame].clientAddress, gameData[currentGame].lastDatagramSent.command, gameData[currentGame].lastDatagramSent.game, &gameData[currentGame].lastDatagramSent);
        }
        break;
      }
      case GAMEOVER: {
        /*
        dataPacketOut.version = VERSION;
        dataPacketOut.command = GAMEOVER;
        dataPacketOut.move = 0;
        dataPacketOut.game = (currentGame);
        dataPacketOut.sequenceNumber = dataPacketIn.sequenceNumber + 1;
        */
        printf("Game over, resetting game #: %d\n", currentGame); 

        gameData[currentGame].timeSinceLastMove = 0;          // Reset time of last client move
        gameData[currentGame].gameProgress = 0;               // Set to 0 = Game not in progress 
        gameData[currentGame].timeoutCounter = 0;             // Reset timeout counter
        gameData[currentGame].turn = 1;                       // Reset to Server makes 1st Move
        initSharedState(gameData[currentGame].gameBoard);     // Reset tictactoe board
        //sendMove(sock, choice, &myAddr, command, currentGame, &dataPacketOut);
      }
    }

  } while (1);

  return 0;
}

int randomMove()
{
  int choice;
  /******************************************************************/
  /* We'll generate a random move between 1 - 9 as the server's     */
  /* move.                                                          */
  /******************************************************************/
  choice = (rand() % 9) + 1;
  return choice;
}

int checkwin(char board[ROWS][COLUMNS])
{
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

    return 0; // Return of 0 means game over
  else
    return -1; // return of -1 means keep playing
}

void print_board(char board[ROWS][COLUMNS])
{
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

int recvMoveFromClient(int sock, struct sockaddr_in *myAddr, int *currentGame, int *commandInOut, int freeGames[], struct info *dataPacketIn){
  int returnCode;
  int fromLength;
  struct timeval tv;
  //int count = 0;
  fromLength = sizeof(struct sockaddr_in);

  tv.tv_sec = TIMEOUT;
  tv.tv_usec = 0;

  if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv))){
    perror("Error");
    exit(1);
  }

  do {
    returnCode = recvfrom(sock, dataPacketIn, sizeof(struct info), 0, (struct sockaddr *)myAddr, (socklen_t *)&fromLength);
    
    /*
    if (returnCode <= 0){
      printf("Timeout Occured, attempting to resend packet.\n");
      count++;

      if (count == 10){
        printf("Timeout Exceeded");
        exit(1);
      }
    }*/
    
  } while (returnCode <= 0);

  printf("Received from Player 2:\nVersion: %d\tCommand: %d\tMove: %d\t\tGame #: %d\tSequence #: %d\n", dataPacketIn->version, dataPacketIn->command, dataPacketIn->move, dataPacketIn->game, dataPacketIn->sequenceNumber);
  return 0;
}

int sendMove(int sock, int move, struct sockaddr_in *myAddr, int command, int gameInOut, struct info *dataPacketOut){
  int rc;
  int fromLength = sizeof(struct sockaddr_in);
  int len = 20;
  char buffer[len];

  inet_ntop(AF_INET, &(myAddr->sin_addr), buffer, len);

  printf("*************************\nSent to Client:\nVersion: %d\tCommand: %d\tMove: %d\t\tGame #: %d\tSequence #: %d\n*************************\n", dataPacketOut->version, dataPacketOut->command, dataPacketOut->move, dataPacketOut->game, dataPacketOut->sequenceNumber);

  rc = sendto(sock, dataPacketOut, sizeof(struct info), 0, (struct sockaddr *)myAddr, fromLength);
  if (rc < 0){
    perror("Error sending datagram");
    exit(2);
  }

  return 0;
}

int initSharedState(char board[ROWS][COLUMNS]){
  /* this just initializing the shared state aka the board */
  int i, j, count = 1;

  for (i = 0; i < 3; i++)
    for (j = 0; j < 3; j++){
      board[i][j] = count + '0';
      count++;
    }
  return 0;
}

int createServerSocket(int portNumber, int *sock){
  struct sockaddr_in myAddr;

  /***********************************************************/
  /*                 Create the socket                       */
  /***********************************************************/
  *sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (*sock < 0){
    perror("Error opening datagram socket");
    exit(1);
  }
  // Create myAddr with parameters and bind myAddr to socket
  myAddr.sin_family = AF_INET;
  myAddr.sin_port = htons(portNumber);
  myAddr.sin_addr.s_addr = INADDR_ANY;
  if (bind(*sock, (struct sockaddr *)&myAddr, sizeof(myAddr)) < 0){
    perror("getting socket name");
    exit(2);
  }

  return 1;
}

int findNextGame(struct game *gameData){
  int i;
  printf("[SERVER] Searching for a new game\n*****************\n");

  for (i = 0; i < MAXGAMES; i++)
    if (gameData[i].gameProgress == 0){
      gameData[i].gameProgress = 1;
      printf("[SERVER] Success, starting game #: %d\n", i);
      return i;
    }
  return -1;
}

int findNextSquare(char squareD[ROWS][COLUMNS])
{
  int i, j, count = 0;
  printf("in find open square\n");
  for (i = 0; i < 3; i++)
    for (j = 0; j < 3; j++)
    {
      count++;
      if (squareD[i][j] == count + '0')
      { // then the square is open, choose it
        printf("found open square at %d %d %d\n", i, j, (int)((i * 3) + j + 1));
        return (int)(i * 3) + j + 1;
      }
    }

  return 0;
}

int resetGame(struct game *gameData, int currentGame)
{
  int i, j, count = 1;
  gameData[currentGame].gameProgress = 0;
  gameData[currentGame].turn = 1;
  //gameData[currentGame].lastTimeMoved = 0;
  for (i = 0; i < 3; i++)
    for (j = 0; j < 3; j++)
    {
      gameData[currentGame].gameBoard[i][j] = count + '0';
      count++;
    }
  return 0;
}

int isSquareOccupied(int choice, char board[ROWS][COLUMNS])
{
  int row, column;

  row = (int)((choice - 1) / ROWS);
  column = (choice - 1) % COLUMNS;

  if (board[row][column] == (choice + '0'))
  {
    return 0;
  }
  else
  {
    printf("Error: Invalid move\n");
    return 1;
  }
}