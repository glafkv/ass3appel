CC = gcc
CFLAGS = -g -Wall
TARGET1 = oss
TARGET2 = user
OBJS1 = oss.o
OBJS2 = user.o
.SUFFIXES: .c .o
all: oss user

$(TARGET1): $(OBJS1)
	$(CC) -o $(TARGET1) $(OBJS1) -lpthread

$(TARGET2): $(OBJS2)
	$(CC) -o $(TARGET2) $(OBJS2) -lpthread

.c.o: 
	$(CC) $(CFLAGS) -c $< 

.PHONY: clean

clean:
	/bin/rm -rf *.o *.txt $(TARGET1) $(TARGET2)
