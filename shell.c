/**
 * Linux Job Control Shell Project
 *
 * Operating Systems
 * Grados Ing. Informatica & Software
 * Dept. de Arquitectura de Computadores - UMA
 *
 * Some code adapted from "OS Concepts Essentials", Silberschatz et al.
 *
 * To compile and run the program:
 *   $ gcc shell.c job_control.c -o shell
 *   $ ./shell
 *	(then type ^D to exit program)
 **/



#include "job_control.h"   /* Remember to compile with module job_control.c */
#include <signal.h>

#define MAX_LINE 256 /* 256 chars per line, per command, should be enough */

/**
 * MAIN
 **/
int main(void)
{
	char inputBuffer[MAX_LINE]; /* Buffer to hold the command entered */
	int background;             /* Equals 1 if a command is followed by '&' */
	char *args[MAX_LINE/2];     /* Command line (of 256) has max of 128 arguments */
	/* Probably useful variables: */
	int pid_fork, pid_wait; /* PIDs for created and waited processes */
	int status;             /* Status returned by wait */
	enum status status_res; /* Status processed by analyze_status() */
	int info;				/* Info processed by analyze_status() */
	
	while (1)   /* Program terminates normally inside get_command() after ^D is typed*/
	{   		
		ignore_terminal_signals();
		printf("COMMAND->");
		fflush(stdout);
		get_command(inputBuffer, MAX_LINE, args, &background);  /* Get next command */
		
		if(args[0]==NULL) continue;   /* Do nothing if empty command */

		/** The steps are:
		 *	 (1) Fork a child process using fork()
		 *	 (2) The child process will invoke execvp()
		 * 	 (3) If background == 0, the parent will wait, otherwise continue
		 *	 (4) Shell shows a status message for processed command
		 * 	 (5) Loop returns to get_commnad() function
		 **/
		pid_fork = fork();
		if(pid_fork == 0){
			restore_terminal_signals();
			pid_fork = execvp(args[0], args);
			printf("hubo un eggroll!!! Comando: %s\n", args[0]);
			exit(status);
		}
		if(pid_fork > 0){
			new_process_group(pid_fork);
			if(!background){
				//set_terminal(pid_fork);
				pid_wait = waitpid(pid_fork, &status, 0);
				analyze_status(status,&info);
				printf("Foreground pid: %d, command: %s, Exited, info: %d\n",pid_fork, args[0], info);
				//set_terminal(pid_fork);
			} else {
				printf("Background job running... pid: %d, command: %s\n", pid_fork, args[0]);	
			}
		}
		
		

	} /* End while */
}
