CC = gcc
RM = rm -rf

TARGET = client

all: $(TARGET)

$(TARGET): client.c 
	@echo "COMPILE BINARY"
	@$(CC) client.c -g -o $(TARGET)

clean:
	@echo "DELETE *.o, *.a"
	@$(RM) *.o
	@$(RM) *.a

.PHONY: clean