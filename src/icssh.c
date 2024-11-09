#include "icssh.h"
#include "helpers.h"
#include "linkedlist.h"
#include <readline/readline.h>

int time_comparator(const void* node1, const void* node2);
void bgentryPrinter(void* data, void* file);
void sigchld_handler(int sig);
void sigusr2_handler(int sig);
void history_printer(void* data, void* file);
char* FindProcessByPID(list_t* list, pid_t target_pid);



bool reap = false;

list_t* list; 

int main(int argc, char* argv[]) {

	//printf("%d", getpid());
   	
	signal(SIGUSR2, sigusr2_handler);
	signal(SIGCHLD, sigchld_handler);
	//	printf("signal error");
	//	exit(1);

	int max_bgprocs = -1;
	int exec_result;
	int exit_status = -100;
	pid_t pid;
	pid_t wait_result;
	char* line;

	bool input_flag;
	bool output_flag;
	bool error_flag;

	int input_fd;
	int output_fd;

	char cwd[1024];

	list_t* history = CreateList(NULL, &history_printer, NULL);
	int history_i = 1;

	list = CreateList(&time_comparator, &bgentryPrinter, NULL);


#ifdef GS
    rl_outstream = fopen("/dev/null", "w");
#endif

    // check command line arg
    if(argc > 1) { 
        int check = atoi(argv[1]);
        if(check != 0)
            max_bgprocs = check;
        else {
            printf("Invalid command line argument value\n");
            exit(EXIT_FAILURE);
        }
    }

	// Setup segmentation fault handler
	if (signal(SIGSEGV, sigsegv_handler) == SIG_ERR) {
		perror("Failed to set signal handler");
		exit(EXIT_FAILURE);
	}

    	// print the prompt & wait for the user to enter commands string
	while ((line = readline(SHELL_PROMPT)) != NULL) {

		input_flag = false;
		output_flag = false;
		error_flag = false;

		if (reap && strcmp(line, "exit") != 0){

			pid_t pid;

			
			//while ((pid = wait(NULL)) > 0){
			while ((pid = waitpid(-1, NULL, WNOHANG)) > 0){

				char* s = FindProcessByPID(list, pid);

				if (s != NULL){
					printf(BG_TERM, pid, s);
					free(s);
				}
			
			}
			
			reap = false;
		}

		// MAGIC HAPPENS! Command string is parsed into a job struct
        // Will print out error message if command string is invalid

		//check for ! to re run old command (need to re-assign line).
		if (*((char*)line) == '!' ) {

			if (*((char*)line+1)  == '5'){
				history_i = 5;
			}
			else if (*((char*)line+1) == '4'){
				history_i = 4;
			}
			else if (*((char*)line+1)== '3'){
				history_i = 3;
			}
			else if (*((char*)line+1) == '2'){
				history_i = 2;
			}
			else if (*((char*)line+1) == '1' || *((char*)line+1)== '\0'){
				history_i = 1;
			}
			else{ 
				free(line);
				continue;
			}

			node_t* head = history->head;
			for (int i = 0; i < history_i; i++) {
				line = head->data;
				head = head->next;
			}
			printf("%s\n", line);

		}

		job_info* job = validate_input(line);
        if (job == NULL) { // Command was empty string or invalid
			free_job(job);  
			free(line);
			continue;
		}
		else if (strcmp(job->procs->cmd, "history") != 0 && *(job->procs->cmd) != '!' ) {
			//store for history
			InsertAtHead(history, strdup(line)); 
			if(history->length == 6){
				RemoveFromTail(history);	
			}


		}

		proc_info *procs = job->procs;
		int num_pipes = 0;
		while (procs != NULL) {
			num_pipes++;
			procs = procs->next_proc;
		}
		num_pipes--;

		//printf("%d\n", num_pipes);



        //Prints out the job linked list struture for debugging
        #ifdef DEBUG   // If DEBUG flag removed in makefile, this will not longer print
     		debug_print_job(job);
        #endif

//──────────────────────────────────────────────────────────────────────────────────────
		if (strcmp(job->procs->cmd, "exit") == 0) {

			// Kill all running background jobs
			node_t* bg_kill_node = list->head;
			while (bg_kill_node != NULL) {
				bgentry_t* bg_kill_entry = (bgentry_t*)bg_kill_node->data;

				// Print the information about the killed process
				printf(BG_TERM, bg_kill_entry->pid, bg_kill_entry->job->line);

				// Kill the background process
				if (kill(bg_kill_entry->pid, SIGKILL) == -1) {
   					 perror("Error killing background process");
				}

				bg_kill_node = bg_kill_node->next;
			}

			free(line);
			free_job(job);
			validate_input(NULL);   // calling validate_input with NULL will free the memory it has allocated
			DeleteList(history);
			DeleteList(list);

			return 0;
		
		}

//──────────────────────────────────────────────────────────────────────────────────────

		// example built-in: cd
		if (strcmp(job->procs->cmd, "cd") == 0) {
			
			//printf("\n\n\n\n");

			//check num of argvs (1 or 0). Any following arguments are ignored
			size_t row = 0;
			while(*(job->procs->argv+row) != NULL){
				row++;
			}

			if (row == 1){
				const char* homeDir = getenv("HOME");
				//printf("%s\n", homeDir);
				if (homeDir != NULL) {
        			//printf("Home directory: %s\n", homeDir);
					int return_chdir = chdir(homeDir);
					if (return_chdir){
						fprintf(stderr, "%s", DIR_ERR);
					}
					getcwd(cwd, sizeof(cwd));
					printf("%s\n", cwd);
				}
				else{
					fprintf(stderr, "%s", DIR_ERR);
				}
			}

			else{

				
				//printf("%s\n", *(job->procs->argv+1));
				int return_chdir = chdir(*(job->procs->argv+1));
				if (return_chdir){
					fprintf(stderr, "%s", DIR_ERR);
				}
				else{
					getcwd(cwd, sizeof(cwd));
					printf("%s\n", cwd);
				}
			}
			free_job(job);  
			free(line);
			continue;
		}


//──────────────────────────────────────────────────────────────────────────────────────

		if (strcmp(job->procs->cmd, "estatus") == 0) {
			printf("%d\n", exit_status);
			free_job(job);  
			free(line);
			continue;
		}


//──────────────────────────────────────────────────────────────────────────────────────

		if (strcmp(job->procs->cmd, "bglist") == 0) {  

			if (list != NULL && list->head != NULL) {
				node_t* head = list->head;
				while (head != NULL) {
					
					print_bgentry(head->data);
					head = head->next;
				}
			}
			continue;
		}


//──────────────────────────────────────────────────────────────────────────────────────

		if (strcmp(job->procs->cmd, "history") == 0) {

			//PrintLinkedList(history, stdout);	
			history_i = 1;

			if(history != NULL){
				node_t* head = history->head;
				while (head != NULL) {
					printf("%d: ", history_i);
					history->printer(head->data, stdout);
					printf("\n");
					head = head->next;
					history_i++;
				}
			}
			free_job(job);  
			free(line);
			continue;
		}

//──────────────────────────────────────────────────────────────────────────────────────


/*
		if (*(job->procs->cmd) == '!' ) {

			if (*(job->procs->cmd+1) == '5'){
				history_i = 5;
			}
			else if (*(job->procs->cmd+1) == '4'){
				history_i = 4;
			}
			else if (*(job->procs->cmd+1) == '3'){
				history_i = 3;
			}
			else if (*(job->procs->cmd+1) == '2'){
				history_i = 2;
			}
			else if (*(job->procs->cmd+1) == '1' || *(job->procs->cmd+1) == '\0'){
				history_i = 1;
			}
			else{
				continue;
			}

			node_t* head = history->head;
			for (int i = 0; i < history_i; i++) {
				line = head->data;
				head = head->next;
			}

			
			printf("%s\n", line);
			continue;
		}
*/

//──────────────────────────────────────────────────────────────────────────────────────

	
		int pipe_fd[2];  // File descriptors for the pipe

		//check for single pipe

		if(num_pipes == 1){
			if (pipe(pipe_fd) == -1) {
				perror("pipe");
				continue;;
			}
		}


		//printf("before checks\n");
		//check for input / output / files (err files later)
		if (job->in_file != NULL) {
			input_flag = true;
			
			// O_RDONLY read only access
			// S_IRUSR | S_IWUSR read and write for the user
			input_fd = open(job->in_file, O_RDONLY, S_IRUSR | S_IWUSR);

			if (input_fd == -1) {
				fprintf(stderr, "%s", RD_ERR);
				free_job(job);  
				free(line);
				continue;
			}

		} 

		if (job->out_file != NULL) {
			output_flag = true;	
		
			// O_WRONLY write only access
			// O_CREAT creates the file if it doesn't exist 
			// O_TRUNC and truncates it to zero length if it already exists
			// S_IRUSR | S_IWUSR read and write for the user
			output_fd = open(job->out_file, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
		} 

		if (input_flag == true && output_flag == true && strcmp(job->in_file, job->out_file) == 0) {
			fprintf(stderr, "%s", RD_ERR);
			free_job(job); 
			free(line);
			continue;
		}
		//printf("after checks\n");


		// example of good error handling!
        // create the child proccess
		if ((pid = fork()) < 0) {
			perror("fork error");
			exit(EXIT_FAILURE);
		}

		//job is created, check if it is a bg or fg job
		if (job->bg == true){
			
			bgentry_t *new_bgentry = malloc(sizeof(bgentry_t));
			
			// Check if memory allocation was successful
			if (new_bgentry != NULL) {
				new_bgentry->job = job;
				new_bgentry->pid = pid;
				new_bgentry->seconds = time(NULL);
			}

			InsertInOrder(list, new_bgentry);

		}


		if (pid == 0) {  //If zero, then it's the child process
            //get the first command in the job list to execute
			if(input_flag == true){	
				//printf("hi from input\n");
				dup2(input_fd, 0);
			}

			if(output_flag == true){
				//printf("hi from output\n");
				dup2(output_fd, 1);
			}

			//this will be treated as the "first" child
			if(num_pipes == 1){
				close(pipe_fd[0]);  // Close unused read end
                dup2(pipe_fd[1], STDOUT_FILENO); // Redirect stdout to the pipe
			}


			//printf("Child process %d executing\n", getpid());

		    proc_info* proc = job->procs;
			exec_result = execvp(proc->cmd, proc->argv);
			if (exec_result < 0) {  //Error checking
				printf(EXEC_ERR, proc->cmd);
				
				// Cleaning up to make Valgrind happy 
				// (not necessary because child will exit. Resources will be reaped by parent)
				free_job(job);  
				free(line);
    			validate_input(NULL);  // calling validate_input with NULL will free the memory it has allocated

				exit(EXIT_FAILURE);
			}
		} else {

			//now in parent, spawn second child and carry out second process in pipe
			if (num_pipes == 1){
				if ((pid = fork()) < 0) {
					perror("fork error");
					exit(EXIT_FAILURE);
				}
				
				if (pid == 0) {
            		// Child process (second command)
            		close(pipe_fd[1]);  // Close unused write end
					dup2(pipe_fd[0], STDIN_FILENO);  // Redirect stdin to the pipe

					proc_info* proc = job->procs;
					
					proc = proc->next_proc;
					exec_result = execvp(proc->cmd, proc->argv);
					
					if (exec_result < 0) {  //Error checking
						printf(EXEC_ERR, proc->cmd);
						free_job(job);  
						free(line);
						validate_input(NULL);  
						exit(EXIT_FAILURE);
					}
				}

				else {
            		// Parent process
            		close(pipe_fd[0]);  // Close unused read end
            		close(pipe_fd[1]);  // Close unused write end

					wait(NULL);
					wait(NULL);
					
					free(line);
					free_job(job);

					continue;
				}

			}



        	// As the parent, wait for the foreground job to finish
			if(job->bg == false){
				wait_result = waitpid(pid, &exit_status, 0);
				if (wait_result < 0) {
					printf(WAIT_ERR);
					exit(EXIT_FAILURE);
				}
				free(line);
				free_job(job);
			}
 
		}

	}

    	// calling validate_input with NULL will free the memory it has allocated
    	validate_input(NULL);

#ifndef GS
	fclose(rl_outstream);
#endif
	return 0;
}


// return -1 if first_arg < second_arg
// return  0 if first_arg = second_arg
// return  1 if first_arg > second_arg

int time_comparator(const void* data1, const void* data2){

	time_t firstTime = ((bgentry_t*)data1)->seconds;
	time_t secondTime = ((bgentry_t*)data2)->seconds;

	if (firstTime < secondTime) {
        return -1;
    }
	else if (firstTime > secondTime) {
       return 1;
    }
    return 0;
}

void bgentryPrinter(void* data, void* file) {
    FILE* fp = (FILE*)file;
	bgentry_t* bgentry = (bgentry_t*)data;
    fprintf(fp, "PID: %d, Time: %ld seconds\n", bgentry->pid, (long)bgentry->seconds);
}

void history_printer(void* data, void* file) {
    FILE* fp = (FILE*)file;
    char* stringValue = (char*)data;
    fprintf(fp, "%s", stringValue);
}


void sigchld_handler(int sig) {
	reap = true;
}


char* FindProcessByPID(list_t* list, pid_t target_pid) {
    if (list == NULL || list->head == NULL) {
        return NULL;  // Return NULL if the list is empty or not initialized
    }

    node_t* current = list->head;
    int index = 0;

    while (current != NULL) {
        bgentry_t* current_entry = (bgentry_t*)current->data;

        if (current_entry->pid == target_pid) {
			char* line = strdup(current_entry->job->line);  // Make a copy of the line
            RemoveByIndex(list, index);  // Remove the node by index
            return line;  // Return the line stored in the found entry
        }
        current = current->next;
        index++;
    }

    return NULL;  // Return NULL if the process with the specified PID is not found
} 


void sigusr2_handler(int sig) {
	
	fprintf(stderr, "Num of Background processes: %d\n", list->length);
}
