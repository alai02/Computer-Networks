Email: alai02@uoguelph.ca
Port Number: 12045

Description:
    This program consists of 2 executables. A client that sends messages and a server that receives 
    messages in the form of files. The server will save the messages sent into files in the server_files
    folder. The program is written in C and uses TCP protocols. 

Compilation:
    make all: creates 2 executables called server and client as well as a folder to store server files
    make clean: removes executables server and client as well as any files in server_files folder

Usage: 
    ./server: will start the server on port 12045. No arguements required.
    ./client [hostname]:[portnumber] -f [filepath] -b [buffersize]
        hostname: provide the host name or IP of the destination machine
        portnumber: provide the port number the server is running on (12045)
        filepath: the path to the file to be sent to the server
        buffersize: the size of each message containing part the file 
        
Limitations:
    - The buffer length is capped at 20000 bytes and must be at least 1 byte
    - The file must not be empty



    
