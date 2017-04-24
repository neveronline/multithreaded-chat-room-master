# Chat Room
A chat room including the client and server. 

## Instructions
1. Clone the project to your desktop.
2. Navigate to "hw6" then "make"
3. Start the client and server on the localhost.

### We have added an optional parameter -a to specify the location of the audit file.
./client [-hcv] [-a FILE] NAME SERVER_IP SERVER_PORT
-a FILE Path to the audit log file.
-h Displays this help menu, and returns EXIT_SUCCESS.
-c Requests to server to create a new user
-v Verbose print all incoming and outgoing protocol verbs
& content.

NAME        The username to display when chatting.
SERVER_IP   The ipaddress of the server to connect to.
SERVER_PORT The port to connect to.

### We have added a new positional argument AUDIT_FILE_FD to pass the file descriptor of the audit log file.
./chat UNIX_SOCKET_FD AUDIT_FILE_FD
UNIX_SOCKET_FD  The Unix Domain file descriptor number.
AUDIT_FILE_FD   The file descriptor of the audit.log created in the
client program.

### We have added an optional parameter -t to specify the number of threads used for the login queue.
./server [-hv] [-t THREAD_COUNT] PORT_NUMBER MOTD [ACCOUNTS_FILE]
-h Displays help menu & returns EXIT_SUCCESS.
-t THREAD_COUNT The number of threads used for the login queue.
-v Verbose print all incoming and outgoing protocol verbs & content.
PORT_NUMBER Port number to listen on.
MOTD Message to display to the client when they connect.
ACCOUNTS_FILE File containing username and password data to be loaded upon
execution.
