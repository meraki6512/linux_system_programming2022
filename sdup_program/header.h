#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <wait.h>
#include <errno.h>
#include <dirent.h>
#include <time.h>
#include <math.h>
#include <openssl/md5.h>
#include <openssl/sha.h>

#define ARG_MAX 5
#define OPT_MIN 2
#define OPT_MAX 3
#define EXT_PT_MAX 10
#define TM_MAX 20
#define SIZ_MAX 64
#define PATH_MAX 4096
#define NAME_MAX 255
#define BUF_MAX 1024
#define TRASH_SIZE 30
#define TRASH "/.local/share/Trash/files"



typedef struct Node{
	char path[PATH_MAX];
	size_t size;
	char md5[MD5_DIGEST_LENGTH];
	char sha1[SHA_DIGEST_LENGTH];
	struct Node* next;
}Node;


//sdup
int split(char* string, char* seperator, char* argv[]);

//fmd5, fsha1
//int split(char* string, char* seperator, char* argv[]);
void command_fmd5(char* argv[]);
void command_fsha1(char* argv[]); 
void read_directory(char* directory, char* extension, double min, double max);

void get_string_time(char* path, int a_m_c, char* time);
void get_element_num(int index, int* element__num);
void pop_md5_diff(void);
void pop_sha1_diff(void);
void pop_is_1(void);

void print_array(void);
void sort_array(void);
void swap_head(int i, int j);
double to_byte(char* size);
const char* to_comma_byte(size_t size);
_Bool is_extension_matching(char* extension, char* d_name);

void get_hash(char* path, char* md5, char* sha1);

struct Node* create_node(char path[PATH_MAX], size_t size);
void add_node(int index, struct Node* new_node);
int delete_node(int index, int del);
struct Node* copy_node(struct Node* old_node);
void free_all(void);
void free_list(struct Node** free_head);
void memmove_head(int index);

void command_delete_dup(void);
void option_d(int array_idx, int linked_idx);
void option_i(int array_idx);
void option_f(int array_idx);
void option_t(int array_idx);

void find_latest_modified(int array_idx, int* latest_modified);
void remove_files(int array_idx, int latest_modified, char* left_path, time_t* left_mtime, _Bool to_trash);
