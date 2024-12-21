

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <ctype.h>
#include <stdbool.h>

#define MAX_CHARACTERS 1024
#define MAX_ARGS 4

typedef struct
{
    char key[MAX_CHARACTERS];
    char value[MAX_CHARACTERS];
    int flag_character;
} Alias;

// func
char **split_command(char *command, char *arr[], int *token_count);
int execute_command(char *command, char *arr[], int *token_count);
void execute(char *command, int *token_count, int *args, int *num_alias, int *flag);
int is_alias_or_unalias(const char *str);
void remove_alias(Alias aliases[], int *num_alias, char *input, int *args);
void add_alias(Alias aliases[], int *count, char *input, int *args);
void execute_script(const char *filename, Alias aliases[], int *args, int *num_alias, int *num_args_apostrophes, int *num_script, int *flag);
bool check_format(const char *str);
int count_words(const char *str);
void removeSpaces(char *str);
bool check_apostrophes(char *str);

bool check_apostrophes(char *str)
{
    int double_quotes_count = 0;
    int single_quotes_count = 0;

    while (*str)
    {
        if (*str == '"')
        {
            double_quotes_count++;
        }
        else if (*str == '\'')
        {
            single_quotes_count++;
        }
        str++;
    }

    if (double_quotes_count > 0 || single_quotes_count > 2)
    {
        return true;
    }
    return false;
}

bool check_format(const char *str)
{
    const char *alias = "alias";
    int alias_len = strlen(alias);

    // Skip leading spaces
    while (*str && isspace(*str))
        str++;

    // Check if the string is exactly "alias" with or without spaces
    const char *temp_str = str;
    if (strncmp(temp_str, alias, alias_len) == 0)
    {
        temp_str += alias_len;
        while (*temp_str && isspace(*temp_str))
            temp_str++;
        if (*temp_str == '\0')
        {
            return true;
        }
    }

    // Check if the string starts with "alias "
    const char *alias_full = "alias ";
    int alias_full_len = strlen(alias_full);
    if (strncmp(str, alias_full, alias_full_len) != 0)
    {
        return false;
    }
    str += alias_full_len;

    // Skip leading spaces
    while (*str && isspace(*str))
        str++;

    // Check if <key> is a valid key (alphanumeric and/or underscores)
    while (*str && (isalnum(*str)))
    {
        str++;
    }

    // Skip spaces before '='
    while (*str && isspace(*str))
        str++;
    if (*str != '=')
    {
        return false;
    }
    str++;

    // Skip spaces after '='
    while (*str && isspace(*str))
        str++;
    if (*str != '\'')
    {
        return false;
    }
    str++;

    // Check if <com> is inside single quotes
    while (*str && *str != '\'')
    {
        str++;
    }
    if (*str != '\'')
    {
        return false;
    }
    str++;

    // Skip trailing spaces
    while (*str && isspace(*str))
        str++;

    // Ensure the end of the string is reached
    if (*str != '\0')
    {
        return false;
    }

    return true;
}

int count_words(const char *str)
{
    int word_count = 0;
    int in_word = 0; // Flag to track if we're currently in a word

    for (const char *c = str; *c != '\0'; c++)
    {
        if ((*c) == '\"')
        {
            c++;
            word_count++;

            while (*c != '\"')
            {
                c++;
                in_word = 1;
            }
        }
        if (isalnum(*c))
        {
            // If we encounter an alphanumeric character or underscore, we're in a word
            if (!in_word)
            {
                word_count++;
                in_word = 1;
            }
        }
        else if (*c == '=')
        {
            // If we encounter '=', we do not count it as a word and ensure we're not in a word
            in_word = 0;
        }

        else if (*c == '\'')
        {
            // If we encounter '=', we do not count it as a word and ensure we're not in a word
            in_word = 0;
        }
        else
        {
            // For any other non-alphanumeric character, we're not in a word
            in_word = 0;
        }
    }
    return word_count;
}

void removeSpaces(char *str)
{
    int len = strlen(str);
    int i, j;

    // i tracks the original string, j tracks the new position in the string without spaces
    for (i = 0, j = 0; i < len; i++)
    {
        if (!isspace((unsigned char)str[i]))
        {
            str[j++] = str[i];
        }
    }
    str[j] = '\0'; // Null-terminate the modified string
}
char **split_command(char *command, char *arr[], int *token_count)
{
    char *token;

    char *original_command = strdup(command); // Duplicate the original command
    token = strtok(command, " =\"\'");
    arr[0] = token;
    *token_count = 1;

    // Handle non-echo commands
    while ((token = strtok(NULL, " =\"\'")) != NULL)
    {
        if (*token_count < MAX_ARGS)
        { // Ensure not to exceed array bounds
            arr[*token_count] = token;
            (*token_count)++;
        }
        else
        {
            fprintf(stderr, "Error: Too many arguments. Maximum allowed is 4.\n");
            break;
        }
    }

    arr[*token_count] = NULL; // Null-terminate the array
    free(original_command);   // Free the duplicated command
    return arr;
}

int execute_command(char *command, char *arr[], int *token_count)
{
    char **arg = split_command(command, arr, token_count);
    int secc = execvp(arg[0], arg);
    if (secc == -1)
    {

        perror("execvp");
        exit(errno); // Exit with error number
    }

    // This line will never be reached if execvp is successful
    return 0;
}

void execute(char *command, int *token_count, int *args, int *num_alias, int *flag)
{
    int num = 0;
    pid_t pid = fork();
    if (pid == 0)
    {
        char *tokens[MAX_CHARACTERS] = {NULL}; // Initialize tokens array to NULL
        num = execute_command(command, tokens, token_count);
        exit(num); // Ensure child process exits after execution
    }
    else if (pid < 0)
    {
        perror("fork");
        exit(1);
    }
    else
    {
        // Wait for child process to finish
        int status;
        waitpid(pid, &status, 0);

        if (WIFEXITED(status))
        {
            int exit_status = WEXITSTATUS(status);
            if (exit_status == 0)
            {
                //                printf("Command executed successfully.\n");
                (*args)++;
            }
            else
            {
                (*flag) = 0;
                printf("Command failed with exit status: %d\n", exit_status);
            }
        }
        else
        {
            (*flag) = 0;
            //            printf("Child process did not exit normally.\n");
            perror("Child process did not exit normally\n");
        }
    }
}

// Function to parse the input and store the key-value pair in the array
void add_alias(Alias aliases[], int *count, char *input, int *args)
{

    //    delete_character(input);
    char *equal_sign = strchr(input, '=');
    if (equal_sign == NULL)
    {
        for (int i = 0; i < *count; i++)
        {
            printf("alias %s = %s\n", aliases[i].key, aliases[i].value);
        }
        (*args)++;
        return;
    }

    int key_length = equal_sign - input - 6; // subtracting 6 to account for "alias " prefix
    int value_length = strlen(equal_sign + 1);

    if (value_length <= 0 || key_length <= 0)
    {
        printf("Invalid format. Use alias <str>='<commend>'\n");
        return;
    }

    char value[MAX_CHARACTERS];
    char key[MAX_CHARACTERS];

    strncpy(key, input + 6, key_length);
    key[key_length] = '\0';

    strncpy(value, equal_sign + 1, value_length);
    value[value_length] = '\0';
    removeSpaces(key);
    // Check if the key already exists
    for (int i = 0; i < *count; i++)
    {
        if (strcmp(aliases[i].key, key) == 0)
        {
            // Key exists, overwrite the value
            strncpy(aliases[i].value, value, MAX_CHARACTERS);
            aliases[i].value[MAX_CHARACTERS - 1] = '\0'; // Ensure null-termination
            (*args)++;
            printf("Alias updated: %s=%s\n", key, value);
            return;
        }
    }
    // Key does not exist, add new alias
    removeSpaces(key);
    bool apo = check_apostrophes(value);
    strncpy(aliases[*count].key, key, MAX_CHARACTERS);
    aliases[*count].key[MAX_CHARACTERS - 1] = '\0'; // Ensure null-termination

    strncpy(aliases[*count].value, value, MAX_CHARACTERS);
    aliases[*count].value[MAX_CHARACTERS - 1] = '\0'; // Ensure null-termination
                                                      //    if (strstr(value, "\"") != NULL || strstr(value, "\'") != NULL){
    if (apo)
    {
        aliases[*count].flag_character = 1;
    }
    else
    {
        aliases[*count].flag_character = 0;
    }
    (*args)++;
    (*count)++;
    printf("Alias added: %s = %s\n", key, value);
}
void remove_alias(Alias aliases[], int *num_alias, char *input, int *args)
{

    char key[MAX_CHARACTERS];

    int space_index = strcspn(input, " ");
    if (space_index == strlen(input))
    {
        strcpy(key, input);
    }
    else
    {
        // Copy characters from the space onwards (excluding the space itself)
        strcpy(key, input + space_index + 1);
    }
    for (int i = 0; i < *num_alias; i++)
    {
        if (strcmp(aliases[i].key, key) == 0)
        {
            // Found the alias to delete
            for (int j = i; j < *num_alias - 1; j++)
            {
                aliases[j] = aliases[j + 1];
            }
            (*num_alias)--;
            (*args)++;
            printf("Alias deleted: %s\n", key);
            return;
        }
    }
}
int is_alias_or_unalias(const char *str)
{
    // Check if the string is empty or starts with whitespace
    if (str == NULL || *str == '\0' || isspace(*str))
    {
        return 0; // Not a valid command
    }

    // Compare the first 5 characters (case-insensitive)
    int cmp = strncasecmp(str, "alias", 5);
    if (cmp == 0)
    {
        return 1; // It's "alias"
    }

    cmp = strncasecmp(str, "unalias", 7);
    if (cmp == 0)
    {
        return 2; // It's "unalias"
    }

    return 0; // Not "alias" or "unalias"
}
void execute_script(const char *filename, Alias aliases[], int *args, int *num_alias, int *num_args_apostrophes, int *num_script, int *flag)
{
    int num_args_apostrophes_2 = 0;
    FILE *file = fopen(filename, "r");
    char command[MAX_CHARACTERS];

    int token_count = 0;

    if (!file)
    {
        (*args)--;
        perror("fopen");
        return;
    }
    fgets(command, sizeof(command), file); // Read the first line
    int len = strlen(command);
    if (command[len - 1] == '\n')
    {
        command[len - 1] = '\0';
    }
    if (strcmp(command, "#!/bin/bash") != 0)
    {
        fprintf(stderr, "Error: First line of the file is not '#!/bin/bash'\n");
        fclose(file);
        exit(1); // Exit with error code 1
    }
    else
    {
        (*num_script)++;
    }

    while (fgets(command, sizeof(command), file) != NULL)
    {
        (*num_script)++;
        command[strcspn(command, "\n")] = 0; // Remove newline character

        int argument = count_words(command);
        if (argument > MAX_ARGS)
        {
            perror("ERR: Too many argument");
            continue;
        }

        int type = is_alias_or_unalias(command);
        if (strstr(command, "\"") != NULL || strstr(command, "\'") != NULL)
        {
            (*num_args_apostrophes)++;
            num_args_apostrophes_2 = 1;
        }

        if (type == 1)
        {
            bool f = check_format(command);
            if (f)
                add_alias(aliases, num_alias, command, args);
            else
                printf("invalid format\n");
            continue;
        }
        else if (type == 2)
        {
            remove_alias(aliases, num_alias, command, args);
        }
        else if (strcmp(command, "") == 0)
        {
            //            (*num_script)++;
            continue;
        }
        else if (strcmp(command, "exit_shell") == 0)
        {
            //            printf("Exiting shell.\n");
            printf("%d\n", *num_args_apostrophes);
            exit(1);
        }
        else
        {

            for (int i = 0; i < *num_alias; i++)
            {
                if (strcmp(aliases[i].key, command) == 0)
                {
                    strncpy(command, aliases[i].value, MAX_CHARACTERS);
                    command[MAX_CHARACTERS - 1] = '\0'; // Ensure null-termination

                    if (aliases[i].flag_character == 1)
                    {
                        (*num_args_apostrophes)++;
                    }
                }
            }
            execute(command, &token_count, args, num_alias, flag);
            if (*flag == 0 && num_args_apostrophes_2 == 1)
            {
                (*num_args_apostrophes)--;
                *flag = 1;
                num_args_apostrophes_2 = 0;
            }
        }
    }
    //    printf(" %d %d",flag,2)
    fclose(file);
}

int main()
{
    int args = 0;
    int num_alias = 0;
    int num_scripts = 0;
    int num_args_apostrophes = 0;
    int token_count = 0;
    char command[MAX_CHARACTERS];
    Alias aliases[MAX_CHARACTERS];
    int status = 1;
    int flag = 1;
    int num_args_apostrophes_2 = 0;

    while (status)
    {
        memset(command, 0, sizeof(command)); // Correct size for memset
        command[strcspn(command, "\n")] = 0;
        printf("#cmd:%d|#alias:%d|#script lines:%d>", args, num_alias, num_scripts);

        if (fgets(command, sizeof(command), stdin) == NULL)
        { // Check fgets result
            printf("Error reading command.\n");
            continue;
        }
        int argument = count_words(command);
        if (argument > MAX_ARGS)
        {
            perror("ERR: Too many argument");
            continue;
        }
        int type = is_alias_or_unalias(command);

        size_t len = strlen(command);
        if (len > 0 && command[len - 1] == '\n')
        {
            command[len - 1] = '\0';
        }

        if (strstr(command, "\"") != NULL || strstr(command, "\'") != NULL)
        {
            num_args_apostrophes++;
            num_args_apostrophes_2 = 1;
        }

        if (type == 1)
        {
            bool f = check_format(command);
            if (f)
                add_alias(aliases, &num_alias, command, &args);
            else
                printf("invalid format\n");
            continue;
        }
        else if (type == 2)
        {

            remove_alias(aliases, &num_alias, command, &args);
            continue;
        }
        else if (strncmp(command, "source ", 7) == 0)
        {
            char *filename = command + 7;
            args++;
            execute_script(filename, aliases, &args, &num_alias, &num_args_apostrophes, &num_scripts, &flag);
        }
        else if (strcmp(command, "exit_shell") == 0)
        {
            //            printf("Exiting shell.\n");
            printf("%d\n", num_args_apostrophes);
            status = 0;
        }
        else
        {
            for (int i = 0; i < num_alias; i++)
            {
                if (strcmp(aliases[i].key, command) == 0)
                {
                    strncpy(command, aliases[i].value, MAX_CHARACTERS);
                    command[MAX_CHARACTERS - 1] = '\0'; // Ensure null-termination
                                                        //                    printf("%d ------->",aliases[i].flag_character);
                    if (aliases[i].flag_character == 1)
                    {
                        num_args_apostrophes++;
                    }
                }
            }
            execute(command, &token_count, &args, &num_alias, &flag);
            if (flag == 0 && num_args_apostrophes_2 == 1)
            {
                num_args_apostrophes--;
                flag = 1;
                num_args_apostrophes_2 = 0;
            }
        }
    }
    return 0;
}