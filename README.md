# Talk
Very simple chat system for Unix OS, developed within “Operating System” course of my BoS of Computer Science &amp; Control Engineering. It is written in C language and is based on socket programming.

# How to run
1. Compile *server* module typing on terminal `$ gcc server.c stack.c -lpthread -o bin/server`.  
2. Compile *client* module typing on terminal `$ gcc client.c -lpthread -o bin/client`.  
3. Run *server* module to initialize the system, typing `$ bin/server`.  
4. Run as many *clients* as wanted, typing `$ bin/client` (on many different terminals).

*NOTE*: executables are created in bin folder, so that Git doesn't keep track of them.

# Running modules on different machines
In client.c file, it is possible to define the IP address of the machine where server module is running (localhost by default).  
To enable this option, compile client module typing `$ gcc client.c -lpthread -o bin/client -DCUSTOM_IP`.  

*NOTE*: if machines are not within the same subnet, be sure that the one where server module is running is able to accept HTTP requests.