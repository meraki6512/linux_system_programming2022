CC = gcc

SFINDER = ssu_sfinder

all : $(SFINDER) 

$(SFINDER) : ssu_sfinder.c
	$(CC) -o $@ $^ -g -lcrypto -lpthread

clean :
	rm -rf *.o
	rm -rf $(SFINDER)
