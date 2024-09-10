#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <dirent.h>
#include <sys/wait.h>
#include <pwd.h>
#include <sys/types.h>

#define HISTORY_SIZE 20

void show_history(char **history, int size) {
    for (int i = 0; i < size; i++) {
        printf("%d %s\n", i + 1, history[i]);
    }
}

void add_history(char **history, int *size, char *command) {
    if (*size < HISTORY_SIZE) {
        history[*size] = strdup(command);
        (*size)++;
    } else {
        free(history[0]);
        for (int i = 1; i < HISTORY_SIZE; i++) {
            history[i - 1] = history[i];
        }
        history[HISTORY_SIZE - 1] = strdup(command);
    }
}

void execute_command(char *command) {
    if (!command) {
        printf("NULL\n");
        exit(1);
    }

    if (strcmp(command, "exit") == 0) {
        exit(0);
    }

    if (strcmp(command, "pwd") == 0) {
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s\n", cwd);
        } else {
            perror("getcwd error");
        }
        return;
    }

    if (strncmp(command, "cd ", 3) == 0) {
        char *path = command + 3;
        if (chdir(path) != 0) {
            perror("chdir error");
        }
        return;
    }

    pid_t id = fork();
    if (id < 0) {
        perror("fork error");
        exit(1);
    }

    if (id == 0) {
        char *args[256];
        int i = 0;
        char *token = strtok(command, " ");
        while (token != NULL) {
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL;

        if (execvp(args[0], args) == -1) {
            perror("execvp error");
            exit(1);
        }
    } else {
        wait(NULL);
    }
}

void show_help() {
    printf("Available commands:\n");
    printf("  help       : Display this help message\n");
    printf("  history    : Show command history\n");
    printf("  pwd        : Print the current working directory\n");
    printf("  cd <dir>   : Change directory to <dir>\n");
    printf("  setenv <var> <value> : Set or modify an environment variable\n");
    printf("  unsetenv <var>       : Remove an environment variable\n");
    printf("  exit       : Exit the shell\n");
    printf("  chprompt <prompt> : Change the shell prompt\n");
}

void set_env_var(char *command) {
    char *args[256];
    int i = 0;
    char *token = strtok(command, " ");
    while (token != NULL) {
        args[i++] = token;
        token = strtok(NULL, " ");
    }
    args[i] = NULL;

    if (i != 3) {
        printf("Invalid Arguments\n");
        return;
    }

    if (setenv(args[1], args[2], 1) == 0) {
        printf("Environment variable %s set to %s\n", args[1], args[2]);
    } else {
        perror("setenv error");
    }
}

void unset_env_var(char *command) {
    char *args[256];
    int i = 0;
    char *token = strtok(command, " ");
    while (token != NULL) {
        args[i++] = token;
        token = strtok(NULL, " ");
    }
    args[i] = NULL;


    if (i != 2) {
        printf("Invalid Arguments\n");
        return;
    }

    if (unsetenv(args[1]) == 0) {
        printf("Environment variable %s removed\n", args[1]);
    } else {
        perror("unsetenv error");
    }
}

char *expand_variable(char *command) {
    static char expanded_command[1024];
    memset(expanded_command, 0, sizeof(expanded_command));

    char *pos = strchr(command, '$');
    if (pos) {
        strncat(expanded_command, command, pos - command); 
        char *var_name = pos + 1;
        char *env_value = getenv(var_name);
        if (env_value) {
            strcat(expanded_command, env_value);
        }
    } else {
        strcpy(expanded_command, command);
    }

    return expanded_command;
}

int main() {
    char shell_name[128];
    system("clear");
    char command[256];
    int history_count = 0;
    char *history[HISTORY_SIZE] = {NULL};

    strcpy(shell_name, "myshell>> ");

    while (1) {
        printf("%s", shell_name);
        if (fgets(command, sizeof(command), stdin) == NULL) {
            perror("fgets error");
            exit(1);
        }
        command[strcspn(command, "\n")] = '\0'; 

        char *expanded_command = expand_variable(command);

        if (strcmp(expanded_command, "history") == 0) {
            show_history(history, history_count);
            continue;
        } else if (strcmp(expanded_command, "help") == 0) {
            show_help();
            continue;
        } else if (strncmp(expanded_command, "chprompt ", 9) == 0) {
            char *new_prompt = expanded_command + 9;
            if (strlen(new_prompt) > 0) {
                strcpy(shell_name, new_prompt);
            } else {
                printf("Error: No prompt provided.\n");
            }
        } else if (strncmp(expanded_command, "setenv ", 7) == 0) {
            set_env_var(expanded_command);
        } else if (strncmp(expanded_command, "unsetenv ", 9) == 0) {
            unset_env_var(expanded_command);
        } else if (strlen(expanded_command) > 0) {
            add_history(history, &history_count, expanded_command);
            execute_command(expanded_command);
        }
    }

    for (int i = 0; i < history_count; i++) {
        free(history[i]);
    }

    return 0;
}

