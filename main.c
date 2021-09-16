#include "shell.h"

/*
 * Input: void
 * Function: -Reads commands in a loop from user. Checks the type of command i.e. whether it is shell's feature
		(e.g. bg, kill, fg, jobs, etc) or a program execution.
	     -It also checks for pipe'|' and acts accordingly.
 * Output: returns 0 on normal exit.
 */

int main(){
	char rawCommand[MAX], infile[MAX], outfile[MAX];
	int argc;
	char *argv[MAX_NUM];
	char *label = "\n$hell:> ";
	
	char *token = (char *)malloc(100*sizeof(char));

	signal(SIGINT, sigintHandler);
	signal(SIGCHLD, sigchldhandler);
	signal(SIGTSTP, sigstophandler);
	
	shell_pid = getpid();

	setpgid(shell_pid, shell_pid);

	intialise_processlist();
	IN = FALSE;
	OUT = FALSE;
	runback = FALSE;

	while(1){

		write(1, label, strlen(label));
		
		custom_gets(rawCommand);

		if(strcmp(rawCommand, "exit") == 0){
			break;
		}
	
		if(checkForBuiltIns(rawCommand))
			continue;
		
		   
		if(strchr(rawCommand,'|') == NULL){
			/*Command w/o pipe*/
			argc = parse(rawCommand, argv, infile, outfile);
			if(argc == -1)
				continue;
			runNormalCommand(argv, infile, outfile, FALSE);
			
			free_argv(argc, argv);
		}
		else{
			/* Command with pipe.*/
			token = strtok(rawCommand, "|");

   			int last= FALSE, first = TRUE;
			int input = 0;

			while( token != NULL ) {
				char *nexttoken = strtok(NULL, "|");
				
				if(nexttoken == NULL)
					last = TRUE;

				argc = parse(token, argv, infile, outfile);
				if(argc == -1)
					continue;
					
				input = runcommand(input, first, last, argv, infile, outfile);

				first = FALSE;
				if(nexttoken != NULL)
					strcpy(token, nexttoken);
				else
					token = NULL;
					
				free_argv(argc, argv);
			}
			

		}
	}
	/* Kill all the jobs while exiting*/
	kill_all();
	return 0;
}
