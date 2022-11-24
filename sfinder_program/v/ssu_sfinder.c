#include "ssu_header.h"

int main(void)
{
	init();

	while (1) {
		char input[STRMAX];
		char *argv[ARGMAX];
		int argc = 0;
		pid_t pid;

		printf("20220000> ");
		fgets(input, sizeof(input), stdin);
		input[strlen(input) - 1] = '\0';

		argc = tokenize(input, argv);
		argv[argc] = (char *)0;

		if (argc == 0)
			continue;

		if (!strcmp(argv[0], "exit"))
			break;

		if (!strcmp(argv[0], "help")) {
			print_help();
			continue;
		}

		if (!strcmp(argv[0], "fsha1")) {
			hash_function = sha1;
			ssu_find(argc, argv);
		}
		else if (!strcmp(argv[0], "fmd5")) {
			hash_function = md5;
			ssu_find(argc, argv);
		}
		else if (!strcmp(argv[0], "list")) 
			ssu_list(argc, argv);
		else if (!strcmp(argv[0], "trash")) 
			ssu_trash(argc, argv);
		else if (!strcmp(argv[0], "restore")) 
			ssu_restore(argc, argv);
	}

	printf("Prompt End\n");

	return 0;
}