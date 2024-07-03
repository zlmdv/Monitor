CC = gcc 

CFLAGS = -Wall  -std=c11 -pthread

TARGET = monitor

SRC = monitor.c 

all: $(TARGET)

$(TARGET): $(SRC) 
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) 

clean: 
	rm -rf $(TARGET)
	
run: $(TARGET)
	./$(TARGET)