CC = gcc

SDUP = ssu_sdup
FIND_MD5 = fmd5
FIND_SHA1 = fsha1
HELP = help

all : $(SDUP) $(FIND_MD5) $(FIND_SHA1) $(HELP)

$(SDUP) : ssu_sdup.c
	$(CC) -o $@ $^ 

$(FIND_MD5) : ssu_find-md5.c 
	$(CC) -o $@ $^ -lcrypto

$(FIND_SHA1) : ssu_find-sha1.c 
	$(CC) -o $@ $^ -lcrypto

$(HELP) :ssu_help.c
	$(CC) -o $@ $^

clean :
	rm -rf *.o
	rm -rf $(SDUP)
	rm -rf $(FIND_MD5)
	rm -rf $(FIND_SHA1)
	rm -rf $(HELP)
