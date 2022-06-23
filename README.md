Installation guide and running guide

To install the program on Ubuntu, first you need to make sure that the OpenSSL library is installed on your machine.
Then, follow the steps given below.

1. Navigate to the project directory and run the makefile from the terminal with the command ’make’
2. You will get two executable files inside the tracker and the peer folder.
3. Go inside each of these folders and run the commands ./peer and ./tracker.
4. Create another folder named peer2, and copy executable peer file and config file to the corresponding location.
5. Modify the config contents of peer2 as needed. Note that the config file’s first line denotes the peer’s IP
address and port, the second line denotes its ID, and last line denotes the tracker server’s IP address and port.
6. We have two peers and one tracker server running.
7. We can start sharing files, see shared files’ list, and download from the other peers.

You need to follow the steps given below to issue commands for sharing files, see all the available files to
download, and download from other peers.

1. To share any file, you need to give an input to the terminal as "create filename description"
2. To see all the shared file, you need to give an input to the terminal as "req list"
3. To download a file from the list, you need to give an input to the terminal as "get filename full"
