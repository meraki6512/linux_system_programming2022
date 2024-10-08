#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>
#include <dirent.h>
#include <ctype.h>
#include <math.h>
#include <openssl/sha.h>
#include <fcntl.h>
#include <pthread.h>

#define NAMEMAX 255
#define PATHMAX 4096
#define HASHMAX 41
#define STRMAX 10000
#define ARGMAX 12
#define TIMEMAX 40

#define SIZE_ERROR -2
#define DIRECTORY 1
#define REGFILE 2

#define KB 1000
#define MB 1000000
#define GB 1000000000
#define KiB 1024
#define MiB 1048576
#define GiB 1073741824



/* variables */

typedef struct fileInfo {
	char path[PATHMAX];
	struct stat statbuf;
	struct fileInfo *next;
} fileInfo;

typedef struct fileList {
	long long filesize;
	char hash[HASHMAX];
	fileInfo *fileInfoList;
	struct fileList *next;
} fileList;

typedef struct dirList {
	char dirpath[PATHMAX];
	struct dirList *next;
} dirList;

char extension[10];
long long minbsize;
long long maxbsize;
char target_dir[PATH_MAX];
int thread_num = 1;

fileInfo *trash_list;
fileList *dups_list_h;

_Bool fsha1_called = 0;
int list_options[3] = {0,};
int sort_by = 0;
_Bool reverse = 0;

int trash_list_cnt = 0;
char trash_dt[PATHMAX][30] = {0, };
char trash_filename[PATHMAX][NAMEMAX] = {0, };

int log_fd;
char log_path[PATHMAX];
char trash_path[PATHMAX];
char trash_info_path[PATHMAX];
char same_size_files_dir[PATHMAX];

int delete_set_idx = 0;
int delete_list_idx = 0;




/* prototypes */

int split(char* string, char* seperator, char* argv[]);
void help(void);
int fsha1(void);
int list(void);
int trash(void);
int restore(int set_num);
int sha1(char *target_path, char *hash_result);
void swap(fileInfo* f1, fileInfo* f2);
int get_fsha1_options(int argc, char* argv[]);
int get_list_options(int argc, char* argv[]);
int get_trash_options(int argc, char* argv[]);
void get_path_from_home(char *path, char *path_from_home);
long long get_size(char *filesize);
char *get_extension(char *filename);
int is_dir(char *target_dir);
void get_same_size_files_dir(void);
int get_dirlist(char *target_dir, struct dirent ***namelist);
void dirlist_append(dirList *head, char *path);
void dir_traverse(dirList *dirlist);
void find_duplicates(void);
void remove_no_duplicates(void);
void filelist_print_format(fileList *head);
void get_trash_path(void);
void get_log_path(void);
void delete_prompt(void);
void remove_files(char *dir);
int get_file_mode(char *target_file, struct stat *statbuf);
void fileinfo_append(fileInfo *head, char *path);
char* get_username(void);
char* get_current_dt(void);
fileInfo *fileinfo_delete_node(fileInfo *head, char *path);
void write_trash_info(char* write_path, char* write_dt);
int get_delete_options(int argc, char* argv[]);
void filesize_with_comma(long long filesize, char *filesize_w_comma);
void sec_to_ymdt(struct tm *time, char *ymdt);
time_t get_recent_mtime(fileInfo *head, char *last_filepath);
void get_new_file_name(char *org_filename, char *new_filename);
void get_filename(char *path, char *filename);
void filelist_delete_node(fileList *head, char *hash);
void filelist_append(fileList *head, long long filesize, char *path, char *hash);
int fileinfolist_size(fileInfo *head);
int filelist_size(fileList *head);
int filelist_search(fileList *head, char *hash);
void get_fullpath(char *target_dir, char *target_file, char *fullpath);
_Bool is_root(void);
void dirlist_delete_all(dirList *head);
fileList* sort(fileList *filelist_cur);
fileInfo* reverse_fileinfo(fileInfo* cur);
fileList* reverse_filelist(fileList* head);




/* functions */

int main(void){
	
	char input[STRMAX];
	char* argv[ARGMAX];
	int argc = 0;
	
	while (1){
		
		printf("20201482> ");
		fgets(input, sizeof(input), stdin);
		argc = split(input, " ", argv);

		if (argc == 0)
			continue;

		if ((argc == 1) && (!strcmp(argv[0], "exit"))){
			printf("Prompt End\n");
			break;
		}
		else if (!strcmp(argv[0], "fsha1")){
			if (get_fsha1_options(argc, argv)<0)
				continue;
			fsha1();
		}
		else if (!strcmp(argv[0], "list")){
			if (!fsha1_called){
				printf("ERROR: call command 1 (fsha1) first.\n");
				continue;
			}
			if (get_list_options(argc, argv)<0)
				continue;
			if (list()<0)
				continue;
		}
		else if (!strcmp(argv[0], "trash")){
			if (get_trash_options(argc, argv)<0)
				continue;
			if (trash()<0)
				continue;
		}
		else if (!strcmp(argv[0], "restore")){
			int set_num = 0;
			if ((set_num = atoi(argv[1]))<=0){
				printf("ERROR: restore index error\n");
				continue;
			}
			if (restore(set_num)<0)
				continue;
			else if (trash()<0)
				continue;
		}
		else {
			help();
		}
	}
	
	close(log_fd);
	return 0;
}

//split input by given separator
//return argc (separated parse count)
int split(char* string, char* seperator, char* argv[]){

	int argc = 0;
	char* ptr = NULL;

	ptr = strtok(string, seperator);
	while (ptr!=NULL){
		argv[argc++] = ptr;
		ptr = strtok(NULL, seperator);
	}

	argv[argc-1][strlen(argv[argc-1])-1] = '\0';

	return argc;
}

//print help message
void help(void){
	printf("Usage:\n");
	printf("> fsha1 -e [FILE_EXTENSION] -l [MINSIZE] -h [MAXSIZE] -d [TARGET_DIRECTORY] -t [THREAD_NUM]\n");
	printf("	>> delete -l [SET_INDEX] -d [OPTARG] -i -f -t\n");
	printf("> trash -c [CATEGORY] -o [ORDER]\n");
	printf("> restore [RESTORE_INDEX]\n");
	printf("> list\n");
	printf("> help\n");
	printf("> exit\n");
}

int restore(int set_num){

	char trash_info_fpath[PATHMAX];
	char trash_fpath[PATHMAX];
	char write_buf[STRMAX];
	char write_dt[TIMEMAX];
	char write_user[NAMEMAX];
	strcpy(write_user, get_username());
	strcpy(write_dt, get_current_dt());
	
	if (trash_list_cnt<set_num){
		printf("ERROR: restore index error\n");
		return -1;
	}

	//trash_filename[set_num-1] : path to restore
	sprintf(trash_info_fpath, "%s%s", trash_info_path, strrchr(trash_filename[set_num-1], '/') + 1);
	sprintf(trash_fpath, "%s%s", trash_path, strrchr(trash_filename[set_num-1], '/') + 1);

	//writelog
	sprintf(write_buf, "[%s] %s %s %s\n", "RESTORE", trash_filename[set_num-1], write_dt, write_user);
	if (write(log_fd, write_buf, strlen(write_buf))!=strlen(write_buf)){
		fprintf(stderr, "write error for log\n");
		exit(1);
	}

	//remove trash info file
	if (unlink(trash_info_fpath)<0){
		fprintf(stderr, "unlink error\n");
		return -1;
	}

	//move from trash file
	if (rename(trash_fpath, trash_filename[set_num-1]) == -1){
		printf("ERROR: Fail to move duplicates to Trash\n");
		return -1;
	}
	
	//add fileInfo to fileList
	char hash[HASHMAX];
	fileInfo *newinfo = (fileInfo *)malloc(sizeof(fileInfo));
	memset(newinfo, 0, sizeof(fileInfo));
	strcpy(newinfo->path, trash_filename[set_num-1]);
	lstat(newinfo->path, &newinfo->statbuf);
	sha1(newinfo->path, hash);
	newinfo->next = NULL;
	
	fileList* filelist_cur = dups_list_h->next;
	while (filelist_cur != NULL){
		if (!strcmp(hash, filelist_cur->hash)){
			fileInfo *fileinfolist_cur = filelist_cur->fileInfoList->next;
			while (fileinfolist_cur != NULL){
				fileinfolist_cur = fileinfolist_cur->next;
			}
			fileinfolist_cur = newinfo;
		}
		filelist_cur = filelist_cur->next;
	}

	//delete fileInfo in trashlist
	fileinfo_delete_node(trash_list, trash_filename[set_num-1]);

	//delete dt in trash_dt
	memcpy(trash_dt[set_num-1], trash_dt[set_num], sizeof(trash_dt)-sizeof(trash_dt[set_num-1]));

	//delete filename in trash_filename
	memcpy(trash_filename[set_num-1], trash_filename[set_num], sizeof(trash_filename)-sizeof(trash_filename[set_num-1]));
	
	trash_list_cnt --;
}

//reverse sorting sets
fileList* reverse_filelist(fileList* head){
	
	fileList* p, *q, *r;
	p = head->next;
    q = NULL;
 
    while (p != NULL)
    {
        r = q;
        q = p;
        p = p->next;
        q->next = r;
    }
    
	return q;

}

//reverse sorting files	
fileInfo* reverse_fileinfo(fileInfo* cur){

	fileInfo* p, *q, *r;
	p = cur;
    q = NULL;
 
    while (p != NULL)
    {
        r = q;
        q = p;
        p = p->next;
        q->next = r;
    }
    
	return q;
}

//make, sort, print list in trash file directory
//return -1 when err 
int trash(void){

	char path[STRMAX];
	char info_path[STRMAX];
	struct stat statbuf;
	struct dirent **namelist;
	int listcnt =0, k = 0;
	FILE* info_fp;
	trash_list = (fileInfo*)malloc(sizeof(fileInfo));
	trash_list->next = NULL;
	fileInfo * trash_list_head = trash_list;
	if ((listcnt = get_dirlist(trash_path, &namelist))<3){
		printf("No files in trash can.\n");
		return -1;
	}
	trash_list_cnt = listcnt - 2;
	char line[STRMAX];
	char* argv[2];

	//make trash list
	for (int i = 0; i < listcnt; i++){
		if (!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, ".."))
			continue;

		sprintf(path, "%s%s", trash_path, namelist[i]->d_name);

		fileinfo_append(trash_list, path);
		
		sprintf(info_path, "%s%s", trash_info_path, namelist[i]->d_name);
		if ((info_fp=fopen(info_path, "r"))==NULL){
			fprintf(stderr, "fopen error - trash_info_path: %s\n", info_path);
			exit(1);
		}
		fgets(line, sizeof(line), info_fp);
		int argc = split(line, " ", argv);
		strcpy(trash_filename[k], line);
		sprintf(trash_dt[k++], "%s   %s", argv[argc-2], argv[argc-1]);
	}

	//sort trash list
	fileInfo* cur = trash_list->next;
	for(int i=0; cur != NULL; i++){

		fileInfo *horse = cur->next;
		for (int j=0; horse!=NULL; j++){
			if (sort_by == 0){ //filename
				if (strcmp(trash_filename[i], trash_filename[j])>0){
					swap(cur, horse);
					char tmp[PATHMAX];
					strcpy(tmp, trash_filename[i]);
					strcpy(trash_filename[i], trash_filename[j]);
					strcpy(trash_filename[j], tmp);
				}
			} 
			else if (sort_by == 1){ //size
				if (cur->statbuf.st_size > horse->statbuf.st_size)
					swap(cur, horse);
			}
			else if (sort_by == 2 || sort_by==3){ //compare date|time
				if (strcmp(trash_dt[i], trash_dt[j])>0){
					swap(cur, horse);
					char tmp[STRMAX];
					strcpy(tmp, trash_dt[i]);
					strcpy(trash_dt[i], trash_dt[j]);
					strcpy(trash_dt[j], tmp);
				}
			}
			horse = horse->next;
		}
		
		cur = cur->next;
	}

	//print
	if (reverse)
		trash_list->next = reverse_fileinfo(trash_list->next);

	printf("    %s %25s %15s %15s\n", "FILENAME", "SIZE", "DELETION DATE", "DELETION TIME");
	fileInfo *horse = trash_list->next;

	int i=1;
	while (horse != NULL){
		printf("[%d] %s %ld      %s\n", i++, trash_filename[i-1], horse->statbuf.st_size, trash_dt[i-1]);
		horse = horse->next;
	}
	printf("\n");
	
	trash_list = trash_list_head;

	return 0;

}

int get_trash_options(int argc, char* argv[]){

	optind = 1;
	int option = 0;
	reverse = 0;
	
	while ((option = getopt(argc, argv, "c:o:"))!=EOF){
		switch(option){
			
			case 'c':
				if (!strcmp(optarg, "filename")||!strcmp(optarg, ":")){
					sort_by = 0;
				}
				else if (!strcmp(optarg, "size")){
					sort_by = 1;
				}
				else if (!strcmp(optarg, "date")){
					sort_by = 2;
				}
				else if (!strcmp(optarg, "time")){
					sort_by = 3;
				}
				else{
					printf("ERROR: category can only be filename, size, date, time\n");
					return -1;
				}
				break;
			
			case 'o':
				if (!strcmp(optarg, "-1"))
					reverse = 1;
				break;

			case '?':
				printf("ERROR: incorrect option\n");
				return -1;
		}
	}

	return 0;
}

//swap data(path, statbuf) of fileInfo
void swap(fileInfo* f1, fileInfo* f2){
	
	char path[PATHMAX];
	struct stat statbuf;

	strcpy(path, f1->path);
	statbuf = f1->statbuf;

	strcpy(f1->path, f2->path);
	f1->statbuf = f2->statbuf;

	strcpy(f2->path, path);
	f2->statbuf = statbuf;
}

//sort file list
fileList* sort(fileList *filelist_cur){

	fileList * print_h;

	//sort by list_options
	if (list_options[0] && list_options[1]!=0) { //sort file(1)
		
		//file set
		while (filelist_cur != NULL){
			fileInfo *fileinfolist_cur = filelist_cur->fileInfoList->next;
			fileInfo *horse = fileinfolist_cur -> next;
		
			//file list
			while (fileinfolist_cur != NULL){	
			
				if (list_options[1]==1) { //compare filename(path)
					while (horse!=NULL){
						if(strcmp(fileinfolist_cur->path, horse->path)>0)
							swap(fileinfolist_cur, horse);
						horse = horse->next;
					}
				}
				else if (list_options[1]==2) { //uid
					while (horse!=NULL){
						if(fileinfolist_cur->statbuf.st_uid > horse->statbuf.st_uid)
							swap(fileinfolist_cur, horse);
						horse = horse->next;
					}
				}
				else if (list_options[1]==3) { //gid
					while (horse!=NULL){
						if(fileinfolist_cur->statbuf.st_gid > horse->statbuf.st_gid)
							swap(fileinfolist_cur, horse);
						horse = horse->next;
					}
				}
				else if (list_options[1]==0) { //mode
					while (horse!=NULL){
						if(fileinfolist_cur->statbuf.st_mode > horse->statbuf.st_mode)
							swap(fileinfolist_cur, horse);
						horse = horse->next;
					}
				}
				fileinfolist_cur = fileinfolist_cur->next;
			}
			filelist_cur = filelist_cur->next;
		}
		
		if (list_options[2]){
			print_h = dups_list_h -> next;
			fileList * cur = print_h;
			while (cur!=NULL){
				cur->fileInfoList->next = reverse_fileinfo(cur->fileInfoList->next);
				cur = cur -> next;
			}
		}
		
	}
	else if (!list_options[0] && list_options[1]==0) { //sort set(0)
		while (filelist_cur != NULL){
			fileList *horse = filelist_cur->next;
			while (horse!=NULL){
				fileInfo *fileinfolist_cur = filelist_cur->fileInfoList->next;
				fileInfo *fileinfo_horse = horse->fileInfoList->next;
				if (fileinfolist_cur->statbuf.st_size > fileinfo_horse->statbuf.st_size) {//compare size
					//swap_set();
					fileList* tmp = filelist_cur;
					filelist_cur = horse;
					horse = tmp;
				}
				horse = horse->next;
			}
			filelist_cur = filelist_cur->next;
		}
		
		if (list_options[2])
			print_h = reverse_filelist(dups_list_h);
	}
	
	return print_h;
}

//print file list 
int list(void){

	struct stat statbuf;
	fileList* print_list;
	fileList *filelist_cur = dups_list_h -> next;

	//print_list = sort(filelist_cur); //change
	print_list = filelist_cur;
	filelist_print_format(print_list);

}


//find sha1 dups
int fsha1(void){

	struct timeval begin_t, end_t;	
	dirList *dirlist = (dirList *)malloc(sizeof(dirList)); //target directory list
	dirlist->next = NULL;
	dups_list_h = (fileList *)malloc(sizeof(fileList)); //duplicate file list head
	dups_list_h -> next = NULL;

	get_same_size_files_dir(); //make dup list folder 
	gettimeofday(&begin_t, NULL);

	dirlist_append(dirlist, target_dir); //begin from target_dir
	dir_traverse(dirlist); //dirlist searching
	find_duplicates(); //find duplicates
	remove_no_duplicates();
	
	gettimeofday(&end_t, NULL);

	if (dups_list_h->next == NULL)
		printf("No duplicates in %s\n", target_dir);
	else 
		filelist_print_format(dups_list_h);

	end_t.tv_usec -= begin_t.tv_usec;
	printf("Searching time: %ld:%06ld(sec:usec)\n\n", end_t.tv_sec, end_t.tv_usec);

	get_log_path();
	get_trash_path();
	delete_prompt();

	fsha1_called = 1;

	return 0;
}

//get SHA1 hash
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

//append path to the end of the dirlist
void dirlist_append(dirList *head, char *path){

	dirList *newFile = (dirList *)malloc(sizeof(dirList));

	strcpy(newFile->dirpath, path);
	newFile->next = NULL;

	if (head->next == NULL)
		head->next = newFile;
	else{
		dirList *cur = head->next;

		while(cur->next != NULL)
			cur = cur->next;

		cur->next = newFile;
	}
}

//free dirList
void dirlist_delete_all(dirList *head)
{
	dirList *dirlist_cur = head->next;
	dirList *tmp;

	while (dirlist_cur != NULL){
		tmp = dirlist_cur->next;
		free(dirlist_cur);
		dirlist_cur = tmp;
	}

	head->next = NULL;
}

//scandir target_dir and get namelist
int get_dirlist(char *target_dir, struct dirent ***namelist)
{
	int cnt = 0;

	if ((cnt = scandir(target_dir, namelist, NULL, alphasort)) == -1){
		printf("ERROR: scandir error for %s\n", target_dir);
		return -1;
	}

	return cnt;
}

//check if root
_Bool is_root(void){
	return (getuid() == 0) ? 1: 0;
}

//get directory for store same_size_files 
void get_same_size_files_dir(void)
{
	//check user and make path
	if (is_root())
		sprintf(same_size_files_dir, "/root/20201482/");
	else
		get_path_from_home("~/20201482", same_size_files_dir);
	
	//make dir
	if (access(same_size_files_dir, F_OK) == 0) //exist
		remove_files(same_size_files_dir); //clean
	else
		mkdir(same_size_files_dir, 0755); 
}

//get fullpath = "target_dir/target_file"
void get_fullpath(char *target_dir, char *target_file, char *fullpath)
{
	strcat(fullpath, target_dir);

	if(fullpath[strlen(target_dir) - 1] != '/')
		strcat(fullpath, "/");

	strcat(fullpath, target_file);
	fullpath[strlen(fullpath)] = '\0';
}

//get trash path
void get_trash_path(void) 
{
	char tmp_trash_path[256];

	//check user and make path
	if (is_root()){
		sprintf(tmp_trash_path, "/root/.Trash/");
		sprintf(trash_path, "/root/.Trash/files/");
		sprintf(trash_info_path, "/root/.Trash/info/");
	}
	else{
		get_path_from_home("~/.Trash/", tmp_trash_path);
		get_path_from_home("~/.Trash/files/", trash_path);
		get_path_from_home("~/.Trash/info/", trash_info_path);
	}
	
	//make dir - trash file
	if (access(tmp_trash_path, F_OK) == 0)
		remove_files(tmp_trash_path);
	else{
		if (mkdir(tmp_trash_path, 0755)<0){
			fprintf(stderr, "mkdir trash_path error\n");
			exit(1);
		}
	}

	//make dir - trash file
	if (access(trash_path, F_OK) == 0)
		remove_files(trash_path);
	else{
		if (mkdir(trash_path, 0755)<0){
			fprintf(stderr, "mkdir trash_path(files) error\n");
			exit(1);
		}
	}

	//make dir - trash info
	if (access(trash_info_path, F_OK) == 0)
		remove_files(trash_info_path);
	else{
		if (mkdir(trash_info_path, 0755)<0){
			fprintf(stderr, "mkdir trash_path(info) error\n");
			exit(1);
		}
	}
}

//get log path
void get_log_path(void){
	
	//check user and make path
	if (is_root())
		sprintf(log_path, "/root/.duplicate_20201482.log");
	else
		get_path_from_home("~/.duplicate_20201482.log", log_path);
	
	//make file
	if ((log_fd = open(log_path, O_RDWR|O_CREAT|O_TRUNC, 0755))<0){
		fprintf(stderr, "open error for log file: %s\n", log_path);
		exit(1);
	}
}

//get path from home (~/...)
void get_path_from_home(char *path, char *path_from_home){

	char path_without_home[PATHMAX] = {0,};
	char *home_path;

    home_path = getenv("HOME");

    if (strlen(path) == 1){
		strncpy(path_from_home, home_path, strlen(home_path));
	}
    else {
        strncpy(path_without_home, path + 1, strlen(path)-1);
        sprintf(path_from_home, "%s%s", home_path, path_without_home);
    }
}

//search same hash idx in fileList
int filelist_search(fileList *head, char *hash)
{
	fileList *cur = head;
	int idx = 0;

	while (cur != NULL){
		if (!strcmp(cur->hash, hash))
			return idx;
		cur = cur->next;
		idx++;
	}

	return 0;
}

//get count of fileLists
int filelist_size(fileList *head)
{
	fileList *cur = head->next;
	int size = 0;

	while (cur != NULL){
		size++;
		cur = cur->next;
	}

	return size;
}

//remove files of dir
void remove_files(char *dir)
{
	struct dirent **namelist;
	int listcnt = get_dirlist(dir, &namelist);

	for (int i = 0; i < listcnt; i++){
		char fullpath[PATHMAX] = {0, };

		if (!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, ".."))
			continue;

		get_fullpath(dir, namelist[i]->d_name, fullpath);

		remove(fullpath);
	}
}

//append a file
void fileinfo_append(fileInfo *head, char *path)
{
	fileInfo *fileinfo_cur;

	fileInfo *newinfo = (fileInfo *)malloc(sizeof(fileInfo));
	memset(newinfo, 0, sizeof(fileInfo));
	strcpy(newinfo->path, path);
	lstat(newinfo->path, &newinfo->statbuf);
	newinfo->next = NULL;
	
	if (head->next == NULL){
		head->next = newinfo;
	}
	else {
		fileinfo_cur = head->next;
		while (fileinfo_cur->next != NULL){
			fileinfo_cur = fileinfo_cur->next;
		}

		fileinfo_cur->next = newinfo;
	}
	
}

//delete a file
fileInfo *fileinfo_delete_node(fileInfo *head, char *path)
{
	fileInfo *deleted;

	if (!strcmp(head->next->path, path)){
		deleted = head->next;
		head->next = head->next->next;
		return head->next;
	}
	else {
		fileInfo *fileinfo_cur = head->next;

		while (fileinfo_cur->next != NULL){
			if (!strcmp(fileinfo_cur->next->path, path)){
				deleted = fileinfo_cur->next;
				
				fileinfo_cur->next = fileinfo_cur->next->next;
				return fileinfo_cur->next;
			}

			fileinfo_cur = fileinfo_cur->next;
		}
	}
}

//get count of files
int fileinfolist_size(fileInfo *head)
{
	fileInfo *cur = head->next;
	int size = 0;

	while (cur != NULL){
		size++;
		cur = cur->next;
	}

	return size;
}

//append filelist
void filelist_append(fileList *head, long long filesize, char *path, char *hash)
{
    fileList *newfile = (fileList *)malloc(sizeof(fileList));
    memset(newfile, 0, sizeof(fileList));

    newfile->filesize = filesize;
    strcpy(newfile->hash, hash);

    newfile->fileInfoList = (fileInfo *)malloc(sizeof(fileInfo));
    memset(newfile->fileInfoList, 0, sizeof(fileInfo));

    fileinfo_append(newfile->fileInfoList, path);
    newfile->next = NULL;

    if (head->next == NULL) {
        head->next = newfile;
    }
    else {
        fileList *cur_node = head->next, *prev_node = head, *next_node;

        while (cur_node != NULL && cur_node->filesize < newfile->filesize) {
            prev_node = cur_node;
            cur_node = cur_node->next;
        }

        newfile->next = cur_node;
        prev_node->next = newfile;
    }
}

//delete filelist
void filelist_delete_node(fileList *head, char *hash)
{
	fileList *deleted;

	if (!strcmp(head->next->hash, hash)){
		deleted = head->next;
		head->next = head->next->next;
	}
	else {
		fileList *filelist_cur = head->next;

		while (filelist_cur->next != NULL){
			if (!strcmp(filelist_cur->next->hash, hash)){
				deleted = filelist_cur->next;

				filelist_cur->next = filelist_cur->next->next;

				break;
			}

			filelist_cur = filelist_cur->next;
		}
	}

	free(deleted);
}

//err: return -1 
//get options and check if they are correct
int get_fsha1_options(int argc, char* argv[]){

	optind = 1;
	int option = 0, cnt = 0;
	char* tmp_ext;
	
	while ((option = getopt(argc, argv, "e:l:h:d:t:"))!=EOF){
		switch(option){
			case 'e': //extension
				if (!strcmp(optarg, ":")){ //no input
					printf("ERROR: [FILE_EXTENSION] should exist\n");
					return -1;
				}
				else{
					if (strchr(optarg, '*')==NULL){ 
						printf("ERROR: [FILE_EXTENSION] should be be '*' or '*.extension'\n");
						return -1;
					}
					if (!strcmp(optarg, "*"))
						tmp_ext = optarg;	
					else if ((tmp_ext = get_extension(optarg))==NULL){
						printf("ERROR: [FILE_EXTENSION] should be be '*' or '*.extension'\n");
						return -1;
					}
					else{
						strcpy(extension, tmp_ext);
					}
				}
				break;
			case 'l': //minbsize	
				if (!strcmp(optarg, ":")){ //no input
					printf("ERROR: [MINSIZE] should exist\n");
					return -1;
				}
				else{
					if ((minbsize = get_size(optarg))==-1)
						minbsize = 0;
					if (minbsize == SIZE_ERROR){
						printf("ERROR: Size wrong -min size : %s\n", optarg);
						return -1;
					}
				}
				break;
			case 'h': //maxbsize
				if (!strcmp(optarg, ":")){ //no input
					printf("ERROR: [MAXSIZE] should exist\n");
					return -1;
				}
				else{
					maxbsize = get_size(optarg);
					if (maxbsize == SIZE_ERROR){
						printf("ERROR: Size wrong -max size : %s\n", optarg);
						return -1;
					}
				}
				break;
			case 'd': //target directory
				if (strchr(optarg, '~') != NULL) //from home
					get_path_from_home(optarg, target_dir);
				else if (realpath(optarg, target_dir) == NULL){
					printf("ERROR: [TARGET_DIRECTORY] should exist\n");
					return -1;
				}
				break;
			case 't': //thread number
				if ((thread_num = atoi(optarg))<=0){
					printf("optarg: %s999\n", optarg);
					printf("[TREAD_NUM] should be more than 0\n");
					thread_num = 1;
					return -1;
				}
				break;
			case '?':
				printf("ERROR: Incorrect Option\n");
				return -1;
		}

		cnt++;
	}
	
	//check cnt 
	if (cnt == 0)
		return -1;
	else if (cnt<4){
		help();
		return -1;
	}
	
	//check size
	if (maxbsize != -1 && minbsize > maxbsize){
		printf("ERROR: [MAXSIZE] should be bigger than [MINSIZE]\n");
		return -1;
	}

	//check target directory access
	if (access(target_dir, F_OK)<0){
		printf("ERROR: %s directory doesn't exist\n", target_dir);
		return -1;
	}

	//check target directory is_dir
	if (!is_dir(target_dir)){
		printf("ERROR: [TARGET_DIRECTORY] should be a directory\n");
		return -1;
	}

	return 0;
}

int get_list_options(int argc, char* argv[]){

	optind = 1;
	int option = 0;
	list_options[0] = 0;
	list_options[1] = 0;
	list_options[2] = 0;
	
	while ((option = getopt(argc, argv, "l:c:o:"))!=EOF){
		switch(option){

			case 'l':
				if (!strcmp(optarg, "fileset")||!strcmp(optarg, ":")){
					list_options[0] = 0;
				}
				else if (!strcmp(optarg, "filelist")){
					list_options[0] = 1;
				}
				break;
			
			case 'c':
				if (!strcmp(optarg, "size")||!strcmp(optarg, ":")){
					list_options[1] = 0;
				}
				else if (!strcmp(optarg, "filename")){
					list_options[1] = 1;
				}
				else if (!strcmp(optarg, "uid")){
					list_options[1] = 2;
				}
				else if (!strcmp(optarg, "gid")){
					list_options[1] = 3;
				}
				else if (!strcmp(optarg, "mod")){
					list_options[1] = 4;
				}
				break;
			
			case 'o':
				if (!strcmp(optarg, "1")||!strcmp(optarg, ":")){
					list_options[2] = 0;
				}
				else if (!strcmp(optarg, "-1")){
					list_options[2] = 1;
				}
				break;

			case '?':
				printf("ERROR: incorrect option\n");
				return -1;
		}
	}

	return 0;
}

//check if given path is directory
int is_dir(char *target_dir)
{
    struct stat statbuf;

    if (lstat(target_dir, &statbuf) < 0){
        printf("ERROR: lstat error for %s\n", target_dir);
        return 1;
    }
    return S_ISDIR(statbuf.st_mode) ? DIRECTORY : 0;

}

//get extension
char *get_extension(char *filename)
{
	char *tmp_ext;

	if ((tmp_ext = strstr(filename, ".tar")) != NULL || (tmp_ext = strrchr(filename, '.')) != NULL)
		return tmp_ext + 1;
	else
		return NULL;
}

//rm extension, path ... extract just filename
void get_filename(char *path, char *filename)
{
	char tmp_name[NAMEMAX];
	char *pt = NULL;

	memset(tmp_name, 0, sizeof(tmp_name));
	
	if (strrchr(path, '/') != NULL)
		strcpy(tmp_name, strrchr(path, '/') + 1);
	else
		strcpy(tmp_name, path);
	
	if ((pt = get_extension(tmp_name)) != NULL)
		pt[-1] = '\0';

	if (strchr(path, '/') == NULL && (pt = strrchr(tmp_name, '.')) != NULL)
		pt[0] = '\0';
	
	strcpy(filename, tmp_name);
}

//get new_filename : count of same name in trash path (.extension)
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

//get size
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

//get file mode
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

//get recent changed mtime
time_t get_recent_mtime(fileInfo *head, char *last_filepath)
{
	fileInfo *fileinfo_cur = head->next;
	time_t mtime = 0;

	while (fileinfo_cur != NULL){
		if (fileinfo_cur->statbuf.st_mtime > mtime){
			mtime = fileinfo_cur->statbuf.st_mtime;
			strcpy(last_filepath, fileinfo_cur->path);
		}
		fileinfo_cur = fileinfo_cur->next;
	}
	return mtime;
}

//time format
void sec_to_ymdt(struct tm *time, char *ymdt)
{
	sprintf(ymdt, "%04d-%02d-%02d %02d:%02d:%02d", time->tm_year + 1900, time->tm_mon + 1, time->tm_mday, time->tm_hour, time->tm_min, time->tm_sec);
}

//file size with comma(1,000)
void filesize_with_comma(long long filesize, char *filesize_w_comma)
{
	char filesize_wo_comma[STRMAX] = {0, };
	int comma;
	int idx = 0;

	sprintf(filesize_wo_comma, "%lld", filesize);
	comma = strlen(filesize_wo_comma)%3;

	for (int i = 0 ; i < strlen(filesize_wo_comma); i++){
		if (i > 0 && (i%3) == comma)
			filesize_w_comma[idx++] = ',';

		filesize_w_comma[idx++] = filesize_wo_comma[i];
;}

	filesize_w_comma[idx] = '\0';
}

//print file List
void filelist_print_format(fileList *head)
{
	fileList *filelist_cur = head->next;
	int set_idx = 1;	

	while (filelist_cur != NULL){
		fileInfo *fileinfolist_cur = filelist_cur->fileInfoList->next;
		char mtime[STRMAX];
		char atime[STRMAX];
		char filesize_w_comma[STRMAX] = {0, };
		int i = 1;

		filesize_with_comma(filelist_cur->filesize, filesize_w_comma);

		printf("---- Identical files #%d (%s bytes - %s) ----\n", set_idx++, filesize_w_comma, filelist_cur->hash);

		while (fileinfolist_cur != NULL){
			sec_to_ymdt(localtime(&fileinfolist_cur->statbuf.st_mtime), mtime);
			sec_to_ymdt(localtime(&fileinfolist_cur->statbuf.st_atime), atime);
			printf("[%d] %s (mtime : %s) (atime : %s)\n", i++, fileinfolist_cur->path, mtime, atime);

			fileinfolist_cur = fileinfolist_cur->next;
		}

		printf("\n");

		filelist_cur = filelist_cur->next;
	}	
}

//bfs
void dir_traverse(dirList *dirlist)
{
	pthread_t tid[thread_num];
	dirList *cur = dirlist->next;
	dirList *subdirs = (dirList *)malloc(sizeof(dirList));
	memset(subdirs, 0 , sizeof(dirList));

	while (cur != NULL){
		struct dirent **namelist;
		int listcnt;

		listcnt = get_dirlist(cur->dirpath, &namelist);

		for (int i = 0; i < listcnt; i++){

			char fullpath[PATHMAX] = {0, };
			struct stat statbuf;
			int file_mode;
			long long filesize;

			if (!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, ".."))
				continue;

			get_fullpath(cur->dirpath, namelist[i]->d_name, fullpath);

			if (!strcmp(fullpath,"/proc") || !strcmp(fullpath, "/run") || !strcmp(fullpath, "/sys") || !strcmp(fullpath, trash_path))
				continue;

			file_mode = get_file_mode(fullpath, &statbuf);

			if ((filesize = (long long)statbuf.st_size) == 0)
				continue;

			if (filesize < minbsize)
				continue;

			if (maxbsize != -1 && filesize > maxbsize)
				continue;

			if (file_mode == DIRECTORY)
				dirlist_append(subdirs, fullpath);
			else if (file_mode == REGFILE){
				FILE *fp;
				char filename[PATHMAX*2];
				char *path_extension;
				char hash[HASHMAX];

				sprintf(filename, "%s/%lld", same_size_files_dir, filesize);

				memset(hash, 0, HASHMAX);
				sha1(fullpath, hash);

				path_extension = get_extension(fullpath);

				if (strlen(extension) != 0){
					if (path_extension == NULL || strcmp(extension, path_extension))
						continue;
				}

				if ((fp = fopen(filename, "a")) == NULL){
					printf("ERROR: fopen error for %s\n", filename);
					return;
				}

				fprintf(fp, "%s %s\n", hash, fullpath);

				fclose(fp);
			}
		}

		cur = cur->next;
	}

	dirlist_delete_all(dirlist); //: free dirlist

	if (subdirs->next != NULL)
		dir_traverse(subdirs);
}

//find dups in same_size_files_dir
void find_duplicates(void)
{
	struct dirent **namelist;
	int listcnt;
	char hash[HASHMAX];
	FILE *fp;

	listcnt = get_dirlist(same_size_files_dir, &namelist);

	for (int i = 0; i < listcnt; i++){
		char filename[PATHMAX*2];
		long long filesize;
		char filepath[PATHMAX];
		char hash[HASHMAX];
		char line[STRMAX];

		if (!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, ".."))
			continue;

		filesize = atoll(namelist[i]->d_name);
		sprintf(filename, "%s/%s", same_size_files_dir, namelist[i]->d_name);

		if ((fp = fopen(filename, "r")) == NULL){
			printf("ERROR: fopen error for %s\n", filename);
			continue;
		}

		while (fgets(line, sizeof(line), fp) != NULL){
			int idx;

			strncpy(hash, line, HASHMAX);
			hash[HASHMAX-1] = '\0';

			strcpy(filepath, line+HASHMAX);
			filepath[strlen(filepath)-1] = '\0';

			if ((idx = filelist_search(dups_list_h, hash)) == 0)
				filelist_append(dups_list_h, filesize, filepath, hash);
			else{
				fileList *filelist_cur = dups_list_h;
				while (idx--){
					filelist_cur = filelist_cur->next;
				}
				fileinfo_append(filelist_cur->fileInfoList, filepath);
			}
		}

		fclose(fp);
	}
}

//remove if not dup
void remove_no_duplicates(void)
{
	fileList *filelist_cur = dups_list_h->next;

	while (filelist_cur != NULL){
		fileInfo *fileinfo_cur = filelist_cur->fileInfoList;

		if (fileinfolist_size(fileinfo_cur) < 2)
			filelist_delete_node(dups_list_h, filelist_cur->hash);

		filelist_cur = filelist_cur->next;
	}
}

//get options and check arguments
int get_delete_options(int argc, char* argv[]){

	optind = 1;
	int option = 0, cnt = 0;
	_Bool l_flag = 0;
	int option_flag = 0;

	while ((option = getopt(argc, argv, "l:d:ift"))!=EOF) {

		switch(option){

			case 'l': //set index
				if (!strcmp(optarg, ":")){
					printf("ERROR: [SET_IDX] should exist\n");
					return -1;
				}
				if ((delete_set_idx = atoi(optarg))<=0){
					printf("ERROR: [SET_IDX] should be positive number\n");
					return -1;
				}
				l_flag = 1;
				break;

			case 'd': //list index
				if (!strcmp(optarg, ":")){
					printf("ERROR: [LIST_IDX] should exist (when using option d)\n");
					return -1;
				}
				if ((delete_list_idx = atoi(optarg))<=0){
					printf("ERROR: [LIST_IDX] should be positive number\n");
					return -1;
				}
				option_flag = 1;
				break;

			case 'i':
				option_flag = 2;
				break;
			case 'f':
				option_flag = 3;
				break;
			case 't':
				option_flag = 4;
				break;

			case '?':
				printf("ERROR: Only f, t, i, d options are available\n");
				return -1;
		}

		cnt++;
	}

	if (cnt!=2){
		printf("ERROR: incorrect command\n");
		printf("Usage: >> delete -l [SET_IDX] -d [LIST_IDX]\n");
		printf("                                     -i\n");
		printf("                                     -f\n");
		printf("                                     -t\n");
		return -1;
	}

	return option_flag;
}

char* get_username(void){

	static char username[NAMEMAX];
	getlogin_r(username, NAMEMAX);
	return ((is_root()) ? "root" : username);

}

char* get_current_dt(void){
	
	static char current_time[TIMEMAX];
	time_t ct;
	struct tm tm;
    ct = time(NULL);
	sec_to_ymdt(localtime(&ct), current_time);

	return current_time;

}

void write_trash_info(char* write_path, char* write_dt){
	
	FILE* fp;
	char fpath[PATHMAX];
	char buf[STRMAX];

	sprintf(fpath, "%s%s", trash_info_path, strrchr(write_path, '/') + 1);
	if ((fp = fopen(fpath, "w"))==NULL){
		fprintf(stderr, "fopen error in write_trash_info()\n");
		exit(1);
	}

	sprintf(buf, "%s %s", write_path, write_dt);
	if (fwrite(buf, sizeof(char), STRMAX, fp)<0){
		fprintf(stderr, "fwrite error\n");
		exit(1);
	}

	fclose(fp);
}

void delete_prompt(void)
{
	while (filelist_size(dups_list_h) > 0){

		char input[STRMAX];
		char* argv[7];
		int argc;
		int set_idx;
		int option;
		char last_filepath[PATHMAX];
		char modifiedtime[TIMEMAX];
		time_t mtime = 0;
		fileList *target_filelist_p;
		fileInfo *target_infolist_p;
		char write_buf[STRMAX];
		char write_path[PATHMAX];
		char write_dt[TIMEMAX];
		char write_user[NAMEMAX];
		strcpy(write_user, get_username());
		strcpy(write_dt, get_current_dt());

		printf(">> ");
		fgets(input, sizeof(input), stdin);

		if (!strcmp(input, "exit\n")){
			printf(">> Back to Prompt\n");
			break;
		} 
		else if(!strcmp(input, "\n"))
			continue;

		//get options and check arguments
		argc = split(input, " ", argv);

		if ((option = get_delete_options(argc, argv))<0)
			continue;

		if (delete_set_idx <= 0 || delete_set_idx > filelist_size(dups_list_h)){
			printf("ERROR: [SET_IDX] out of range: %d\n", delete_set_idx);
			continue;
		}

		//target file list
		target_filelist_p = dups_list_h->next;
		set_idx = delete_set_idx;

		while (--set_idx)
			target_filelist_p = target_filelist_p->next;

		target_infolist_p = target_filelist_p->fileInfoList;

		mtime = get_recent_mtime(target_infolist_p, last_filepath);
		sec_to_ymdt(localtime(&mtime), modifiedtime);

		//delete options
		if (option == 3){ //option f 
			fileInfo *tmp;
			fileInfo *deleted = target_infolist_p->next;

			while (deleted != NULL){
				tmp = deleted->next;

				if (!strcmp(deleted->path, last_filepath)){
					deleted = tmp;
					continue;
				}

				//writelog
				if (realpath(deleted->path, write_path)==NULL){
					fprintf(stderr, "realpath error\n");
					exit(1);
				}
				sprintf(write_buf, "[%s] %s %s %s\n", "DELETE", write_path, write_dt, write_user);
				if (write(log_fd, write_buf, strlen(write_buf))!=strlen(write_buf)){
					fprintf(stderr, "write error for log\n");
					exit(1);
				}

				//remove
				remove(deleted->path);
				free(deleted);
				deleted = tmp;
			}

			filelist_delete_node(dups_list_h, target_filelist_p->hash);
			printf("Left file in #%d : %s (%s)\n\n", atoi(argv[2]), last_filepath, modifiedtime);
		}
		else if(option == 4){ //option t
			fileInfo *tmp;
			fileInfo *deleted = target_infolist_p->next;
			char move_to_trash[PATHMAX];
			char filename[PATHMAX];

			while (deleted != NULL){
				tmp = deleted->next;

				if (!strcmp(deleted->path, last_filepath)){
					deleted = tmp;
					continue;
				}

				memset(move_to_trash, 0, sizeof(move_to_trash));
				memset(filename, 0, sizeof(filename));

				sprintf(move_to_trash, "%s%s", trash_path, strrchr(deleted->path, '/') + 1);

				if (access(move_to_trash, F_OK) == 0){ //if path exist
					get_new_file_name(deleted->path, filename);
					strncpy(strrchr(move_to_trash, '/') + 1, filename, strlen(filename));
				}
				else
					strcpy(filename, strrchr(deleted->path, '/') + 1);

				//writelog
				if (realpath(deleted->path, write_path)==NULL){
					fprintf(stderr, "realpath error\n");
					exit(1);
				}
				sprintf(write_buf, "[%s] %s %s %s\n", "REMOVE", write_path, write_dt, write_user);
				if (write(log_fd, write_buf, strlen(write_buf))!=strlen(write_buf)){
					fprintf(stderr, "write error for log\n");
					exit(1);
				}

				//make trash info file
				write_trash_info(write_path, write_dt);

				//move to trash
				if (rename(deleted->path, move_to_trash) == -1){
					printf("ERROR: Fail to move duplicates to Trash\n");
					return;
				}

				free(deleted);
				deleted = tmp;
			}

			filelist_delete_node(dups_list_h, target_filelist_p->hash);
			printf("All files in #%d have moved to Trash except \"%s\" (%s)\n\n", atoi(argv[2]), last_filepath, modifiedtime);
		}
		else if (option == 2) { //option i
			char ans[STRMAX];
			fileInfo *fileinfo_cur = target_infolist_p->next;
			fileInfo *deleted_list = (fileInfo *)malloc(sizeof(fileInfo));
			fileInfo *tmp;
			int listcnt = fileinfolist_size(target_infolist_p);


			while (fileinfo_cur != NULL && listcnt--){
				printf("Delete \"%s\"? [y/n] ", fileinfo_cur->path);
				memset(ans, 0, sizeof(ans));
				fgets(ans, sizeof(ans), stdin);

				if (!strcmp(ans, "y\n") || !strcmp(ans, "Y\n")){
					//writelog
					if (realpath(fileinfo_cur->path, write_path)==NULL){
						fprintf(stderr, "realpath error\n");
						exit(1);
					}
					sprintf(write_buf, "[%s] %s %s %s\n", "DELETE", write_path, write_dt, write_user);
					if (write(log_fd, write_buf, strlen(write_buf))!=strlen(write_buf)){
						fprintf(stderr, "write error for log\n");
						exit(1);
					}
					//remove
					remove(fileinfo_cur->path);
					fileinfo_cur = fileinfo_delete_node(target_infolist_p, fileinfo_cur->path);
				}
				else if (!strcmp(ans, "n\n") || !strcmp(ans, "N\n"))
					fileinfo_cur = fileinfo_cur->next;
				else {
					printf("ERROR: Answer should be 'y/Y' or 'n/N'\n");
					break;
				}
			}
			printf("\n");

			if (fileinfolist_size(target_infolist_p) < 2)
				filelist_delete_node(dups_list_h, target_filelist_p->hash);

		}
		else if(option == 1){ //option d
			fileInfo *deleted;
			int list_idx = delete_list_idx;

			if (delete_list_idx <= 0 || delete_list_idx > fileinfolist_size(target_infolist_p)){
				printf("ERROR: [LIST_IDX] out of range\n");
				continue;
			}

			deleted = target_infolist_p;

			while (list_idx--)
				deleted = deleted->next;

			printf("\"%s\" has been deleted in #%d\n\n", deleted->path, atoi(argv[2]));
			//writelog
			if (realpath(deleted->path, write_path)==NULL){
				fprintf(stderr, "realpath error\n");
				exit(1);
			}
			sprintf(write_buf, "[%s] %s %s %s\n", "DELETE", write_path, write_dt, write_user);
			if (write(log_fd, write_buf, strlen(write_buf))!=strlen(write_buf)){
				fprintf(stderr, "write error for log\n");
				exit(1);
			}
			remove(deleted->path);
			fileinfo_delete_node(target_infolist_p, deleted->path);

			if (fileinfolist_size(target_infolist_p) < 2)
				filelist_delete_node(dups_list_h, target_filelist_p->hash);
		}

		filelist_print_format(dups_list_h);
	}
}


