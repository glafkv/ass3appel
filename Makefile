CC = gcc
CFLAGS = -g
TARGET = oss
OBJS = oss.o
LIBOBJS = user.o
all: oss user
.SUFFIXES: .c .o

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS)

.c.o:
	$(CC) $(CLAGS) -c $<

.PHONY: clean
clean:
	/bin/rm -f *.txt *.o $(TARGET)
