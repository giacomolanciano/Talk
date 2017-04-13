# Talk
Very simple chat system for Unix OS, developed within Operating Systems course of B.Sc. in Computer Science & Control Engineering at Università degli studi di Roma "La Sapienza" (A.Y. 2014/2015). It is written in C language and is based on socket programming. This project has been initially developed in collaboration with Davide Mazza.

# How to run
It is possible to use the Makefile:  
1. Compile all modules typing on terminal `$ make` or `$ make all`.  
2. Run **server** module to initialize the system, typing `$ make run-server`.  
3. Run as many **clients** as wanted, typing `$ make run-client` (on many different terminals).

In alternative:  
1. Compile **server** module typing on terminal `$ gcc server.c stack.c -lpthread -o bin/server`.  
2. Compile **client** module typing on terminal `$ gcc client.c -lpthread -o bin/client`.  
3. Run **server** module to initialize the system, typing `$ bin/server`.  
4. Run as many **clients** as wanted, typing `$ bin/client` (on many different terminals).

**NOTE**: executables are created in bin folder, so that Git does not keep track of them.

## Run modules on different machines
In **client.c** file, it is possible to define the IP address of the machine where **server** module is running, giving a value to `CUSTOM_IP` macro (localhost by default).  
To enable this option, type on terminal `$ make distr`. In alternative, compile the only **client** module typing  
`$ gcc client.c -lpthread -o bin/client -DCUSTOM_IP`.

**NOTE**: if machines are not within the same subnet, be sure that the one where server module is running is able to accept HTTP requests.
