#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

void print_prompt() {
    printf("# ");
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

    int buff_size = 100;
    int index = 0;

    char **tokens = malloc(buff_size * sizeof(char*));
    char *token;

    if (!tokens) {
        fprintf(stderr, "Allocation error");
    }

    token = strtok(line, " \t");
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

int cd_command(char **args) {
    
    if (args[1] == NULL) {
        printf("Expected argument after cd\n");
    } else {
        chdir(args[1]);
    }
    return 1;
}

int execute(char **args) {
    pid_t pid, wpid;
    int status;

    pid = fork();
    if (pid == 0) {
        // We are in child process
        if (execvp(args[0], args) == -1) {
            printf("Didn't run correctly\n");
        }
    } else if (pid < 0) {
        printf("Fork error");
    } else {
        // Wait for parent process
        wpid = waitpid(pid, NULL, 0);
    }

    return 1;
}

int main(int argc, char **argv)
{

    bool RUNNING = true;
    char *line;
    char **args;
    int status = 0;
    printf("NASH - Nilay's Shell\n");

    while (RUNNING) {
        
        print_prompt();

        line = get_input();
        args = parse_input(line);

        if (!strcmp(args[0], "cd")) {
            printf("Running cd command\n");
            status = cd_command(args);
        } else if (!strcmp(tolower(args[0]), "exit")) {
            exit(0);
        } else {
            status  = execute(args);
        }

        free(line);
        free(args);        

    }
  return EXIT_SUCCESS;
}