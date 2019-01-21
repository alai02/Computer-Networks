Email: alai02@uoguelph.ca & ldembekb@uoguelph.ca
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
    To shutdown the server:
      Enter 2 while it is running:
      You will be prompted to do a hard or soft shutdown.
      Enter h for a soft shutdown and any other key for a hard shutdown.
      A hard shutdown will stop the server without waiting for any in progress
      file transfers.
      A soft shutdown will stop accepting new file transfers and wait for
      any currently running transfers before shutting down.
    Enter 1 while the server is running in order to view a list of currently
    running file transfers. If there are no active transfers this will wait
    until an active transfer is started and then will display it.

    ./client [hostname]:[portnumber] -f [filepath] -b [buffersize]
        hostname: provide the host name or IP of the destination machine
        portnumber: provide the port number the server is running on (12045)
        filepath: the path to the file to be sent to the server
        buffersize: the size of each message containing part the file

limitations:
    - The buffer length is capped at 20000 bytes and must be at least 1 byte
    - The file must not be empty
    - The server does not accept multiple active transfers with duplicate file names.

script: 
    We used wonderland.txt and duplicated the file with incrementing file names and 
    used those to send multiple client requests from the runMultiClient.sh script.

