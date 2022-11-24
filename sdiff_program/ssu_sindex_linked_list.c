#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>

#define ARGMAX 3
#define BUFMAX 1024
#define PATHMAX BUFMAX*2
#define REGFILE 1
#define DIRECTORY 2

typedef struct fileinfo {
	char path[PATHMAX];
	size_t size;
	int type;
	int depth;
} fileinfo;

fileinfo origin;
int list_index = 1;
int is_dir_compare;
int r_option, i_option, q_option, s_option;
int block_num;
// o_: origin file, n_: new file
int o_line, n_line;
int o_common[BUFMAX], n_common[BUFMAX];
int o_block[BUFMAX][2], n_block[BUFMAX][2];
char o_content[BUFMAX][BUFMAX], n_content[BUFMAX][BUFMAX];

void command_help(void);
void command_find(int argc, char* argv[]);
void compare_directory(char* fname1, char* fname2);
void find_diff_block(void);
void read_directory(int depth, char* dirname, fileinfo* filelist);
void print_diff(void);
int find_diff(void);
int compare_depth(const void* a, const void* b);
int compare_file(char* fname1, char* fname2);
int split(char* string, char* seperator, char* argv[]);
char get_type(struct stat statbuf);
char* extract_name(char* string);
char* get_option(void);
char* get_time(time_t stime);
char* get_relpath(char* abspath);
char* get_permission(struct stat statbuf);
off_t get_directory_size(char* dirname);

int main(void)
{
	struct timeval begin_t, end_t;
	gettimeofday(&begin_t, NULL);
	
	int argc = 0;
	char input[BUFMAX];
	char* argv[ARGMAX];

	// 프롬프트 시작
	while (1) {
		printf("20221234> ");
		fgets(input, sizeof(input), stdin);
		input[strlen(input) - 1] = '\0';
		argc = split(input, " ", argv);

		if (argc == 0)
			continue;
		
		if (!strcmp(argv[0], "find"))
			command_find(argc, argv);
		
		else if (!strcmp(argv[0], "help")) 
			command_help();	

		else if (!strcmp(argv[0], "exit")) {
			printf("Prompt End\n");
			break;
		}
		else 
			command_help();
	}

	gettimeofday(&end_t, NULL);

	// 프로그램 실행 시간 측정
	end_t.tv_sec -= begin_t.tv_sec;
	if (end_t.tv_sec < begin_t.tv_usec) {
		end_t.tv_sec--;
		end_t.tv_usec += 1000000;
	}
	end_t.tv_usec -= begin_t.tv_usec;
	printf("Runtime: %ld:%06ld(sec:usec)\n", end_t.tv_sec, end_t.tv_usec);

	exit(0);
}

// find 명령어 실행 함수
void command_find(int argc, char* argv[])
{
	char filename[PATHMAX];
	char dirname[PATHMAX];
	struct stat statbuf;
	fileinfo filelist[BUFMAX];

	// 입력 관련 에러 처리
	if (argc != ARGMAX) {
		printf("ERROR: Arguments error\n");
		return;
	}
	
	if (realpath(argv[1], filename) == NULL || realpath(argv[2], dirname) == NULL) {
		printf("ERROR: Path exist error\n");
		return;
	}

	if (lstat(dirname, &statbuf) < 0) {
		fprintf(stderr, "lstat error for %s\n", dirname);
		exit(1);
	}

	if (!S_ISDIR(statbuf.st_mode)) {
		printf("ERROR: Path must be directory\n");
		return;
	}

	if (lstat(filename, &statbuf) < 0) {
		fprintf(stderr, "lstat error for %s\n", filename);
		exit(1);
	}

	strcpy(origin.path, filename);
	origin.type = S_ISDIR(statbuf.st_mode) ? DIRECTORY : REGFILE;
	origin.size = (origin.type == DIRECTORY) ? get_directory_size(filename) : statbuf.st_size;
	origin.depth = -1; 
	filelist[0] = origin;

	read_directory(0, dirname, filelist);

	printf("%-5s %-4s %-10s %-6s %-5s %-4s %-4s %-15s %-15s %-15s %s\n",
		"Index", "Size", "Mode", "Blocks", "Links", "UID", "GID", "Access", "Change", "Modify", "Path");

	qsort(filelist, list_index, sizeof(filelist[0]), compare_depth);

	for (int idx = 0; idx < list_index; idx++) {
		struct stat statbuf;

		if (lstat(filelist[idx].path, &statbuf) < 0) {
			fprintf(stderr, "lstat error for %s\n", filelist[idx].path);
			exit(1);
		}

		printf("%-5d %-4ld %c%-9s %-6ld %-5ld %-4d %-4d %-15s %-15s %-15s %s\n",
			idx, filelist[idx].size, get_type(statbuf), get_permission(statbuf), statbuf.st_blocks, 
			statbuf.st_nlink, statbuf.st_uid, statbuf.st_gid, get_time(statbuf.st_atime), 
			get_time(statbuf.st_ctime), get_time(statbuf.st_mtime), filelist[idx].path);
	}

	if (list_index == 1) {
		printf("(None)\n");
		return;
	}

	while (1) {
		int idx;
		int count;
		char input[BUFMAX];
		char* arr[10];

		printf(">> ");
		fgets(input, sizeof(input), stdin);
		count = split(input, " ", arr);

		if (atoi(arr[0]) < 0 || atoi(arr[0]) > list_index) {
			printf("ERROR: out of index\n");
			continue;
		}

		idx = atoi(arr[0]);
		for (int i = 1; i < count; i++) {
			if (arr[i][0] == 'r')
				r_option = 1;
			else if (arr[i][0] == 'q')
				q_option = 1;
			else if (arr[i][0] == 's')
				s_option = 1;
			else if (arr[i][0] == 'i')
				i_option = 1;
		}
		
		if (origin.type == DIRECTORY)
			compare_directory(origin.path, filelist[idx].path);
		else
			compare_file(origin.path, filelist[idx].path);
		
		break;
	}
	list_index = 1;
	r_option = q_option = s_option = i_option = 0;

	return;
}

// 이름과 크기가 동일한 파일을 찾기 위한 디렉토리 탐색
void read_directory(int depth, char* dirname, fileinfo* filelist)
{
	int count;
	char pathname[PATHMAX];
	struct stat statbuf;
	struct dirent** namelist;

	if (access(dirname, F_OK) != 0) {
		// for Linux Permission denied
		if (errno == 13)
			return; 	
		fprintf(stderr, "access error for %s\n", dirname);
		exit(1);
	}

	if ((count = scandir(dirname, &namelist, NULL, alphasort)) == -1) {
		if (errno == 1)
			return;
		fprintf(stderr, "scandir error for %s\n", dirname);
		exit(1);
	}

	for (int i = 0; i < count; i++) {
		if (!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, ".."))
			continue;

		if (!strcmp(dirname, "/"))
			sprintf(pathname, "%s%s", dirname, namelist[i]->d_name);
		else
			sprintf(pathname, "%s/%s", dirname, namelist[i]->d_name);

		// 원본 파일이면 스킵
		if (!strcmp(pathname, origin.path))
			continue;

		if ((lstat(pathname, &statbuf) < 0) && (!access(pathname, F_OK))) {
			// for macOS Operation not permitted
			if (errno == 1)
				continue;
			fprintf(stderr, "lstat error for %s\n", pathname);
			exit(1);
		}

		if (S_ISDIR(statbuf.st_mode)) {
			if (origin.type == DIRECTORY) {
				if (!strcmp(namelist[i]->d_name, extract_name(origin.path)) && get_directory_size(pathname) == origin.size) {
					strcpy(filelist[list_index].path, pathname);
					filelist[list_index].type = DIRECTORY;
					filelist[list_index].size = get_directory_size(pathname);
					filelist[list_index].depth = depth;
					list_index++;
				}
			}
			read_directory(depth + 1, pathname, filelist);
		}
		else if (S_ISREG(statbuf.st_mode)) {
			if (origin.type == REGFILE) {
				if (!strcmp(namelist[i]->d_name, extract_name(origin.path)) && statbuf.st_size == origin.size) {
					strcpy(filelist[list_index].path, pathname);
					filelist[list_index].type = REGFILE;
					filelist[list_index].size = statbuf.st_size;
					filelist[list_index].depth = depth;
					list_index++;
				}
			}
		}
	}
}

// 파일 비교 실행
int compare_file(char* fname1, char* fname2)
{
	FILE* fp[2];
	int is_diff = 0;
	char temp[BUFMAX] = {0, };
	char o_buf[BUFMAX] = {0, };
	char n_buf[BUFMAX] = {0, };
	char* o_path;
	char* n_path;

	fp[0] = fopen(fname1, "r");
	fp[1] = fopen(fname2, "r");

	o_line = n_line = 0;
	for (int i = 0; i < BUFMAX; i++) {
		memset(o_content[0], 0x00, sizeof(o_content[0]));
		memset(n_content[0], 0x00, sizeof(n_content[0]));
	}

	while (fgets(o_buf, sizeof(o_buf), fp[0]) != NULL)
		strcpy(o_content[++o_line], o_buf);
	
	while (fgets(n_buf, sizeof(n_buf), fp[1]) != NULL)
		strcpy(n_content[++n_line], n_buf);

	if (o_content[o_line][strlen(o_content[o_line])-1] != '\n')
		sprintf(o_content[o_line], "%s\n%s", o_content[o_line], "\\ No newline at end of file\n");

	if (n_content[n_line][strlen(n_content[n_line])-1] != '\n')
		sprintf(n_content[n_line], "%s\n%s", n_content[n_line], "\\ No newline at end of file\n");
	
	o_path = get_relpath(fname1);
	n_path = get_relpath(fname2);

	if (find_diff())
		is_diff = 1;

	// 파일 다른 경우
	if (is_diff) {
		if (q_option)
			printf("Files %s and %s differ\n", o_path, n_path);
		else {
			if (is_dir_compare) {
				char* option = get_option();
				if (!strcmp(option, "-"))
					printf("diff %s %s\n", o_path, n_path);
				else
					printf("diff %s %s %s\n", option, o_path, n_path);
			}
			print_diff();
		}
	}
	// 파일 같은 경우
	else {
		if (s_option)
			printf("Files %s and %s are identical\n", o_path, n_path);
	}

	fclose(fp[0]);
	fclose(fp[1]);

	return is_diff;
}

// 디렉토리 비교 실행
void compare_directory(char* fname1, char* fname2)
{
	int count[2];
	struct dirent** namelist[2];

	if ((count[0] = scandir(fname1, &namelist[0], NULL, alphasort)) == -1) {
		fprintf(stderr, "scandir error for %s\n", fname1);
		exit(1);
	}

	if ((count[1] = scandir(fname2, &namelist[1], NULL, alphasort)) == -1) {
		fprintf(stderr, "scandir error for %s\n", fname2);
		exit(1);
	}

	int i, j, k;
	char* relpath[2];
	char pathname[2][PATHMAX];
	struct stat statbuf[2];

	relpath[0] = get_relpath(fname1);
	relpath[1] = get_relpath(fname2);

	i = j = k = 0;

	while (i < count[0] && j < count[1]) {
		if (!strcmp(namelist[0][i]->d_name, ".") || !strcmp(namelist[0][i]->d_name, "..")) {
			i++;
			continue;
		}
		else if (!strcmp(namelist[1][j]->d_name, ".") || !strcmp(namelist[1][j]->d_name, "..")) {
			j++;
			continue;
		}

		sprintf(pathname[0], "%s/%s", fname1, namelist[0][i]->d_name);
		if (lstat(pathname[0], &statbuf[0]) < 0) {
			fprintf(stderr, "lstat error for %s\n", pathname[0]);
			exit(1);
		}

		sprintf(pathname[1], "%s/%s", fname2, namelist[1][j]->d_name);
		if (lstat(pathname[1], &statbuf[1]) < 0) {
			fprintf(stderr, "lstat error for %s\n", pathname[1]);
			exit(1);
		}

		if (strcmp(namelist[0][i]->d_name, namelist[1][j]->d_name) == 0) {
			if (S_ISDIR(statbuf[0].st_mode) && S_ISDIR(statbuf[1].st_mode)) {
				if (r_option) {
					compare_directory(pathname[0], pathname[1]);
				}
				else {
					printf("Common subdirectories: %s/%s and %s/%s\n",
						relpath[0], namelist[0][i]->d_name, relpath[1], namelist[1][j]->d_name);
				}
			}
			else if (S_ISDIR(statbuf[0].st_mode) && S_ISREG(statbuf[1].st_mode)) {
				printf("File %s/%s is a directory while file %s/%s is a regular file\n",
					relpath[0], namelist[0][i]->d_name, relpath[1], namelist[1][j]->d_name);
			}
			else if (S_ISREG(statbuf[0].st_mode) && S_ISDIR(statbuf[1].st_mode)) {
				printf("File %s/%s is a regular file while file %s/%s is a directory\n",
					relpath[0], namelist[0][i]->d_name, relpath[1], namelist[1][j]->d_name);
			}
			else {
				is_dir_compare = 1;
				compare_file(pathname[0], pathname[1]);
				is_dir_compare = 0;
			}
			i++;
			j++;
		} 
		else if (strcmp(namelist[0][i]->d_name, namelist[1][j]->d_name) < 0) {
			printf("Only in %s: %s\n", relpath[0], namelist[0][i++]->d_name);
		}
		
		else if (strcmp(namelist[0][i]->d_name, namelist[1][j]->d_name) > 0) {
			printf("Only in %s: %s\n", relpath[1], namelist[1][j++]->d_name);
		}
	}

	while (i < count[0]) {
		if (!strcmp(namelist[0][i]->d_name, ".") || !strcmp(namelist[0][i]->d_name, "..")) {
			i++;
			continue;
		}
		printf("Only in %s: %s\n", relpath[0], namelist[0][i++]->d_name);
	}

	while (j < count[1]) {
		if (!strcmp(namelist[1][j]->d_name, ".") || !strcmp(namelist[1][j]->d_name, "..")) {
			j++;
			continue;
		}
		printf("Only in %s: %s\n", relpath[1], namelist[1][j++]->d_name);
	}
}

// 파일 내용 차이점 찾는 함수
int find_diff(void)
{
	int i, j;
	int is_diff = 0;
	int arr[BUFMAX][BUFMAX] = {0, };

	for (int i = 1; i <= o_line; i++) {
		for (int j = 1; j <= n_line; j++) {
			if (i_option) {
				if (!strcasecmp(o_content[i], n_content[j]))
					arr[i][j] = arr[i-1][j-1] + 1;
				else 
					arr[i][j] = (arr[i-1][j] >= arr[i][j-1]) ? arr[i-1][j] : arr[i][j-1];	
			}
			else {
				if (!strcmp(o_content[i], n_content[j]))
					arr[i][j] = arr[i-1][j-1] + 1;
				else 
					arr[i][j] = (arr[i-1][j] >= arr[i][j-1]) ? arr[i-1][j] : arr[i][j-1];
			}
		}
	}

	i = o_line;
	j = n_line;

	memset(o_common, 0, sizeof(o_common));
	memset(n_common, 0, sizeof(n_common));

	// 공통 부분 체크
	while (arr[i][j] != 0) {
		if (i_option) {
			if (!strcasecmp(o_content[i], n_content[j])) {
				o_common[i--] = 1;
				n_common[j--] = 1;
			}
			else {
				if (arr[i-1][j] > arr[i][j-1])
					i--;
				else
					j--;
			}
		}
		else {
			if (!strcmp(o_content[i], n_content[j])) {
				o_common[i--] = 1;
				n_common[j--] = 1;
			}
			else {
				if (arr[i-1][j] > arr[i][j-1])
					i--;
				else
					j--;
			}
		}
	}

	is_diff = (o_line == n_line && o_line == arr[o_line][n_line]) ? 0 : 1;
	if (is_diff)
		find_diff_block();
	
	return is_diff;
}

// 차이점 출력 블록 구하는 함수
void find_diff_block(void)
{
	int i, j;
	int o_total_line = o_line+1;
	int n_total_line = n_line+1;

	// 초기화
	block_num = 0;
	for (int i = 0; i < BUFMAX; i++) {
		memset(o_block[i], 0, sizeof(o_block[i]));
		memset(n_block[i], 0, sizeof(n_block[i]));
	}

	i = j = 1;
	int flag = 0;

	// 공통된 부분이 끝나면 그 다음부터는 차이가 나는 부분이므로,
	// 공통 부분이 시작되는 라인부터 끝나는 라인을 찾음
	// 해당 라인들을 블록에 저장 후 출력할 때 사용
	while (i <= o_total_line && j <= n_total_line) {
		if (o_common[i] && n_common[j]) {
			if (!flag) {
				block_num++;
				o_block[block_num][0] = i;
				n_block[block_num][0] = j;
				flag = 1;
			}
			i++;
			j++;
		} 
		else {
			if (flag) {
				o_block[block_num][1] = i-1;
				n_block[block_num][1] = j-1;
				flag = 0;
			}

			if (!o_common[i] && !n_common[j]) {
				i++;
				j++;
			}
			else if (o_common[i] && !n_common[j])
				j++;
			else if (!o_common[i] && n_common[j])
				i++;
		}
	}
	
	for (int k = 0; k < 2; k++) {
		o_block[block_num+1][k] = o_total_line;
		n_block[block_num+1][k] = n_total_line;
	}
}

// 차이점 출력 함수
void print_diff(void)
{
	int idx;
	char flag;

	for (int i = 0; i <= block_num; i++) {
		if (o_block[i+1][0] - o_block[i][1] > 1) {
			if (n_block[i+1][0] - n_block[i][1] > 1)
				flag = 'c';
			else
				flag = 'd';
		}
		else {
			if (n_block[i+1][0] - n_block[i][1] > 1)
				flag = 'a';
			else
				continue;
		}

		// 차이점 시작 라인, 끝 지점 라인 지정
		int o_start = o_block[i][1];
		int o_end = o_block[i+1][0];
		int n_start = n_block[i][1];
		int n_end = n_block[i+1][0];

		if (flag == 'a')
			printf("%d", o_start);
		else
			printf("%d", o_start+1);

		if (o_end - o_start > 2)
			printf(",%d", o_end-1);

		printf("%c", flag);

		if (flag == 'd')
			printf("%d", n_start);
		else
			printf("%d", n_start+1);

		if (n_end - n_start > 2)
			printf(",%d", n_end-1);

		printf("\n");

		if (flag == 'c' || flag == 'd') {
			for (idx = o_start+1; idx < o_end; idx++)
				printf("< %s", o_content[idx]);
		}

		if (flag == 'c')
			printf("---\n");

		if (flag == 'c' || flag == 'a') {
			for (idx = n_start+1; idx < n_end; idx++)
				printf("> %s", n_content[idx]);
		}
	}
}

// help 명령어 실행 함수
void command_help(void)
{
	printf("Usage:\n");
	printf("  > find [FILENAME] [PATH]\n");
	printf("     >> [INDEX] [OPTION ... ]\n");
	printf("  > help\n");
	printf("  > exit\n\n");
	printf("  [OPTION ... ]\n");
	printf("   q : report only when files differ\n");
	printf("   s : report when two files are the same\n");
	printf("   i : ignore case differences in file contents\n");
	printf("   r : recursively compare any subdirectories found\n");
}

// 파일 리스트 출력 시 파일명 정렬 함수
int compare_depth(const void* a, const void* b)
{
	if (((fileinfo *)a)->depth > ((fileinfo *)b)->depth)
		return 1;
	else if (((fileinfo *)a)->depth < ((fileinfo *)b)->depth)
		return -1;
	else {
		if (strcmp(((fileinfo *)a)->path, ((fileinfo *)b)->path) > 0)
			return 1;
		else if(strcmp(((fileinfo *)a)->path, ((fileinfo *)b)->path) < 0)
			return -1;
	}
	return 0;
}

// 디렉토리 크기 구하는 함수
off_t get_directory_size(char* dirname)
{
	DIR* dp;
	off_t size = 0;
	char pathname[PATHMAX];
	struct dirent* dentry;
	struct stat statbuf;
	
	if ((dp = opendir(dirname)) == NULL) {
		fprintf(stderr, "opendir error for %s\n", dirname);
		exit(1);
	}

	while ((dentry = readdir(dp)) != NULL) {
		if (!strcmp(dentry->d_name, ".") || !strcmp(dentry->d_name, ".."))
			continue;
		
		sprintf(pathname, "%s/%s", dirname, dentry->d_name);
		if ((lstat(pathname, &statbuf) < 0) && (!access(pathname, F_OK))) {
			fprintf(stderr, "lstat error for %s\n", pathname);
			exit(1);
		}

		if (S_ISDIR(statbuf.st_mode))
			size += get_directory_size(pathname);
		else 
			size += statbuf.st_size;
	}

	return size;
}

// 입력 문자열 토크나이징 함수
int split(char* string, char* seperator, char* argv[])
{
	int argc = 0;
	char* ptr = NULL;

	ptr = strtok(string, seperator);
	while (ptr != NULL) {
		argv[argc++] = ptr;
		ptr = strtok(NULL, " ");
	}

	return argc;
}

// 파일 종류 구하는 함수
char get_type(struct stat statbuf)
{
	char type = 'u';

	if (S_ISREG(statbuf.st_mode))
		type = '-';
	else if (S_ISDIR(statbuf.st_mode))
		type = 'd';
	else if (S_ISCHR(statbuf.st_mode))
		type = 'c';
	else if (S_ISBLK(statbuf.st_mode))
		type = 'b';
	else if (S_ISFIFO(statbuf.st_mode))
		type = 'f';
	else if (S_ISSOCK(statbuf.st_mode))
		type = 's';

	return type;
}

// 전체 경로에서 파일명만 추출하는 함수
char* extract_name(char* string)
{
	char* filename = strrchr(string, '/');
	return filename ? filename + 1 : string;
}

// 중복되는 경로 제거하는 함수
char* get_relpath(char* abspath)
{
	char* ptr;
	char* path;
	char cwd[PATHMAX];
	getcwd(cwd, sizeof(cwd));

	if ((ptr = strstr(abspath, cwd)) != NULL)
		path = &abspath[strlen(cwd) + 1];
	else
		path = abspath;

	return path;
}

// 현재 지정된 옵션 구하는 함수
char* get_option(void)
{
	char* option = (char*)calloc(2, sizeof(char));
	option[0] = '-';
	option[1] = '\0';

	if (r_option) {
		char temp[2] = "r";
		strcat(option, temp);
	}
	if (q_option) {
		char temp[2] = "q";
		strcat(option, temp);
	}
	if (s_option) {
		char temp[2] = "s";
		strcat(option, temp);
	}
	if (i_option) {
		char temp[2] = "i";
		strcat(option, temp);
	}

	return option;
}

// 파일 시간 출력을 위한 함수
char* get_time(time_t stime)
{
	char* time = (char*)malloc(sizeof(char) * BUFMAX);
	struct tm *tm;

	tm = localtime(&stime);
	sprintf(time, "%02d-%02d-%02d %02d:%02d", tm->tm_year-100, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min);

	return time;
}

// 파일 권한 구하는 함수
char* get_permission(struct stat statbuf)
{
	char* permission = (char*)malloc(sizeof(char) * 9) + 1;

	for (int i = 6; i >= 0; i = i-3) {
		if ((S_IROTH << i) & statbuf.st_mode)
			permission[6-i] = 'r';
		else
			permission[6-i] = '-';

		if ((S_IWOTH << i) & statbuf.st_mode)
			permission[7-i] = 'w';
		else 
			permission[7-i] = '-';

		if ((S_IXOTH << i) & statbuf.st_mode)
			permission[8-i] = 'x';
		else
			permission[8-i] = '-';
	}
	permission[9] = '\0';

	return permission;
}
