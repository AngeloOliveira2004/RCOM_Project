# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall 

# Source files
SRC_READ = read_noncanonical.c
SRC_WRITE = write_noncanonical.c

# Executable files (without the .c extension)
EXE_READ = read_noncanonical
EXE_WRITE = write_noncanonical

# Default target: compile both programs
all: $(EXE_READ) $(EXE_WRITE)

# Compile read_noncanonical.c
$(EXE_READ): $(SRC_READ)
	$(CC) $(CFLAGS) -o $(EXE_READ) $(SRC_READ)

# Compile write_noncanonical.c
$(EXE_WRITE): $(SRC_WRITE)
	$(CC) $(CFLAGS) -o $(EXE_WRITE) $(SRC_WRITE)

# Clean up the compiled executables
clean:
	rm -f $(EXE_READ) $(EXE_WRITE)

# Phony targets
.PHONY: all clean
