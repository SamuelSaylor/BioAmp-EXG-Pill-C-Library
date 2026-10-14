CC = gcc
CFLAGS = -Wall -Wextra -Iinclude

LIB_NAME = bioampexgpillclib
LIB_FILE = lib$(LIB_NAME).a
LIB_SRC = bioampexgpillClib.c
LIB_OBJ = $(LIB_SRC:.c=.o)

TEST_SRC = main.c
TEST_BIN = test_main

all: $(TEST_BIN)

$(LIB_OBJ): $(LIB_SRC) bioampexgpillClib.h
	$(CC) $(CFLAGS) -c $(LIB_SRC) -o $(LIB_OBJ)

$(LIB_FILE): $(LIB_OBJ)
	ar rcs $(LIB_FILE) $(LIB_OBJ)

$(TEST_BIN): $(TEST_SRC) $(LIB_FILE)
	$(CC) $(CFLAGS) $(TEST_SRC) -L. -l$(LIB_NAME) -o $(TEST_BIN)

clean:
	rm -f $(LIB_OBJ) $(LIB_FILE) $(TEST_BIN)

.PHONY: all clean