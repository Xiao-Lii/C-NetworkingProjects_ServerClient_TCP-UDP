/*
Name: Lee Phonthongsy
Assignment: Assignment/Lab 2
Program Description: Client program which will read the size of a file. Send that file size to the server, 
in a 4-byte header, then a file name with a max limit of 255 characters, as well as the entire file. Once the
server receives everything properly, the client will wait until it receives a 4-byte header of the file size
received and display the success back to the user. 
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>           // FPR GETTING FILE SIZE USING STAT()
#include <sys/sendfile.h>       // FOR SENDING FILES
#include <fcntl.h>              // FOR 0_RDONLY

// Function Headers 
int checkConnection(int rc);
int getFileSize(FILE *inputfile, int fileSize);
int readServerHeader(int client, int filesize); 
void sendFile(int client_sock, int returnCode, int fileSize, char *buffer, FILE *localfile); 

int main(int argc, char *argv[]){
    int client_sock, returnCode, fileSize;
    struct sockaddr_in server_ip;
    char buffer[1], fileName[255], msgBuffer[255], *filePointer;
    int sizeBuffer[1];
    FILE *localfile;

    // Verifying Correct # of Parameters 
    // 1. ftpc 2.<Remote-IP> 3.<Remote-Port> 4.<Local-File-to-Transfer>
    if (argc < 4){
        printf("Error: Incorrect # of parameters, run format should be:\n"
        "./ftpc <Remote-IP> <Remote-Port> <Local-File-to-Transfer>\n");
        exit(1);
    }
    
    // INITIALIZE THE SOCKET
    client_sock = socket(AF_INET, SOCK_STREAM, 0);

    // ERROR-HANDLING - INITIALIZING THE SOCKET
    if (client_sock < 0) {
        printf("[CLIENT] ERROR: Socket creation failed.\n"); 
        exit(1);
    }
    else
        printf("[CLIENT] Socket created successfully.\n");

    // Attaching port number and ip address to connect socket to
    server_ip.sin_family = AF_INET;
    server_ip.sin_port = htons(atoi(argv[2]));                  // Port # from Server Side
    server_ip.sin_addr.s_addr = INADDR_ANY;                     // inet_addr(serverIP)

    // Connect & check connection with Server 
    checkConnection(connect(client_sock, (struct sockaddr *) &server_ip, sizeof(struct sockaddr_in)));

    // Resetting the buffers
    memset(sizeBuffer, 0, 4);                   // Resetting the Size Buffer
    memset(buffer, 0, 1);                       // Resetting the Buffer
    memset(fileName, ' ', 255);                 // Resetting the file name Buffer 
    
    // We have to tell program to open file in read binary(rb) mode
    if ((localfile = fopen(argv[3], "rb")) == NULL){
        printf("Error opening the input file: %s\n", argv[3]);
        exit(1);
    }

    // Call getFileSize Method, save # to buffer, & Send to Server - 4-Byte Header 
    fileSize = getFileSize(localfile, fileSize);
    sizeBuffer[0] = htonl(fileSize);
    send(client_sock, sizeBuffer, sizeof(sizeBuffer), 0);        

    // Saving File Name to fileName Buffer & Sending to Server
    memcpy(fileName, argv[3], sizeof(argv[3])); 
    printf("[CLIENT] File name: %s\n", fileName);
    send(client_sock, fileName, sizeof(fileName), 0);
    
    // Reads Entire File into a buffer and sends to the Server
    for (int i; i < fileSize; i++){
        returnCode = fread(buffer, 1, sizeof(buffer), localfile);
        send(client_sock, buffer, sizeof(buffer), 0);
    }

    // Close file
    fclose(localfile); 

    // Client waits to receive 4-byte header from Server 
    readServerHeader(client_sock, fileSize);
}


// Check Client Connection to Server = Successful 
int checkConnection(int rc){
    // Error Handling - Connection
    if (rc < 0){
        perror("[CLIENT] ERROR: Failed to connect to the server.\n");
        exit(1);
    }
    else
        printf("[CLIENT] Connection to server successful.\n");
}


// Reads 4-Byte Header from Server & Saves to a Return Code 
int readServerHeader(int client, int filesize){
    int returncode = read(client, &filesize, 4);

    if (returncode != 4){                       // Failed Received or Incorrect Header Size from Server 
        printf("[CLIENT]: Error: Header never received from server or isn't 4-bytes\n");
        exit(1);
    }
    else {                                      // Successful Receive from Server
        filesize = ntohl(filesize);             // Prepping Filesize Integer received from Server
        printf("[CLIENT] Header received from Server. Server received a file size of: %d\n", filesize);
        return filesize;
    }
}


// Calculates & Returns a file's size
int getFileSize(FILE *inputfile, int fileSize){
    fseek(inputfile, 0, SEEK_END);              // Read file size from beginning to the end of inputFile
    fileSize = (ftell(inputfile));              // Save file size with ftell 

    rewind(inputfile);                          // Rewind back to starting index of reading file
    fseek(inputfile, 0, SEEK_CUR);              // Seek/Change starting index of read file
    printf("[CLIENT] Size of file is: %d\n", fileSize);
    return fileSize;                            // Return FileSize to Main 
}
