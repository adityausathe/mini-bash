#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>

#define MAX 100
#define MAX_STACK 32
#define MAX_NUM 10

#define ERR_PAR "[-] Error: Invalid parameters\n"
#define ERR_COM "[-] Error: Invalid Command\n"

#define CTRLZ 1
#define CTRLC 2
#define NORM_EX 0
#define NOT_EXIST 3
#define CONT 4

 
#define TRUE 1
#define FALSE 0

#define READ  0
#define WRITE 1

/*job status*/
#define STAT_RUN 1
#define STAT_EXIT 2
#define STAT_STOP 3
#define STAT_BACK 4

int IN;
int OUT;
int runback;

int child;
int shell_pid;


/* REGISTER maintained by shell to keep track of bg running children, stopped children, etc.*/
typedef struct{
	pid_t pid;
	char name[100];
	int status;
} bgjobs;

typedef struct{
	bgjobs runningprocess[MAX_STACK];
	int activeprocesses[MAX_STACK];
	int count;
}ProcessList;


ProcessList processlist;


/*Function Declarations*/
int parse(char *command, char *argv[], char *infile, char *outfile);
int startwaiting(pid_t pid, int showlog);
int runcommand(int input, int first, int last, char* argv[], char* infile, char* outfile);
void runNormalCommand(char *argv[], char* infile, char* outfile, int ispipe);
char *extractCommand(char *line);
int checkForBuiltIns(char *line);
void sigchldhandler(int sig);
void sigstophandler(int sig);
void sigintHandler(int sig_num);
void show_activejobs();
void get_activejobs();
int update_status(pid_t pid, int new_status);
void intialise_processlist();
void kill_all();
void custom_gets(char *rawCommand);
int getIndexFrom_pid(pid_t pid);
