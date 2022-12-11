# TFTP Program
TFTP project for CSS432

# How to compile
1. Create the .out files

just type
`make` 

to spawn the s.out (server executable) and
the c.out (client executable).

2. Execute server first
`./SERVER/s.out -p 51917`

3. Execute client next
`./CLIENT/c.out -w client.txt -p 51917`