# OnlineTicTacToe_CServerClient_TCP-UDP
A collection of Computer Networking Projects utilizing either a TCP or UDP connection between a Server and Client(s) playing a tictactoe game. Each progressive project is an additional feature or change in protocol use(TCP vs UDP). Acknowledging that UDP is unreliable and creating a detection system to accomodate data that is lost over the network and then resent. 

### ----------- Team Members -----------
    smebellis   -   Ryan Ellis
    Xiao-Lii    -   Lee P.
    rinv12      -   Loureen(Rin) Viloria

## Project 1: Read & Write a File to Send Over Network(TCP)
A server and client will connect remotely with each other, knowing the IP address and the port #. 
The client program will first send a 4-byte header of the file size to be sent, which the server will confirm.
Afterwards, the server will confirm the file name and how many bytes were actually received on the server's end. 
The client should successfully receive this and will print confirmation of the file size received by the server. 
### ----------- Project Tasks -----------
    smebellis -  Ryan Ellis      - Created the server
    Xiao-Lii  -  Lee P.          - Created the client

## Project 2: TicTacToe TCP Connection (Server-Side)
A server and client will connect with each other through TCP to play a game of tictactoe. This tictactoe code will be running as the server and player 1.
### ----------- Project Tasks -----------
    smebellis -  Ryan Ellis      - Created the server
    Xiao-Lii  -  Lee P.          - Created the client

## Project 3: TicTacToe UDP Connection (Server-Side)
A server and client will connect with each other through UDP to play a game of tictactoe. This tictactoe code will be running as the server and player 1. The server and client will need to be running the same VERSION # to play a game of tictactoe w/ each other.
### ----------- Project Tasks -----------
    smebellis -  Ryan Ellis      - Created the server
    Xiao-Lii  -  Lee P.          - Created the client

## Project 4: TicTacToe UDP Connection - Multiple Clients(Server-Side Single Thread)
A server and client will connect with each other through a UDP connection, to play a game of tictactoe. This tictactoe code will be running as the server / player 1. For a tictactoe game to start, the server must be initiated first and will wait for a 2-byte datagram from the client with the game version(hex format) in the 1st index and the command(hex format) in the 2nd index. The server will send back a 4-byte datagram consisting of the version #, command #, move, and game ID #. Moving forward, all datagrams will be this 4-byte datagram format.
### ----------- Project Tasks -----------
    smebellis -  Ryan Ellis      - Created the client
    Xiao-Lii  -  Lee P.          - Created the server

## Project 5: TicTacToe UDP Connection - Multiple Clients(Server-Side Single Thread) - Lost/Dup Packets
A server and client program will connect remotely with each other through a UDP connection, to play a game of tictactoe. This tictactoe code will be running as the server / player 1. For a tictactoe game to start, the server must be initiated first and will wait for a 2-byte datagram from the client with the game version(hex format) in the 1st index and the command(hex format) in the 2nd index. The server will send back a 5-byte datagram consisting of the version #, command #, move, game ID #, and sequence #.

The sequence # will be utilized for keeping track of lost or duplicate packets. In the case the server or client receives a duplicate or old packet by keeping track of the expected sequence #, then it will ignore that packet and attempt to resend its previous packet. Moving forward, all datagrams will be this 5-byte datagram format but should be able to account for a 40-byte buffer.

### ----------- Project Tasks -----------
    smebellis -  Ryan Ellis      - Created the client
    Xiao-Lii  -  Lee P.          - Created a server
    rinv12    -  Rin Viloria     - Created a server 
