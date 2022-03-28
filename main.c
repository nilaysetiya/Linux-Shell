#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <limits.h>
#include <malloc.h>

#define HIST_MAX 10

void print_prompt() {
    printf("> ");
}

char *get_input() {
    
    char *line = NULL;
    size_t buff_size = 0;

    getline (&line, &buff_size, stdin);

    // Replace new line character with null terminator
    if (line[strlen(line)-1] == '\n') {
        line[strlen(line)-1] = '\0';
    }
    
    return line;
}

char **parse_input(char *line) {

    // Function to parse input and return a null terminated array of tokens

    int buff_size = 100;
    int index = 0;

    char **tokens = malloc(buff_size * sizeof(char*));
    char *token;

    if (!tokens) {
        fprintf(stderr, "Allocation error");
    }

    token = strtok(strdup(line), " \t");
    tokens[index] = token;
    index++;

    while (token != NULL) {
        token = strtok(NULL, " \t");
        tokens[index] = token;
        index++;
    }

    tokens[index] = NULL;
    return tokens;

}

int cd_command(char **args, char *cwd) {
    
    if (args[1] == NULL) {
        // Change to home directory
        chdir(cwd);

    } else {
        // Change dir to what user specifies
        chdir(args[1]);
    }
    return 1;
}

void help_command() {
    
    // Display help menu
    printf("Type in a command to get started!\n");
    printf("Use the 'exit' command to exit the shell\n");
    printf("Remember that all comands are case sensitive\n");
    
}

int history_handler(char **hist, int max_index, char **args) {
    
    if (args[1] == NULL) {
        // print out history
        int min_index = 0;
        if (max_index > 9) {
            min_index = max_index - 9;
        }

        for (int i = min_index; i < max_index; i++) {
            printf("%d: %s\n", i + 1, hist[i]);
        }

        return 0;
    } else {

        return 1;  
    } 
}

char ***init_pipe_cmd(char *line) {

    char *cmd = NULL;
    char **args = malloc(100 * sizeof(char*));
    char *input = strdup(line);
    char ***out = malloc(100 * sizeof(char**));
    int index = 0;

    cmd = strtok(strdup(input), "|");

    while (cmd != NULL) {
        args[index++] = cmd;
        cmd = strtok(NULL, "|");
    }
    int j;
    for (j = 0; j < index; j++) {
        out[j] = parse_input(args[j]);
    }
    out[j+1] = NULL;

    return out;

}


int execute_pipe(char ***cmd) {
    
    int fd[2];
	pid_t pid;
	int fdd = 0;				/* Backup */

	while (*cmd != NULL) {
		pipe(fd);				/* Sharing bidiflow */
		if ((pid = fork()) == -1) {
			perror("fork");
			exit(1);
		}
		else if (pid == 0) {
			dup2(fdd, 0);
			if (*(cmd + 1) != NULL) {
				dup2(fd[1], 1);
			}
			close(fd[0]);
			execvp((*cmd)[0], *cmd);
			exit(1);
		}
		else {
			wait(NULL); 		/* Collect childs */
			close(fd[1]);
			fdd = fd[0];
			cmd++;
		}
	}

    
    // char **firstargs = malloc(100 * sizeof(char*));
    // char **secondargs = malloc(100 * sizeof(char*));
    // char *input = strdup(line);
    // char *token1 = strtok(input, "|");
    // char *token2 = strtok(NULL, "|");

    // firstargs = parse_input(token1);
    // secondargs = parse_input(token2);

    // int fd[2];

    // if (pipe(fd) == -1) {
    //     printf("Couldn't create pipe\n");
    //     exit(1);
    // }

    // int pid1 = fork();
    // if (pid1 < 0) {
    //     printf("Fork error\n");
    // }

    // if (pid1 == 0) {
    //     // child process
    //     dup2(fd[1], STDOUT_FILENO);
    //     close(fd[0]);
    //     close(fd[1]);
    //     execvp(firstargs[0], firstargs);
    // } 

    // int pid2 = fork();
    // if (pid2 < 0) {
    //     printf("Fork error\n");
    // }

    // if (pid2 == 0) {
    //     // child process
    //     dup2(fd[0], STDIN_FILENO);
    //     close(fd[0]);
    //     close(fd[1]);
    //     execvp(secondargs[0], secondargs);
    // }

    // close(fd[0]);
    // close(fd[1]);

    // waitpid(pid1, NULL, 0);
    // waitpid(pid2, NULL, 0);

}

int execute_standard(char **args) {
    pid_t pid, wpid;
    int status;

    // Create child process and execute command
    pid = fork();
    if (pid == 0) {
        // We are in child process
        if (execvp(args[0], args) == -1) {
            printf("Didn't run correctly\n");
        }
    } else if (pid < 0) {
        // Error forking
        printf("Fork error");
    } else {
        // Wait for parent process
        wpid = waitpid(pid, NULL, 0);
    }

    return 1;
}

int main(int argc, char **argv) {

    bool RUNNING = true;
    char *line;
    char **args;
    int status = 0;

    int num_pipes = 0;
    int hist_index = 0;
    char **hist = malloc(100 * sizeof(char*)); 

    // Get current directory
    char cwd[PATH_MAX];
    getcwd(cwd, sizeof(cwd));

    printf("NASH - Nilay's Shell\n");
    printf("Use the 'help' command to get started\n");

    // Main loop
    while (RUNNING) {
        
        print_prompt();

        line = get_input();

        hist[hist_index] = line;
        hist_index++;

        for (int i = 0; line[i] != '\0'; i++) {
            if (line[i] == '|' && line[i-1] != '|' && line[i+1] != '|') {
                num_pipes++;
            }
        }

        if (num_pipes) {
            // piping
            execute_pipe(init_pipe_cmd(line));


            num_pipes = 0;

        } else {
            args = parse_input(line);

            if (!strcmp(args[0], "cd")) {
                status = cd_command(args, cwd);

            } else if (!strcmp(args[0], "exit")) {
                break;

            } else if (!strcmp(args[0], "help")) {
                help_command();

            } else if (!strcmp(args[0], "history") || !strcmp(args[0], "h")) {
                if (history_handler(hist, hist_index, args)) {
                    char *hist_line;
                    char **hist_args;

                    hist_line = hist[atoi(args[1])-1];
                    hist_args = parse_input(hist_line);

                    if (!strcmp(hist_args[0], "cd")) {
                        status = cd_command(hist_args, cwd);

                    } else if (!strcmp(hist_args[0], "exit")) {
                        exit(0);

                    } else if (!strcmp(hist_args[0], "help")) {
                        help_command();

                    } else {
                        status = execute_standard(hist_args);
                    }

                }

            } else {
                status  = execute_standard(args);
            }

            free(args);
        }

                
    }

    return EXIT_SUCCESS;
}