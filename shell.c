#include "shell.h"

/*
 * Input: An input line taken from user, empty argv[], and reference to input and output file names.
 * Function: It traverses input string command char by char and breaks it into program binary name and its arguments.
	     It also checks for I/O redirection and extract the file names.
 * Output: Length of argv array.
 */
int parse(char *command, char *argv[], char *infile, char *outfile){
	
	int i, j = 0;
	int argc = 0;
	int isfirst = TRUE;
	int in = FALSE; 
	int out = FALSE;
	char token[MAX];

	IN = OUT = FALSE;
	runback = FALSE;

	void extract(){
		token[j] = '\0';
		if(j == 0)
			return;
		if(in){
			strcpy(infile, token);
			in = FALSE;
			IN = TRUE;
		}
		else if(out){
			strcpy(outfile, token);
			out = FALSE; 
			OUT = TRUE;
		}
		else{
			argv[argc] = (char *)malloc(MAX*sizeof(char));
			strcpy(argv[argc], token);
			argc++;
		}
		j = 0;
	}
	
	for(i = 0;i < strlen(command);i++){
	
		if(command[i] == '&'){
			if(j)
				runback = TRUE;
			else{
				printf("\n%s", ERR_COM);
				return -1;
			}
			continue;
		}
		if(command[i] == ' ' || command[i] == '\n' || command[i] == '\t'){
			if(isfirst)
				continue;
			extract();
		}
		else if(command[i] == '<'){
			extract();
			in = TRUE;
		}
		else if(command[i] == '>'){
			extract();
			out = TRUE;
		}
		else{
			token[j++] = command[i];
			isfirst = FALSE;
		}
		
	}

	extract();

	argv[argc] = NULL;
	return argc;
}


/*
 * Input: child's pid and boolean to enable messages on the STDOUT.
 * Function: It is called by shell to wait for child process to terminate and get its status.
 * Output: Returns the status of child process.
 */
int startwaiting(pid_t pid, int showlog){
	int status;
	child = pid;
	do {
		int w = waitpid(pid, &status, WUNTRACED | WCONTINUED);
		if (w == -1) {
			printf("\n[+] Job done!\n");
			return NOT_EXIST;
		}
		if(showlog==TRUE){
			if (WIFEXITED(status)) {
		       		printf("exited, status=%d\n", WEXITSTATUS(status));
			} else if (WIFSIGNALED(status)) {
				printf("killed by signal %d\n", WTERMSIG(status));
			} else if (WIFSTOPPED(status)) {
				printf("stopped by signal %d\n", WSTOPSIG(status));
			} else if (WIFCONTINUED(status)) {
			       	printf("continued\n");
			}
		}
	} while (!WIFEXITED(status) && !WIFSIGNALED(status) && !WIFSTOPPED(status));

	if (WIFEXITED(status)) {
       		return NORM_EX;
	} else if (WIFSIGNALED(status)) {
		return CTRLC;
	} else if (WIFSTOPPED(status)) {
		return CTRLZ;
	} else if (WIFCONTINUED(status)) {
	       return CONT;
	}
	return -1;
}

/*
 * Input: parsed command in the form of *argv[], etc
 * Function: It creates pipe and redirects I/O accordingly. After creation of pipe it executes the command 
	     normally by calling runNormalCommand();
 * Output: File descriptor of previous command's pipe
 */ 
int runcommand(int input, int first, int last, char* argv[], char* infile, char* outfile){

	int pipefd[2];
 
	pipe( pipefd );	
	pid_t pid = fork();

	if (pid == 0) {
		if (first && !last && !input) {
			dup2( pipefd[WRITE], STDOUT_FILENO );
		} else if (!first && !last && input) {
			dup2(input, STDIN_FILENO);
			dup2(pipefd[WRITE], STDOUT_FILENO);
		} else {
			dup2( input, STDIN_FILENO );
		}
 
		runNormalCommand(argv, infile, outfile, TRUE);

	}
 	else{
		if(!runback){
			switch(startwaiting(pid, FALSE)){
				case(CTRLZ):{
					add_job(pid, argv[0], STAT_STOP);
				}
			}
		}
		else{
			add_job(pid, argv[0], STAT_BACK);
		}
	}
	if (input != 0) 
		close(input);
 
	close(pipefd[WRITE]);
 
	if (last == 1)
		close(pipefd[READ]);
 
	return pipefd[READ];
}


/*
 * Input: parsed command in the form of *argv[], etc
 * Function: It redirects I/O accordingly.
	     Call to fork() to create child process.
     	     Call to execvp() to load and execute program.
	     Call to startwaiting() to wait untill child process terminates if its running in foreground.
 * Output: void
 */ 
void runNormalCommand(char *argv[], char* infile, char* outfile, int ispipe){
	
	int chid;
	
	if(ispipe == TRUE)
		chid = 0;
	else
		chid = fork();

	if(chid == 0){
		//signal(SIGTSTP, sigstophandler);
		if (IN) {
		    int fd = open(infile, O_RDONLY, 0);
		    dup2(fd, STDIN_FILENO);
		    dup(0);
		    close(fd);
		}

		if (OUT) {
		    int fd = creat(outfile, 0644);
		    dup2(fd, STDOUT_FILENO);
		    dup(1);
		    close(fd);
		}
	
		if(runback){
			pid_t id;
			id = getpid();
			setpgid(id, id);
			
		}
		if (execvp( argv[0], argv) == -1){
			printf("\n%s", ERR_COM);
			_exit(EXIT_FAILURE);
		}
		
		exit(0);
	}
	else{
		if(!runback){
			
			switch(startwaiting(chid, FALSE)){
				case(CTRLZ):{

					add_job(chid, argv[0], STAT_STOP);

				}
			}
		}
		else{
			add_job(chid, argv[0], STAT_BACK);
		}
		
	}
	//printf("%d\n", runback);
}

/*
 * Utility Function to strip the program name from the user input.
 */
char *extractCommand(char *line){
	char *strippedline;
	
	strippedline = (char *)malloc(MAX*sizeof(char));

	int first = TRUE, i=0;

	while(*line != '\0'){
		if(*line == ' ' && !first)
			break;
		else if(*line != ' '){
			first = FALSE;
			strippedline[i++] = *line;
		}
		line++;
	}
	
	return strippedline;
}


/*
 * Input: User input string as it is.
 * Funcion: Checks for built in shell jobs viz. bg, fg, kill, jobs and act accordigly.
 * Output: boolean whether command is built in job or proram excution request.
 */
int checkForBuiltIns(char *line){

	char *c = extractCommand(line);

	if(strcmp(c, "jobs")==0){
		
		get_activejobs();
		show_activejobs();

	}
	else if(strcmp(c, "bg")==0){

		char *requestedproc = strchr(line, '%');
		int reqindex;

		if(requestedproc == NULL){
			char *reqpid = strchr(line, ' ');
			if(reqpid == NULL){
				printf("Usage: bg <pid> or bg %%<job_number>\n");
				return TRUE;
			}
			int reqpidint = atoi(++reqpid);
			
			if((reqindex = getIndexFrom_pid(reqpidint)) == -1){
				printf("%s", ERR_PAR);
				return TRUE;
			}
		}
		else
			reqindex = atoi(++requestedproc);

		
		if(reqindex > processlist.count){
			printf("%s", ERR_PAR);
			return TRUE;
		}
		get_activejobs();
		
		setpgid(processlist.activeprocesses[reqindex-1], processlist.activeprocesses[reqindex-1]);

		kill(processlist.activeprocesses[reqindex-1], SIGCONT);
		
		update_status(processlist.activeprocesses[reqindex-1], STAT_BACK);

		
	}
	else if(strcmp(c, "fg")==0){

		char *requestedproc = strchr(line, '%');
		int reqindex;

		if(requestedproc == NULL){
			char *reqpid = strchr(line, ' ');
			if(reqpid == NULL){
				printf("Usage: fg <pid> or fg %%<job_number>\n");
				return TRUE;
			}
			int reqpidint = atoi(++reqpid);
			
			if((reqindex = getIndexFrom_pid(reqpidint)) == -1){
				printf("%s", ERR_PAR);
				return TRUE;
			}
		}
		else
			reqindex = atoi(++requestedproc);

		if(reqindex > processlist.count){
			printf("%s", ERR_PAR);
			return TRUE;
		}
		get_activejobs();

		child = processlist.activeprocesses[reqindex-1];

		kill(processlist.activeprocesses[reqindex-1], SIGCONT);
		
		update_status(processlist.activeprocesses[reqindex-1], STAT_RUN);

		int grpid = getpgid(processlist.activeprocesses[reqindex-1]);

		tcsetpgrp(0, grpid);

		if(kill(grpid, SIGCONT)< 0)
			perror ("kill (SIGCONT)");
		
		if(startwaiting(processlist.activeprocesses[reqindex-1], TRUE) == NOT_EXIST){
			tcsetpgrp(0, shell_pid);
		}
	}
	else if(strcmp(c, "kill")==0){
		char *requestedproc = strchr(line, '%');
		int reqindex;

		if(requestedproc == NULL){
			char *reqpid = strchr(line, ' ');
			if(reqpid == NULL){
				printf("Usage: kill <pid> or kill %%<job_number>\n");
				return TRUE;
			}
			int reqpidint = atoi(++reqpid);
			
			if((reqindex = getIndexFrom_pid(reqpidint)) == -1){
				printf("%s", ERR_PAR);
				return TRUE;
			}
		}
		else
			reqindex = atoi(++requestedproc);
		

		if(reqindex > processlist.count){
			printf("%s", ERR_PAR);
			return TRUE;
		}

		get_activejobs();

		if(kill(processlist.activeprocesses[reqindex-1], SIGKILL) != 0){
			char *message2 = "\n[-] Process already dead!!!\n";		
			write(1, message2, strlen(message2));
		}
		else{
			char *message2 = "\n[-] Process Killed successfully!!!\n";		
			write(1, message2, strlen(message2));
		}
	}
	else{
		return FALSE;
	}
	return TRUE;
}


/*
 * Signal Handler for Ctrl+C signal. Shell does nothing on getting this signal.
 */
void sigintHandler(int sig_num){

	signal(SIGINT, sigintHandler);

}

/*
 * When child changes its state this signal handler is called.
 * It calls waitpid() to get the child's pid and updates its state.
 */
void sigchldhandler(int sig){
	int status;
	pid_t pid;
	
    	while((pid = waitpid(-1, &status, WNOHANG))>0){
		update_status(pid, STAT_EXIT);
	}
}

/*
 * Signal Handler for Ctrl+Z signal. Shell @overrides default behaviour of this signal.
 */
void sigstophandler(int sig){

	signal(SIGTSTP,sigstophandler);
	return;
}

/*
 * Utility function to initialse the list which keeps status of background running and stopped jobs.
 */
void intialise_processlist(){
	processlist.count = 0;
}


/*
 * Input: child pid, job list(global struct)
 * Function: Adds a child program info executed by the shell to a list.
 * Output: returns TRUE on successful addition of job.
 */
int add_job(pid_t pid, char *procname, int stat){
	int i;
	for(i=0;i<MAX_STACK;i++){
		int status = processlist.runningprocess[i].status;
		if(!(status==STAT_STOP|status==STAT_BACK|status==STAT_RUN)){
			processlist.runningprocess[i].pid = pid;
			strcpy(processlist.runningprocess[i].name, procname);
			processlist.runningprocess[i].status = stat;
			processlist.count++;
			return TRUE;
		}
	}
	return FALSE;
}

/*
 * Input: child pid, job list(global struct)
 * Function: Update status of a child with pid pid in the processlist.
 * Output: returns TRUE on successful status update.
 */
int update_status(pid_t pid, int new_status){
	int i;
	for(i=0;i<MAX_STACK;i++){
		if(pid == processlist.runningprocess[i].pid){
			processlist.runningprocess[i].status = new_status;
			return TRUE;
		}
	}
	return FALSE;

}

/*
 * Input: void
 * Function: Traverses the processlist to get all the live jobs and adds them to a global array processlist.runningprocess[]
 * Output: void
 */
void get_activejobs(){
	int i;
	int ac=0;
	
	for(i=0;i<MAX_STACK;i++){
		int status = processlist.runningprocess[i].status;
		if(status==STAT_STOP|status==STAT_BACK|status==STAT_RUN){
			processlist.activeprocesses[ac] = processlist.runningprocess[i].pid;
			ac++;
		}
	}
}

/*
 * Input: void
 * Function: Traverses the processlist to get all the live jobs and display them on STDOUT.
 * Output: void
 */
void show_activejobs(){
	int i;
	char proc[MAX];
	int ac=1;

	char *status_label[] = {"", "Running", "Terminated", "Stopped", "Background Run.."};

	for(i=0;i<MAX_STACK;i++){
		int status = processlist.runningprocess[i].status;
		if(status==STAT_STOP|status==STAT_BACK|status==STAT_RUN){
			sprintf(proc, "[%d]\t<%d>\t%s\t%s\n", ac, processlist.runningprocess[i].pid, status_label[status],
						processlist.runningprocess[i].name); 
			write(1, proc, strlen(proc));
			ac++;
		}
	}
}


int getIndexFrom_pid(pid_t pid){
	int i;

	for(i=0;i<processlist.count;i++){
		if(processlist.activeprocesses[i] == pid){
			return i+1;
		}
	}
	return -1;
}

/*
 * Input: void
 * Function: Traverses the processlist to get all the live jobs and kills them all.
 * Output: void
 */
void kill_all(){
	int i;

	for(i=0;i<MAX_STACK;i++){
		int status = processlist.runningprocess[i].status;
		if(status==STAT_STOP|status==STAT_BACK|status==STAT_RUN){

			kill(processlist.runningprocess[i].pid, SIGKILL);

		}
	}
}

/*
 * Custom function similar to gets() which reads user input.
 */
void custom_gets(char *rawCommand){
	int i=0;
	char c;
	while((c = getchar()) != '\n'){
		rawCommand[i++] = c;
	}
	rawCommand[i] = '\0';

}

void free_argv(int argc, char *argv[]){
	int i=0;
	for(i = 0;i < argc;i++)
		free(argv[i]);
}
