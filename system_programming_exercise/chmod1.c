#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

int main(void){

	struct stat statbuf;
	char * fname1 = "ssu_file1";
	char * fname2 = "ssu_file2";

	if (stat(fname1, &statbuf)<0){
		fprintf(stderr, "stat error for %s\n", fname1);
		exit(1);
	}

	if (chmod(fname1, (statbuf.st_mode&~S_IXGRP) | S_ISUID)<0){
		fprintf(stderr, "chmod error for %s\n", fname1);
		exit(1);
	}

	if (chmod(fname2, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH|S_IXOTH)<0){
		fprintf(stderr, "chmod error for %s\n", fname2);
		exit(1);
	}

	exit(0);
}
