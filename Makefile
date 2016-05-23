BIN = bin/
SRC = src/

all:
	gcc $(SRC)server.c $(SRC)stack.c -lpthread -o $(BIN)server
	gcc $(SRC)client.c -lpthread -o $(BIN)client

distr:
	gcc $(SRC)server.c $(SRC)stack.c -lpthread -o $(BIN)server
	gcc $(SRC)client.c -lpthread -o $(BIN)client -DCUSTOM_IP

run-server:
	$(BIN)server

run-client:
	$(BIN)client

.phony: clean

clean:
	rm -rf $(BIN)*
