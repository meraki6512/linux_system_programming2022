#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define BUFFER_SIZE 128
   
int main(int argc, char *argv[])
{
   char buf[BUFFER_SIZE];
   int fd1, fd2;
   ssize_t size;

   if (argc!=3){
	   fprintf(stderr,"Usage: %s origin_filename copy_filename\n", argv[0]);
	   exit(1);
   }

   if ((fd1 = open(argv[1], O_RDONLY))<0){
	   fprintf(stderr,"open error for %s\n", argv[1]);
	   exit(1);
   }

   if ((fd2 = open(argv[2], O_WRONLY | O_CREAT | O_EXCL, 0666))<0){
	   fprintf(stderr, "open error for %s\n", argv[2]);
	   exit(1);
   }

   size = read(fd1, buf, BUFFER_SIZE);

   if (write(fd2, buf, size)!=size){
	   fprintf(stderr, "write error\n");
	   exit(1);
   }

   close(fd1);
   close(fd2);
   
   return 0;
}

