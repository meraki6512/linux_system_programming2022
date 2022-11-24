#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>
#include <dirent.h>
#include <ctype.h>
#include <math.h>
#include <pwd.h>
#include <openssl/sha.h>
#include <openssl/md5.h>

#define MD5_PARTIAL 4096

#define DIRECTORY 1
#define REGFILE 2

#define KB 1000
#define MB 1000000
#define GB 1000000000
#define KiB 1024
#define MiB 1048576
#define GiB 1073741824

#define SIZE_ERROR -2

#define NAMEMAX 255
#define PATHMAX 4096
#define TRASHPATHMAX 1024
//#define HASHMAX 41
#define HASHMAX 33
#define STRMAX 10000 
#define SHORTSTRMAX 128
#define ARGMAX 20
#define PTHREADMAX 10
#define DELETIONDATEMAX 11
#define DELETIONTIMEMAX 9

typedef struct fileList {
	char path[PATHMAX];
	struct stat statbuf;
	struct fileList *next;
} fileList;

typedef struct fileSet {
	long long filesize;
	char hash[HASHMAX];
	fileList *filelist_head;
	struct fileSet *next;
} fileSet;

typedef struct dirList {
	char dirpath[PATHMAX];
	struct dirList *next;
} dirList;

typedef struct trashInfo {
	char trash_name[PATHMAX];
	char path[PATHMAX];
	char deletion_date[DELETIONDATEMAX];
	char deletion_time[DELETIONTIMEMAX];
	struct stat statbuf;

	struct trashInfo *next;
} trashInfo;

typedef enum {
	DELETE = 0,
	REMOVE,
	RESTORE
} LOG_TAG;

typedef enum {
	FILESET = 0,
	FILELIST,
	TRASHINFO
} LIST_TYPE;

char *log_msg[] = {"[DELETE]", "[REMOVE]", "[RESTORE]"};

char extension[10];
char same_size_files_dir[PATHMAX];
char trash_path[TRASHPATHMAX];
char trash_info[TRASHPATHMAX];
char filelist_directory[PATHMAX];
char daemon_file_name[PATHMAX];
char log_file[PATHMAX];
long long minbsize;
long long maxbsize;
int thread_max = 1;
pthread_t parent_pid;
FILE *log_fp;

struct passwd *passwd_info;
fileSet *dups_list_h;
dirList *dirlist_h;

int (*hash_function)(char *, char *); // hash function pointer

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int tokenize(char *input, char *argv[]),
	is_dir(char *target_dir),
	get_file_mode (char *target_file, struct stat *statbuf),
	get_dirlist(char *target_dir, struct dirent ***namelist),
	get_dirlist_length(dirList *head),
	get_fileset_size(fileSet *head),
	get_filelist_size(fileList *head),
	sha1(char *target_path, char *hash_result),
	md5(char *target_path, char *hash_result),
	comp_func_filename(void *source, void *target, LIST_TYPE list_type),
	comp_func_uid(void *source, void *target, LIST_TYPE list_type),
	comp_func_gid(void *source, void *target, LIST_TYPE list_type),
	comp_func_size(void *source, void *target, LIST_TYPE list_type),
	comp_func_mode(void *source, void *target, LIST_TYPE list_type),
	comp_func_deletion_date(void *source, void *target, LIST_TYPE list_type),
	comp_func_deletion_time(void *source, void *target, LIST_TYPE list_type),
	get_trashinfo_list_size(trashInfo *head),
	(*get_comp_function(char *sortby))(void *, void *, LIST_TYPE);

void get_new_file_name(char *org_filename, char *new_filename),
	 filesize_with_comma(long filesize, char *filesize_w_comma),
	 get_fullpath(char *target_dir, char *target_file, char *fullpath),
	 fileset_print_format(fileSet *head),
	 get_path_from_home(char *path, char *target_dir),
	 find_same_size_in_dir(dirList *cur_dir, void* (*traverse_func)(void *)),
	 *dir_traverse(void *target_dir),
	 find_same_size_in_target_dir(char *target_dir),
	 remove_no_duplicates(void),
	 find_duplicates(void),
	 sec_to_ymdt(struct tm *sec, char *ymdt),
	 deleted_sec_to_ymdt(struct tm *time, char *ymdt),
	 print_runtime(struct timeval begin_t, struct timeval end_t),
	 print_help(void),
	 restore_file(trashInfo *head, int file_index),
	 trash_init(void),
	 trashinfo_append(trashInfo *head, trashInfo *node),
	 trashinfo_print(trashInfo *head),
	 fileset_sort(fileSet *head, int (*comp_func)(void *, void *, LIST_TYPE), int order),
	 filelist_sort(fileList *head, int (*comp_func)(void *, void *, LIST_TYPE), int order),
	 trashinfo_sort(trashInfo *head, int (*comp_func)(void *, void *, LIST_TYPE), int order),
	 linkedlist_sort(LIST_TYPE list_type, char *sortby, int orderby);

char* get_extension(char *filename);

trashInfo* trashinfo_delete(trashInfo *head, int file_index);
time_t get_recent_mtime(fileList *head, char *last_filepath);
long long get_size(char *filesize);

trashInfo *trash_head;

void init(void)
{
	parent_pid = pthread_self();

	if ((passwd_info = getpwuid(getuid())) == NULL) {
		fprintf(stderr, "getpwuid error for %d\n", getuid());
		exit (1);
	}

	// trash files path
	strcpy(trash_path, passwd_info->pw_dir);
	strcat(trash_path, "/.Trash/");

	// if trash files directory not exist, make it
	if (access(trash_path, F_OK) != 0)
		mkdir(trash_path, 0700);

	strcat(trash_path, "/files/");

	// if trash files directory not exist, make it
	if (access(trash_path, F_OK) != 0)
		mkdir(trash_path, 0700);

	// trash info path
	strcpy(trash_info, passwd_info->pw_dir);
	strcat(trash_info, "/.Trash/info/");

	// if trash info directory not exist, make it
	if (access(trash_info, F_OK) != 0)
		mkdir(trash_info, 0700);

	// if trash info directory not exist, make it
	if (access(trash_info, F_OK) != 0)
		mkdir(trash_path, 0700);

	// filelist directory path (for save hash value and file path)
	strcpy(filelist_directory, passwd_info->pw_dir);
	strcat(filelist_directory, "/.filelist");

	if (access(filelist_directory, F_OK) != 0)
		mkdir(filelist_directory, 0700);

	// log path
	strcpy(log_file, passwd_info->pw_dir);
	strcat(log_file, "/.duplicate_20200000.log");

	// open log file
	if ((log_fp = fopen(log_file, "a")) == NULL) {
		fprintf(stderr, "fopen error for %s\n", log_file);
		exit(1);
	}
}

void ssu_find_init(void)
{
	dups_list_h = (fileSet *)malloc(sizeof(fileSet));
	dirlist_h = (dirList *)malloc(sizeof(dirlist_h));

	// if filelist already exists, it is removed
	if (access(filelist_directory, F_OK) == 0) {
		char filelist_dir_del_comm[STRMAX] = "rm -rf ";
		strcat(filelist_dir_del_comm, filelist_directory);
		system(filelist_dir_del_comm);
	}

	// make filelist directory 
	mkdir(filelist_directory, 0700);
}

void write_log(LOG_TAG tag, char *msg)
{
	char modified_time[STRMAX];
	time_t log_time;

	log_time = time(NULL);
	sec_to_ymdt(localtime(&log_time), modified_time);

	fprintf(log_fp, "%s %s %s %s\n", log_msg[tag], msg, modified_time, passwd_info->pw_name);
}

int tokenize(char *input, char *argv[])
{
	char *ptr = NULL;
	int argc = 0;
	ptr = strtok(input, " ");

	while (ptr != NULL){
		argv[argc++] = ptr;
		ptr = strtok(NULL, " ");
	}

	return argc;
}

void filelist_append(fileList *head, char *path)
{
	fileList *filelist_cur;
	fileList *newlist = (fileList *)malloc(sizeof(fileList));

	memset(newlist, 0, sizeof(fileList));
	strcpy(newlist->path, path);

	lstat(newlist->path, &newlist->statbuf);
	newlist->next = NULL;

	if (head->next == NULL)
		head->next = newlist;
	else {
		filelist_cur = head->next;
		while (filelist_cur->next != NULL)
			filelist_cur = filelist_cur->next;

		filelist_cur->next = newlist;
	}
}

fileList *filelist_delete_node(fileList *head, char *path)
{
	if (!strcmp(head->next->path, path)){
		head->next = head->next->next;
		return head->next;
	}
	else {
		fileList *filelist_cur = head->next;

		while (filelist_cur->next != NULL){
			if (!strcmp(filelist_cur->next->path, path)){
				filelist_cur->next = filelist_cur->next->next;
				return filelist_cur->next;
			}

			filelist_cur = filelist_cur->next;
		}
	}

	return NULL;
}

int get_fileset_size(fileSet *head)
{
	fileSet *cur = head->next;
	int size = 0;

	while (cur != NULL){
		size++;
		cur = cur->next;
	}

	return size;
}

int get_filelist_size(fileList *head)
{
	fileList *cur = head->next;
	int size = 0;

	while (cur != NULL){
		size++;
		cur = cur->next;
	}

	return size;
}

void fileset_append(fileSet *head, long long filesize, char *path, char *hash)
{
	fileSet *newset = (fileSet *)malloc(sizeof(fileSet));
	memset(newset, 0, sizeof(fileSet));

	newset->filesize = filesize;
	strcpy(newset->hash, hash);

	newset->filelist_head = (fileList *)malloc(sizeof(fileList));
	memset(newset->filelist_head, 0, sizeof(fileList));

	filelist_append(newset->filelist_head, path);
	newset->next = NULL;

	if (head->next == NULL) 
		head->next = newset;
	else {
		fileSet *cur_node = head;

		while (cur_node->next != NULL && (cur_node->next->filesize < newset->filesize)) 
			cur_node = cur_node->next;

		newset->next = cur_node->next;
		cur_node->next = newset;
	}
}

void fileset_delete_node(fileSet *head, char *hash)
{
	fileSet *deleted;

	if (!strcmp(head->next->hash, hash)) {
		deleted = head->next;
		head->next = head->next->next;
	}
	else {
		fileSet *fileset_cur = head->next;

		while (fileset_cur->next != NULL){
			if (!strcmp(fileset_cur->next->hash, hash)){
				deleted = fileset_cur->next;

				fileset_cur->next = fileset_cur->next->next;

				break;
			}

			fileset_cur = fileset_cur->next;
		}
	}

	free(deleted);
}

fileSet* fileset_search(fileSet *head, char *hash)
{
	fileSet *cur = head;

	while (cur != NULL){
		if (!strcmp(cur->hash, hash))
			return cur;

		cur = cur->next;
	}

	return NULL;
}

fileList* fileinfo_search(fileList *head, char *path)
{
	fileList *cur = head;

	while (cur != NULL){
		if (!strcmp(cur->path, path))
			return cur;

		cur = cur->next;
	}

	return NULL;
}

void dirlist_append(dirList *head, char *path)
{
	dirList *newFile = (dirList *)malloc(sizeof(dirList));

	strcpy(newFile->dirpath, path);
	newFile->next = NULL;

	if (head->next == NULL)
		head->next = newFile;
	else {
		dirList *cur = head->next;

		while (cur->next != NULL)
			cur = cur->next;

		cur->next = newFile;
	}
}

void dirlist_print(dirList *head, int index)
{
	dirList *cur = head->next;
	int i = 1;

	if (!index)
		return ;

	while (cur != NULL) {
		printf("[%d] ", i++);
		printf("%s\n", cur->dirpath);
		cur = cur->next;
	}
}

int get_dirlist_length(dirList *head)
{
	int length = 0;
	dirList *cur = head->next;

	while (cur != NULL) {
		length++;
		cur = cur->next;
	}

	return length;
}

void dirlist_delete_all(dirList *head)
{
	dirList *dirlist_cur = head->next;
	dirList *tmp;

	while (dirlist_cur != NULL){
		tmp = dirlist_cur->next;
		free(dirlist_cur);
		dirlist_cur = tmp;
	}

	free(head);
}

void get_path_from_home(char *path, char *target_dir)
{
	if (!strcmp("~", path)) 
		strcpy(target_dir, passwd_info->pw_dir);
	else 
		sprintf(target_dir, "%s%s", passwd_info->pw_dir, path+1);
}

void find_same_size_in_dir(dirList *cur_dir, void* (*traverse_func)(void *))
{
	int thread_num, thread_count, depth_length;
	pthread_t tid[PTHREADMAX];

	thread_count = 0;

	if ((depth_length = get_dirlist_length(cur_dir)) == 0) {
		dirlist_delete_all(dirlist_h);
		return ;
	}

	while (depth_length > 0) {
		thread_num = depth_length > thread_max ? thread_max : depth_length;
		thread_count = 0;

		if (thread_num == 1) {
			cur_dir = cur_dir->next;
			traverse_func((void *)cur_dir->dirpath);

			depth_length--;
			continue;
		}

		while (thread_num--) {
			cur_dir = cur_dir->next;

			if (strcmp(cur_dir->dirpath, "/proc") && strcmp(cur_dir->dirpath, "/run") && strcmp(cur_dir->dirpath, "/sys")) {
				if (pthread_create(&(tid[thread_count]), NULL, traverse_func, (void *)cur_dir->dirpath) != 0) {
					fprintf(stderr, "pthread_create error for %s\n", cur_dir->dirpath);
					exit(1);
				}

				thread_count++;
				depth_length--;
			}

			if (cur_dir->next == NULL)
				break;
		} 

		for (int thread_index = 0; thread_index < thread_count; thread_index++) 
			pthread_join(tid[thread_index], NULL);
	}

	find_same_size_in_dir(cur_dir, traverse_func);
}

int get_file_mode(char *target_file, struct stat *statbuf)
{
	if (lstat(target_file, statbuf) < 0){
		printf("ERROR: lstat error for %s\n", target_file);
		return 0;
	}

	if (S_ISREG(statbuf->st_mode))
		return REGFILE;
	else if(S_ISDIR(statbuf->st_mode))
		return DIRECTORY;
	else
		return 0;
}

void get_fullpath(char *target_dir, char *target_file, char *fullpath)
{
	strcat(fullpath, target_dir);

	if(fullpath[strlen(target_dir) - 1] != '/')
		strcat(fullpath, "/");

	strcat(fullpath, target_file);
	fullpath[strlen(fullpath)] = '\0';
}

int get_dirlist(char *target_dir, struct dirent ***namelist)
{
	int cnt = 0;

	if ((cnt = scandir(target_dir, namelist, NULL, alphasort)) == -1){
		printf("ERROR: scandir error for %s\n", target_dir);
		return -1;
	}

	return cnt;
}

char *get_extension(char *filename)
{
	char *extension = strrchr(filename, '.');
	return extension != NULL ? extension + 1 : NULL;
}

long long get_size(char *filesize)
{
	double size_bytes = 0;
	char size[STRMAX] = {0, };
	char size_unit[4] = {0, };
	int size_idx = 0;

	if (!strcmp(filesize, "~"))
		size_bytes = -1;
	else {
		for (int i = 0; i < strlen(filesize); i++){
			if (isdigit(filesize[i]) || filesize[i] == '.'){
				size[size_idx++] = filesize[i];
				if (filesize[i] == '.' && !isdigit(filesize[i + 1]))
					return SIZE_ERROR;
			}
			else {
				strcpy(size_unit, filesize + i);
				break;
			}
		}

		size_bytes = atof(size);

		if (strlen(size_unit) != 0){
			if (!strcmp(size_unit, "kb") || !strcmp(size_unit, "KB"))
				size_bytes *= KB;
			else if (!strcmp(size_unit, "mb") || !strcmp(size_unit, "MB"))
				size_bytes *= MB;
			else if (!strcmp(size_unit, "gb") || !strcmp(size_unit, "GB"))
				size_bytes *= GB;
			else if (!strcmp(size_unit, "kib") || !strcmp(size_unit, "KiB"))
				size_bytes *= KiB;
			else if (!strcmp(size_unit, "mib") || !strcmp(size_unit, "MiB"))
				size_bytes *= MiB;
			else if (!strcmp(size_unit, "gib") || !strcmp(size_unit, "GiB"))
				size_bytes *= GiB;
			else
				return SIZE_ERROR;
		}
	}

	return (long long)size_bytes;
}

void print_runtime(struct timeval begin_t, struct timeval end_t)
{
	end_t.tv_sec -= begin_t.tv_sec;

	if (end_t.tv_usec < begin_t.tv_usec){
		end_t.tv_sec--;
		end_t.tv_usec += 1000000;
	}

	end_t.tv_usec -= begin_t.tv_usec;

	printf("Searching time: %ld:%06ld(sec:usec)\n\n", end_t.tv_sec, end_t.tv_usec);
}

void sec_to_ymdt(struct tm *time, char *ymdt)
{
	sprintf(ymdt, "%04d-%02d-%02d %02d:%02d:%02d", time->tm_year + 1900, time->tm_mon + 1, time->tm_mday, time->tm_hour, time->tm_min, time->tm_sec);
}

int is_dir(char *target_dir)
{
	struct stat statbuf;

	if (lstat(target_dir, &statbuf) < 0){
		printf("ERROR: lstat error for %s\n", target_dir);
		return 1;
	}

	return S_ISDIR(statbuf.st_mode) ? DIRECTORY : 0;
}

void get_filename(char *path, char *filename)
{
	char tmp_name[NAMEMAX];
	char *pt = NULL;

	memset(tmp_name, 0, sizeof(tmp_name));

	if (strrchr(path, '/') != NULL)
		strcpy(tmp_name, strrchr(path, '/') + 1);
	else
		strcpy(tmp_name, path);

	if ((pt = strrchr(tmp_name, '.')) != NULL){
		if (strrchr(tmp_name, '.') != NULL)
			pt = strrchr(tmp_name, '.');

		pt[0] = '\0';
	}

	strcpy(filename, tmp_name);
}

void get_new_file_name(char *org_filename, char *new_filename)
{
	char new_trash_path[PATHMAX];
	char tmp[NAMEMAX];
	struct dirent **namelist;
	int trashlist_cnt;
	int same_name_cnt = 1;

	get_filename(org_filename, new_filename);
	trashlist_cnt = get_dirlist(trash_path, &namelist);

	for (int i = 0; i < trashlist_cnt; i++){
		if (!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, ".."))
			continue;

		memset(tmp, 0, sizeof(tmp));
		get_filename(namelist[i]->d_name, tmp);

		if (!strcmp(new_filename, tmp))
			same_name_cnt++;
	}

	sprintf(new_filename + strlen(new_filename), ".%d", same_name_cnt);

	if (get_extension(org_filename) != NULL)
		sprintf(new_filename + strlen(new_filename), ".%s", get_extension(org_filename));
}

void creat_trashinfo_file(char *trashinfo_file, char *deleted_path)
{
	FILE *fp;
	time_t deleted_time;
	char deleted_ymdt[50];

	deleted_time = time(NULL);

	deleted_sec_to_ymdt(localtime(&deleted_time), deleted_ymdt);

	if ((fp = fopen(trashinfo_file, "w")) == NULL){
		printf("ERROR: fail to open %s\n", trashinfo_file);
		return;
	}

	fprintf(fp, "[Trash Info]\n");
	fprintf(fp, "Path=%s\n", deleted_path);
	fprintf(fp, "DeletionDate=%s\n", deleted_ymdt);

	fclose(fp);
}

void filesize_with_comma(long filesize, char *filesize_w_comma)
{
	char filesize_wo_comma[STRMAX] = {0, };
	int comma;
	int idx = 0;

	sprintf(filesize_wo_comma, "%ld", filesize);
	comma = strlen(filesize_wo_comma)%3;

	for (int i = 0 ; i < strlen(filesize_wo_comma); i++){
		if (i > 0 && (i % 3) == comma)
			filesize_w_comma[idx++] = ',';

		filesize_w_comma[idx++] = filesize_wo_comma[i];
	}

	filesize_w_comma[idx] = '\0';
}

void fileset_print_format(fileSet *head)
{
	fileSet *fileset_cur = head->next;
	int set_idx = 1, filelist_size;

	if (fileset_cur == NULL) {
		fprintf(stderr, "There are not have duplicate file\n");
		exit(1);
	}

	while (fileset_cur != NULL){
		fileList *filelist_cur = fileset_cur->filelist_head->next;

		if ((filelist_size = get_filelist_size(fileset_cur->filelist_head)) <= 1) {
			fileset_cur = fileset_cur->next;
			continue ;
		}

		char mtime[STRMAX];
		char atime[STRMAX];
		char filesize_w_comma[STRMAX] = {0, };
		int list_idx = 1;

		filesize_with_comma(fileset_cur->filesize, filesize_w_comma);

		printf("---- Identical files #%d (%s bytes - %s) ----\n", set_idx++, filesize_w_comma, fileset_cur->hash);

		while (filelist_cur != NULL){
			sec_to_ymdt(localtime(&filelist_cur->statbuf.st_mtime), mtime);
			sec_to_ymdt(localtime(&filelist_cur->statbuf.st_atime), atime);
			printf("[%d] %s (mtime : %s) (atime : %s) (uid : %d) (gid : %d) (mode : %lo)\n", 
					list_idx++, filelist_cur->path, mtime, atime, 
					(filelist_cur->statbuf).st_uid,
					(filelist_cur->statbuf).st_gid,
					(unsigned long) (filelist_cur->statbuf).st_mode);

			filelist_cur = filelist_cur->next;
		}

		printf("\n");

		fileset_cur = fileset_cur->next;
	}
}

void *dir_traverse(void *target_dir)
{
	int list_cnt = 0, file_mode = 0;
	long file_size = 0;
	char hash[HASHMAX] = {0, };
	char full_path[PATHMAX] = {0, };
	char *copy_parameter;

	struct stat statbuf;
	struct dirent **name_list = NULL;

	copy_parameter = (char *) malloc (strlen(target_dir) + 1);
	strcpy(copy_parameter, (char *)target_dir);
	list_cnt = get_dirlist(copy_parameter, &name_list);

	for (int i = 0; i < list_cnt; i++) {
		if (!strcmp(name_list[i]->d_name, ".") || !strcmp(name_list[i]->d_name, ".."))
			continue;

		memset(full_path, 0, sizeof(full_path));

		get_fullpath(copy_parameter, name_list[i]->d_name, full_path);
		file_mode = get_file_mode(full_path, &statbuf);

		if (file_mode == DIRECTORY) {
			pthread_mutex_lock(&mutex);
			dirlist_append(dirlist_h, full_path);
			pthread_mutex_unlock(&mutex);
			continue;
		}

		file_size = statbuf.st_size;

		if (!file_size)
			continue;

		if (minbsize != -1 && file_size < minbsize)
			continue;

		if (maxbsize != -1 && file_size > maxbsize)
			continue;

		if (file_mode == REGFILE) {
			FILE *fp;
			char file_name[PATH_MAX];
			char *file_extension;

			sprintf(file_name, "%s/%ld", filelist_directory, file_size);

			memset(hash, 0, HASHMAX);

			if (hash_function(full_path, hash))
				continue ;

			file_extension = get_extension(full_path);

			if ((strlen(extension) != 0) && (file_extension == NULL || strcmp(extension, file_extension)))
				continue ;

			if ((fp = fopen(file_name, "a")) == NULL) {
				fprintf(stderr, "ERROR: fopen error for %s\n", file_name);
				exit(1);
			}

			fprintf(fp, "%s %s\n", hash, full_path);
			fclose(fp);
		}
	}

	if (!pthread_equal(parent_pid, pthread_self()))
		pthread_exit(NULL);

	return NULL;
}

void find_duplicates(void)
{
	FILE *fp;
	int listcnt;
	char hash[HASHMAX];
	fileSet *fileset_location;
	struct dirent **namelist;

	listcnt = get_dirlist(filelist_directory, &namelist);

	for (int i = 0; i < listcnt; i++) {
		char filename[PATHMAX*2];
		long long filesize;
		char filepath[PATHMAX];
		char hash[HASHMAX];
		char line[STRMAX];

		if (!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, ".."))
			continue;

		filesize = (long long)atoi(namelist[i]->d_name);
		sprintf(filename, "%s/%s", filelist_directory, namelist[i]->d_name);

		if ((fp = fopen(filename, "r")) == NULL) {
			printf("ERROR: fopen error for %s\n", filename);
			continue;
		}

		while (fgets(line, sizeof(line), fp) != NULL) {
			strncpy(hash, line, HASHMAX);
			hash[HASHMAX-1] = '\0';

			strcpy(filepath, line+HASHMAX);
			filepath[strlen(filepath)-1] = '\0';

			if ((fileset_location = fileset_search(dups_list_h, hash)) == NULL) 
				fileset_append(dups_list_h, filesize, filepath, hash);
			else 
				filelist_append(fileset_location->filelist_head, filepath);
		}

		fclose(fp);
	}
}

void remove_no_duplicates(void)
{
	fileSet *fileset_cur = dups_list_h->next;

	while (fileset_cur != NULL){
		fileList *filelist_cur = fileset_cur->filelist_head;

		if (get_filelist_size(filelist_cur) < 2)
			fileset_delete_node(dups_list_h, fileset_cur->hash);

		fileset_cur = fileset_cur->next;
	}
}

time_t get_recent_mtime(fileList *head, char *last_filepath)
{
	fileList *filelist_cur = head->next;
	time_t mtime = 0;

	while (filelist_cur != NULL){
		if (filelist_cur->statbuf.st_mtime > mtime){
			mtime = filelist_cur->statbuf.st_mtime;
			strcpy(last_filepath, filelist_cur->path);
		}

		filelist_cur = filelist_cur->next;
	}

	return mtime;
}

void deleted_sec_to_ymdt(struct tm *time, char *ymdt)
{
	sprintf(ymdt, "%04d-%02d-%02dT%02d:%02d:%02d", time->tm_year + 1900, time->tm_mon + 1, time->tm_mday, time->tm_hour, time->tm_min, time->tm_sec);
}

void print_help(void)
{
	printf("Usage:\n");
	printf("  > fmd5/fsha1 [FILE_EXTENSION] [MINSIZE] [MAXSIZE] [TARGET_DIRECTORY]\n");
	printf("     >> [SET_INDEX] [OPTION ... ]\n");
	printf("        [OPTION ... ]\n");
	printf("        d [LIST_IDX] : delete [LIST_IDX] file\n");
	printf("        i : ask for confirmation before delete\n");
	printf("        f : force delete except the recently modified file\n");
	printf("        t : force move to Trash except the recently modified file\n");
	printf("  > help\n");
	printf("  > exit\n\n");
}

int sha1(char *target_path, char *hash_result)
{
	FILE *fp;
	unsigned char hash[SHA_DIGEST_LENGTH];
	unsigned char buffer[SHRT_MAX];
	int bytes = 0;
	SHA_CTX sha1;

	if ((fp = fopen(target_path, "rb")) == NULL){
		printf("ERROR: fopen error for %s\n", target_path);
		return 1;
	}

	SHA1_Init(&sha1);

	while ((bytes = fread(buffer, 1, SHRT_MAX, fp)) != 0)
		SHA1_Update(&sha1, buffer, bytes);

	SHA1_Final(hash, &sha1);

	for (int i = 0; i < SHA_DIGEST_LENGTH; i++)
		sprintf(hash_result + (i * 2), "%02x", hash[i]);

	hash_result[HASHMAX-1] = 0;
	fclose(fp);

	return 0;
}

int md5(char *target_path, char *hash_result)
{
	FILE *fp;
	unsigned char hash[MD5_DIGEST_LENGTH];
	unsigned char buffer[SHRT_MAX];
	int bytes = 0;
	MD5_CTX md5;

	if ((fp = fopen(target_path, "rb")) == NULL){
		printf("ERROR: fopen error for %s\n", target_path);
		return 1;
	}

	MD5_Init(&md5);

	while ((bytes = fread(buffer, 1, SHRT_MAX, fp)) != 0)
		MD5_Update(&md5, buffer, bytes);

	MD5_Final(hash, &md5);

	for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
		sprintf(hash_result + (i * 2), "%02x", hash[i]);
	hash_result[HASHMAX-1] = 0;

	fclose(fp);

	return 0;
}

int ssu_find(int argc, char *argv[]) {
	ssu_find_init();

	char target_dir[PATH_MAX] = {0, };
	int c;

	optind = 0;
	while ((c = getopt(argc, argv, "e:l:h:d:t:")) != -1) {
		switch (c) {
			case 'e':
				if (strchr(optarg, '*') == NULL){
					printf("ERROR: [FILE_EXTENSION] should be '*' or '*.extension'\n");
					exit(1);
				}

				if (strchr(optarg, '.') != NULL){
					strcpy(extension, get_extension(optarg));

					if (strlen(extension) == 0){
						printf("ERROR: There should be extension\n");
						return 1;
					}
				}

				break;
			case 'l':
				minbsize = get_size(optarg);
				break;
			case 'h':
				maxbsize = get_size(optarg);
				break;
			case 'd':
				if (optarg[0] == '~') 
					get_path_from_home(optarg, target_dir);
				else {
					if (realpath(optarg, target_dir) == NULL) {
						printf("ERROR: [TARGET_DIRECTORY] should exist\n");
						return 1;
					}
				}

				break;
			case 't':
				thread_max = atoi(optarg);
				break;
		}
	}

	if (minbsize == SIZE_ERROR || maxbsize == SIZE_ERROR){
		printf("ERROR: size wrong -min size : %lld -max size : %lld\n", minbsize, maxbsize);
		return 1;
	}

	if (minbsize != -1 && maxbsize != -1 && minbsize > maxbsize){
		printf("ERROR: [MAXSIZE] should be bigger than [MINSIZE]\n");
		return 1;
	}

	if (access(target_dir, F_OK) == -1) {
		printf("ERROR: %s directory doesn't exist\n", target_dir);
		return 1;
	}

	if (!is_dir(target_dir)) {
		printf("ERROR: [TARGET_DIRECTORY] should be a directory\n");
		return 1;
	}

	struct timeval begin_t, traverse_end_t, find_duplicate_end_t, end_t;

	gettimeofday(&begin_t, NULL);

	dirlist_append(dirlist_h, target_dir);
	find_same_size_in_dir(dirlist_h, dir_traverse);

	gettimeofday(&traverse_end_t, NULL);
	//print_runtime(begin_t, traverse_end_t);

	find_duplicates();

	gettimeofday(&find_duplicate_end_t, NULL);
	//print_runtime(traverse_end_t, find_duplicate_end_t);

	remove_no_duplicates();

	gettimeofday(&end_t, NULL);

	if (dups_list_h->next == NULL)
		printf("No duplicates in %s\n", target_dir);
	else 
		fileset_print_format(dups_list_h);

	print_runtime(begin_t, end_t);

	while (get_fileset_size(dups_list_h) > 0){
		char input[STRMAX];
		char last_filepath[PATHMAX];
		char last_filehash[HASHMAX];
		char modifiedtime[STRMAX];
		char *shell_argv[3];
		int shell_argc;
		int set_idx;
		int c;
		time_t mtime = 0;
		fileSet *target_set_p;
		fileList *target_list_p;

		printf(">> ");

		fgets(input, sizeof(input), stdin);
		input[strlen(input)-1] = '\0';

		shell_argc = tokenize(input, shell_argv);

		if (!strcmp(shell_argv[0], "exit")) {
			printf(">> Back to Prompt\n");
			break;
		}

		if (strcmp(shell_argv[0], "delete"))
			continue ;

		target_set_p = dups_list_h->next;

		int list_idx;
		int option_d, option_i, option_f, option_t;
		option_d = option_i = option_f = option_t = optind = 0;

		while ((c = getopt(shell_argc, shell_argv, "l:d:ift")) != -1) {
			switch (c) {
				case 'l': 
					set_idx = atoi(optarg);

					int filelist_size;

					for (int count = 1; count < get_fileset_size(dups_list_h); count++) {
						if ((filelist_size = get_filelist_size(target_set_p->filelist_head)) <= 1) {
							count--;
							//target_filelist_p = target_filelist_p->next;
						}

						if (count == set_idx)
							break;

						target_set_p = target_set_p->next;
					}

					target_list_p = target_set_p->filelist_head;

					mtime = get_recent_mtime(target_set_p->filelist_head, last_filepath);
					sec_to_ymdt(localtime(&mtime), modifiedtime);

					break;
				case 'd':
					option_d = 1;
					list_idx = atoi(optarg);
					break;
				case 'i':
					option_i = 1;
					break;
				case 'f':
					option_f = 1;
					break;
				case 't':
					option_t = 1;
					break;
			}
		}

		if (option_f) {
			fileList *deleted = target_list_p;

			while (deleted->next != NULL){
				if (!strcmp(deleted->next->path, last_filepath)){
					deleted = deleted->next;
					continue;
				}
				remove(deleted->next->path);
				write_log(DELETE, deleted->next->path);
				deleted->next = deleted->next->next;
			}

			fileset_delete_node(dups_list_h, target_set_p->hash);
			printf("Left file in #%d : %s (%s)\n\n", set_idx, last_filepath, modifiedtime);
		}
		else if (option_t) {
			fileList *deleted = target_set_p->filelist_head;
			char move_to_trash[PATHMAX];
			char trashinfo_file[PATHMAX];
			char filename[PATHMAX];

			while (deleted->next != NULL) {
				if (!strcmp(deleted->next->path, last_filepath)) {
					deleted = deleted->next;
					continue;
				}

				memset(move_to_trash, 0, sizeof(move_to_trash));
				memset(trashinfo_file, 0, sizeof(trashinfo_file));
				memset(filename, 0, sizeof(filename));

				sprintf(move_to_trash, "%s%s", trash_path, strrchr(deleted->next->path, '/') + 1);

				if (access(move_to_trash, F_OK) == 0)
					get_new_file_name(deleted->next->path, filename);
				else
					strcpy(filename, strrchr(deleted->next->path, '/') + 1);

				memset(move_to_trash, 0, sizeof(move_to_trash));

				strncpy(move_to_trash, trash_path, strlen(trash_path));
				strncat(move_to_trash, filename, strlen(filename));

				strncpy(trashinfo_file, trash_info, strlen(trash_info));
				strncat(trashinfo_file, filename, strlen(filename));
				strcat(trashinfo_file, ".trashinfo");

				creat_trashinfo_file(trashinfo_file, deleted->next->path);

				if (rename(deleted->next->path, move_to_trash) == -1){
					printf("[ERROR] Fail to move duplicates to Trash : %s to %s\n", deleted->next->path, move_to_trash);
					continue;
				}

				write_log(REMOVE, deleted->next->path);
				deleted->next = deleted->next->next;
			}

			printf("All files in #%d have moved to Trash except \"%s\" (%s)\n\n", set_idx, last_filepath, modifiedtime);
		}
		else if (option_i){
			char ans[STRMAX];
			int listcnt = get_filelist_size(target_list_p);
			fileList *filelist_cur = target_list_p->next;
			fileList *deleted_list = (fileList *)malloc(sizeof(fileList));
			fileList *tmp;

			while (filelist_cur != NULL && listcnt--){
				printf("Delete \"%s\"? [y/n] ", filelist_cur->path);
				memset(ans, 0, sizeof(ans));
				fgets(ans, sizeof(ans), stdin);

				if (!strcmp(ans, "y\n") || !strcmp(ans, "Y\n")){
					remove(filelist_cur->path);
					write_log(DELETE, filelist_cur->path);
					filelist_cur = filelist_delete_node(target_list_p, filelist_cur->path);
				}
				else if (!strcmp(ans, "n\n") || !strcmp(ans, "N\n"))
					filelist_cur = filelist_cur->next;
				else
					printf("ERROR: Answer should be 'y/Y' or 'n/N'\n");
			}
			printf("\n");

			if (get_filelist_size(target_list_p) < 2)
				fileset_delete_node(dups_list_h, target_set_p->hash);
		}
		else if (option_d) {
			fileList *deleted;

			if (!list_idx) {
				printf("ERROR: There should be an index\n");
				continue;
			}

			if (list_idx < 0 || list_idx > get_filelist_size(target_list_p)){
				printf("ERROR: [LIST_IDX] out of range\n");
				continue;
			}

			deleted = target_list_p;

			while (list_idx--)
				deleted = deleted->next;

			printf("\"%s\" has been deleted in #%d\n\n", deleted->path, set_idx);
			remove(deleted->path);
			filelist_delete_node(target_list_p, deleted->path);
			write_log(DELETE, deleted->path);

			if (get_filelist_size(target_list_p) < 2)
				fileset_delete_node(dups_list_h, target_set_p->hash);
		}

		fileset_print_format(dups_list_h);
	}

}

void ssu_trash(int argc, char *argv[])
{
	char sortby[SHORTSTRMAX] = "filename";
	int orderby, c;

	trash_init();

	if (trash_head->next == NULL) {
		printf("Trash bin is empty\n");
		return ;
	}

	optind = 0;
	orderby = 1;

	while ((c = getopt(argc, argv, "c:o:")) != -1) {
		switch (c) {
			case 'c':
				if (strcmp(optarg, "filename") && strcmp(optarg, "date") && strcmp(optarg, "time")) {
					fprintf(stderr, "[ERROR] trash -c [filename or date or time]"); 
					return ;
				}
				strcpy(sortby, optarg);

				break;
			case 'o':
				orderby = atoi(optarg);

				if (orderby != 1 && orderby != -1) {
					fprintf(stderr, "[ERROR] trash -o [-1 or 1]"); 
					return ;
				}

				break;
		}
	}

	if (!strcmp(sortby, "filename")) 
		trashinfo_sort(trash_head, comp_func_filename, orderby);
	else if (!strcmp(sortby, "date"))
		trashinfo_sort(trash_head, comp_func_deletion_date, orderby);
	else if (!strcmp(sortby, "time"))
		trashinfo_sort(trash_head, comp_func_deletion_time, orderby);

	trashinfo_print(trash_head);
}

void ssu_list(int argc, char *argv[])
{
	char sortby[SHORTSTRMAX] = "size";
	int orderby, c;
	LIST_TYPE list_type;

	optind = 0;
	orderby = 1;

	list_type = FILESET;

	if (dups_list_h->next == NULL) {
		fprintf(stderr, "[ERROR] need to search first\n");
		return ;
	}

	while ((c = getopt(argc, argv, "l:c:o:")) != -1) { // category
		switch (c) {
			case 'l':
				if (!strcmp(optarg, "filelist"))
					list_type = FILELIST;
				else if (!strcmp(optarg, "fileset"))
					list_type = FILESET;
				else {
					fprintf(stderr, "[ERROR] please input fileinfo or filelist\n");
					return ;
				}

				break;
			case 'c':
				if (strcmp(optarg, "filename") && strcmp(optarg, "uid") && strcmp(optarg, "gid") && strcmp(optarg, "size") && strcmp(optarg, "mode")) {
					fprintf(stderr, "[ERROR] please input filename or uid or gid or size or mode\n");
					return ;
				}
				strcpy(sortby, optarg);

				break;
			case 'o':
				orderby = atoi(optarg);

				if (orderby != 1 && orderby != -1) {
					fprintf(stderr, "[ERROR] please input 1 or -1\n"); // need to modified
					return ;
				}

				break;
		}
	}

	linkedlist_sort(list_type, sortby, orderby);
	fileset_print_format(dups_list_h);
}

void linkedlist_sort(LIST_TYPE list_type, char *sortby, int orderby)
{
	if (list_type == FILELIST) {
		fileSet *cur_node = dups_list_h->next;

		while (cur_node != NULL) {
			filelist_sort(cur_node->filelist_head, get_comp_function(sortby), orderby);
			cur_node = cur_node->next;
		}

		return ;
	}

	fileset_sort(dups_list_h, get_comp_function(sortby), orderby);
}

int (*get_comp_function(char *sortby))(void *, void *, LIST_TYPE) {
	if (!strcmp(sortby, "filename"))
		return comp_func_filename;

	if (!strcmp(sortby, "uid"))
		return comp_func_uid;

	if (!strcmp(sortby, "gid"))
		return comp_func_gid;

	if (!strcmp(sortby, "size"))
		return comp_func_size;

	if (!strcmp(sortby, "mode"))
		return comp_func_mode;
}

void ssu_restore(int argc, char *argv[])
{
	int restore_index = atoi(argv[1])-1;
	int list_length = get_trashinfo_list_size(trash_head);

	if (restore_index < 0 || restore_index > list_length) {
		fprintf(stderr, "[ERROR] need to check restore index\n"); // need to modified
		return ;
	}

	char hash[HASHMAX];
	restore_file(trash_head, restore_index);

	trashInfo *deleted_trashinfo = trashinfo_delete(trash_head, restore_index); // need to test

	if (hash_function(deleted_trashinfo->path, hash))
		return ;

	fileSet *fileset_location;
	if ((fileset_location = fileset_search(dups_list_h, hash)) == NULL)
		fileset_append(dups_list_h, (deleted_trashinfo->statbuf).st_size, deleted_trashinfo->path, hash);
	else 
		filelist_append(fileset_location->filelist_head, deleted_trashinfo->path);

	trashinfo_print(trash_head);
}

void restore_file(trashInfo *head, int file_index)
{
	trashInfo *trashinfo_cur = head->next;

	for (int i = 0; i < file_index; i++)
		trashinfo_cur = trashinfo_cur->next;

	if (access(trashinfo_cur->path, F_OK) == 0) {
		char ans;

		printf("%s is aleady exist, ignore? (y/n) >> ", trashinfo_cur->path);
		scanf("%c\n", &ans);

		if (ans == 'n')
			return ;
	}

	char trash_file_path[PATH_MAX], trash_info_path[PATH_MAX];

	sprintf(trash_file_path, "%s%s", trash_path, trashinfo_cur->trash_name);
	sprintf(trash_info_path, "%s%s", trash_info, trashinfo_cur->trash_name);
	strcat(trash_info_path, ".trashinfo");

	if (rename(trash_file_path, trashinfo_cur->path) == -1) {
		fprintf(stderr, "[ERROR] rename error for %s\n", trash_file_path);
		return ;
	}

	write_log(RESTORE, trashinfo_cur->path);

	if (remove(trash_info_path) != 0) {
		fprintf(stderr, "[ERROR] remove error for %s\n", trash_info_path);
		return ;
	}

	printf("[RESTORE] success for %s\n", trashinfo_cur->path);
}

void trash_init(void)
{
	int trashlist_cnt;
	FILE *trashinfo_fp;
	char trashinfo_name[PATH_MAX];
	struct dirent **namelist;

	trash_head = (trashInfo *) malloc (sizeof(trashInfo));
	trash_head->next = NULL;

	if ((trashlist_cnt = get_dirlist(trash_path, &namelist)) == 0) 
		return ;

	for (int i = 0; i < trashlist_cnt; i++) {
		if (!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, ".."))
			continue;

		sprintf(trashinfo_name, "%s%s.trashinfo", trash_info, namelist[i]->d_name);

		if ((trashinfo_fp = fopen(trashinfo_name, "r")) == NULL) {
			fprintf(stderr, "fopen error for %s\n", trashinfo_name);
			exit(1);
		}

		trashInfo *trashinfo_node = (trashInfo *) malloc (sizeof(trashInfo));

		fscanf(trashinfo_fp, "[Trash Info]\nPath=%s\nDeletionDate=%10sT%8s\n", trashinfo_node->path, trashinfo_node->deletion_date, trashinfo_node->deletion_time);
		strcpy(trashinfo_node->trash_name, namelist[i]->d_name);

		char trashfile_name[PATHMAX];
		struct stat trash_statbuf;
		sprintf(trashfile_name, "%s%s", trash_path, namelist[i]->d_name);

		if (lstat(trashfile_name, &trash_statbuf) < 0) {
			fprintf(stderr, "lstat error for %s\n", trashfile_name);
			exit(1);
		}

		trashinfo_node->statbuf = trash_statbuf;
		trashinfo_append(trash_head, trashinfo_node);
	}
}

void trashinfo_append(trashInfo *head, trashInfo *node)
{
	trashInfo *trashinfo_cur = NULL;

	node->next = NULL;

	if (head->next == NULL)
		head->next = node;
	else {
		trashinfo_cur = head->next;

		while (trashinfo_cur->next != NULL)
			trashinfo_cur = trashinfo_cur->next;

		trashinfo_cur->next = node;
	}
}

trashInfo* trashinfo_delete(trashInfo *head, int file_index)
{
	trashInfo *trashinfo_cur = head;
	trashInfo *trashinfo_target;

	for (int i = 0; i < file_index; i++)
		trashinfo_cur = trashinfo_cur->next;

	trashinfo_target = trashinfo_cur->next;
	trashinfo_cur->next = trashinfo_cur->next->next;

	return trashinfo_target;
}

void trashinfo_print(trashInfo *head)
{
	int idx = 1;
	trashInfo *trashinfo_cur = head->next;

	if (trashinfo_cur == NULL) {
		printf("Trash bin is empty\n");
		return ;
	}

	printf("      %-100s\t%-20s\t%-20s\t%-20s\n", "FILENAME", "SIZE", "DELETION DATE", "DELETION TIME");

	while (trashinfo_cur != NULL) {
		printf("[%3d] %-100s\t%-20ld\t%-20s\t%-20s\n", idx++, trashinfo_cur->path, (trashinfo_cur->statbuf).st_size, trashinfo_cur->deletion_date, trashinfo_cur->deletion_time);
		trashinfo_cur = trashinfo_cur->next;
	}
}

void filelist_swap(fileList *prev_node, fileList *source, fileList *target)
{
	prev_node->next = target;
	source->next = target->next;
	target->next = source;
}

void fileset_swap(fileSet *prev_node, fileSet *source, fileSet *target)
{
	prev_node->next = target;
	source->next = target->next;
	target->next = source;
}

void trash_list_swap(trashInfo *prev_node, trashInfo *source, trashInfo *target)
{
	prev_node->next = target;
	source->next = target->next;
	target->next = source;
}

int get_trashinfo_list_size(trashInfo *head)
{
	trashInfo *trashinfo_cur = head->next;
	int length = 0;

	while (trashinfo_cur != NULL) {
		trashinfo_cur = trashinfo_cur->next;
		length++;
	}

	return length;
}

int comp_func_filename(void *source, void *target, LIST_TYPE list_type)
{
	switch (list_type) {
		case FILELIST: {
						   fileList *s = (fileList *)source;
						   fileList *t = (fileList *)target;
						   return strcmp(s->path, t->path);
					   }
		case TRASHINFO: {
							trashInfo *s = (trashInfo *)source;
							trashInfo *t = (trashInfo *)target;
							return strcmp(s->path, t->path);
						}
	}
}

int comp_func_uid(void *source, void *target, LIST_TYPE list_type)
{
	switch (list_type) {
		case FILELIST: {
						   fileList *s = (fileList *)source;
						   fileList *t = (fileList *)target;
						   return (s->statbuf).st_uid > (t->statbuf).st_uid ? 1 : -1;
					   }
		case TRASHINFO: {
							trashInfo *s = (trashInfo *)source;
							trashInfo *t = (trashInfo *)target;
							return (s->statbuf).st_uid > (t->statbuf).st_uid ? 1 : -1;
						}
	}
}

int comp_func_gid(void *source, void *target, LIST_TYPE list_type)
{
	switch (list_type) {
		case FILELIST: {
						   fileList *s = (fileList *)source;
						   fileList *t = (fileList *)target;
						   return (s->statbuf).st_gid > (t->statbuf).st_gid ? 1 : -1;
					   }
		case TRASHINFO: {
							trashInfo *s = (trashInfo *)source;
							trashInfo *t = (trashInfo *)target;
							return (s->statbuf).st_gid > (t->statbuf).st_gid ? 1 : -1;
						}
	}
}

int comp_func_size(void *source, void *target, LIST_TYPE list_type)
{
	switch (list_type) {
		case FILELIST: {
						   fileList *s = (fileList *)source;
						   fileList *t = (fileList *)target;
						   return (s->statbuf).st_size > (t->statbuf).st_size ? 1 : -1;
					   }
		case FILESET: {
						  fileSet *s = (fileSet *)source;
						  fileSet *t = (fileSet *)target;
						  return s->filesize > t->filesize ? 1 : -1;
					  }
		case TRASHINFO: {
							trashInfo *s = (trashInfo *)source;
							trashInfo *t = (trashInfo *)target;
							return (s->statbuf).st_size > (t->statbuf).st_size ? 1 : -1;
						}
	}
}

int comp_func_mode(void *source, void *target, LIST_TYPE list_type)
{
	switch (list_type) {
		case FILELIST: {
						   fileList *s = (fileList *)source;
						   fileList *t = (fileList *)target;
						   return (s->statbuf).st_mode > (t->statbuf).st_mode ? 1 : -1;
					   }
		case TRASHINFO: {
							trashInfo *s = (trashInfo *)source;
							trashInfo *t = (trashInfo *)target;
							return (s->statbuf).st_mode > (t->statbuf).st_mode ? 1 : -1;
						}
	}
}

int comp_func_deletion_date(void *source, void *target, LIST_TYPE list_type)
{
	trashInfo *s = (trashInfo *)source;
	trashInfo *t = (trashInfo *)target;
	return strcmp(s->deletion_date, t->deletion_date);
}

int comp_func_deletion_time(void *source, void *target, LIST_TYPE list_type)
{
	trashInfo *s = (trashInfo *)source;
	trashInfo *t = (trashInfo *)target;
	return strcmp(s->deletion_time, t->deletion_time);
}

void fileset_sort(fileSet *head, int (*comp_func)(void *, void *, LIST_TYPE), int order)
{
	fileSet *fileset_cur;
	int list_length;

	list_length = get_fileset_size(head) + 1;

	for (int i = 0; i < list_length; i++) {
		fileset_cur = head;

		for (int j = 0; j < list_length-i-2; j++) {
			if (comp_func(fileset_cur->next, fileset_cur->next->next, FILESET) * order > 0) 
				fileset_swap(fileset_cur, fileset_cur->next, fileset_cur->next->next);

			fileset_cur = fileset_cur->next;
		}
	}
}

void filelist_sort(fileList *head, int (*comp_func)(void *, void *, LIST_TYPE), int order)
{
	fileList *filelist_cur;
	int list_length;

	list_length = get_filelist_size(head) + 1;

	for (int i = 0; i < list_length; i++) {
		filelist_cur = head;

		for (int j = 0; j < list_length-i-2; j++) {
			if (comp_func(filelist_cur->next, filelist_cur->next->next, FILELIST) * order > 0) 
				filelist_swap(filelist_cur, filelist_cur->next, filelist_cur->next->next);

			filelist_cur = filelist_cur->next;
		}
	}
}

void trashinfo_sort(trashInfo *head, int (*comp_func)(void *, void *, LIST_TYPE), int order)
{
	trashInfo *trashinfo_cur;
	int list_length;

	list_length = get_trashinfo_list_size(head) + 1;

	for (int i = 0; i < list_length; i++) {
		trashinfo_cur = head;

		for (int j = 0; j < list_length-i-2; j++) {

			if (comp_func(trashinfo_cur->next, trashinfo_cur->next->next, TRASHINFO) * order > 0) 
				trash_list_swap(trashinfo_cur, trashinfo_cur->next, trashinfo_cur->next->next);

			trashinfo_cur = trashinfo_cur->next;
		}
	}
}
