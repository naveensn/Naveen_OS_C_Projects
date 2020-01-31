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

//Naveen Sokke Nagarajappa

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#define SEMICOLON ";"               // We want to split our command line up into commands and tokens
                                    // so we need to define what delimits our tokens.
#define WHITESPACE " \t\n"          // In this case SEMICOLON and white space
                                    // will separate the tokens on our command line

#define NUM_of_PATHS 4

#define MAX_COMMAND_SIZE 255        // The maximum command-line size

#define MAX_NUM_ARGUMENTS 10        // Mv shell only supports five arguments

#define MAX_NUM_HISTORY 15          // Mv shell only stores 15 history commands

#define MAX_NUM_CMD_PER_LINE 5     // Mv shell only supports five commands per line

int main()
{
    char * cmd_str = (char*) malloc( MAX_COMMAND_SIZE );

    int max_history = 0, pid_max_history = 0, i, pid_history[MAX_NUM_HISTORY];

    char * cmd_history[MAX_NUM_HISTORY];         //to store command history

    for (i=0;i<MAX_NUM_HISTORY;i++)              //allocating memory to store commands
    {
        cmd_history[i] = (char*) malloc(MAX_COMMAND_SIZE);
    }

    char *path[NUM_of_PATHS] = {"./", "/usr/local/bin/", "/local/bin/", "/bin/"};

    while( 1 )
    {

        // Print out the msh prompt
        printf ("msh> ");

        // Read the command from the commandline.  The
        // maximum command that will be read is MAX_COMMAND_SIZE
        // This while command will wait here until the user
        // inputs something since fgets returns NULL when there
        // is no input
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

        //print commands history.
//        for( i = 0; i < max_history; i++)
//        {
//            printf("%d %s", i, cmd_history[i]);
//        }

        /* Parse input */
        char *cmd[MAX_NUM_CMD_PER_LINE];

        int   cmd_count = 0;

        // Pointer to point to the commands and token
        // parsed by strsep
        char *cmd_ptr, *arg_ptr, *cmd_path;

        char *working_str  = strndup( cmd_str, MAX_COMMAND_SIZE );

        // we are going to move the working_str pointer so
        // keep track of its original value so we can deallocate
        // the correct amount at the end
        char *working_root = working_str;

        // Parse the input commands with SEMICOLON used as the delimiter
        while ( ( (cmd_ptr = strsep(&working_str, SEMICOLON ) ) !=NULL) &&
                  (cmd_count<MAX_NUM_CMD_PER_LINE))
        {
 //          printf("cmd_ptr = %s", cmd_ptr);
            char *token[MAX_NUM_ARGUMENTS];

            int token_count = 0;
            // Tokenize the input strings with whitespace used as the delimiter
            while ( ( (arg_ptr = strsep(&cmd_ptr, WHITESPACE ) ) != NULL) &&
                      (token_count<MAX_NUM_ARGUMENTS))
            {
//                printf("arg_ptr = %s [%d]\n", arg_ptr, token_count);
                token[token_count] = strndup( arg_ptr, MAX_COMMAND_SIZE );
                if( strlen( token[token_count] ) == 0 )
                {
                    token_count--;             //removing extra spaces in command.
                }
                token_count++;
            }
            token[token_count] = NULL;

            // Now print the tokenized input as a debug check
            // \TODO Remove this code and replace with your shell functionality

//            printf("token = %s [%d]", token[0], token_count);
            if (token[0] != NULL)
            {
                if ((strcmp(token[0], "exit") == 0) || (strcmp(token[0], "quit") == 0))
                {
                    exit( EXIT_SUCCESS );
                }
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
                            printf("error in creating directory : %s\n", token[1]);
                        }
                    }
                }
                else if (strcmp(token[0], "history") == 0)
                {
                    //print commands history.
                    for( i = 0; i < max_history; i++)
                    {
                        printf("%d %s", i+1, cmd_history[i]);
                    }
                }
                else if (strcmp(token[0], "listpids") == 0)
                {
                    //print commands history.
                    printf("pid_max_history = %d, PID[%d] %d\n", pid_max_history, i, pid_history[i]);
                    for( i = 0; i < pid_max_history; i++)
                    {
                        printf("%d %d\n", i+1, pid_history[i]);
                    }
                }
                else
                {
                pid_t child_pid = fork();

                int status;
//                printf("child_pid = %d & status = %d", child_pid, status);

                if( child_pid == -1 )
                {
                    perror("fork failed: ");
                    exit( EXIT_FAILURE );
                }
                else if(child_pid == 0)
                {
                    for( i = 0; i < pid_max_history; i++)
                    {
                        pid_history[pid_max_history-i] = pid_history[pid_max_history-(i+1)];
                    }
                    pid_history[0] = getpid();

                    if (pid_max_history < MAX_NUM_HISTORY)
                    {
                        pid_max_history++;
                    }

                    //incrementing number of commands stored in history.
                    if (pid_max_history < MAX_NUM_HISTORY)
                    {
                        pid_max_history++;
                    }
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
                     // child process.
                     int status;

                     // Force the parent process to wait until the child process
                     // exits
                     waitpid(child_pid, &status, 0 );
//                     printf("Hello from the parent process\n");
                     fflush(NULL);
                 }
                 }

                 int token_index  = 0;
                 for( token_index = 0; token_index < token_count; token_index ++ )
                 {
                     printf("token[%d] = %s\n", token_index, token[token_index] );
                 }
            }
        }
        free( working_root );
    }
    return 0;
}

