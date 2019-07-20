#==========================================

# build an executable named ass1 from ass1.c

# the compiler: gcc for C program, define as g++ for C++
CC=gcc

# compiler flags:
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
CFLAGS  = -g -Wall

# the build target executable (The name of our executable):
TARGET = radio_control
# define the C sorce files:
SRCS=main.c clientFunc.c
# define the C objects files
OBJS = radio_client.o
# clean
RM := rm -rf
  
$(TARGET): 
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS) -pthread

clean:
	rm -f $(TARGET)


#==========================================

