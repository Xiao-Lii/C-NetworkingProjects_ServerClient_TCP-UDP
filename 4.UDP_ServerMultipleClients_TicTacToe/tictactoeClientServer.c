/**********************************************************/
/* This program is a client server version of the simple  */
/* pass back and forth game. In this game, you pass in a  */
/* port number and whether you want to be player 1 or 2   */
/**********************************************************/

#include <stdio.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <net/if.h>
#include <signal.h>
#include <sys/time.h>

#define TIMETOWAIT 45
#define ROWS  3
#define COLUMNS  3
#define NUMBEROFBYTESINMESSAGE 1
#define CURRENTVERSION 2
#define NEWGAME 0
#define MOVE 1
#define SIZEOFMESSAGE 5
// DMO ERRORS
#define NOERROR 0
#define BADMOVE 1
#define GAMEOUTOFSYNC 2
#define INVALIDREQUEST 3
#define GAMEOVER 4
#define GAMEOVERACK 5
#define BADVERSION 6
/**********************************************************/
/* pre define all the functions we will write/call        */
/**********************************************************/

int checkwin(char board[ROWS][COLUMNS]);
void print_board(char board[ROWS][COLUMNS]);
int tictactoe(int cd, int playerNumber, char board[ROWS][COLUMNS], int playingGame, struct sockaddr_in *from);
int getMoveFromNet(int cd, char result[NUMBEROFBYTESINMESSAGE], int playingGame, struct sockaddr_in * from);
int createServerSocket(int portNumber, int *sock);
int createClientSocket(char *ipAddress, int portno, struct sockaddr_in *sin_addr, int *sock);
int sendToNet(int sock, char move [4], struct sockaddr_in *to);
int initSharedState(char board[ROWS][COLUMNS]);
int isSquareTaken(int choice, char board[ROWS][COLUMNS]);

int main(int argc, char *argv[]) /* server program called with port # */
{
  /* variable definitions */
  char board[ROWS][COLUMNS];
  int sock, connectedSocket,   rc;
  int portNumber, playerNumber;
  struct sockaddr_in from;
  struct sockaddr_in sin_addr; /* structure for socket name 
				* setup */

  if(argc < 3) { // means we didn't get enough parameters
    printf("usage: tictactoe <port_number> <1 or 2 - which player are you?> <ip addr> (if player 2)\n");
    exit(1);
  }
  if (argc < 4 && strtol(argv[2],NULL, 10) == 2){
    printf("usage: tictactoe <port_number> <1 or 2 - which player are you?> <ip addr> (if player 2)\n");
    exit(1);
    
  }
  int playingGame = 0;
  signal (SIGPIPE, SIG_IGN);
  /**********************************************************/
  /* convert the parameter for port and player number       */
  /**********************************************************/

  portNumber = strtol(argv[1], NULL, 10);
  playerNumber = strtol (argv[2], NULL, 10);


  if (playerNumber == 1){ // Create a socket, listen on it, then accept 1 connection and die after
    /***************************************************************************/
    /* If player 1, then create a socket and get ready to receive a connection */
    /***************************************************************************/
    rc = createServerSocket(portNumber, &sock); // call a subroutine to create a socket


    /***************************************************************************/
    /* If here then we have a valid socket. Init the state and get ready for   */
    /* an incoming connection request. This will loop forever                  */
    /***************************************************************************/
    while(1){ //loop forever and accept all comers for games!
      char data[5];
      int fromLength = sizeof(struct sockaddr_in); // HAS to be set in linux...
      initSharedState(board);

      // minimizing changes from STREAM version.
      connectedSocket = sock ; // DGRAM doesn't understand connections, but STREAM did, so using same variable

      // for DGRAM we have to receive a game request.
      memset (data, 0, 5); // always zero out the data in C
      
      rc = recvfrom(connectedSocket, data, SIZEOFMESSAGE, 0, (struct sockaddr *)&from, (socklen_t *)&fromLength);
      if (rc <0){
	printf ("main: timeout waiting for a game. \n");
	continue; // lets me skip to the top of the loop
      }
      if (data[0] != CURRENTVERSION){
	printf ("Bad version\n");
	printf ("received %x %x\n", data[0], data[1]);
	continue; // let's me skip to the top of the loop
      }
      if (data[1] != NEWGAME){
	printf("main: got a move, was expecting a newgame, punting\n");
	continue; // error - invalid state change
      }
      if (playingGame == 1 && data[1] == NEWGAME){
	printf ("received a newgame request, ignore it\n");
	continue; // error - invalid state change
      }
      else {
	printf ("received game request with right version %x %x \n", data[0], data[1]);
	playingGame = 1;
      }
      tictactoe(connectedSocket, playerNumber, board, playingGame, &from );
      playingGame = 0; // returned from game, so ready for another
      
    } // end of the infinite while loop for player 1
  } // end of the if player==1 statement
  else if (playerNumber ==2){
    /***************************************************************************/
    /* If player 2 create a socket for sending, ask for new game, then         */
    /* play the game                                                           */
    /***************************************************************************/

    /* initialize the shared board */
    initSharedState(board);

    rc =  createClientSocket(argv[3], (int)strtol(argv[1], NULL, 10), &sin_addr, &sock);
    playingGame = 1; // for consistency
    tictactoe(sock, playerNumber, board , playingGame, &from); // I have a connection, call tictactoe, with new socket connection and playerNumber
    close (sock);
  }
  return 0;
}


int tictactoe(int sock, int playerNumber, char board[ROWS][COLUMNS], int playingGame, struct sockaddr_in *from)
{
  int player = 1, i, choice;
  int row, column;
  char mark;
  int rc;
  char junk[10000];
  char result [SIZEOFMESSAGE];
  int moveCount = 0;
  //  int iWon = 0;
  e

  memset (result, 0, SIZEOFMESSAGE); //always zero out the data in C

  do{
    print_board(board);
    player = (player % 2) ? 1 : 2;
    /******************************************************************/
    /* If it is 'this' player's turn, ask the human for a move, then  */
    /* send the move to the other player. If it is not this player's  */
    /* turn, then wait for the other player to SEND you a move        */
    /******************************************************************/

    if (player == playerNumber){
      do{
	printf("Player %d, enter a number:  ", player);
	rc = scanf("%d", &choice); // get input
	if (rc ==0){ 
	  choice = 0;// cleanup needed bad input
	  printf ("bad input trying to recover\n");
	  rc = scanf("%s", junk);
	  if (rc ==0){
	    /* they entered more than 10000 bad characters, quit) */
	    printf ("garbage input\n");
	    return 1;
	  }
	}
      }while ( ((choice < 1) || (choice > 9)) || (isSquareTaken (choice, board))); // loop until good data
      
      mark = (player == 1) ? 'X' : 'O'; // determine which player is playing, set mark accordingly
      printf ("tictactoe: playerNumber is %d, player is %d\n", playerNumber, player);
      printf ("tictactoe: mark is '%c'\n", mark);

      /******************************************************************/
      /*  little math here. you know the squares are numbered 1-9, but  */
      /* the program is using 3 rows and 3 columns. We have to do some  */
      /* simple math to conver a 1-9 to the right row/column            */
      /******************************************************************/
      row = (int)((choice-1) / ROWS);
      column = (choice-1) % COLUMNS;

      board[row][column] = mark;


      result [0] = CURRENTVERSION;
      result [1] = MOVE;
      result [2] = (char)choice ;

#ifdef OSUCODE      
      result [2] = (char)choice + 0x30;
#endif

      moveCount = moveCount +2; // not used...yet

      i = checkwin(board);

      switch (checkwin(board)){
         case 0:
         case 1:
	   printf ("found a winner, its me!\n");
	   break;
      }

      printf ("tictactoe: sending hex value of %x  %xto network\n", result[0], result[1]);
      sendToNet(sock, result, from); // send it to the network 
    }
    else{ // this 'else' means that it is the other player's move (network)
      choice = getMoveFromNet(sock, result, playingGame, from); // Wait for move from the network
      printf ("tictactoe: received move %d\n", choice);
      if (choice == -1){
	printf ("tictactoe: move indicates timeout\n");
	return -1; //timeout
      }
      if (isSquareTaken (choice, board)){
	printf ("tictactoe - bad move '%d' from other side, exiting out \n", choice);
	return -1; // can't happen YET
      }
      if (choice <=0){
	printf ("did the other side die?\n");
	return -1; // timeout dup of above
      }
    } // end of 'else' - got move from network
    
    mark = (player == 1) ? 'X' : 'O'; // determine which player is playing, set mark accordingly
    /******************************************************************/
    /*  little math here. you know the squares are numbered 1-9, but  */
    /* the program is using 3 rows and 3 columns. We have to do some  */
    /* simple math to conver a 1-9 to the right row/column            */
    /******************************************************************/
    row = (int)((choice-1) / ROWS);
    column = (choice-1) % COLUMNS;

    board[row][column] = mark;

    /* check is someone won */
    i = checkwin(board);
    player++; // change the player number
  }while (i ==  - 1);


  /* print the board one last time */
  print_board(board);

  /* check to see who won */
  if (i == 1)
    printf("==>\aPlayer %d wins\n ", --player);
  else
    printf("==>\aGame draw");

  return 0;
}



int checkwin(char board[ROWS][COLUMNS])
{
  /************************************************************************/
  /* brute force check to see if someone won, or if there is a draw       */
  /* return a 0/1 if the game is 'over',  return -1 if game should go on  */
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

    return 0;
  else
    return  - 1;
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

 
int  createServerSocket(int portNumber, int *sock){
  /*****************************************************************/
  /* create a server socket and bind to the 'portNumber' passed in */
  /*****************************************************************/
  
  struct sockaddr_in name;

  /*create socket*/
  *sock = socket(AF_INET, SOCK_DGRAM, 0);
  if(*sock < 0) {
    perror("opening datagram socket");
    exit(1);
  }

  /* fill in name structure for socket, based on input   */
  name.sin_family = AF_INET;
  name.sin_port = htons(portNumber);
  name.sin_addr.s_addr = INADDR_ANY;
  /* bind to the socket address */
  if(bind(*sock, (struct sockaddr *)&name, sizeof(name)) < 0) {
    perror("getting socket name");
    exit(2);
  }

  return 1;

}
int getMoveFromNet(int sd, char result[SIZEOFMESSAGE], int playingGame, struct sockaddr_in * from)
{
  /*****************************************************************/
  /* instead of getting a move from the user, get if from the net  */
  /* by doing a receive on the socket. this is all still STREAM    */
  /*****************************************************************/
  int movePosition;
  int rc;
  struct timeval tv;

  int fromLength = sizeof(struct sockaddr_in);

  //DMO  unsigned int ret;
  //DMO  char *data = (char*)&ret;
  char *data = result;
  //  int left = sizeof(ret);
  tv.tv_sec = TIMETOWAIT;
  tv.tv_usec = 0;
  if (setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)))
    perror ("error"); // might want to exit here   
  

  do {
    rc = recvfrom(sd, data, SIZEOFMESSAGE, 0, (struct sockaddr *) from, (socklen_t *)&fromLength);
    if (rc <0){
      printf ("getMoveFromNet: timeout occured\n");
      return -1;
    }
    // have to handle timeout still. if timeout and playing game, return from here
    printf ("getMoveFromNet: received rc %d, hex %x %x\n", rc, data[0], data[1]);
    if (data[0] != CURRENTVERSION){
      printf ("Bad version\n");
      continue;
    }
    if (playingGame == 1 && data[1] == NEWGAME){
      printf ("received a newgame request, ignore it\n");
      continue;
    }
    else if (data[1] == MOVE){
      printf ("getMoveFromNet: received  %x %x %x the network \n",data[0], data[1], data[2]);
      movePosition = (int)data[2];

#ifdef OSUCODE
      movePosition -= 0x30;
#endif
      return (movePosition);
    }
  }
  while (1);
    
  return -1; // should never get here
}

int createClientSocket(char *ipAddress, int portno, struct sockaddr_in *sin_addr, int *sock){
  /*****************************************************************/
  /* if player 2, create a socket that is used to bind TO player   */
  /* 1's socket.                                                   */
  /*****************************************************************/

  int namelen = sizeof (struct sockaddr_in);

  printf ("creating a socket\n");
  if((*sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
    perror("error openting datagram socket");
    exit(1);
  }

  inet_pton(AF_INET, ipAddress, &(sin_addr->sin_addr));

  /* copy in data for socket             */

  sin_addr->sin_family = AF_INET;
  sin_addr->sin_port = htons(portno); // both sides use NETWORK order for ports

    // for DGRAM i need to send a connect request. will do that here
  char buffer[SIZEOFMESSAGE];
  memset (buffer, 0, SIZEOFMESSAGE);
  buffer[0] = CURRENTVERSION;
  buffer[1] = NEWGAME;
  if(sendto(*sock, buffer, SIZEOFMESSAGE, 0, (struct sockaddr *)sin_addr, namelen) < 0) {
    perror("error sending datagram");
    exit(5);
  }

  printf ("send the game request\n");

  return 0;
  // At this point we have a valid socket structure. We should now go open the file and setup to read and send
}

int sendToNet(int sock, char result[SIZEOFMESSAGE], struct sockaddr_in * to){
  /*****************************************************************/
  /* This function is used to send data to the other side. make    */
  /* sure data is in network order.                                */
  /*****************************************************************/
  
  //  char data[NUMBEROFBYTESINMESSAGE];

  char * data = result;
  int namelen = sizeof (struct sockaddr_in);

  /*****************************************************************/
  /* Write data out.                                               */
  /* loop to make sure all the data is sent                        */
  /*****************************************************************/


  if(sendto(sock, data, SIZEOFMESSAGE, 0, (struct sockaddr *)to, namelen) < 0) {
    perror("error sending datagram");
    exit(5);
  }


  return 0;
}

int initSharedState(char board[ROWS][COLUMNS]){    
  /*****************************************************************/
  /* This function is used to initialize the shared board          */
  /*****************************************************************/
  int i, j, count = 1;
  printf ("in sharedstate area\n");
  for (i=0;i<3;i++)
    for (j=0;j<3;j++){
      board[i][j] = count + '0';
      //    printf("square [%d][%d] = %c\n", i,j,board[i][j]);
      count++;
    }


  return 0;

}
int isSquareTaken(int choice, char board[ROWS][COLUMNS]){
  /******************************************************************/
  /*  little math here. you know the squares are numbered 1-9, but  */
  /* the program is using 3 rows and 3 columns. We have to do some  */
  /* simple math to conver a 1-9 to the right row/column            */
  /******************************************************************/
  int row, column; 

  row = (int)((choice-1) / ROWS);
  column = (choice-1) % COLUMNS;

  /* see if square is 'free' */
  if (board[row][column] == (choice+'0'))
    return 0; // 0 means the square is free
  else
    {
      printf("Invalid move ");
      return 1; // means the square is taken
    }
}
