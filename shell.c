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
	job_iterator iter;
	job *j;
	while ((pid_wait = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0) {
        iter = get_iterator(job_list);
        while (has_next(iter)) {
            j = next(iter);
            if (j->pgid == pid_wait) {
                enum status status_res = analyze_status(status, &info);
                printf("Background pid: %d, command %s, %s, info: %d\n", j->pgid, j->command, status_strings[status_res], info);
                if (status_res == SUSPENDED) {
                    j->state = STOPPED;
                } else if (status_res == CONTINUED) {
                    j->state = BACKGROUND;
                } else {
                    delete_job(job_list, j);
                }
                break;
            }
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
	char *file_in, *file_out;
	FILE *infile, *outfile;
	
	ignore_terminal_signals();
	signal(SIGCHLD, sigchld_handler);
	job_list = new_list("job list");
	while (1)   /* Program terminates normally inside get_command() after ^D is typed*/
	{   		
		printf("COMMAND->");
		fflush(stdout);
		get_command(inputBuffer, MAX_LINE, args, &background);  /* Get next command */
		parse_redirections(args, &file_in, &file_out);
		
		if(args[0]==NULL) continue;   /* Do nothing if empty command */

		/** The steps are:
		 *	 (1) Fork a child process using fork()
		 *	 (2) The child process will invoke execvp()
		 * 	 (3) If background == 0, the parent will wait, otherwise continue
		 *	 (4) Shell shows a status message for processed command
		 * 	 (5) Loop returns to get_commnad() function
		 **/
		//comandos internos
		if(!strcmp(args[0], "cd")){
			if(chdir(args[1]) != 0) printf("No existe el directorio %s\n", args[1]);
		} 
		else if(!strcmp(args[0], "exit")){
			printf("Bye\n"); exit(EXIT_SUCCESS);
		} else if(!strcmp(args[0], "jobs"))	
			if(empty_list(job_list)) printf("no jobs running at the moment.\n"); 
			else print_job_list(job_list);
		else if(!strcmp(args[0], "fg") || !strcmp(args[0], "bg")) {
			if (empty_list(job_list)) {
				printf("no jobs to manipulate.\n");
				continue;
			}
			job *job_to_resume;
			unsigned short int index = 1; // Default to most recent job (position 1)
			if (args[1] != NULL) {
				index = atoi(args[1]);
				if (index < 1 || index > list_size(job_list)) {
					printf("Index %d out of bounds for jobs\n", index);
					continue;
				}
			}
			job_to_resume = get_item_bypos(job_list, index);
			int is_stopped = (job_to_resume->state == STOPPED);
			pid_t pid_job = job_to_resume->pgid;
			char *command = strdup(job_to_resume->command);

			if (!strcmp(args[0], "fg")) {
				block_SIGCHLD();
				delete_job(job_list, job_to_resume);
				set_terminal(pid_job);
				if (is_stopped)
					killpg(pid_job, SIGCONT);
				pid_wait = waitpid(pid_job, &status, WUNTRACED);
				set_terminal(getpid());
				status_res = analyze_status(status, &info);
				if (status_res == SUSPENDED)
					add_job(job_list, new_job(pid_wait, command, STOPPED));
				printf("Foreground pid: %d, command: %s, %s, info: %d\n",
					   pid_wait, command, status_strings[status_res], info);
				unblock_SIGCHLD();				
			}
			else{//bring from stopped to background
				if(!is_stopped){
					printf("This job (%s) is already in the background!\n",job_to_resume->command);
					continue;
				}
				job_to_resume->state = BACKGROUND;
				killpg(job_to_resume->pgid, SIGCONT);
			}
			free(command);
		} else {
			pid_fork = fork();
			if(pid_fork > 0){ //hijo
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
		if(pid_fork == 0){ //padre
			new_process_group(pid_fork);
			if(!background) set_terminal(pid_fork);
			if(file_in != NULL){
				infile = fopen(file_in, "r");
				if(infile == NULL){
					perror("Error opening input file");
					continue;
				}
				if(dup2(fileno(infile), STDIN_FILENO) == -1){
					perror("Error redirecting input");
					continue;
				}
				
			}
			if(file_out != NULL){
				outfile = fopen(file_out, "w");
				if(outfile == NULL){
					perror("Error opening output file");
					continue;
				}
				if( dup2(fileno(outfile), STDOUT_FILENO) == -1){
					perror("Error redirecting output");
					continue;
				}
			}
			restore_terminal_signals();
			/*pid_fork = */execvp(args[0], args);
			printf("hubo un eggroll!!! Comando: %s\n", args[0]);
			exit(-1);
		}
	}
		

	} /* End while */
}
