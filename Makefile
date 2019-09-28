CC = gcc
CFLAGS = -g

all: oss user

oss: oss.c
	$(CC) -o oss oss.c

user: user.c
	$(CC) -o user user.c

.PHONY: clean
clean:
	rm -rf *.o *~ oss
	rm -rf *.o *~ user
