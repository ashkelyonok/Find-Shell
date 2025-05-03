CC = gcc
CFLAGS = -Wall -g -Iinclude -D_XOPEN_SOURCE=600
LDFLAGS = -lncursesw

# List of source files in src folder
SRCS = src/main.c src/shellManager.c src/inputHandler.c src/findExecutor.c \
       src/historyManager.c src/uiRenderer.c src/configManager.c

# Output executable (placed in project root, not src)
EXEC = find_shell

# Default target
all: $(EXEC)

# Compile all source files directly into the executable (no .o files)
$(EXEC): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(EXEC) $(LDFLAGS)

# Clean up only the executable
clean:
	rm -f $(EXEC)

.PHONY: all clean
