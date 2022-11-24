#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

int main(void){

	int operator;
	int character;
	FILE* fp;
	int number = 0;

	if ((fp=fopen("ssu_expr.txt", "r"))==NULL){
		fprintf(stderr, "fopen error for ssu_expr.txt\n");
		exit(1);
	}

	while (!feof(fp)){

		while((character=fgetc(fp))!=EOF&&isdigit(character))
			number = 10* number + character - 48;

		fprintf(stdout, "%d\n", number);
		number = 0;

		if (character!=EOF){
			ungetc(character, fp);
			operator = fgetc(fp);
			printf("Operator => %c\n", operator);
		}

	}

	fclose(fp);
	exit(0);

}
