CC = gcc --std=c17
CFLAGS = -Wall -Wextra -pedantic
TARGET = client
OBJECTS = client.o

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm $(TARGET) $(OBJECTS)
