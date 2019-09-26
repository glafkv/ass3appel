CC = gcc
CFLAGS = -g
TARGET = oss
OBJS = oss.o user.o
.SUFFIXES: .c .o

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS)

.c.o:
	$(CC) $(CLAGS) -c $<

.PHONY: clean
clean:
	/bin/rm -f *.o $(TARGET)
