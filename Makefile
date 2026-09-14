CC = gcc
CFLAGS = -g -Wall

all: myshell mypipeline

myshell: myshell.o LineParser.o
	$(CC) $(CFLAGS) myshell.o LineParser.o -o myshell

mypipeline.o: mypipeline.c
	$(CC) $(CFLAGS) -c mypipeline.c

myshell.o: myshell.c LineParser.h
	$(CC) $(CFLAGS) -c myshell.c 

LineParser.o: LineParser.c LineParser.h

clean:
	rm -f myshell.o LineParser.o mypipeline.o myshell mypipeline

valgrind_shell:
	valgrind --leak-check=full ./myshell

valgrind_pipe:
	valgrind --leak-check=full ./mypipeline

