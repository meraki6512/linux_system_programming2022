#define HASH_MD5
#include "ssu_sdup.h"

int main(int argc, char *argv[])
{
	char target_dir[PATHMAX];
	dirList *dirlist = (dirList *)malloc(sizeof(dirList));
	
	dups_list_h = (fileList *)malloc(sizeof(fileList));

	if (check_args(argc, argv))
		return 1;

	if (strchr(argv[4], '~') != NULL)
		get_path_from_home(argv[4], target_dir);
	else
		realpath(argv[4], target_dir);

	get_same_size_files_dir();

	struct timeval begin_t, end_t;

	gettimeofday(&begin_t, NULL);

	dirlist_append(dirlist, target_dir);
	dir_traverse(dirlist);
	find_duplicates();
	remove_no_duplicates();

	gettimeofday(&end_t, NULL);

	end_t.tv_sec -= begin_t.tv_sec;

	if (end_t.tv_usec < begin_t.tv_usec){
		end_t.tv_sec--;
		end_t.tv_usec += 1000000;
	}

	end_t.tv_usec -= begin_t.tv_usec;

	if (dups_list_h->next == NULL)
		printf("No duplicates in %s\n", target_dir);
	else 
		filelist_print_format(dups_list_h);

	printf("Searching time: %ld:%06ld(sec:usec)\n\n", end_t.tv_sec, end_t.tv_usec);

	get_trash_path();

	delete_prompt();

	return 0;
}