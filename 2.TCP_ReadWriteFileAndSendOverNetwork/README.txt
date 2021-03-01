Read Me

Program Description:

Simultaneously, we'll have a server and client program in which they will connect remotely with each other, knowing the IP address and the port #. 

The client program will first send a 4-byte header of the file size to be sent, which the server will confirm.
Next, the file name with a max size of 255-bytes will be sent from the client to the server. 
Afterwards, the server will confirm the file name and how many bytes were actually received on the server's end. 
The client should successfully receive this and will print confirmation of the file size received by the server. 
To test with a server executable, please look at the 'TestExecutable' directory.

To setup the server executable, the following command line should be input:
sudo chmod +x <FilenameOfExecutable>

To compile the client program, format should be:
gcc -o <Client-FileName> <Client-FileName-With-FileTypeExtension>

To run the client program, text format should be:
./<FileName> <IP-Address> <Port-#> <File-To-Send-Name> 

Note: If running program and attempting to send a text file, you DO NOT need to declare the file type extension. However, with any other file types,
the extension must be explicitly typed in the initial run command of the program. 

