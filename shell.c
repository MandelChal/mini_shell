#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <ctype.h>
#include <stdbool.h>
#include <fcntl.h>

#define MAX_CHARACTERS 1024
#define MAX_ARGS 4

typedef struct
{
    char key[MAX_CHARACTERS];
    char value[MAX_CHARACTERS];
    int flag_character;
} Alias;

typedef struct
{
    pid_t pid;
    char cmd[256];
} Job;

Job jobs[MAX_CHARACTERS];
int job_count = 0;
//
// void add_job(pid_t pid, const char *cmd) {
//    if (job_count < MAX_CHARACTERS) {
//        jobs[job_count].pid = pid;
//        strncpy(jobs[job_count].cmd, cmd, 255);
//        jobs[job_count].cmd[255] = '\0';
//        job_count++;
//    } else {
//        printf("Job list is full!\n");
//    }
//}

void remove_job(pid_t pid)
{
    for (int i = 0; i < job_count; i++)
    {
        if (jobs[i].pid == pid)
        {
            for (int j = i; j < job_count - 1; j++)
            {
                jobs[j] = jobs[j + 1];
            }
            job_count--;
            break;
        }
    }
}

void list_jobs()
{
    for (int i = 0; i < job_count; i++)
    {
        printf("[%d] Runinng %s&\n", i + 1, jobs[i].cmd);
    }
}

void handle_child_exit()
{
    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        remove_job(pid);
    }
}

int count_words_p(char *section)
{
    int count = 0;
    char *token = strtok(section, " \t\n");

    while (token != NULL)
    {
        count++;
        token = strtok(NULL, " \t\n");
    }

    return count;
}

void parse_commands_p(char *input, char **commands, int *num_commands)
{
    char *token = strtok(input, "||");
    while (token != NULL)
    {
        commands[(*num_commands)++] = token;
        token = strtok(NULL, "||");
    }
}
void parse_commands2(char *input, char **commands, int *num_commands)
{
    bool in_quotes = false;
    bool in_dashes = false;
    char *start = input;
    char *ptr = NULL;

    while (*ptr)
    {
        if (*ptr == '"' && !in_dashes)
        {
            in_quotes = !in_quotes;
        }
        else if (*ptr == '\'' && !in_quotes)
        {
            in_dashes = !in_dashes;
        }
        else if (*ptr == '&' && !in_quotes && !in_dashes && *(ptr + 1) == '&')
        {
            // Null-terminate the current command
            *ptr = '\0';
            // Add the command to the commands array
            commands[(*num_commands)++] = start;
            // Skip the "&&"
            ptr += 2;
            // Set the new start to the next character
            start = ptr;
            continue;
        }
        ptr++;
    }

    // Add the last command
    commands[(*num_commands)++] = start;
}
int count_words_in_sections(char *input, int type)
{
    char *sections[MAX_CHARACTERS];
    int num_sections = 0;

    // Copy input to preserve original
    char input_copy[1024];
    strncpy(input_copy, input, sizeof(input_copy));
    input_copy[sizeof(input_copy) - 1] = '\0';

    //    printf("%d\n\n",type);
    int word_count;
    for (int i = 0; i < num_sections; ++i)
    {
        // Copy section to preserve original section
        char section_copy[1024];
        strncpy(section_copy, sections[i], sizeof(section_copy));
        section_copy[sizeof(section_copy) - 1] = '\0';

        word_count = count_words_p(section_copy);
        if (word_count > MAX_ARGS)
        {
            perror("ERR: Too many argument");
            return -1;
        }
        //        printf("The section \"%s\" contains %d word(s).\n", sections[i], word_count);
    }

    //    return 0;
    //    printf("\n\n\n---> %d",word_count);
    return word_count;
}

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
        else if (*c == '&' && *(c + 1) == '&')
        {
            // If we encounter '=', we do not count it as a word and ensure we're not in a word
            in_word = 0;
        }
        else if (*c == '|' && *(c + 1) == '|')
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
        // Ensure not to exceed array bounds
        arr[*token_count] = token;
        (*token_count)++;
    }
    arr[*token_count] = NULL; // Null-terminate the array
    free(original_command);   // Free the duplicated command
    return arr;
}
// Helper function to parse the command for 2> redirection
int parse_redirection(char *command, char **cmd, char **error_file)
{

    // Find the position of '2>'
    char *redirection_pos = strstr(command, "2>");
    if (redirection_pos)
    {

        // Split the command into the actual command and the error file part
        *redirection_pos = '\0';
        *cmd = command;
        *error_file = redirection_pos + 2;

        // Trim leading spaces from the error file part
        while (isspace((unsigned char)**error_file))
        {
            (*error_file)++;
        }
        // Trim trailing spaces from the command part
        char *end = *cmd + strlen(*cmd) - 1;
        while (end > *cmd && isspace((unsigned char)*end))
        {
            end--;
        }
        end[1] = '\0';
        return 1;
    }
    else
    {
        *cmd = command;
        *error_file = NULL;
        return 0;
    }
}
int execute_command(char *command, char *arr[], int *token_count, int *argument)
{
    char *cmd = NULL;
    char *error_file = NULL;

    int redirect_stderr = parse_redirection(command, &cmd, &error_file);
    char **arg = split_command(cmd, arr, token_count);
    if (redirect_stderr)
    {

        int fd = open(error_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (fd == -1)
        {
            perror("open");
            return 1;
        }

        if (dup2(fd, STDERR_FILENO) == -1)
        {

            perror("dup2");
            close(fd);
            return 1;
        }
        close(fd);
    }
    execvp(arg[0], arg);
    perror(cmd);
    exit(errno); // Exit with error number

    // This line will never be reached if execvp is successful
    return 0;
}

void execute(char *command, int *token_count, int *args, int *num_alias, int *flag, int background)
{
    int num = 0;
    pid_t pid = fork();
    if (pid == 0)
    {
        char *tokens[MAX_CHARACTERS] = {NULL}; // Initialize tokens array to NULL

        num = execute_command(command, tokens, token_count, args);
        exit(num); // Ensure child process exits after execution
    }
    else if (pid < 0)
    {
        perror("fork");
        exit(1);
    }
    else
    {
        // Parent process
        if (!background)
        {
            // Wait for the child process if it's not running in the background
            int status;
            waitpid(pid, &status, 0);
            if (WIFEXITED(status))
            {
                int exit_status = WEXITSTATUS(status);
                if (exit_status == 0)
                {
                    (*args)++;
                }
                else
                {
                    (*flag) = 0;
                    //                    printf("Command not found\n");
                }
            }
            else
            {
                (*flag) = 0;
            }
        }
        else
        {

            // Add job to the jobs array
            if (job_count < MAX_CHARACTERS)
            {
                jobs[job_count].pid = pid;
                strncpy(jobs[job_count].cmd, command, MAX_CHARACTERS - 1);
                jobs[job_count].cmd[MAX_CHARACTERS - 1] = '\0';
                job_count++;
            }
            else
            {
                printf("Job list full, cannot add more jobs.\n");
            }
            printf("[%d] %d\n", job_count, pid);
        }
    }
}

void trim_trailing_whitespace(char *str)
{
    int length = strlen(str);
    while (length > 0 && (str[length - 1] == ' ' || str[length - 1] == '\t' || str[length - 1] == '\n'))
    {
        str[length - 1] = '\0';
        length--;
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
int contains_unquoted_ampersand(const char *str)
{
    bool in_single_quote = false;
    bool in_double_quote = false;
    bool in_dash_comment = false;
    bool has_and = false;
    bool has_or = false;

    for (int i = 0; str[i] != '\0'; ++i)
    {
        char c = str[i];

        if (c == '\'' && !in_double_quote && !in_dash_comment)
        {
            in_single_quote = !in_single_quote;
        }
        else if (c == '\"' && !in_single_quote && !in_dash_comment)
        {
            in_double_quote = !in_double_quote;
        }
        else if (c == '-' && str[i + 1] == '-' && !in_single_quote && !in_double_quote)
        {
            in_dash_comment = true;
        }
        else if (c == '\n' && in_dash_comment)
        {
            in_dash_comment = false;
        }
        else if (c == '&' && str[i + 1] == '&' && !in_single_quote && !in_double_quote && !in_dash_comment)
        {
            has_and = true;
        }
        else if (c == '|' && str[i + 1] == '|' && !in_single_quote && !in_double_quote && !in_dash_comment)
        {
            has_or = true;
        }
    }

    if (has_and && has_or)
    {
        return 3;
    }
    else if (has_and)
    {
        return 2;
    }
    else if (has_or)
    {
        return 1;
    }

    return 0;
}

void parse_arguments(char *command, char **args)
{
    char *token = strtok(command, " \t\n\"\'");
    int i = 0;
    while (token != NULL)
    {
        args[i++] = token;
        token = strtok(NULL, " \t\n\"\'");
    }
    args[i] = NULL;
}
int execute_command2(char **args, int background, int *flag)
{
    if (strcmp(args[0], "echo") == 0)
    {
        for (int i = 1; args[i] != NULL; i++)
        {
            if (i > 1)
                printf(" ");
            printf("%s", args[i]);
        }
        printf("\n");
        return 0; // Echo always succeeds
    }
    pid_t pid = fork();
    if (pid == 0)
    { // Child process
        execvp(args[0], args);
        perror("execvp failed");

        exit(1);
    }
    else if (pid < 0)
    {
        perror("fork");
        return 1;
    }
    else
    {
        if (!background)
        {
            int status;
            waitpid(pid, &status, 0);
            if (WIFEXITED(status))
            {
                int exit_status = WEXITSTATUS(status);
                if (exit_status == 0)
                {
                    (*args)++;
                }
                else
                {
                    (*flag) = 0;
                    //                   printf("Command not found\n");
                }
            }
            else
            {
                (*flag) = 0;
            }
        }
        else
        {

            // Add job to the jobs array
            if (job_count < MAX_CHARACTERS)
            {
                jobs[job_count].pid = pid;
                strncpy(jobs[job_count].cmd, args[0], MAX_CHARACTERS - 1);
                jobs[job_count].cmd[MAX_CHARACTERS - 1] = '\0';
                job_count++;
            }
            else
            {
                printf("Job list full, cannot add more jobs.\n");
            }
            printf("[%d] %d\n", job_count, pid);
        }
    }
    return 0;
}
int execute_combined_command(char *command, int *argument, int background, int *flag)
{
    char *commands[3] = {NULL, NULL, NULL};
    char *delimiters[2] = {NULL, NULL};
    int num_commands = 0;

    // Parse the command for && and ||
    char *token = strtok(command, " ");
    while (token != NULL)
    {
        if (strcmp(token, "&&") == 0)
        {
            delimiters[num_commands] = "&&";
            num_commands++;
        }
        else if (strcmp(token, "||") == 0)
        {
            delimiters[num_commands] = "||";
            num_commands++;
        }
        else
        {
            if (commands[num_commands] == NULL)
            {
                commands[num_commands] = strdup(token);
            }
            else
            {
                commands[num_commands] = realloc(commands[num_commands], strlen(commands[num_commands]) + strlen(token) + 2);
                strcat(commands[num_commands], " ");
                strcat(commands[num_commands], token);
            }
        }
        token = strtok(NULL, " ");
    }

    int result = 0;
    bool execute_next = true;

    for (int i = 0; i <= num_commands; i++)
    {
        if (execute_next)
        {
            char *args[MAX_ARGS] = {NULL};
            parse_arguments(commands[i], args);
            (*argument)++;
            // Check if the command ends with '&'
            size_t len = strlen(command);
            if (len > 0 && command[len - 1] == '\n')
            {
                command[len - 1] = '\0';
                len--;
            }

            // Check if the command ends with '&'
            size_t len_2 = strlen(command);
            if (len_2 > 0 && command[len_2 - 1] == '\n')
            {
                command[len_2 - 1] = '\0';
                len_2--;
            }

            if (len_2 > 0 && command[len_2 - 1] == '&')
            {
                (*args)++;
                background = 1;
                command[len_2 - 1] = '\0'; // Remove '&' from the command
                len_2--;
            }
            else
            {
                background = 0;
            }

            // Remove trailing whitespace
            while (len_2 > 0 && isspace(command[len_2 - 1]))
            {
                command[len_2 - 1] = '\0';
                len_2--;
            }
            result = execute_command2(args, background, flag);
            if (result == 1)
                (*argument)--;
        }

        if (i < num_commands)
        {
            if (strcmp(delimiters[i], "&&") == 0)
            {
                execute_next = (result == 0);
            }
            else if (strcmp(delimiters[i], "||") == 0)
            {
                execute_next = (result != 0);
            }
        }
    }

    for (int i = 0; i <= num_commands; i++)
    {
        free(commands[i]);
    }

    return result;
}

void execute_script(const char *filename, Alias aliases[], int *args, int *num_alias, int *num_args_apostrophes, int *num_script, int *flag)
{
    char *argument_in[MAX_CHARACTERS];
    int background;
    int type;
    int num_commands;
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
        int swich = contains_unquoted_ampersand(command);
        if (command[0] == '(')
        {
            fflush(stdout);
            command[strcspn(command, "\n")] = '\0';
            parse_and_execute(command);
            continue;
        }
        if (swich == 3)
        {
            //            printf("kkkkkkk\n");
            // Execute combined command with && and ||
            type = 3;
            int four3 = count_words_in_sections(command, type);
            if (four3 == -1)
                continue;
            execute_combined_command(command, args, background, flag);

            continue;
        }
        //        &&
        else if (swich == 2)
        {
            type = 2;
            int four2 = count_words_in_sections(command, type);
            if (four2 == -1)
                continue;
            //            int redirect_stderr = parse_redirection(command, , &error_file);
            parse_commands2(command, argument_in, &num_commands);
            for (int i = 0; i < num_commands; ++i)
            {
                char *args_arry[MAX_CHARACTERS];
                int argument = count_words(argument_in[i]);
                if (argument > MAX_ARGS)
                {
                    perror("ERR: Too many argument");
                    break;
                }
                if (strstr(argument_in[i], "\"") != NULL || strstr(argument_in[i], "\'") != NULL)
                {
                    (*num_args_apostrophes)++;
                    num_args_apostrophes_2 = 1;
                }
                if (strstr(argument_in[i], "jobs") != 0)
                {
                    (*args)++;
                    list_jobs();
                    continue;
                }
                else
                {
                    char *cmd = argument_in[i];
                    trim_trailing_whitespace(cmd);
                    // Check if the command ends with '&'
                    size_t len = strlen(cmd);
                    if (len > 0 && cmd[len - 1] == '\n')
                    {
                        cmd[len - 1] = '\0';
                        len--;
                    }
                    if (len > 0 && cmd[len - 1] == '&')
                    {
                        background = 1;
                        cmd[len - 1] = '\0'; // Remove '&' from the command
                        len--;
                    }
                    else
                    {
                        background = 0;
                    }

                    // Remove trailing whitespace
                    while (len > 0 && isspace(cmd[len - 1]))
                    {
                        cmd[len - 1] = '\0';
                        len--;
                    }
                    parse_arguments(argument_in[i], args_arry);
                    int exec_sucsse = execute_command2(args_arry, background, flag);
                    if (exec_sucsse != 0)
                    {
                        break;
                    }
                    if (exec_sucsse == 0)
                    {
                        (*args)++;
                    }
                }
            }
            num_commands = 0;
            continue;
        }
        //        ||
        else if (swich == 1)
        {
            type = 1;
            int four = count_words_in_sections(command, type);
            if (four == -1)
                continue;
            parse_commands_p(command, argument_in, &num_commands);
            //            for (int i = 0; i < num_commands; ++i) {
            //                printf("                             %s            \n",argument_in[i]);
            //            }
            for (int i = 0; i < num_commands; ++i)
            {

                char *args_arry[MAX_CHARACTERS];
                int argument = count_words(argument_in[i]);
                if (argument > MAX_ARGS)
                {
                    perror("ERR: Too many argument");
                    break;
                }
                if (strstr(argument_in[i], "\"") != NULL || strstr(argument_in[i], "\'") != NULL)
                {
                    (*num_args_apostrophes)++;
                    num_args_apostrophes_2 = 1;
                }
                if (strstr(argument_in[i], "jobs") != 0)
                {
                    (*args)++;
                    list_jobs();
                    break;
                }

                else
                {
                    char *cmd = argument_in[i];
                    trim_trailing_whitespace(cmd);
                    // Check if the command ends with '&'
                    size_t len = strlen(cmd);
                    if (len > 0 && cmd[len - 1] == '\n')
                    {
                        cmd[len - 1] = '\0';
                        len--;
                    }
                    //                    printf("                    %s        cmd__1",cmd);
                    if (len > 0 && cmd[len - 1] == '&')
                    {
                        args++;
                        background = 1;
                        cmd[len - 1] = '\0'; // Remove '&' from the command
                        len--;
                        printf("                    %s        cmd__2", cmd);
                    }
                    else
                    {
                        background = 0;
                    }

                    // Remove trailing whitespace
                    while (len > 0 && isspace(cmd[len - 1]))
                    {
                        cmd[len - 1] = '\0';
                        len--;
                    }
                    //                    args++;
                    parse_arguments(argument_in[i], args_arry);
                    if (execute_command2(args_arry, background, flag) == 0)
                    {
                        (*args)++;
                        break;
                    }
                }
            }
            num_commands = 0;
            continue;
        }

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
        else if (strcmp(command, "jobs") == 0)
        {
            args++;
            list_jobs();
            continue;
        }

        else
        {
            size_t len = strlen(command);
            if (len > 0 && command[len - 1] == '\n')
            {
                command[len - 1] = '\0';
                len--;
            }

            if (len > 0 && command[len - 1] == '&')
            {
                //                (*  args)++;
                background = 1;
                command[len - 1] = '\0'; // Remove '&' from the command
                len--;
            }
            else
            {
                background = 0;
            }

            // Remove trailing whitespace
            while (len > 0 && isspace(command[len - 1]))
            {
                command[len - 1] = '\0';
                len--;
            }
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
            execute(command, &token_count, args, num_alias, flag, background);
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
// Function to check if a string ends with '&'
bool ends_with_ampersand(const char *str)
{
    int len = strlen(str); // Get the length of the string
    if (len > 0 && str[len - 1] == '&')
    {
        return true; // The last character is '&'
    }
    return false; // The last character is not '&'
}
void execute_command_mul(char *cmd)
{
    char *args[MAX_CHARACTERS];
    int i = 0;
    args[i] = strtok(cmd, " ");
    while (args[i] != NULL)
    {
        i++;
        args[i] = strtok(NULL, " ");
    }
    args[i] = NULL;

    execvp(args[0], args);
    perror("execvp failed");
    exit(EXIT_FAILURE);
}

void parse_and_execute(char *input)
{
    char *redirect_out = strstr(input, "2>");
    char *cmd = NULL;
    if (redirect_out)
    {
        *redirect_out = '\0';
        redirect_out += 2;
        while (*redirect_out == ' ')
            redirect_out++;
    }

    if (input[0] == '(' && input[strlen(input) - 1] == ')')
    {
        input[strlen(input) - 1] = '\0';
        cmd = input + 1;
    }
    else
    {
        cmd = input;
    }

    int fd = -1;
    if (redirect_out)
    {
        fd = open(redirect_out, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0)
        {
            perror("open");
            return;
        }
    }

    char *commands[2];
    commands[0] = strtok(cmd, "&&");
    commands[1] = strtok(NULL, "&&");

    int pids[2], status;
    for (int i = 0; i < 2; i++)
    {
        if ((pids[i] = fork()) == 0)
        {
            if (fd >= 0)
            {
                dup2(fd, STDERR_FILENO);
                close(fd);
            }
            execute_command_mul(commands[i]);
        }
        else if (pids[i] < 0)
        {
            perror("fork");
            return;
        }
    }

    for (int i = 0; i < 2; i++)
    {
        waitpid(pids[i], &status, 0);
        if (WEXITSTATUS(status) != 0)
        {
            break;
        }
    }

    if (fd >= 0)
    {
        close(fd);
    }
}

int main()
{
    int background = 0;
    int args = 0;
    int num_alias = 0;
    int num_scripts = 0;
    int num_args_apostrophes = 0;
    int token_count = 0;
    char command[MAX_CHARACTERS];
    char *argument_in[MAX_CHARACTERS];
    int num_commands = 0;
    Alias *aliases = NULL;
    int status = 1;
    int flag = 1;
    int type;
    int num_args_apostrophes_2 = 0;
    // Set up signal handler for terminated child processes
    struct sigaction sa;
    sa.sa_handler = handle_child_exit;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(1);
    }
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
        if (command[0] == '(')
        {
            fflush(stdout);
            command[strcspn(command, "\n")] = '\0';
            parse_and_execute(command);
            continue;
        }
        int swich = contains_unquoted_ampersand(command);
        if (swich == 3)
        {
            //            printf("kkkkkkk\n");
            // Execute combined command with && and ||
            type = 3;
            int four3 = count_words_in_sections(command, type);
            if (four3 == -1)
                continue;
            execute_combined_command(command, &args, background, &flag);

            continue;
        }
        //        &&
        else if (swich == 2)
        {
            type = 2;
            int four2 = count_words_in_sections(command, type);
            if (four2 == -1)
                continue;

            //            int redirect_stderr = parse_redirection(command, , &error_file);
            parse_commands2(command, argument_in, &num_commands);
            for (int i = 0; i < num_commands; ++i)
            {
                char *args_arry[MAX_CHARACTERS];
                int argument = count_words(argument_in[i]);
                if (argument > MAX_ARGS)
                {
                    perror("ERR: Too many argument");
                    break;
                }
                if (strstr(argument_in[i], "\"") != NULL || strstr(argument_in[i], "\'") != NULL)
                {
                    num_args_apostrophes++;
                    num_args_apostrophes_2 = 1;
                }
                if (strstr(argument_in[i], "jobs") != 0)
                {
                    args++;
                    list_jobs();
                    continue;
                }
                else
                {
                    char *cmd = argument_in[i];
                    trim_trailing_whitespace(cmd);
                    // Check if the command ends with '&'
                    size_t len = strlen(cmd);
                    if (len > 0 && cmd[len - 1] == '\n')
                    {
                        cmd[len - 1] = '\0';
                        len--;
                    }
                    if (len > 0 && cmd[len - 1] == '&')
                    {
                        //                        args++;
                        background = 1;
                        cmd[len - 1] = '\0'; // Remove '&' from the command
                        len--;
                    }
                    else
                    {
                        background = 0;
                    }

                    // Remove trailing whitespace
                    while (len > 0 && isspace(cmd[len - 1]))
                    {
                        cmd[len - 1] = '\0';
                        len--;
                    }
                    //                    printf("             %s             cmd__2",cmd);
                    //                execute_command2(args);
                    //                    args++;
                    parse_arguments(argument_in[i], args_arry);
                    int exec_sucsse = execute_command2(args_arry, background, &flag);
                    if (exec_sucsse != 0)
                    {
                        //                        args++;
                        //                    printf("Command failed: %s\n", argument_in[i]);
                        break;
                    }
                    if (exec_sucsse == 0)
                    {
                        args++;
                    }
                }
            }
            num_commands = 0;
            continue;
        }
        //        ||
        else if (swich == 1)
        {
            type = 1;
            int four = count_words_in_sections(command, type);
            if (four == -1)
                continue;
            parse_commands_p(command, argument_in, &num_commands);
            //            for (int i = 0; i < num_commands; ++i) {
            //                printf("                             %s            \n",argument_in[i]);
            //            }
            for (int i = 0; i < num_commands; ++i)
            {

                char *args_arry[MAX_CHARACTERS];
                int argument = count_words(argument_in[i]);
                if (argument > MAX_ARGS)
                {
                    perror("ERR: Too many argument");
                    break;
                }
                if (strstr(argument_in[i], "\"") != NULL || strstr(argument_in[i], "\'") != NULL)
                {
                    num_args_apostrophes++;
                    num_args_apostrophes_2 = 1;
                }
                if (strstr(argument_in[i], "jobs") != 0)
                {
                    args++;
                    list_jobs();
                    break;
                }

                else
                {
                    char *cmd = argument_in[i];
                    trim_trailing_whitespace(cmd);
                    // Check if the command ends with '&'
                    size_t len = strlen(cmd);
                    if (len > 0 && cmd[len - 1] == '\n')
                    {
                        cmd[len - 1] = '\0';
                        len--;
                    }
                    //                    printf("                    %s        cmd__1",cmd);
                    if (len > 0 && cmd[len - 1] == '&')
                    {
                        args++;
                        background = 1;
                        cmd[len - 1] = '\0'; // Remove '&' from the command
                        len--;
                        printf("                    %s        cmd__2", cmd);
                    }
                    else
                    {
                        background = 0;
                    }

                    // Remove trailing whitespace
                    while (len > 0 && isspace(cmd[len - 1]))
                    {
                        cmd[len - 1] = '\0';
                        len--;
                    }
                    //                    args++;
                    parse_arguments(argument_in[i], args_arry);
                    if (execute_command2(args_arry, background, &flag) == 0)
                    {
                        args++;
                        break;
                    }
                }
            }
            num_commands = 0;
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
            // Check for alias command
            if (num_alias == 0)
            {
                aliases = (Alias *)malloc(sizeof(Alias));
            }
            else
            {
                Alias *temp = (Alias *)realloc(aliases, (num_alias + 1) * sizeof(Alias));
                if (temp == NULL)
                {
                    perror("malloc");
                    free(aliases);
                    return 1;
                }
                aliases = temp;
            }

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

        else if (strcmp(command, "jobs") == 0)
        {
            args++;
            list_jobs();
            continue;
        }

        else
        {
            // Check if the command ends with '&'
            size_t len = strlen(command);
            if (len > 0 && command[len - 1] == '\n')
            {
                command[len - 1] = '\0';
                len--;
            }

            if (len > 0 && command[len - 1] == '&')
            {
                args++;
                background = 1;
                command[len - 1] = '\0'; // Remove '&' from the command
                len--;
            }
            else
            {
                background = 0;
            }

            // Remove trailing whitespace
            while (len > 0 && isspace(command[len - 1]))
            {
                command[len - 1] = '\0';
                len--;
            }
            for (int i = 0; i < num_alias; i++)
            {
                if (strcmp(aliases[i].key, command) == 0)
                {
                    strncpy(command, aliases[i].value, MAX_CHARACTERS);
                    command[MAX_CHARACTERS - 1] = '\0'; // Ensure null-termination
                    if (aliases[i].flag_character == 1)
                    {
                        num_args_apostrophes++;
                    }
                }
            }

            execute(command, &token_count, &args, &num_alias, &flag, background);
            if (flag == 0 && num_args_apostrophes_2 == 1)
            {
                num_args_apostrophes--;
                flag = 1;
                num_args_apostrophes_2 = 0;
            }
        }
    }
    free(aliases);
    return 0;
}
