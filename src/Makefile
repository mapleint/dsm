CC = gcc
CFLAGS = -g -fPIC -Wall -Wextra -pedantic
SERVER = server
SERVER_OBJ = server.o

CLIENT = client
CLIENT_OBJ = client.o

CLIENT_LIB = libclient.so
CLIENT_LIB_OBJ = preload.o

OBJECTS = rpc.o memory.o

ALL: $(CLIENT) $(SERVER) $(CLIENT_LIB)

$(CLIENT): $(CLIENT_OBJ) $(OBJECTS)
	$(CC) $(CLIENT_OBJ) $(OBJECTS) -o $(CLIENT)

$(CLIENT_LIB): $(CLIENT_LIB_OBJ) $(OBJECTS)
	$(CC) -shared $(CLIENT_LIB_OBJ) $(OBJECTS) -o $(CLIENT_LIB)

$(SERVER): $(SERVER_OBJ) $(OBJECTS)
	$(CC) $(SERVER_OBJ) $(OBJECTS) -o $(SERVER)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@



clean:
	rm -f $(CLIENT_LIB) $(SERVER) $(CLIENT) $(OBJECTS) $(CLIENT_OBJ) $(SERVER_OBJ) $(CLIENT_LIB_OBJ)
