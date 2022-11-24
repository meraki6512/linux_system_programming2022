#include <stdio.h> 
#include <string.h> 
#include <stdlib.h> 

#include <unistd.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/time.h>
#include <time.h>

#define PATH_SIZE 1024
#define BUF_SIZE 1024


/*function prototype*/
int find(char* filename, char*path); 
int compare(const void* a, const void* b);
int getDirSize(char* path, long int size);
int printFileInfo(char* real_path, int i);
int printFileList();
void printCompared (char buf_0[][BUF_SIZE], char buf_which[][BUF_SIZE], int line_0, int line_which, int a, int b, int c, int d, int type);
int compareFile(char path_i[PATH_SIZE], char path_j[PATH_SIZE], _Bool options[4]);
int recursiveScandir(char tmp_path_0[PATH_SIZE], char tmp_path_j[PATH_SIZE], _Bool options[4]);
int startDiff(int num);
	

/*global variable*/
char file_list[BUF_SIZE][PATH_SIZE];		 // lists of having same name of given path
int idx; 									// index for file_list
int selected[BUF_SIZE]; 					// index of file_list having same size of given path
char input_fname[PATH_SIZE]; 				//given file name(or path)


/*
   * function: Search for 'filename' in 'path' recursively, and Save same name file/dir as file_list
   * parameter: file name to search for, path where to search in
   * return: just for error handling
 */

int find(char* filename, char* path){

	DIR* dp;
	struct dirent* file;
	struct stat buf;
	char real_path[512];

	if (strcmp(path,"/")==0){
		strcpy(path,"/home");
	}

	if (realpath(path, real_path)==NULL){
		fprintf(stderr, "realpath error for %s\n", path);
		return -1;
	}

	if ((dp=opendir(real_path))==NULL){
		fprintf(stderr, "opendir error for %s\n", real_path);
		return -1;
	}

	//dfs file search
	while ((file=readdir(dp))!=NULL){

		if ((strcmp(file->d_name, ".")==0)||(strcmp(file->d_name,"..")==0)){
			continue;
		}

		char tmp_path[PATH_SIZE];
		snprintf(tmp_path, sizeof(tmp_path), "%s/%s", real_path, file->d_name);

		if (stat(tmp_path, &buf)==-1){
			continue;
		} 

		if (strcmp(file->d_name, filename)==0){
			snprintf(file_list[idx], sizeof(file_list[idx]), "%s", tmp_path);
			idx++;
		}

		if (S_ISDIR(buf.st_mode)){
			find(filename, tmp_path);
		}

	}

	closedir(dp);

	return 0;
}


/* for qsort algorithm*/
int compare(const void* a, const void* b){
	return strcmp((char*)a, (char*)b);
}


/*
   * function: get directoy size by add sub files size recursively
   * parameter: path (which directory), size (to calculate directory size)
   * return: directory size ( = sum of whole sub files size)
 */
int getDirSize(char* path, long int size){

	DIR* dp;
	struct dirent* file;
	struct stat buf;
	char real_path[512];

	if (realpath(path, real_path)==NULL){
		fprintf(stderr, "realpath error for %s\n", path);
		return -1;
	}

	if ((dp=opendir(real_path))==NULL){
		return -1;
	}

	while ((file=readdir(dp))!=NULL){

		if ((strcmp(file->d_name, ".")==0)||(strcmp(file->d_name,"..")==0)){
			continue;
		}

		char tmp_path[PATH_SIZE];
		snprintf(tmp_path, sizeof(tmp_path), "%s/%s", real_path, file->d_name);

		if (stat(tmp_path, &buf)==-1){
			continue;
		} 

		if (S_ISREG(buf.st_mode)){
			size += buf.st_size;
		} 
		else if (S_ISDIR(buf.st_mode)){
			size = getDirSize(tmp_path, size);
		} 
	}

	closedir(dp);

	return size;
}


/*
   * function: print information of a file
   * parameter: real_path (which file), i(idx:selected[num])
   * return: just for error handling
 */
int printFileInfo(char* real_path, int i){

	struct stat buf;	
	if (stat(real_path, &buf)==-1){
		fprintf(stderr, "stat error\n");
		return -1;
	}

	int tm_size = 20;
	char aTime[tm_size], cTime[tm_size], mTime[tm_size];
	struct tm* a, *c, *m;

	long int file_size;

	char mode[12] = "";
	unsigned int p=3, mask = 0700;
	static char *perm[]={"---", "--x", "-w-", "-wx","r--", "r-x", "rw-", "rwx"};


	//time
	if ((a = localtime(&buf.st_atime))==NULL){
		return -1;
	}
	if ((c = localtime(&buf.st_ctime))==NULL){
		return -1;
	}
	if ((m = localtime(&buf.st_mtime))==NULL){
		return -1;
	}
	snprintf(aTime, sizeof(aTime), "%02d-%02d-%02d %02d:%02d",a->tm_year-100, a->tm_mon+1, a->tm_mday, a->tm_hour, a->tm_min);
	snprintf(cTime, sizeof(cTime), "%02d-%02d-%02d %02d:%02d",c->tm_year-100, c->tm_mon+1, c->tm_mday, c->tm_hour, c->tm_min);
	snprintf(mTime, sizeof(mTime), "%02d-%02d-%02d %02d:%02d",m->tm_year-100, m->tm_mon+1, m->tm_mday, m->tm_hour, m->tm_min);

	//size
	if (S_ISDIR(buf.st_mode)){
		file_size = getDirSize(real_path, 0);
		strcat(mode, "d");
	}
	else if (S_ISREG(buf.st_mode)){
		file_size = buf.st_size;
		strcat(mode, "-");
	}

	//mode
	for(p=3; p; --p){
		char tmp[4];
		snprintf(tmp, sizeof(tmp), "%3s", perm[(buf.st_mode & mask) >> (p-1)*3]);
		mask >>= 3;
		strcat(mode, tmp);
	}

	//print
	printf("%-6d %-5ld %-11s %-7ld %-6ld %-5d %-5d %-15s %-15s %-15s %s\n", i, file_size, mode, buf.st_blocks, buf.st_nlink, buf.st_uid, buf.st_gid, aTime, cTime, mTime, real_path);

	return 0;
}



/*
   * function: sort file_list, and print file_list
   * return: just for error handling
 */
int printFileList(){

	qsort(file_list, idx, sizeof(file_list[0]), compare);

	//change idx_0 to input(fname/path)	
	char input_real_path[PATH_SIZE];
	if (realpath(input_fname, input_real_path)==NULL){
		fprintf(stderr, "realpath error for %s\n", input_fname);
		return -1;
	}

	int i;
	for (i=0; i<idx; i++){
		if (strcmp(input_real_path, file_list[i])==0)
			break;
	}

	char tmpForSwapIdx[PATH_SIZE];
	strcpy(tmpForSwapIdx, file_list[0]);
	strcpy(file_list[0], file_list[i]);
	strcpy(file_list[i], tmpForSwapIdx);

	printf("Index  Size  Mode        Blocks  Links  UID   GID   Access          Change          Modify          Path\n");

	//is same size
	int num = 0;
	long int file_size = 0;

	for (i=0; i<idx; i++){	

		struct stat buf;	
		if (stat(file_list[i], &buf)==-1){
			return -1;
		}

		long int tmp_size;
		if (S_ISDIR(buf.st_mode)){
			tmp_size = getDirSize(file_list[i], 0);
		}
		else if (S_ISREG(buf.st_mode)){
			tmp_size = buf.st_size;
		}

		if (i==0){
			file_size = tmp_size;
		}

		if (file_size==tmp_size){
			printFileInfo(file_list[i], num);
			selected[num] = i;
			num++;
		}
	}

	if (idx==0){
		printFileInfo(input_real_path, 0);
	}

	if (num<=1){
		printf("(None)\n");
		return -1;
	}

	return num;
}


/*
   * function: print added/deleted/changed lines 
   * parameter: buf_0 (contents of file_list[index_0]), buf_which (contents of file_list[index[selected[num]]]),
   				line_0 (line count of file_list[index_0]), line_which (line count of ...),
				a, b, c, d (index of start/end to print)
				type (0:add, 1:delete, 2:change)
 */
void printCompared (char buf_0[][BUF_SIZE], char buf_which[][BUF_SIZE], int line_0, int line_which, int a, int b, int c, int d, int type){

	int i;

	switch (type){

		//add
		case 1:
			if (b==c){
				printf("%da%d\n", a, b);
			}
			else{
				printf("%da%d,%d\n", a, b, c);
			}
			for (i=b-1; i<c; i++){
				printf("> %s", buf_which[i]);
			}
			break;

			//delete
		case 2:
			if (a==b){
				printf("%dd%d", b, c);
			}
			else{
				printf("%d,%dd%d\n", a, b, c);
			}
			for (i=a-1; i<b; i++){
				printf("< %s", buf_0[i]);
			}
			break;

			//change
		case 3:
			if (a==b && c==d){
				printf("%dc%d\n", a, c);
			}
			else if (a==b){
				printf("%dc%d,%d\n", a, c, d);
			}
			else if (c==d){
				printf("%d,%dc%d\n", a, b, c);
			}
			else{
				printf("%d,%dc%d,%d\n", a, b, c, d);
			}

			for (i=a-1; i<b; i++){
				printf("< %s", buf_0[i]);
				if (i==line_0-1){
					printf("\\ No newline at end of file.\n");
				}
			}
			printf("---\n");
			for (i=c-1; i<d; i++){
				printf("> %s", buf_which[i]);
				if (i==line_which-1){
					printf("\\ No newline at end of file.\n");
				}
			}
			break;
	}
}


/*
   * function: compare contents of each file/dir
   * parameter: path_i, path_j(which file/dir to compare),
   				options[0],[1],[2],[3]: option 'q', 's', 'i', 'r' (checked=1, not=0)
   * return: just for error handling
 */
int compareFile(char path_i[PATH_SIZE], char path_j[PATH_SIZE], _Bool options[4]){

	//open each file

	FILE* fp_0;
	FILE* fp_which;
	char* f_mode = "r";

	if ((fp_0 = fopen(path_i, f_mode))==NULL){
		fprintf(stderr, "fopen error for %s\n", path_i);
		return -1;
	}

	if ((fp_which = fopen(path_j, f_mode))==NULL){
		fprintf(stderr, "fopen error for %s\n", path_j);
		return -1;
	}

	//i: compare files regardless of case -> tolower/toupper, strcasecmp

	//q: print just it is diff, not a content
	//s: print just it is same, not a content

	if (options[0]==1 || options[1]==1){

		char c_0 = 0;
		char c_which = 0;

		while(!(c_0==EOF&&c_which==EOF)){
			c_0 = fgetc(fp_0);
			c_which = fgetc(fp_which);

			if (c_0!=c_which){
				if ((options[2]==1)&&((toupper(c_0)==c_which)||(tolower(c_0)==c_which))){ 
					continue;
				}
				else if (options[0]==1){
					printf("Files [%s] and [%s] are differ\n", path_i, path_j);
				}
				break;
			}
		}

		if ((options[1]==1)&&(c_0==EOF&&c_which==EOF)){
			printf("Files [%s] and [%s] are same\n", path_i, path_j);
		}

		fclose(fp_0);
		fclose(fp_which);

		return 0;
	}

	//read each file
	//no options: compare each file by contents

	char buf_0[BUF_SIZE][BUF_SIZE] = {"", }; 	
	char buf_which[BUF_SIZE][BUF_SIZE] = {"", };
	int line_0=0;
	int line_which=0;

	while (fgets(buf_0[line_0], BUF_SIZE, fp_0)!=NULL) 
		line_0++;
	while (fgets(buf_which[line_which], BUF_SIZE, fp_which)!=NULL)
		line_which++;

	fclose(fp_0);
	fclose(fp_which);

	//compare each file
	int i=0, j=0;
	_Bool change_flag = 0;
	int change_i = 0, change_j = 0;

	while (1){
		if (i<line_0 && j<line_which){
			if ((strcmp(buf_0[i], buf_which[j])==0) || ((options[2]==1) && (strcasecmp(buf_0[i], buf_which[j])==0))){
				if (change_flag){
					//change		
					printCompared(buf_0, buf_which, line_0, line_which, change_i+1, i, change_j+1, j, 3);
					change_flag = 0;
				}
				i++;
				j++;
				continue;
			}
			else{
				int tmp_j = j;
				while ((++tmp_j) < line_which){
					if ((strcmp(buf_0[i], buf_which[tmp_j])==0) || ((options[2]==1)&&(strcasecmp(buf_0[i], buf_which[tmp_j])==0))){
						if ((tmp_j+1==line_which) || (strcmp(buf_0[i+1], buf_which[tmp_j+1])==0) || ((options[2]==1)&&(strcasecmp(buf_0[i+1], buf_which[tmp_j+1])==0))){
							if (change_flag){
								//change
								printCompared(buf_0, buf_which, line_0, line_which, change_i, i, change_j, tmp_j, 3);
								change_flag = 0;
								break;
							}
							else{
								//add	
								printCompared(buf_0, buf_which, line_0, line_which, i, j+1, tmp_j, 0, 1);
								break;
							}
						}
					}
				}

				if (tmp_j != line_which){
					i++;
					j = tmp_j+1;
				}
				else{

					int tmp_i = i;
					while ((++tmp_i) < line_0){
						if ((strcmp(buf_0[tmp_i], buf_which[j])==0) || ((options[2]==1)&&(strcasecmp(buf_0[tmp_i], buf_which[j])==0))){
							if ((tmp_i+1==line_0) || (strcmp(buf_0[tmp_i+1], buf_which[j+1])==0) || ((options[2]==1)&&(strcasecmp(buf_0[tmp_i+1], buf_which[j+1])==0))){
								if (change_flag){
									//change
									printCompared(buf_0, buf_which, line_0, line_which, change_i+1, tmp_i, change_j+1, j, 3);
									change_flag = 0;
									break;
								}
								else{
									//delete
									printCompared(buf_0, buf_which, line_0, line_which, i+1, tmp_i+1, j, 0, 2);
									break;
								}
							}
						}
					}

					if (tmp_i != line_0){
						i = tmp_i+1;
						j++;
					}
					else{
						//change
						if (!change_flag){
							change_i = i;
							change_j = j;
						}
						change_flag = 1;
						i++;
						j++;
					}
				}
			}
		}
		else if (i<line_0){ // j == line_which
			if (change_flag){
				printCompared(buf_0, buf_which, line_0, line_which, change_i+1, line_0, change_j+1, line_which, 3);
			}
			else{
				if ((strcmp(buf_0[i-1], buf_which[j-1])==0) || ((options[2]==1) && (strcasecmp(buf_0[i-1], buf_which[j-1])==0))){
					//delete
					printCompared(buf_0, buf_which, line_0, line_which, i, line_0, line_which, 0, 2);
				}
				else {
					printCompared(buf_0, buf_which, line_0, line_which, i, line_0, line_which, line_which, 3);
				}
			}
			break;
		}
		else if (j<line_which){ // i == line_0
			if (change_flag){
				printCompared(buf_0, buf_which, line_0, line_which, change_i+1, line_0, change_j+1, line_which, 3);
			}
			else{
				if ((strcmp(buf_0[i-1], buf_which[j-1])==0) || ((options[2]==1) && (strcasecmp(buf_0[i-1], buf_which[j-1])==0))){
					//add
					printCompared(buf_0, buf_which, line_0, line_which, i, j+1, line_which, 0, 1);
				}
				else{
					printCompared(buf_0, buf_which, line_0, line_which, line_0, line_0, j, line_which, 3);
				}
			}
			break;
		}
		else{ //EOF
			if (change_flag){
				printCompared(buf_0, buf_which, line_0, line_which, change_i+1, line_0, change_j+1, line_which, 3);
			}
			break;
		}
	}
	return 0;
}


/*
   * function: diff both directory recursively
   * parameter: tmp_path_0, tmp_path_j (path of sub dir which is dir)
   * return: just for error handling
 */
int recursiveScandir(char tmp_path_0[PATH_SIZE], char tmp_path_j[PATH_SIZE], _Bool options[4]){

	struct dirent ** list_0;
	struct dirent ** list_j;
	int cnt_0, cnt_j;
	int t=0, k=0;
	
	if ((cnt_0 = scandir(tmp_path_0, &list_0, NULL, alphasort))==-1){
		fprintf(stderr, "scandir error for %s\n", tmp_path_0);
		return -1;
	}

	if ((cnt_j = scandir(tmp_path_j, &list_j, NULL, alphasort))==-1){
		fprintf(stderr, "scandir error for %s\n", tmp_path_j);
		return -1;	
	}

	while (!(t>cnt_0 || k>cnt_j)){
		if ((strcmp(list_0[t]->d_name, ".")==0)||(strcmp(list_0[t]->d_name, "..")==0)){
			t++;
			continue;
		}
		if ((strcmp(list_j[k]->d_name, ".")==0)||(strcmp(list_j[k]->d_name, "..")==0)){
			k++;
			continue;
		}
		
		//compare name
		int tmp = strcmp(list_0[t]->d_name, list_j[k]->d_name);

		char path_0[PATH_SIZE];
		char path_j[PATH_SIZE];

		snprintf(path_0, sizeof(path_0), "%s/%s", tmp_path_0, list_0[t]->d_name);
		snprintf(path_j, sizeof(path_j), "%s/%s", tmp_path_j, list_j[k]->d_name);

		if ((t<=cnt_0)&&(k<=cnt_j)){
			if (tmp>0){
				printf("Only in %s: %s\n", tmp_path_j, list_j[k]->d_name);
				k++;
			}
			else if (tmp<0){
				printf("Only in %s: %s\n", tmp_path_0, list_0[t]->d_name);
				t++;
			}
			else if (tmp==0){

				struct stat tmp_buf_0;
				struct stat tmp_buf_j;

				if (stat(path_0, &tmp_buf_0)==-1){
					fprintf(stderr, "stat error for %s\n", path_0);
					return -1;
				}
				if (stat(path_j, &tmp_buf_j)==-1){
					fprintf(stderr, "stat error for %s\n", path_j);
					return -1;
				}

				if (S_ISDIR(tmp_buf_0.st_mode)&&(S_ISDIR(tmp_buf_j.st_mode))){
					if (options[3]==1){
						recursiveScandir(path_0, path_j, options);
					}
					else{
						printf("Common subdirectories: %s and %s\n", path_0, path_j);
					}
				}
				else if (S_ISREG(tmp_buf_0.st_mode)&&(S_ISREG(tmp_buf_j.st_mode))){
					printf("diff %s %s\n", path_0, path_j);
					compareFile(path_0, path_j, options);
				}
				else if (S_ISDIR(tmp_buf_0.st_mode)&&(S_ISREG(tmp_buf_j.st_mode))){
					printf("File [%s] is a [directory], while file [%s] is a [regular file]\n", list_0[t]->d_name, list_j[k]->d_name);
				}
				else if (S_ISREG(tmp_buf_0.st_mode)&&(S_ISDIR(tmp_buf_j.st_mode))){
					printf("File [%s] is a [regular file], while file [%s] is a [directory]\n", list_0[t]->d_name, list_j[k]->d_name);
				}

				if ((t==cnt_0)&&(k==cnt_j)){
					break;
				}
				else if (t==cnt_0){
					k++;
				}
				else if (k==cnt_j){
					t++;
				}
				else{
					t++;
					k++;
				}
			}

		}
	}
	
	//free lists
	for (int i=0; i<cnt_0; i++){
		free(list_0[i]);
	}
	for (int i=0; i<cnt_j; i++){
		free(list_j[i]);
	}

	free(list_0);
	free(list_j);

	return 0;

}



/*
   * function: get Idx to diff and options, and compare file
   * parameter: num (number of file that has same name and size)
   * return: just for error handling
 */
int startDiff(int num){

	int whichIdx;
	_Bool options[4] = {0, };

	//get Idx and Options
	while(1){

		char which[15];
		char tmp[15];
		char* ptr;
		_Bool exitOuterLoop = 0;
		whichIdx = -1; 

		printf(">> ");
		scanf("%[^\n]", which);
		strcpy(tmp, which);

		if ((ptr = strtok(which, " "))!=NULL){

			whichIdx = atoi(ptr);

			char* opt;
			while ((opt = strtok(NULL, " "))!=NULL){

				if (strcmp(opt, "q")==0) {
					options[0] = 1;
				} else if (strcmp(opt, "s")==0) {
					options[1] = 1;
				} else if (strcmp(opt, "i")==0) {
					options[2] = 1;
				} else if (strcmp(opt, "r")==0) {
					options[3] = 1;
				} else {
					fprintf(stderr, "error for getting option\n");
					exitOuterLoop = 1;
					break;
				}
			}

			if (exitOuterLoop){
				getchar();
				continue;
			}
		} 

		if ((whichIdx>=num)||(whichIdx<=0)){
			fprintf(stderr, "error for getting index: Choose idx in (0, %d]\n", num-1);
			getchar();
			continue;
		} 
		else
			break;
	}

	getchar();


	//compare

	struct stat buf_0;
	int j = selected[whichIdx];
	struct stat buf_j;

	if (stat(file_list[0], &buf_0)==-1){
		fprintf(stderr, "stat error for %s\n", file_list[0]);
		return -1;
	}

	if (stat(file_list[j], &buf_j)==-1){
		fprintf(stderr, "stat error for %s\n", file_list[j]);
		return -1;
	}

	if (S_ISDIR(buf_0.st_mode)){
		if (options[3]==1){
			recursiveScandir(file_list[0], file_list[j], options);
		} 
		else {
			//scandir
			struct dirent ** list_0;
			struct dirent ** list_j;
			int cnt_0, cnt_j;
			int t=0, k=0;

			if ((cnt_0 = scandir(file_list[0], &list_0, NULL, alphasort))==-1){
				fprintf(stderr, "scandir error for %s\n", file_list[0]);
				return -1;
			}

			if ((cnt_j = scandir(file_list[j], &list_j, NULL, alphasort))==-1){
				fprintf(stderr, "scandir error for %s\n", file_list[j]);
				return -1;	
			}
		
			//compare
			while (t<=cnt_0&&k<=cnt_j){
				
				if ((strcmp(list_0[t]->d_name, ".")==0)||(strcmp(list_0[t]->d_name, "..")==0)){
					t++;
					continue;
				}
				if ((strcmp(list_j[k]->d_name, ".")==0)||(strcmp(list_j[k]->d_name, "..")==0)){
					k++;
					continue;
				}

				if (t<cnt_0 && k==cnt_j){ 
					printf("Only in %s: %s\n", file_list[0], list_0[t]->d_name);
					t++;
				}
				else if (k<cnt_j && t==cnt_0){
					printf("Only in %s: %s\n", file_list[j], list_j[k]->d_name);
					k++;
				}
				else if (t<cnt_0 || k<cnt_j){

					int tmp = strcmp(list_0[t]->d_name, list_j[k]->d_name);

					char path_0[PATH_SIZE*2];
					char path_j[PATH_SIZE*2];
					snprintf(path_0, sizeof(path_0), "%s/%s", file_list[0], list_0[t]->d_name);
					snprintf(path_j, sizeof(path_j), "%s/%s", file_list[j], list_j[k]->d_name);

					if (tmp>0){
						printf("Only in %s: %s\n", file_list[j], list_j[k]->d_name);
						k++;
					}
					else if (tmp<0){
						printf("Only in %s: %s\n", file_list[0], list_0[t]->d_name);
						t++;
					}
					else if (tmp==0){

						struct stat tmp_buf_0;
						struct stat tmp_buf_j;

						if (stat(path_0, &tmp_buf_0)==-1){
							fprintf(stderr, "stat error for %s\n", path_0);
							return -1;
						}
						if (stat(path_j, &tmp_buf_j)==-1){
							fprintf(stderr, "stat error for %s\n", path_j);
							return -1;
						}

						if (S_ISDIR(tmp_buf_0.st_mode)&&(S_ISDIR(tmp_buf_j.st_mode))){
							printf("Common subdirectories: %s and %s\n", path_0, path_j);
						}
						else if (S_ISREG(tmp_buf_0.st_mode)&&(S_ISREG(tmp_buf_j.st_mode))){
							printf("diff %s %s\n", path_0, path_j);
							compareFile(path_0, path_j, options);
						}
						else if (S_ISDIR(tmp_buf_0.st_mode)&&(S_ISREG(tmp_buf_j.st_mode))){
							printf("File [%s] is a [directory], while file [%s] is a [regular file]\n", list_0[t]->d_name, list_j[k]->d_name);
						}
						else if (S_ISREG(tmp_buf_0.st_mode)&&(S_ISDIR(tmp_buf_j.st_mode))){
							printf("File [%s] is a [regular file], while file [%s] is a [directory]\n", list_0[t]->d_name, list_j[k]->d_name);
						}
						t++;
						k++;
					}
				}
			}

			//free lists
			for (int i=0; i<cnt_0; i++){
				free(list_0[i]);
			}
			for (int i=0; i<cnt_j; i++){
				free(list_j[i]);
			}

			free(list_0);
			free(list_j);

		}
	}
	else if (S_ISREG(buf_0.st_mode)){
		if (options[3]==1){
			fprintf(stderr, "option 'r' can't applied to regular file\n");
			return -1;
		}

		if (S_ISDIR(buf_j.st_mode)){
			printf("File [%s] is a [regular file], while file [%s] is a [directory]\n", file_list[0], file_list[j]);
		}
		else{
			compareFile(file_list[0], file_list[selected[whichIdx]], options);	
		}
	}

	return 0;
}




int main(void) {

	char* ptr;
	char* input;

	struct timeval startTime, endTime;
	gettimeofday(&startTime, NULL);

	//prompt
	while(1){

		//input
		printf("20201482> ");
		if (fgets(input, 1024, stdin)!=NULL){
			input[strlen(input)-1]='\0';
		} else {
			exit(1);
		}

		//when u pressed enter key only
		if (input[0]=='\0'){
			continue;
		}

		//when u pressed space key only
		if ((ptr = strtok(input, " "))==NULL){
			printf("Usage:\n  > find [FILENAME] [PATH]\n    >> [idx] [OPTION ... ]\n  > help\n  > exit\n\n  [OPTION ... ]\n   q : report only when files differ\n   s : report when two files are the same\n   i : ignore case differences in file contents\n   r : recursively compare any subdirectories found\n");

			continue;
		}

		//basic functions: 1) find 2) exit 3) help

		if (!strcmp(ptr, "find")){

			char* filename;
			char* path;	

			filename = strtok(NULL, " ");
			if (filename != NULL){
				path = strtok(NULL, " ");
			}

			if (!(filename==NULL||path==NULL)){

				strcpy(input_fname, filename);

				if (strstr(filename,"/")!=NULL) {
					char tmp[PATH_SIZE]; 
					char* p = strtok(filename,"/");
					while (p!=NULL) {
						strcpy(tmp, p);
						p = strtok(NULL,"/");
					}
					strcpy(filename, tmp);
				}

				idx = 0;
				find(filename, path);
				int p = printFileList();
				if (p!=-1){
					startDiff(p);
				}
			}
			else {
				printf("not exist. check filename/path.\n");
			}

		} 
		else if (!strcmp(ptr, "exit")){

			printf("Prompt End\n");
			gettimeofday(&endTime, NULL);
			printf("Runtime: %ld:%llu\n", 
					endTime.tv_sec-startTime.tv_sec, 
					(unsigned long long)endTime.tv_usec-startTime.tv_usec);
			exit(1);

		} 
		else {

			printf("Usage:\n  > find [FILENAME] [PATH]\n    >> [idx] [OPTION ... ]\n  > help\n  > exit\n\n  [OPTION ... ]\n   q : report only when files differ\n   s : report when two files are the same\n   i : ignore case differences in file contents\n   r : recursively compare any subdirectories found\n");

		}

	}

	return 0;
}
