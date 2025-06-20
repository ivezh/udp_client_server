CC = gcc
RM = rm -rf
CFLAGS = -g -Iinclude

SRC_DIR = src
INC_DIR = include

TARGET = server

SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@echo "COMPILE $@"
	@$(CC) $(CFLAGS) $(OBJECTS) -o $@

$(OBJECTS): %.o: %.c
	@echo "COMPILE $<"
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@echo "CLEANING UP"
	@$(RM) $(SRC_DIR)/*.o $(TARGET)

.PHONY: all clean