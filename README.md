# TFTP Program
TFTP project for CSS432

**How to compile**

1. Create the .out files

just type

`make` 

to spawn the s.out (server executable) and
the c.out (client executable).

2. Execute server first, specify port number

`./SERVER/s.out -p 51917`

3. Execute client next, specify write/read, file name, port number, etc.

`./CLIENT/c.out -w client.txt -p 51917`

*Illustration of directory tree below*

>/TFTP-Program
>|--/CLIENT
>    |--Client.cpp
>    |--client.txt
>|--/SERVER
>    |--Server.cpp
>    |--server.txt
>|--Makefile
>|--README.md`