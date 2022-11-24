#gcc
CC = gcc

# compile options
CFLAGS = -lcrypto -lm

#targets
all : sdup help fmd5 fsha1


#Target: Dependencies
#	Commands
sdup: ssu_sdup.c
	$(CC) ssu_sdup.c -o sdup

help: ssu_help.c
	$(CC) ssu_help.c -o help

fmd5: ssu_find-md5.c
	$(CC) ssu_find-md5.c -o fmd5 $(CFLAGS)

fsha1: ssu_find-sha1.c
	$(CC) ssu_find-sha1.c -o fsha1 $(CFLAGS)


#clean 
clean: 
	rm sdup
	rm help
	rm fmd5
	rm fsha1
