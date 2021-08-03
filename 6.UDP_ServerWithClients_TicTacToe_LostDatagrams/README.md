## Assignment 6 README
### TicTacToe UDP Connection - Multiple Clients(Server-Side Single Thread) - Lost/Dup Packets

### Program Description:

Simultaneously, we'll have a server and client program in which they will connect remotely with each other through a UDP connection, to play a game of tictactoe. This tictactoe code will be running as the server / player 1. For a tictactoe game to start, the server must be initiated first and will wait for a 2-byte datagram from the client with the game version(hex format) in the 1st index and the command(hex format) in the 2nd index. The server will send back a 5-byte datagram consisting of the version #, command #, move, game ID #, and sequence #. 

<p>The sequence # will be utilized for keeping track of lost or duplicate packets. In the case the server or client receives a duplicate or old packet by keeping track of the expected sequence #, then it will ignore that packet and attempt to resend its previous packet. Moving forward, all datagrams will be this 5-byte datagram format but should be able to account for a 40-byte buffer.</p></br>

<b>Note:</b> The server and client will need to be running the same VERSION # to play a game of tictactoe w/ each other. To test with a client-side executable, please utilize the file called: tictactoeClientServerCU. <b><p>Please don't forget to set up executable prior to running.</b></p>

### Command Line Definitions:
<p>CURRENT VERSION = 4 </p>
<p>COMMAND = 0 ---> NEW GAME</p>
<p>COMMAND = 1 ---> CONTINUE GAME / MAKE A MOVE</p>
<p>COMMAND = 2 ---> GAME OVER</p>
</br>

Once the client is connected, the game will proceed to ask for each players' move until either one of them wins or the game ends in a draw. <b>Note: If the client wishes to connect to the server, the client will need the server's IP address and port number to connect properly.</b></br>

<b><p>To compile the program, format should be:</b></p>
gcc -o tictactoeServer tictactoeServer.c

<b><p>To run the server of this program, the command line text format should be:</b></p>
./tictactoeServer <Port-#> 

<b><p>To run the client of this program, the command line text format should be:</b></p>
./tictactoeServer <IP_Address> <Port-#> <Player-#(2)> 

<b><p>To run the troll client of this program, the command line text format should be:</b></p>
./tictactoeServer <Client_IP> <Client_Port-#> <Server_IP> <Server_Port-#> <TrollPort#> 

