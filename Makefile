CC = gcc
CFLAGS = -Wall -Wextra -g -pthread
SRC = main.c ./Server/server.c ./Signal_Handlers/utils.c ./utils.c ./Cache/cache.c ./History/history.c 
OBJ = $(SRC:.c=.o)
TARGET = my_program

all: $(TARGET)

$(TARGET): $(OBJ)
	@$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) -pthread

%.o: %.c
	@$(CC) $(CFLAGS) -c $< -o $@

run: 
	@ python3 ./GUI/main.py 

clean:
	rm -f $(OBJ) $(TARGET)
