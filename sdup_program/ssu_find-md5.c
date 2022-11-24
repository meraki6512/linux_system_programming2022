#include "header.h"


int size_count = 0; // number of "sets" of duplicates
struct Node* head[BUF_MAX] = {0,}; // array of linked list (of duplicates)


int main(int argc, char* argv[]){

	if (argc!=5){
		printf("argc: %d\n", argc);
		printf("ERROR: argument error\n");
		return -1;
	}

	command_fmd5(argv);
	return 0;
}


//argv[0]: fmd5 (command)
//argv[1]: file extension 
//argv[2]: min size //argv[3]: max size
//argv[4]: target directory 
void command_fmd5(char* argv[]){

	struct timeval start, end;
	char target_directory[PATH_MAX];
	struct stat statbuf;
	double min_byte, max_byte;
	char tmp[PATH_MAX];

	//error handling 
	//(path, stat, size, extension ...)

	if (!strcmp(argv[4], "~")){
		strcpy(argv[4], getenv("HOME"));
	}
	else if (!strncmp(argv[4], "~", 1)){
		sprintf(tmp, "%s%s", getenv("HOME"), argv[4]+1);
		strcpy(argv[4], tmp);
	}

	if (realpath(argv[4], target_directory) == NULL){
		printf("ERROR: Path exist error\n");
		return;
	}

	if (stat(target_directory, &statbuf)<0){
		fprintf(stderr, "stat error for %s\n", target_directory);
		exit(1);
	}

	if (!S_ISDIR(statbuf.st_mode)){
		printf("ERROR: Path must be directory\n");
		return;
	}
	
	if ( (strcmp(argv[1], "*")!=0) && (strncmp(argv[1], "*.", 2)!=0) ){
		printf("ERROR: invalid FILE_EXTENSION\n");
		return;
	}

	if (strcmp(argv[2], "~")!=0){
		if ((min_byte = to_byte(argv[2]))<0){
			printf("ERROR: invalid MINSIZE\n");
			return;
		}
	}
	else
		min_byte = -1.0;

	if (strcmp(argv[3], "~")!=0){
		if ((max_byte = to_byte(argv[3]))<0){
			printf("ERROR: invalid MAXSIZE\n");
			return;
		}
	}
	else
		max_byte = -1.0;

	//read
	gettimeofday(&start, NULL);
	read_directory(target_directory, argv[1], min_byte, max_byte);
	gettimeofday(&end, NULL);

	pop_is_1();	
	pop_md5_diff();

	//print
	if (size_count==0){
		printf("No duplicates in %s\n", target_directory);
		printf("Searching time: %ld:%ld\n", end.tv_sec-start.tv_sec, end.tv_usec-start.tv_usec);
	}
	else{ 
		print_array();
		printf("Searching time: %ld:%ld(sec:usec)\n", end.tv_sec-start.tv_sec, end.tv_usec-start.tv_usec);
		command_delete_dup();
	}
	
	free_all();
	return;
}






int split(char* string, char* seperator, char* argv[]){

	int argc = 0;
	char* ptr = NULL;

	ptr = strtok(string, seperator);
	while (ptr!=NULL){
		argv[argc++] = ptr;
		ptr = strtok(NULL, seperator);
	}
	return argc;
}


//find same file while reading directory
//input value 'directory': realpath of target_directory
void read_directory(char* directory, char* extension, double min, double max){

	int i;
	int count = 0;
	struct stat statbuf;
	_Bool has_extension = strcmp(extension, "*");
	char queue[BUF_MAX][PATH_MAX];
	int queue_index = 0;
	int queue_num = 1;
	strcpy(queue[queue_index], directory);
	
	//BFS
	while (queue_index < queue_num) {
		
		//access check
		if (access(queue[queue_index], F_OK)!=0){
			if (errno == 13)
				return;
			fprintf(stderr, "access error for %s\n", queue[queue_index]);
			exit(1);
		}

		//stat info
		if (lstat(queue[queue_index], &statbuf)<0){
			fprintf(stderr, "lstat error for %s\n", queue[queue_index]);
			exit(1);
		}

		//if Que[idx]==DIR
		if (S_ISDIR(statbuf.st_mode)){

			//scandir (Que[idx])
			struct dirent** namelist;
			if ((count = scandir(queue[queue_index], &namelist, NULL, alphasort))==-1){
				fprintf(stderr, "scandir error for %s\n", directory);
				exit(1);
			}

			//enQue (namelist)
			for (i = 0; i<count; i++){

				if (!strcmp(namelist[i]->d_name, ".")||!strcmp(namelist[i]->d_name,"..")){
					continue;
				}

				if (!strcmp(queue[queue_index], "/")){
					if ((!strcmp(namelist[i]->d_name, "proc"))||(!strcmp(namelist[i]->d_name, "run"))||(!strcmp(namelist[i]->d_name, "sys"))){
						continue;
					}
				}
				
				char pathname[PATH_MAX+NAME_MAX+1];
				if (!strcmp(queue[queue_index],"/"))
					sprintf(pathname, "%s%s", "/", namelist[i]->d_name);
				else
					sprintf(pathname, "%s/%s", queue[queue_index], namelist[i]->d_name);
				strcpy(queue[queue_num++], pathname);
			}

			//free (namelist)
			for(i = 0; i < count; i++) 
				free(namelist[i]); 
			free(namelist);
		}

		else { //if Que[idx]!=DIR 
			//size check, extension check (add to ... array of same size file linked list)

			if (statbuf.st_size == 0){
				queue_index++;
				continue;
			}

			if (min>=0) {
				if (min > (double)statbuf.st_size){
					queue_index++;
					continue;
				}
			}

			if (max>=0){
				if (max < (double)(statbuf.st_size)){
					queue_index++;
					continue;
				}
			}

			if (has_extension){
				if (!is_extension_matching(extension, queue[queue_index])){
					queue_index++;
					continue;
				}
			}
			
			//size compare
			for (i = 0; i < size_count; i++){
				if (head[i]->size==statbuf.st_size){
					break;
				}
			}
		
			//create Node and add to a linked list of same_size_index
			struct Node* new_node = create_node(queue[queue_index], statbuf.st_size);
			add_node(i, new_node);
		}
		queue_index++;
	} 
}


/* get length of linked list in head[index]*/ 
void get_element_num(int index, int* element_num){

	struct Node* horse = head[index];
	
	while (horse!=NULL){
		horse = horse->next;
		(*element_num)++;
	}

}


//pop all elements except for duplicates in linked lists of array */
void pop_md5_diff(void){

	int i, j, element_num;
	int same_cnt;
	_Bool else_head_exist;
	struct Node* horse, *same_horse, *else_horse;
	struct Node *same_head, *else_head;

	//for each head
	for (i = 0; i<size_count; i++){
		element_num = 0;
		get_element_num(i, &element_num);

		//if (element_num==1) or (element_num==same_cnt+1) break
		while (1 < element_num){

			horse = head[i];
			same_head = copy_node(head[i]);
			same_horse = same_head;
			same_cnt = 0;
			else_head_exist = 0;

			//separate linked_list(head[i]) to same_list & else_list
			for (j = 1; j<element_num; j++){
				if (!strcmp(same_horse->md5, horse->md5)){
					same_horse->next = copy_node(horse);
					same_horse = same_horse->next;
					same_cnt++;
				}
				else{
					if (else_head_exist){
						else_horse->next = copy_node(horse);
						else_horse = else_horse->next;
					}
					else{
						else_head = copy_node(horse);
						else_horse = else_head;
						else_head_exist = 1;
					}
				}
				horse = horse->next;
			}
			same_horse->next = NULL;
			else_horse->next = NULL;

			//check if dup_exist (not, all, part) 
			if (same_cnt==0){
				delete_node(i, 0);
			}
			else if (same_cnt+1 == element_num){
				break;
			}
			else{
				struct Node* tmp = head[i];
				head[i] = else_head;
				free_list(&tmp);
				size_count++;
				head[size_count++] = same_head;
			}

			element_num = 0;
			get_element_num(i, &element_num);
		}
	}

	pop_is_1();	 
}


//pop array[index] which has 'an' element in list
void pop_is_1(void){
	
	int i, element_num;

	for (i = 0; i<size_count ; i++){
	
		element_num = 0;
		get_element_num(i, &element_num);

		if (element_num < 2){
			free_list(&head[i]);
			memmove_head(i);
			i--;
		}
	}

}

//get time (atime/mtime/ctime) as string
void get_string_time(char* path, int a_m_c, char* time){
	
	struct stat statbuf;
	struct tm* t;
		
	if (lstat(path, &statbuf)<0){
		fprintf(stderr, "lstat error for %s\n", path);
		exit(1);
	}

	if (a_m_c==0)
		t = localtime(&statbuf.st_atime);
	else if (a_m_c==1)
		t = localtime(&statbuf.st_mtime);
	else if (a_m_c==2)
		t = localtime(&statbuf.st_ctime);
	else{
		fprintf(stderr, "a_m_c input error\n");
		return;
	}

	if (t==NULL){
		fprintf(stderr, "localtime error for %s\n", path);
		exit(1);
	}

	snprintf(time, TM_MAX, "%02d-%02d-%02d %02d:%02d",t->tm_year-100, t->tm_mon+1, t->tm_mday, t->tm_hour, t->tm_min);
}




//print Info of Identical files (path, size, hash, mtime, atime ...) in array of linked lists
void print_array(void){
	
	int i, j, t;
	struct Node* horse;
	char mtime[TM_MAX], atime[TM_MAX];
	char size_str[SIZ_MAX];

	//sort
	sort_array();

	//for each head
	for (i = 0; i<size_count; i++){

		horse = head[i];
		
		strcpy(size_str,to_comma_byte(horse->size));
	
		printf("\n---- Identical files #%d (%s bytes - %s) ----\n", i+1, size_str, horse->md5);
	
		//print info
		for (j = 0; horse!=NULL; j++){
	
			get_string_time(horse->path, 0, atime);
			get_string_time(horse->path, 1, mtime);
			printf("[%d] %s (mtime: %s) (atime: %s)\n", j+1, horse->path, mtime, atime);
			horse = horse->next;
		}
		printf("\n");
	}
}


/*Function for selection sort (to sort array of head)*/
void sort_array(void){

	int i, j;

	for (i = 0; i<size_count; i++){
		for (j = i; j<size_count; j++){
			if (head[i]->size > head[j]->size){
				swap_head(i, j);
			}
		}
	}
}

//swap head[i] with head[j]
void swap_head(int i, int j){

	struct Node* temp;
	
	temp = head[i];
	head[i] = head[j];
	head[j] = temp;
}


//transform size string to byte
double to_byte(char* size){

	double dbl;
	char* stop;

	dbl = strtod(size, &stop);

	if (*stop == '\0') //byte
		return dbl;

	else if (!strcmp(stop, "KB") || !strcmp(stop, "kb"))
		return (floor(dbl * 1000)/1000) * 1024;

	else if (!strcmp(stop, "MB") || !strcmp(stop, "mb"))
		return (floor(dbl * 1000000)/1000000) * 1024 * 1024;

	else if (!strcmp(stop, "GB") || !strcmp(stop, "gb"))
		return (floor(dbl * 1000000000)/1000000000) * 1024 * 1024 * 1024;

	return -1.0;
}

const char* to_comma_byte(size_t size){

	static char comma_str[64]; 
	char tmp[SIZ_MAX]; 
	int i, len, mod;
	int index = 0;
	
	sprintf(tmp, "%ld", size); 
	len = strlen(tmp); 
	mod = len % 3;

	for(i = 0; i < len; i++) {

		if(i != 0 && ((i % 3) == mod)) { 
			comma_str[index++] = ','; 
		}

		comma_str[index++] = tmp[i]; 
	}

	comma_str[index] = 0x00;
	return comma_str;
}


//check file extension (same)
_Bool is_extension_matching(char* extension, char* d_name){

	int argc1 = 0, argc2 = 0;
	char ext[NAME_MAX], fname[NAME_MAX];
	char* argv1[EXT_PT_MAX], *argv2[EXT_PT_MAX];
	
	strcpy(ext, extension);
	strcpy(fname, d_name);
	argc1 = split(ext, "." , argv1);
	argc2 = split(fname, "." , argv2);

	if (!strcmp(argv1[argc1-1], argv2[argc2-1])){
		return 1;
	}
	else{
		return 0;
	}

}


//get hash(md5, sha1) info from given path
void get_hash(char* path, char* md5, char* sha1){
	
	int n, i;
	FILE* fp;
	unsigned char md5_tmp[MD5_DIGEST_LENGTH] = "";
	unsigned char sha1_tmp[SHA_DIGEST_LENGTH] = "";
	unsigned char data[BUF_MAX] = "";
	MD5_CTX md5_c;
	SHA_CTX sha1_c;

	if ((fp=fopen(path, "rb"))==NULL){
		fprintf(stderr, "fopen error for %s\n", path);
		exit(1);
	}

	MD5_Init(&md5_c);
	SHA1_Init(&sha1_c);

	while ((n=fread(data, 1, BUF_MAX, fp))!=0){
		MD5_Update(&md5_c, data, n);
		SHA1_Update(&sha1_c, data, n);
	}
	
	MD5_Final(md5_tmp, &md5_c);
	SHA1_Final(sha1_tmp, &sha1_c);

	for (i = 0; i<MD5_DIGEST_LENGTH; i++){
		sprintf(md5+i, "%x", md5_tmp[i]);
	}

	for (i = 0; i<SHA_DIGEST_LENGTH; i++){
		sprintf(sha1+i, "%x", sha1_tmp[i]);
	}

	fclose(fp);
}


//create Node and init fileinfo 
struct Node* create_node(char path[PATH_MAX], size_t size){

	char md5[MD5_DIGEST_LENGTH];
	char sha1[SHA_DIGEST_LENGTH];
	get_hash(path, md5, sha1);

	struct Node* new_node = (struct Node*)malloc(sizeof(Node));
	strcpy(new_node->path, path);
	new_node->size = size;
	strcpy(new_node->md5, md5);
	strcpy(new_node->sha1, sha1);
	new_node->next = NULL;

	return new_node;
}


//add "new_node" to "head[index]'s tail -> next"
void add_node(int index, struct Node* new_node){
	
	struct Node* horse;

	if (head[index]==NULL) {
		head[index] = new_node;
		size_count++;
	}
	else {
		horse = head[index];
		while (horse->next != NULL){
			horse = horse->next;
		}
		horse->next = new_node;
	}
}


//delete "a node" of linked list of head[index] and return path
int delete_node(int index, int del_idx){
	int i;
	struct Node* horse = head[index];
	struct Node* temp;
	
	if (del_idx==0){ //head
		temp = head[index];
		head[index] = head[index]->next;
		free(temp);
		temp = NULL;
	} 
	else { //mid||tail
		for (i = 0 ; i<del_idx-1; i++){
			horse = horse->next;
		}
		temp = horse->next;
		if (temp == NULL){ //del_idx out of range(length of linked list)
			fprintf(stderr, "index error in delete_node\n");
			return -1;
		}
		horse->next = horse->next->next;
		free(temp);
		temp = NULL;
	}
	return 1;
}

//copy info of a node
struct Node* copy_node(struct Node* old_node){

	struct Node* new_node = (struct Node*)malloc(sizeof(Node));
	strcpy(new_node->path, old_node->path);
	new_node->size = old_node->size;
	strcpy(new_node->md5, old_node->md5);
	strcpy(new_node->sha1, old_node->sha1);

	return new_node;
}


//free "all Nodes" of linked lists of array
void free_all(void){
	while (head[0]!=NULL){
		free_list(&head[0]);
		memmove_head(0);
	}
}

//free Nodes of "a Linked list of head"
void free_list (struct Node** free_head){
	struct Node* temp;
	
	while ((*free_head)!=NULL){
		temp = (*free_head);
		(*free_head) = (*free_head)->next;
		free(temp);
		temp = NULL;
	}

	size_count--;
}

//pop empty head
void memmove_head(int index){
	if (index != size_count){ 
		if (memmove(&head[index], &head[index+1], sizeof(head))==NULL){
			fprintf(stderr, "memmove error: failed to move head[%d] to head[%d]\n", index, index+1);
			exit(1);
		}
	}
}


//delete option 
void command_delete_dup(void){

	int argc = 0;
	char input[BUF_MAX];
	char* argv[OPT_MAX];
	int which_dup_set;

	//prompt
	while (1){

		printf("\n>> ");
		fgets(input, sizeof(input), stdin);
		input[strlen(input)-1]='\0';
		argc = split(input, " " , argv);

		if (argc == 0)
			continue;

		//exit
		if ((argc == 1) && (!strcmp(argv[0], "exit"))){
			printf("Back to Prompt\n");
			break;
		}

		//dup_set number (array index + 1) 
		which_dup_set = atoi(argv[0]) - 1;
		if (which_dup_set < 0 || which_dup_set > size_count-1){
			fprintf(stderr, "index error\n");
			continue;
		}

		//option
		if (argc==3){
			if (!strcmp(argv[1], "d")){
				option_d(which_dup_set, atoi(argv[2])-1);
			}
			else{
				fprintf(stderr, "argument error\n");
				continue;
			}
		} 
		else if (argc==2){
			if (!strcmp(argv[1], "i")){
				option_i(which_dup_set);

			}
			else if (!strcmp(argv[1], "f"))
				option_f(which_dup_set);
			else if (!strcmp(argv[1], "t"))
				option_t(which_dup_set);
			else{
				fprintf(stderr, "argument error\n");
				continue;
			}
		}
		else{
			fprintf(stderr, "argument error\n");
			continue;
		}

		//head exist check
		if (size_count>0){
			print_array();
		}
		else{
			break;
		}
	}
}


void option_d(int array_idx, int linked_idx){
	int i;
	struct Node* horse = head[array_idx];
	char del_path[PATH_MAX];

	//get deleting node
	if (linked_idx!=0){
		for (i = 0 ; i<linked_idx; i++){
			horse = horse->next;
		}
	}
	strcpy(del_path, horse->path);
	
	//remove a file
	if (remove(del_path)<0){
		fprintf(stderr, "remove error for %s\n", del_path);
		exit(1);
	}
	//delete a Node
	if ((i = delete_node(array_idx, linked_idx))>0){
		printf("\"%s\" has been deleted in #%d\n", del_path, array_idx+1); 
	}

	//check if length of linked list <=1
	pop_is_1();
}

void option_i(int array_idx){
	int i;
	int yn;
	struct Node* horse = head[array_idx];
	struct Node* temp;
	int deleted = 0;

	//for (i = 0; horse->next!=NULL; i++){
	for (i = 0; horse!=NULL; i++){
		printf("Delete \"%s\"? [y/n] ", horse->path);
		yn = fgetc(stdin);
		getchar();
		
		if (yn=='y'||yn=='Y'){
			//remove a file
			if (remove(horse->path)<0){
				fprintf(stderr, "remove error for %s\n", horse->path);
				exit(1);
			}
			//delete a Node
			temp = horse->next;
			delete_node(array_idx, i-deleted);
			deleted++;
			horse = temp;
		}
		else if (yn=='n'||yn=='N'){ 
			horse = horse->next;
			continue;
		}
		else{
			fprintf(stderr, "input error\n");
			break;
		}
		//check if length of linked list <=1
		pop_is_1();
	}
}

void option_f(int array_idx){
	
	int latest_modified = 0;
	char left_path[PATH_MAX];
	time_t left_mtime = 0;
	char mtime[TM_MAX];

	//get latest modified index
	find_latest_modified(array_idx, &latest_modified);
	
	//remove files
	remove_files(array_idx, latest_modified, left_path, &left_mtime, 0);	

	//print info of not removed file
	get_string_time(left_path, 1, mtime);
	printf("Left file in #%d: %s (mtime: %s)\n", array_idx+1, left_path, mtime);
}


void option_t(int array_idx){

	int latest_modified = 0;
	char left_path[PATH_MAX];
	time_t left_mtime = 0;
	char mtime[TM_MAX];

	//get latest modified index
	find_latest_modified(array_idx, &latest_modified);

	//move files to trash can
	remove_files(array_idx, latest_modified, left_path, &left_mtime, 1);

	//print info of not moved file
	get_string_time(left_path, 1, mtime);
	printf("All files in #%d have moved to Trash except \"%s\" (mtime: %s)\n", array_idx+1, left_path, mtime);
}


void find_latest_modified(int array_idx, int* latest_modified){

	int i;
	_Bool change = 0;
	time_t latest_year, latest_mon, latest_day, latest_hour, latest_min;
	struct stat statbuf;
	struct tm* t;
	struct Node* horse = head[array_idx];
	
	//first
	if (lstat(horse->path, &statbuf)<0){
		fprintf(stderr, "lstat error for %s\n",horse->path);
		exit(1);
	}

	if ((t = localtime(&statbuf.st_mtime))==NULL) {
		fprintf(stderr, "localtime error for %s\n", horse->path);
		exit(1);
	}
	latest_year = t->tm_year;
	latest_mon = t->tm_mon;
	latest_day = t->tm_mday;
	latest_hour = t->tm_hour;
	latest_min = t->tm_min;
	horse = horse->next;

	//else
	for (i = 1; horse!=NULL; i++){

		if (lstat(horse->path, &statbuf)<0){
			fprintf(stderr, "lstat error for %s\n",horse->path);
			exit(1);
		}
		
		if ((t = localtime(&statbuf.st_mtime))==NULL) {
			fprintf(stderr, "localtime error for %s\n", horse->path);
			exit(1);
		}

		//compare
		if (latest_year != t->tm_year){
			if (latest_year < t->tm_year){
				change = 1;
			}
		} else if (latest_mon != t->tm_mon){
			if (latest_mon < t->tm_mon){
				change = 1;
			}
		} else if (latest_day != t->tm_mday){
			if (latest_day < t->tm_mday){
				change = 1;
			}
		} else if (latest_hour != t->tm_hour){
			if (latest_hour < t->tm_hour){
				change = 1;
			}

		} else if (latest_min != t->tm_min){
			if (latest_min < t->tm_min){
				change = 1;
			}
		}

		//change latest
		if (change) {
			latest_year = t->tm_year;
			latest_mon = t->tm_mon;
			latest_day = t->tm_mday;
			latest_hour = t->tm_hour;
			latest_min = t->tm_min;
			(*latest_modified) = i;
			change = 0;
		}

		horse = horse->next;
	}
}


void remove_files(int array_idx, int latest_modified, char* left_path, time_t* left_mtime, _Bool to_trash){
	
	int i;
	struct stat statbuf;
	struct Node* horse = head[array_idx];
	char trash_can[PATH_MAX];
	char temp[PATH_MAX];
	char tmp[PATH_MAX+TRASH_SIZE+NAME_MAX+1];
	int argc = 0;
	char* argv[OPT_MAX];

	//remove files or move files to trash can except for latest_modified
	for (i = 0; horse!=NULL; i++){
		if (i!=latest_modified){
			if (to_trash){ //move to trash
				//get dest path 
				strcpy(temp, horse->path);
				argc = split(temp, "/" , argv);
				sprintf(tmp, "%s%s/%s", getenv("HOME"), TRASH, argv[argc-1]);

				//move a file to trash_can
				if (rename(horse->path, tmp)<0){
					fprintf(stderr, "rename error: %s to TRASH\n", horse->path);
					exit(1);
				}
			}
			else{ //remove
				if (remove(horse->path)<0){
					fprintf(stderr, "remove error for %s\n", horse->path);
					exit(1);
				}
			}
		}
		else{
			//latest_modified: save path and mtime
			strcpy(left_path, horse->path);
			if (lstat(left_path, &statbuf)<0){
				fprintf(stderr, "lstat error for %s\n", left_path);
				exit(1);
			}
			*left_mtime = statbuf.st_mtime;
		}
		horse = horse->next;
	}

	//delete Nodes in head[idx]: free Linked list
	free_list(&head[array_idx]);
	memmove_head(array_idx);
}

