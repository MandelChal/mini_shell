# mini_shell
The code implements a Linux shell-like program in C, providing functionalities for executing commands, managing aliases, and parsing user input.

Shell Program with Alias and Script Execution Support
Overview
This project is a simple shell implementation in C that supports basic command execution, command aliasing, and script execution. It provides an interactive prompt for users to input commands and manages custom aliases for commands.

Features
Command Execution: Execute basic shell commands entered by the user.
Command Aliasing: Define and use aliases for commands to simplify command input.
Script Execution: Execute a series of commands from a script file.
Custom Prompt: Displays a custom prompt with the number of executed commands, defined aliases, and executed scripts.
Error Handling: Provides error messages for various invalid inputs and command failures.
Argument Handling: Limits the number of arguments to prevent overflows and potential issues.
Functionality
Command Execution
The shell executes commands using the execvp system call. If a command contains too many arguments or fails to execute, an error message is displayed.

Aliasing
Users can define aliases for commands using the alias keyword. Aliases are stored in a dynamically allocated array and can be added, updated, or removed. Aliases are particularly useful for frequently used commands.

Script Execution
The shell can execute a script file using the source command. Each line of the script is read and executed in sequence. The shell recognizes and processes alias definitions within the script.

Prompt Display
The shell displays a custom prompt showing the number of executed commands, defined aliases, and executed scripts. This helps users keep track of their shell session activities.

Handling Special Characters
The shell can handle commands containing special characters like quotes. It ensures proper parsing and execution of such commands.

Usage
Starting the Shell: Compile and run the shell program. You will see a custom prompt indicating the number of executed commands, defined aliases, and executed scripts.
Executing Commands: Enter any valid shell command and press Enter. The command will be executed, and the result will be displayed.
Defining Aliases: Use the alias command to define an alias. For example, alias ll='ls -l' will create an alias ll for the ls -l command.
Removing Aliases: Use the unalias command to remove an alias. For example, unalias ll will remove the ll alias.
Executing Scripts: Use the source command followed by a script file name to execute the script. For example, source myscript.sh will execute the commands in myscript.sh.
Exiting the Shell: Enter exit_shell to exit the shell. The shell will display the total number of commands containing quotes executed during the session.
Conclusion
This shell program provides a basic framework for command execution, alias management, and script execution. It is a useful tool for learning about shell implementation and can be extended with additional features as needed.

The code implements a Linux shell-like program in C, providing functionalities for executing commands, managing aliases, and parsing user input.

To compile and run the program, use the following commands:
#!/bin/bash
gcc shell.c -o shell -Wall
./shell
