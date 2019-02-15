#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <unistd.h> 
#include <sys/types.h>  
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "parse.h"   /*include declarations for parse-related structs*/

enum
BUILTIN_COMMANDS { NO_SUCH_BUILTIN=0, EXIT, JOBS, CD, HISTORY, KILL, HELP};
 
int cmdCounter;
int bgCounter;

char *
buildPrompt()
{
	return  "% ";
}
 
/* Read in the user commands and check if builtin*/
int isBuiltInCommand(char * cmd){
	if ( strncmp(cmd, "exit", strlen("exit")) == 0){
    	return EXIT;
  	}
  	if (strncmp(cmd, "jobs", strlen("jobs")) == 0) {
    	return JOBS;
  	}
  	if (strncmp(cmd, "cd", strlen("CD")) == 0) {
    	return CD;
  	}
  	if (strncmp(cmd, "history", strlen("history")) == 0) {
    	return HISTORY;
  	}
  	if (strncmp(cmd, "kill", strlen("Kill")) == 0) {
    	return KILL;
  	}
  	if (strncmp(cmd, "help", strlen("help")) == 0) {
    	return HELP;
  	}
  	return NO_SUCH_BUILTIN;
}

int main (int argc, char **argv)
{
	char * cmdLine;
	parseInfo *info; /*info stores all the information returned by parser.*/
	struct commandType *com; /*com stores command name and Arg list for one command.*/

	cmdCounter = 0;
	bgCounter = 0;

#ifdef UNIX
	fprintf(stdout, "This is the UNIX version\n");
#endif

#ifdef WINDOWS
    fprintf(stdout, "This is the WINDOWS version\n");
#endif

	while(1){
    	/*insert your code to print prompt here*/
    	/*Get current working directory*/
    	char cwd[256]; 
    	/* Get username */
    	char* username = getenv("USER");
    	int cmd_num;
    	register HIST_ENTRY **the_list;
    	int size;
    	getcwd(cwd, sizeof(cwd));
		/* Prints prompt */
    	printf("\033[0;36m" "%s " "\033[m" "in " "\033[1;33m" "%s" "\033[m" , username, cwd);

#ifdef UNIX
    cmdLine = readline(buildPrompt());
    if (cmdLine == NULL) {
    	fprintf(stderr, "Unable to read command\n");
    	continue;
    }
#endif

    /*insert your code about history and !x !-x here*/
    if (cmdLine[0] == 33){
    	sscanf(&cmdLine[1], "%d", &cmd_num);
    	the_list = history_list ();
    	if(the_list){
			if (cmd_num <= cmdCounter && -cmd_num <= cmdCounter){
        		if (cmd_num < 0){
					cmdLine = the_list[cmdCounter + cmd_num]->line;
        		}
				else if (cmd_num == 0){
					printf("Command number 0 can not be used \n");
					continue;
				}
        		else {
          			cmdLine = the_list[cmd_num]->line;
        		}
			}
			else
			{
				printf("Command number not in history \n");
				continue;
			}
      	}
	}

	add_history(cmdLine);
	cmdCounter += 1;
	
	/*calls the parser*/
	info = parse(cmdLine);
	if (info == NULL){
    	free(cmdLine);
    	continue;
    }
    
	/*prints the info struct*/
    print_info(info);

    /*com contains the info. of the command before the first "|"*/
	com=&info->CommArray[0];
    if ((com == NULL)  || (com->command == NULL)) {
    	free_info(info);
      	free(cmdLine);
      	continue;
    }


	if (info->boolBackground){
		bgCounter += 1;
	}
    /*com->command tells the command name of com*/
	/* If command is exit */
    if (isBuiltInCommand(com->command) == EXIT){
		if (bgCounter <= 0){
			exit(1);
		}
		else {
			printf("Please kill all background processes before exiting! \n");
			continue;
		}
    }
	/* if command is jobs */
    else if (isBuiltInCommand(com->command) == JOBS){
		/* Use the ps command to view jobs */
		int status;
      	pid_t pid;
      	if ((pid = fork()) < 0) {     
        	printf("*** ERROR: forking child process failed\n");
        	exit(1);
      	}
      	else if (pid == 0) {
			  char *const parmList[] = {"-o pid,ppid,time", NULL};
        	if (execvp("/bin/ps", parmList) < 0) {  
          		printf("*** ERROR: exec failed\n");
          		exit(1);
        	}
      	}
		else {
			if (!info->boolBackground ){
				while (wait(&status) != pid){}
				}	
		}
    }
	/* if command is CD */
    else if (isBuiltInCommand(com->command) == CD){
    	int result;
    	result = chdir(com -> VarList[1]);
    	if (result == -1){
        	printf("cd command failed. Unable to change directory.");
      	}
    }
	/* if command is history */
    else if (isBuiltInCommand(com->command) == HISTORY){
      	register HIST_ENTRY **the_list;
      	register int i;
      	the_list = history_list ();
      	if (the_list)
        	for (i = 0; the_list[i]; i++)
          	printf ("%d: %s\n", i + history_base, the_list[i]->line);
    }
	/* if command is kill */
    else if (isBuiltInCommand(com->command) == KILL){
      	int result;
		int proc_num;
		sscanf(com->VarList[1], "%d", &proc_num);
      	result = kill (proc_num, SIGKILL);
		if (result != 0) {
			printf("Error killing the process. Please check the ID and try again.\n");
		}else
		{
			printf("\n Process: %d killed. \n",proc_num);
		}
		
    }
	/* if command is help */
    else if (isBuiltInCommand(com->command) == HELP){
      	printf("Possible Commands:\n");
      	printf("\033[0;32;32m" "jobs		: " "\033[0;36m" "Numbered list of all processes currently executing.\n");
      	printf("\033[0;32;32m" "cd [dir]	: " "\033[0;36m" "Changes the working directory. \n");
      	printf("\033[0;32;32m" "history 	: " "\033[0;36m" "Prints list of previously executed commands. \n");
      	printf("\033[0;32;32m" "exit		: " "\033[0;36m" "Terminates shell process. \n");
      	printf("\033[0;32;32m" "kill [pid]	: " "\033[0;36m" "Kills specified process. \n""\033[m");
    }
    /* Not built in command */
    else {
      	int result;
      	int status;
      	pid_t pid;
      	if ((pid = fork()) < 0) {     
        	printf("*** ERROR: forking child process failed\n");
        	exit(1);
      	}
      	else if (pid == 0) {
			/* If in file*/
			if (info->boolInfile) {
				int fd = open(info->inFile, O_RDONLY);
				if (dup2(fd, fileno(stdin))== -1){
					fprintf(stderr, "dup2 failed\n");
					continue;
				}
				close(fd);
			}
			/* If out file*/
			if (info->boolOutfile) {
				int fd2;
				if ((fd2 = open(info->outFile, O_WRONLY | O_CREAT, 0644)) < 0){
					perror("Couldn't open output file\n");
					exit(0);
					continue;
				}
				dup2(fd2, fileno(stdout));
				close(fd2);
			}
        	if (execvp(com->command, com->VarList) < 0) {     
          		printf("*** ERROR: exec failed\n");
          		exit(1);
        	}
      	}
      	else {
			  if (!info->boolBackground ){
				while (wait(&status) != pid){}
			  }	
      	}
	}
	if (isBuiltInCommand(com->command) == KILL){
		bgCounter -= 1;
	}
    free_info(info);
    free(cmdLine);
  }/* while(1) */
}
