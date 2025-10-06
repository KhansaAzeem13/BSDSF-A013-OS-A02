CC = gcc
CFLAGS = -Wall
SRC = src/ls-v1.0.0.c
OUT = bin/ls

all: $(OUT)

$(OUT): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(OUT)

clean:
	rm -f $(OUT)
