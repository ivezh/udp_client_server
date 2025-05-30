CC = gcc
AR = ar rcs
RM = rm -rf
CFLAGS = -L.

TARGET = server
LIBRARY = lib$(TARGET).a
HEADERS = $(wildcard *.h)
SOURCES = $(filter-out server.c, $(wildcard *.c))
OBJECTS = $(SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): server.c $(LIBRARY)
	@echo "COMPILE BINARY"
	@$(CC) server.c -g -o $(TARGET) $(CFLAGS) -l$(TARGET)

$(LIBRARY): $(OBJECTS)
	@echo "LINK OBJECTS"
	@$(AR) $(LIBRARY) $(OBJECTS)

$(OBJECTS): $(SOURCES) $(HEADERS)
	@echo "COMPILE OBJECTS"
	@$(CC) -c $(SOURCES)
  
clean:
	@echo "DELETE *.o, *.a"
	@$(RM) *.o
	@$(RM) *.a

.PHONY: clean