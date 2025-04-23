CC = gcc
CFLAGS = -g -Wall -Wextra -pedantic
SERVER = server
SERVER_OBJ = server.o

CLIENT = client
CLIENT_OBJ = client.o

OBJECTS = rpc.o

ALL: $(CLIENT) $(SERVER)

$(CLIENT): $(CLIENT_OBJ) $(OBJECTS)
	$(CC) $(CLIENT_OBJ) $(OBJECTS) -o $(CLIENT)

$(SERVER): $(SERVER_OBJ) $(OBJECTS)
	$(CC) $(SERVER_OBJ) $(OBJECTS) -o $(SERVER)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(SERVER) $(CLIENT) $(OBJECTS) $(CLIENT_OBJ) $(SERVER_OBJ) 
