/**********************************************************/
/* This program is a 'pass and play' version of tictactoe */
/* Two users, player 1 and player 2, pass the game back   */
/* and forth, on a single computer                        */
/**********************************************************/

// Name: Lee Phonthongsy       Assignment: Assignment / Lab 3 (Server Side - Player 1)

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


/* #define section, for now we will define the number of rows and columns */
#define ROWS  3
#define COLUMNS 3
#define BACKLOG 10


/* C language requires that you predefine all the routines you are writing */
int checkwin(char board[ROWS][COLUMNS]);
void print_board(char board[ROWS][COLUMNS]);
int tictactoe();
int initSharedState(char board[ROWS][COLUMNS]);
int checkArguments(int argc); 
int checkConnection(int sock, char msg[]); 
int startConnection(int argc, char *argv[], int client);
int checkBoard(int choice, char board[ROWS][COLUMNS]); 

int main(int argc, char *argv[]){
  int rc, client;
  char board[ROWS][COLUMNS]; 
  char msg[20]; 

  // Initializes & Establishes the Server's Connection 
  client = startConnection(argc, argv, client);

  rc = initSharedState(board);    // Initialize the 'game' board
  // Error Handling - Check if Board Initialized Properly
  if (rc != 0){
    printf("Error: Board wasn't initialized properly.\n");
    exit(1);
  }

  rc = tictactoe(board, client);  // call the 'game' 
  // Error Handling - Check if Game Ended Properly
  if (rc != 0){
    printf("Error: Game didn't end properly.\n");
    exit(1);
  }

  return 0; 
}

int startConnection(int argc, char *argv[], int client){
  /************************************************************************/
  /* Checks to ensure the correct number of arguments have been placed    */
  /* in command line for server-side / player 1                           */
  /************************************************************************/
  int server;
  struct sockaddr_in server_sock, from_address;
  socklen_t fromLength; 

  // Confirms Correct # of Parameters 
  checkArguments(argc);           

  // Initialize the Socket
  checkConnection(server = socket(AF_INET, SOCK_STREAM, 0), "Socket creation");
  // Create the Socket  
  server_sock.sin_family = AF_INET;                 
  server_sock.sin_port = htons(atoi(argv[1]));      // Saving Port # from Command Line 
  server_sock.sin_addr.s_addr = INADDR_ANY;         // Saving any available IP address to Server's socket
  memset(server_sock.sin_zero, '\0', sizeof(server_sock.sin_zero));

  // Checks Each Step of Establishing the Connection with the Client 
  checkConnection(bind(server, (struct sockaddr *)&server_sock, sizeof(server_sock)), "Bind"); 
  checkConnection(listen(server, BACKLOG), "Listen"); 
  checkConnection(client = accept(server, (struct sockaddr *)&from_address, &fromLength), "Connect"); 

  return client;
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
    if (connection < 0) {   // If connection failed 
      printf("[SERVER] %s failed. Please try again\n", msg); 
      exit(1);
    } 
    else {  // If connection successful 
      printf("[SERVER] %s successful.\n", msg);
      return connection; 
    }
    
}


int tictactoe(char board[ROWS][COLUMNS], int client)
{
  /* this is the meat of the game, you'll look here for how to change it up */
  int player = 1;                                       // keep track of whose turn it is
  int i, choice;                                        // used for keeping track of choice user makes
  int row, column, returnCode;
  char mark;                                            // either an 'x' or an 'o'
  char c; 

  /* loop, first print the board, then ask player 'n' to make a move */

  do {
    print_board(board);                                 // call function to print the board on the screen
    player = (player % 2) ? 1 : 2;                      // Mod math to figure out who the player is

    if (player == 1){
      do {
        printf("Player %d, enter a number: ", player);   // print out player so you can pass game
        returnCode = scanf("%d", &choice);               // using scanf to get the choice

        if (returnCode == 0){
          getchar();
        }
      } while ((choice < 1) || (choice > 9) || (checkBoard(choice, board))); 

      choice = choice + 48; 
      c = (char)choice;                                 // Cast user's choice to char variable, c
      returnCode = write(client, &c, 1);                // Send move(c) to client 

      choice = choice - 48; 
      // Error Handling - Send / Receive between players 
      if (!returnCode){
        printf("[SERVER] Error: Connection was lost with player 2\n");
        exit(1);
      }
    }
    else {
      returnCode = read(client, &c, 1);      // Prepare to receive client's move(c) as a char 
      // Error Handling - Send / Receive between players 
      if (!returnCode){
        printf("[SERVER] Error: Connection was lost with player 2\n");
        exit(1);
      }
      else{
        c = c - 48;
        choice = (int)c;                                // Cast opponent's choice as in interger 
      }
    }

    mark = (player == 1) ? 'X' : 'O';                   // depending on who the player is, either us x or o

    /******************************************************************/
    /** little math here. you know the squares are numbered 1-9, but  */
    /* the program is using 3 rows and 3 columns. We have to do some  */
    /* simple math to conver a 1-9 to the right row/column            */
    /******************************************************************/

    row = (int)((choice-1) / ROWS);
    column = (choice-1) % COLUMNS;

    if (board[row][column] == (choice + '0'))
      board[row][column] = mark;

    /* after a move, check to see if someone won! (or if there is a draw */
    i = checkwin(board);
    
    player++;
  } while (i == - 1);     // -1 means no one won
    
  /* print out the board again */
  print_board(board);
    
  if (i == 1)             // means a player won!! congratulate them
    printf("==>\aPlayer %d wins\n ", --player);
  else
    printf("==>\aGame draw");                         // ran out of squares, it is a draw
  
  return 0;
}


int checkBoard(int choice, char board[ROWS][COLUMNS]){
  /******************************************************************/
  /* We'll pass in the player's choice to check if it's already     */
  /* occupied on our tictactoe board. If so, it will return a -1.   */
  /* If the spot is found to be empty it will return a 0.           */
  /******************************************************************/
  int row, column;

  row = (int)((choice-1) / ROWS);
  column = (choice-1) % COLUMNS;

  /* first check to see if the row/column chosen is has a digit in it, if it */
  /* square 8 has and '8' then it is a valid choice                          */

  if (board[row][column] == (choice + '0'))
    return 0;       // Returns 0 if empty 
  else 
    return -1;      // Returns -1 if spot is already occupied 
}


int checkwin(char board[ROWS][COLUMNS])
{
  /************************************************************************/
  /* brute force check to see if someone won, or if there is a draw       */
  /* return a 0 if the game is 'over' and return -1 if game should go on  */
  /************************************************************************/
  if (board[0][0] == board[0][1] && board[0][1] == board[0][2] ) // row matches
    return 1;
        
  else if (board[1][0] == board[1][1] && board[1][1] == board[1][2] ) // row matches
    return 1;
        
  else if (board[2][0] == board[2][1] && board[2][1] == board[2][2] ) // row matches
    return 1;
        
  else if (board[0][0] == board[1][0] && board[1][0] == board[2][0] ) // column
    return 1;
        
  else if (board[0][1] == board[1][1] && board[1][1] == board[2][1] ) // column
    return 1;
        
  else if (board[0][2] == board[1][2] && board[1][2] == board[2][2] ) // column
    return 1;
        
  else if (board[0][0] == board[1][1] && board[1][1] == board[2][2] ) // diagonal
    return 1;
        
  else if (board[2][0] == board[1][1] && board[1][1] == board[0][2] ) // diagonal
    return 1;
        
  else if (board[0][0] != '1' && board[0][1] != '2' && board[0][2] != '3' &&
	   board[1][0] != '4' && board[1][1] != '5' && board[1][2] != '6' && 
	   board[2][0] != '7' && board[2][1] != '8' && board[2][2] != '9')

    return 0; // Return of 0 means game over
  else
    return  - 1; // return of -1 means keep playing
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



int initSharedState(char board[ROWS][COLUMNS]){    
  /* this just initializing the shared state aka the board */
  int i, j, count = 1;
  printf ("in sharedstate area\n");
  for (i=0;i<3;i++)
    for (j=0;j<3;j++){
      board[i][j] = count + '0';
      count++;
    }
  return 0;
}