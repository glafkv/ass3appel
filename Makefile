CC = gcc

all: oss user

oss: oss.c
	gcc -g -Wall -lpthread -lrt -o oss oss.c

user: user.c
	gcc -g -Wall -lpthread -lrt -o user user.c

clean:
	$(RM) oss user *.txt
