CC = gcc
CFLAGS = -Wall -Wextra -g
SRC = main.c ./Server/server.c ./Signal_Handlers/utils.c ./Protocols/HTTP.c ./utils.c ./Cache/cache.c ./Shared_Mem/Shm.c
OBJ = $(SRC:.c=.o)
TARGET = my_program

all: $(TARGET)

$(TARGET): $(OBJ)
	@$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

%.o: %.c
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
