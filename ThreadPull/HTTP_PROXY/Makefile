CC = gcc
CFLAGS = -Wall -Wextra -g -pthread -I/usr/include/openssl
LDFLAGS = -L/usr/lib/x86_64-linux-gnu -lssl -lcrypto
SRC = main.c ./Server/server.c ./Signal_Handlers/utils.c ./Protocols/HTTP.c ./utils.c ./Cache/cache.c ./Shared_Mem/Shm.c ./Thread/thread.c
# Extract all object files related to C source files
OBJ = $(SRC:.c=.o)
TARGET = proxy

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *grind*
	rm -f $(OBJ) $(TARGET)
