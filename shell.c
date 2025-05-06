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

#define MAX_LINE 256 /* 256 chars per line, per command, should be enough */

job* job_list;

void sigchld_handler(int num_sig) {
	int status;
	pid_t pid_wait;
	int info;
	job_iterator iter = get_iterator(job_list);
	while(has_next(iter)){
		job *j = next(iter);
		pid_wait = waitpid(j->pgid, &status, WNOHANG | WUNTRACED | W_OK);
		if(pid_wait == j->pgid){
			enum status status_res = analyze_status(status, &info);
			printf("Background pid: %d, command %s, %s, info: %d\n",j->pgid, j->command, status_strings[status_res], info);
			if(status_res == SUSPENDED){
				j->state = STOPPED;
			}
			else if(status_res == CONTINUED){
				j->state = BACKGROUND;
			}
			else{
				delete_job(job_list, j);
			}
		}else{
			perror("Wait error");
		}
	}
}

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
	
	ignore_terminal_signals();
	signal(SIGCHLD, sigchld_handler);
	job_list = new_list("job list");
	while (1)   /* Program terminates normally inside get_command() after ^D is typed*/
	{   		
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
		//comandos internos
		if(!strcmp(args[0], "cd")) chdir(args[1]);	
		else if(!strcmp(args[0], "exit")){
			printf("Bye\n"); exit(EXIT_SUCCESS);
		} else if(!strcmp(args[0], "jobs")) print_job_list(job_list);
		else {
			pid_fork = fork();
			if(pid_fork > 0){
				new_process_group(pid_fork);
				if(!background){
					set_terminal(pid_fork);
					pid_wait = waitpid(pid_fork, &status, WUNTRACED);
					set_terminal(getpid());
					if(pid_fork == pid_wait){
						status_res = analyze_status(status,&info);
						if(status_res == SUSPENDED){
							add_job(job_list, new_job(pid_fork, args[0], STOPPED));
						}
						printf("Foreground pid: %d, command: %s, %s, info: %d\n",pid_fork, args[0], status_strings[status_res], info);
						
					} 
					else perror("Wait error");
				} else {
					add_job(job_list, new_job(pid_fork, args[0], BACKGROUND));
					printf("Background job running... pid: %d, command: %s\n", pid_fork, args[0]);	
				}
			}
		if(pid_fork == 0){
			new_process_group(pid_fork);
			if(!background) set_terminal(pid_fork);
			restore_terminal_signals();
			/*pid_fork = */execvp(args[0], args);
			printf("hubo un eggroll!!! Comando: %s\n", args[0]);
			exit(-1);
		}
	}
		

	} /* End while */
}
