/*
    Naveen Sokke Nagarajappa.
    ID : 1001768613
*/

// The MIT License (MIT)
//
// Copyright (c) 2016, 2017 Trevor Bakker
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

/*
  This program is to create our own basic shell, Mav shell (msh), similar to
  bourne shell (bash), c-shell (csh), or korn shell (ksh). It will accept commands,
  fork a child process and execute those commands. The shell, like csh or bash, will run
  and accept commands until the user exits the shell.
*/



#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#define SEMICOLON ";"            // We want to split our command line up into commands
                                 // and tokensso we need to define what delimits our
#define WHITESPACE " \t\n"       // commands and tokens. in this case SEMICOLON will separate
                                 // commands and white space will separate the tokens
                                 // on our command line.

#define NUM_of_PATHS 4           // Number of directories we are checking for built in commands.

#define MAX_COMMAND_SIZE 255     // The maximum command-line size

#define MAX_NUM_ARGUMENTS 10     // Mv shell only supports five arguments

#define MAX_NUM_HISTORY 15       // Mv shell only stores 15 history commands

#define MAX_NUM_CMD_PER_LINE 5   // Mv shell only supports five commands per line


static void handle_signal (int sig)
{

  /*
   this function is to catch SIGINT (Ctrl-C) and SIGTSTP (Ctrl-Z) signals
  */

    switch( sig )
    {
        case SIGINT:    // for Ctrl-C
            break;

        case SIGTSTP:   // for Ctrl-Z
            break;

        default:
            printf("Unable to determine the signal\n");
            break;
    }
}


int main()
{
    //allocating memory for a single line command.
    char * cmd_str = (char*) malloc( MAX_COMMAND_SIZE );

    int max_history = 0, pid_max_history = 0, i, pid_history[15];

    char * cmd_history[MAX_NUM_HISTORY];         //to store command history.

    for (i=0;i<MAX_NUM_HISTORY;i++)              //allocating memory to store commands.
    {
        cmd_history[i] = (char*) malloc(MAX_COMMAND_SIZE);
    }

    //these are the directories we are checking for commands.
    char *path[NUM_of_PATHS] = {"./", "/usr/local/bin/", "/local/bin/", "/bin/"};

    //code to handle signals starts here
    struct sigaction act;

    //Zero out the sigaction struct
    memset (&act, '\0', sizeof(act));

    //Set the handler to use the function handle_signal().
    act.sa_handler = &handle_signal;

    //Install the handler for SIGINT and SIGTSTP and check the return value.

    if (sigaction(SIGINT , &act, NULL) < 0)
    {
        perror ("sigaction: ");
        return 1;
    }

    if (sigaction(SIGTSTP , &act, NULL) < 0)
    {
        perror ("sigaction: ");
        return 1;
    }

    while( 1 )
    {

        // Print out the msh prompt
        printf ("msh> ");

        /*
          Read the command from the commandline.  The
          maximum command that will be read is MAX_COMMAND_SIZE
          This while command will wait here until the user
          inputs something since fgets returns NULL when there
          is no input. user can input commends separated by semicolon
          and arguments of a command separated by spaces.
        */
        while( !fgets (cmd_str, MAX_COMMAND_SIZE, stdin) );

        //storing input commands in array to use for history command.
        for( i = 0; i < max_history; i++)
        {
            cmd_history[max_history-i] = strdup(cmd_history[max_history-(i+1)]);
        }
        cmd_history[0] = strdup(cmd_str);

        //incrementing number of commands stored in history.
        if (max_history < MAX_NUM_HISTORY)
        {
            max_history++;
        }

        int   cmd_count = 0;

        // Pointers to point to the commands and tokens parsed by strsep
        char *cmd_ptr, *arg_ptr, *cmd_path, *exclim_ptr;

        char *working_str  = strndup( cmd_str, MAX_COMMAND_SIZE );

        // we are going to move the working_str pointer so
        // keep track of its original value so we can deallocate
        // the correct amount at the end
        char *working_root = working_str;

        // Parse the input commands with SEMICOLON used as the delimiter
        // User can give any number of spaces or semicolons in between
        while ( ( (cmd_ptr = strsep(&working_str, SEMICOLON ) ) !=NULL) &&
                  (cmd_count<MAX_NUM_CMD_PER_LINE))
        {
            char *token[MAX_NUM_ARGUMENTS];

            int token_count = 0;
            // Tokenize the input strings with whitespace used as the delimiter
            while ( ( (arg_ptr = strsep(&cmd_ptr, WHITESPACE ) ) != NULL) &&
                      (token_count<MAX_NUM_ARGUMENTS))
            {
                token[token_count] = strndup( arg_ptr, MAX_COMMAND_SIZE );
                if( strlen( token[token_count] ) == 0 )
                {
                    token_count--;             //removing any extra spaces in command.
                }
                token_count++;
            }
            token[token_count] = NULL;

            // checking commands and arguments starts from here.
            // if Command has extra spaces or two semicolons we are ignoring it.
            if (token[0] != NULL)
            {
                // exiting shell if command is "exit" and "quit"
                if ((strcmp(token[0], "exit") == 0) || (strcmp(token[0], "quit") == 0))
                {
                    exit( EXIT_SUCCESS );
                }

                //changing the directory if command is "cd"
                else if (strcmp(token[0], "cd") == 0)
                {
                    if (token[1] == NULL)
                    {
                    printf("expected argument to \"cd\"\n");
                    }
                    else
                    {
                        if (chdir(token[1]) != 0)
                        {
                            printf("Directory %s does not exists\n", token[1]);
                        }
                    }
                }

                //Printing last (maximum of 15) stored history commands.
                else if (strcmp(token[0], "history") == 0)
                {
                    for( i = 0; i < max_history; i++)
                    {
                        printf("%d %s", i+1, cmd_history[i]);
                    }
                }

                //listing PIDs of last (maximum of 15) processes run from this shell.
                else if (strcmp(token[0], "listpids") == 0)
                {
                    for( i = 0; i < pid_max_history; i++)
                    {
                        printf("%d %d\n", i+1, pid_history[i]);
                    }
                }

                // if the previous process was suspended it will run in background if
                // user gives "bg" command.
                else if (strcmp(token[0], "bg") == 0)
                {
                    //If a process has been paused by sending it SIGSTOP then
                    //sending SIGCONT will continue that process in background
                    kill(pid_history[0],SIGCONT);
                }

                // if user enters !n we are running nth command from history.
                // Cheking if first bite of command is "!" so we can run the command from history.
                else if (strcmp(strndup(token[0], 1 ), "!") == 0)
                {
                    exclim_ptr = (strndup(token[0], strlen(token[0])));

                    // checking if number n given by user is with is stored history limit.
                    if (atoi(exclim_ptr+1) > 0 && atoi(exclim_ptr+1) <= max_history)
                    {
                        //coping nth command from history to working string so it will be executed.
                        working_str  = strndup(cmd_history[atoi(exclim_ptr+1)], MAX_COMMAND_SIZE);
                    }
                    else
                    {
                        //showing error if n is not in stored history limit.
                        printf(" %s Command not in history.\n", token[0]);
                    }
                }

                // for all other commands we are checking above given directories.
                else
                {
                    // forking a new process so we don't exit shell when we use EXECVP function.
                    pid_t child_pid = fork();

                    int status;

                    if( child_pid == -1 )
                    {
                        perror("fork failed: ");
                        exit( EXIT_FAILURE );
                    }
                    else if(child_pid == 0)
                    {
                        // When fork() returns 0 it is child process
                        // we are checking directories in above given order
                        // to find and process it if it's function is present
                        // in the directory. if it is found execvp will execute
                        // the command and terminate child process. if command
                        // is not found process will com out of for loop and
                        // print the given message.
                        for (i = 0; i < NUM_of_PATHS; i++ )
                        {
                            cmd_path = strdup(path[i]);

                            strcat(cmd_path,token[0]);

                            execvp(cmd_path, token);
                        }
                        printf("%s : commend not found\n", token[0]);
                        exit( EXIT_SUCCESS );
                    }

                    else
                    {
                        // When fork() returns a positive number, we are in the parent
                        // process and the return value is the PID of the newly created
                        // child process. we are storing this value in array so we can
                        // display them in listpids command. and we are using this
                        // for bg command also.
                        for( i = 0; i < pid_max_history; i++)
                        {
                            pid_history[pid_max_history-i] = pid_history[pid_max_history-(i+1)];
                        }
                        pid_history[0] = child_pid;

                        //incrementing number of pids stored in history.
                        if (pid_max_history < MAX_NUM_HISTORY)
                        {
                            pid_max_history++;
                        }

                        // Force the parent process to wait until the child process
                        // exits
                        waitpid(child_pid, &status, 0 );
                        fflush(NULL);
                    }
                }
            }
        }

        //freeing allocated memory.
        free( working_root );
    }
}
