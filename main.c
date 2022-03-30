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
#include <signal.h>

#define HIST_MAX 10

pid_t foreground;

char *COMMAND;

struct Job {
    int number;
    pid_t id;
    char *status;
    char *cmd_line;
};

struct Job jobs[100];
int job_index = 1;
int job_tracker = 0;

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
    bool amp = false;
    while (token != NULL) {
        token = strtok(NULL, " \t");
        tokens[index] = token;
        index++;
    }

    int i = 0;
    while(tokens[i] != NULL) {
        if (!strcmp(tokens[i], "&")) {
            amp = true;
        }
        i++;
    }

    if (amp) {
        tokens[i-1] = NULL;
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
        if (chdir(args[1]) < 0) {
            fprintf(stderr, "cd: %s: No such file or directory\n", args[1]);
        }
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
    
    int fildes[2];
	pid_t pid;
	int fdd = 0;				

    // while loop to iterate through all commands
	while (*cmd != NULL) {

        // create pipe and process
		pipe(fildes);				
		if ((pid = fork()) == -1) {
			perror("fork");
			exit(1);
		}
		else if (pid == 0) {
            // Child process
			dup2(fdd, 0);
			if (*(cmd + 1) != NULL) {
				dup2(fildes[1], 1);
			}
			close(fildes[0]);
			execvp((*cmd)[0], *cmd);
			exit(1);
		}
		else {
			wait(NULL); 		
			close(fildes[1]);
			fdd = fildes[0];
			cmd++;
		}
	}

}

int execute_pipe_background(char ***cmd) {
    int fildes[2];
	pid_t pid;
	int fdd = 0;				

    // while loop to iterate through all commands
	while (*cmd != NULL) {

        // create pipe and process
		pipe(fildes);				
		if ((pid = fork()) == -1) {
			perror("fork");
			exit(1);
		}
		else if (pid == 0) {
            // Child process
			dup2(fdd, 0);
			if (*(cmd + 1) != NULL) {
				dup2(fildes[1], 1);
			}
			close(fildes[0]);
			execvp((*cmd)[0], *cmd);
			exit(1);
		}
		else { 		
			close(fildes[1]);
			fdd = fildes[0];
			cmd++;
		}
	}
}

int execute_standard(char **args, char *line) {
    pid_t pid, wpid;
    int status;

    // Create child process and execute command
    pid = fork();
    foreground = pid;
    COMMAND = strdup(line);
    if (pid == 0) {
        // We are in child process
        if (execvp(args[0], args) == -1) {
            printf("Didn't run correctly\n");
            exit(1);
        }
    } else if (pid < 0) {
        // Error forking
        printf("Fork error");
    } else {
        // Wait for parent process
        wpid = waitpid(pid, NULL, WUNTRACED);
        
    }

    return 1;
}

pid_t execute_background(char **args, char *line) {
    pid_t pid;
    int status;

    // Create child process and execute
    pid = fork();
    jobs[job_index].id = pid;
    jobs[job_index].cmd_line = strdup(line);
    jobs[job_index].number = job_index;
    job_index++;
    if (pid == 0) {
        // We are in child process
        if (execvp(args[0], args) == -1) {
            printf("Didn't run correctly\n");
            exit(1);
        }
    } else if (pid < 0) {
        // Error forking
        printf("Fork error");
    } else {
        waitpid(pid, NULL, WNOHANG);
        printf("[%d]    %d\n", job_index - 1, pid);
    }

    return pid;
}

char *check_status(pid_t p_id) {
    char id[100];
    sprintf(id, "%d", p_id);
    char out[15];
    
    char path[100] = "/proc/"; 
    strcat(path, id);
    strcat(path, "/status");
    
    int lineNumber = 3;
    FILE *file = fopen(path, "r");
    int count = 0;
    if (file != NULL) {
        char line[100];
        while(fgets(line, sizeof line, file) != NULL) {
            if (count == 2) {
                int index1, index2;
                for (int i = 0; i < strlen(line); i++) {
                    if (line[i] == '(') {
                        index1 = i;
                    }
                    if (line[i] == ')') {
                        index2 = i;
                    }
                }

                memcpy(out, &line[index1+1], index2-index1);
                out[index2-index1-1] = '\0';
                return strdup(out);
            } else {
                count++;
            }
        }
        fclose(file);
    } else {
        return NULL;
    }

}

void sighandler(int sig_num) {
    
    // Reset signal to catch Ctrl-Z
    signal(SIGTSTP, sighandler);
    kill (foreground, SIGSTOP);

    // Store job
    jobs[job_index].id = foreground;
    jobs[job_index].status = "stopped";
    jobs[job_index].cmd_line = COMMAND;
    job_index++;
    printf("[%d] %d\n", job_index-1, foreground);

}

int main(int argc, char **argv) {

    bool RUNNING = true;
    char *line;
    char **args;
    int status = 0;

    int num_pipes = 0;
    int hist_index = 0;
    char **hist = malloc(100 * sizeof(char*)); 

    bool background;
    pid_t pid;

    signal(SIGTSTP, sighandler);

    // Get current directory
    char cwd[PATH_MAX];
    getcwd(cwd, sizeof(cwd));

    printf("NASH - Nilay's Shell\n");
    printf("Use the 'help' command to get started\n");

    // Main loop
    while (RUNNING) {

        int k = 1;
        while(jobs[k].id != 0) {
            if (waitpid(jobs[k].id, NULL, WNOHANG) > 0) {
                jobs[k].status = "Done";
                printf("[%d] <%s> %s\n", k, jobs[k].status, jobs[k].cmd_line);
                job_tracker++;
            }
            k++;
        }

        background = false;
        print_prompt();

        line = get_input();
        if (*line == '\0') {
            continue;
        }

        hist[hist_index] = line;
        hist_index++;

        // check for pipes and ampersand
        for (int i = 0; line[i] != '\0'; i++) {
            if (line[i] == '|' && line[i-1] != '|' && line[i+1] != '|') {
                num_pipes++;
            }

            if (line[i] == '&' && line[i-1] != '&' && line[i+1] != '&') {
                background = true;
            }
        }

        if (background) {

            if (num_pipes) {
                //piping
                execute_pipe_background(init_pipe_cmd(line));
                num_pipes = 0;
            } else {
                // Run normal command in the background
                args = parse_input(line);

                if (!strcmp(args[0], "cd")) {
                    status = cd_command(args, cwd);

                } else if (!strcmp(args[0], "exit")) {
                    break;

                } else if (!strcmp(args[0], "help")) {
                    help_command();

                } else if (!strcmp(args[0], "history") || !strcmp(args[0], "h")) {
                    printf("Please execute history without the &\n");

                } else if (!strcmp(args[0], "jobs")) {
                    int j = 1;
                    while(jobs[j].status != NULL) {
                        printf("[%d] <%s> %s\n", j, jobs[j].status, jobs[j].cmd_line);
                        j++;
                    } 
                } else {
                    // standard command
                    pid = execute_background(args, line);
                }
            }

        } else { // Not background
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

                } else if (!strcmp(args[0], "fg")) {
                    if (args[1] == NULL) {
                        kill(foreground, SIGCONT);
                    } else {
                        kill(jobs[atoi(args[1])].id, SIGCONT);
                    }

                } else if (!strcmp(args[0], "bg")) {
                    if (args[1] == NULL) {
                        kill(jobs[job_index-1].id, SIGCONT);
                    } else {
                        kill(jobs[atoi(args[1])].id, SIGCONT);
                    }
                } else if (!strcmp(args[0], "kill")) {
                    if (args[1] == NULL) {
                        kill(foreground, SIGKILL);
                    } else {
                        kill(jobs[atoi(args[1])].id, SIGKILL);
                    }

                } else if (!strcmp(args[0], "jobs")) {
                    int j = 1;
                    while(jobs[j].status != NULL) {
                        printf("[%d] <%s> %s\n", j, jobs[j].status, jobs[j].cmd_line);
                        j++;
                    }

                } else if (!strcmp(args[0], "history") || !strcmp(args[0], "h")) {
                    if (history_handler(hist, hist_index, args)) {
                        char *hist_line = hist[atoi(args[1])-1];
                        char **hist_args = parse_input(hist_line);
                        int hist_pipes = 0;
                        bool hist_background = false;

                        for (int i = 0; hist_line[i] != '\0'; i++) {
                            if (hist_line[i] == '|' && hist_line[i-1] != '|' && hist_line[i+1] != '|') {
                                hist_pipes++;
                            }

                            if (line[i] == '&' && line[i-1] != '&' && line[i+1] != '&') {
                                hist_background = true;
                            }
                        }

                        if (hist_background) {
                            
                            if (hist_pipes) {
                                // hist background piping
                                hist_line = hist[atoi(args[1]-1)];
                                execute_pipe_background(init_pipe_cmd(hist_line));
                            } else {

                                hist_line = hist[atoi(args[1])-1];
                                hist_args = parse_input(hist_line);

                                if (!strcmp(hist_args[0], "cd")) {
                                    status = cd_command(hist_args, cwd);

                                } else if (!strcmp(hist_args[0], "exit")) {
                                    exit(0);

                                } else if (!strcmp(hist_args[0], "help")) {
                                    help_command();

                                } else {
                                    execute_background(hist_args, hist_line);
                                }
                            }
                        } else {

                            if (hist_pipes) {
                                hist_line = hist[atoi(args[1]-1)];
                                execute_pipe(init_pipe_cmd(hist_line));
                            } else {

                                hist_line = hist[atoi(args[1])-1];
                                hist_args = parse_input(hist_line);

                                if (!strcmp(hist_args[0], "cd")) {
                                    status = cd_command(hist_args, cwd);

                                } else if (!strcmp(hist_args[0], "exit")) {
                                    exit(0);

                                } else if (!strcmp(hist_args[0], "help")) {
                                    help_command();

                                } else {
                                    status = execute_standard(hist_args, hist_line);
                                }
                            }
                        }

                    }

                } else {
                    status  = execute_standard(args, line);
                }

                free(args);
            }
        }

        int j = 1;
        
        while (jobs[j].id > 0) {
            jobs[j].status = check_status(jobs[j].id);
            j++;
        }

        if (job_tracker == job_index - 1) {
            int l = 1;
            while(l < 100) {
                jobs[l].id = 0;
                jobs[l].number = 0;
                jobs[l].status = NULL;
                jobs[l].cmd_line = NULL;
                l++;
            }
            job_index = 1;
        }
             
    }

    return EXIT_SUCCESS;
}