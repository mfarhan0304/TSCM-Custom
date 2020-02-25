# the compiler: gcc for C program, define as g++ for C++
CC = gcc

# compiler flags:
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
CFLAGS  = -g -Wall -lm

# the build target executable:
TARGET = tscomp_custom

BUILD_DIR = bin

all: build run

build: $(TARGET).c	
	mkdir -p ${BUILD_DIR}
	$(CC) $(TARGET).c simlib.c -o ${BUILD_DIR}/$(TARGET) $(CFLAGS)

run: 
	./${BUILD_DIR}/${TARGET}

clean:
	$(RM) ${BUILD_DIR}/*