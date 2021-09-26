#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <signal.h>
#include <unistd.h>
#include <wait.h>

#include "config.h"
#include "license.h"

static int num_children;
static pid_t children[MAX_PROCESSES]; 

void help() {
	printf("Runsim help.\n");
	printf("\n");
	printf("n\tNumber of processes to run. (Required)\n");
	printf("[-h]\tShow this help dialogue.\n");
	printf("\n");
}

void cleanup() {
	return;
}

void signal_handler(int signum) {
	// Issue messages	
	if (signum == SIGINT) {
		fprintf(stderr, "Recieved SIGINT signal interrupt, terminating children.\n");
	}
	else if (signum == SIGALRM) {
		fprintf(stderr, "Process execution timeout. Failed to finish in %d seconds.\n", MAX_TIMEOUT);
	}

	// Kill all children processes back to front
	while (num_children > 0) {		
		while (kill(children[num_children - 1], SIGTERM) != 0) {
			waitpid(-1, NULL, WNOHANG);
		} 
		children[num_children--] = 0;
	}

	//TODO: Cleanup memory
	if (signum == SIGINT) exit(EXIT_SUCCESS);
	if (signum == SIGALRM) exit(EXIT_FAILURE);
}

// Issue a execl call
int docommand(char* command) {
	// TODO: getlicense 
	char* program = strtok(command, " ");
	char* cmd1 = strtok(NULL, " ");
	char* cmd2 = strtok(NULL, " ");
	return execl(program, program, cmd1, cmd2, NULL);
}

int main(int argc, char** argv) {
	int option;
	int num_apps;
	char* exe_name = argv[0];
	
	// Process args
	while ((option = getopt(argc, argv, "h")) != -1) {
		switch(option) {
			case 'h':
				help();
				exit(EXIT_SUCCESS);
			case '?':
				// Let getopt handle error msg
				exit(EXIT_FAILURE);
		}
	}
	
	// process non-flag options (number of processes)
	do {
		// Make sure we got a num processes specified
		if (optind >= argc) {
			errno = EINVAL;
			fprintf(stderr, "%s: ", exe_name);
			perror("Did not get number of processes as arg");
			exit(EXIT_FAILURE);
		}
		
		num_apps = atoi(argv[optind++]);
		if (num_apps > MAX_PROCESSES) {
			errno= EINVAL;
			fprintf(stderr, "%s: ", exe_name);
			perror("Too many processes requested. Falling back to max.");
			num_apps = MAX_PROCESSES;
		}
		
		// error if got more than one option
		if (optind < argc) {
			errno = EINVAL;
			fprintf(stderr, "%s: ", exe_name);
			perror("Unkown option");
			exit(EXIT_FAILURE);
		}
		
		// error if got invalid num_apps
		if (num_apps == 0) {
			errno = EINVAL;
			fprintf(stderr, "%s: ", exe_name);
			perror("Passed invalid integer for number of processes");
			exit(EXIT_FAILURE);
		}
	} while (optind < argc);
	
	//TODO: initlicense

	// Setup signal handlers
	signal(SIGINT, signal_handler);
	signal(SIGALRM, signal_handler);

	char input_buffer[MAX_CANON]; // Setup a buffer to read in text
	
	// Read in MAX_CANON chars into buffer until end
	while (fgets(input_buffer, MAX_CANON, stdin) != NULL) {
		// Get new size (in characters) of the string
		size_t input_size = strlen(input_buffer) + 1;
		// Allocate memory for this string
		char* input_text = malloc(input_size * sizeof(char));
		if (input_text == NULL) {
			fprintf(stderr, "%s: ", exe_name);
			perror("Failed to allocate memory for input");
			exit(EXIT_FAILURE);
		}
		
		// Copy buffer characters to string	
		strncpy(input_text, input_buffer, input_size);
	
		// Fork children and have children run docommand
		pid_t pid;
		if (num_children < num_apps) {
			pid = fork();
			if (pid == 0) {
				// We are a child, execute command
				if (docommand(input_text) == -1) {
					fprintf(stderr, "%s: ", exe_name);
					perror("Failed to execute child process");
					exit(EXIT_FAILURE);
				}
			}
			else {
				// We are parent, append this pid to children
				children[num_children++] = pid;
			}
		}
	}
	// Terminate if children do not finish in timeout time	
	alarm(MAX_TIMEOUT);

	// TODO: Remove shared memory

}
