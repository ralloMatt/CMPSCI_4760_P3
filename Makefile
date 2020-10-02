CC	=gcc
CFLAGS	=-c

all: oss user

oss: oss.o 
	$(CC) -o oss oss.o

user: user.o 
	$(CC) -o user user.o
	
oss.o: oss.c
	$(CC) $(CFLAGS) oss.c
	
user.o: user.c
	$(CC) $(CFLAGS) user.c

clean:
	-rm *.o oss user
