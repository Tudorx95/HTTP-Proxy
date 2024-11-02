CC = gcc
CFLAGS = -Wall -Wextra -g
SRC = main.c ./Server/server.c ./Signal_Handlers/utils.c 
OBJ = $(SRC:.c=.o)
TARGET = my_program

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)