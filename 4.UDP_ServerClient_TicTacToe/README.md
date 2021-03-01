## Assignment 4 README - TicTacToe UDP Connection (Server-Side)

### Program Description:

Simultaneously, we'll have a server and client program in which they will connect remotely with each other through a UDP connection, to play a game of tictactoe. This tictactoe code will be running as the server and player 1. For a tictactoe game to start, the server must be initiated first and will wait for a 2-byte header/buffer from the client with the game version(hex format) in the 1st index and the command(hex format) in the 2nd index. </br  >

<b>Note:</b> The server and client will need to be running the same VERSION # to play a game of tictactoe w/ each other. To test with a client-side executable, please utilize the file called: tictactoeClientServerCU. <b><p>To set up executable, please type in the following command line:</b></p> sudo chmod +x FileNameOfExecutable

### Command Line Definitions:
<p>COMMAND = 0 ---> NEW GAME</p>
<p>COMMAND = 1 ---> CONTINUE GAME / MAKE A MOVE</p></br>

Once the client is connected, the game will proceed to ask for each players' move until either one of them wins or the game ends in a draw. <b>Note: If the client wishes to connect to the server, the client will need the server's IP address and port number to connect properly.</b></br>

<b><p>To compile the program, format should be:</b></p>
gcc -o tictactoeOriginal tictactoeOriginal.c

<b><p>To run the server-side of this program, the command line text format should be:</b></p>
./tictactoeOriginal <Port-#> 

<b><p>To run the client-side of this program, the command line text format should be:</b></p>
./tictactoeOriginal <IP_Address> <Port-#> <Player-#(2)> 

