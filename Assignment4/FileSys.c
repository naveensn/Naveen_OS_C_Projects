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
#include <sys/stat.h>
#include <stdint.h>
#include <inttypes.h>
#include <time.h>

#define NUM_BLOCKS 4226
#define BLOCK_SIZE 8192
#define NUM_FILES  128
#define FILE_NAME  32

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


void creatfs(char* filename);
void open(char* filename);
void closefs();
int df();
int put(char* filename);
void list(char* atrb);
int get(char **filename);
int del(char* filename);
int attrib(char **argv);

uint8_t blocks[NUM_BLOCKS][BLOCK_SIZE];
FILE *fd = NULL;

struct Directory_Entry
{
    int8_t valid;
    char name[FILE_NAME];
    uint32_t inode;
};

struct Inode
{
    uint8_t attributes;
    time_t time;
    uint32_t size;
    uint32_t blocks[1250];
};

struct Directory_Entry *dir;
struct Inode	       *inodeList;
uint8_t                *freeBlockList;
uint8_t                *freeInodeList;

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
        printf ("mfs> ");

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

                // checks createfs command and creates file.
                else if (strcmp(token[0], "createfs") == 0)
                {
                    if (token[1] == NULL) //No filename with create.
                    {
                        printf("createfs: File not found\n");
                    }
                    else if (token[2] != NULL) //More the 1 argument.
                    {
                        printf("Incorrect number of parameters.\nUse: \n createfs filename\n");
                    }
                    else
                    {
                        creatfs(token[1]);
                    }
                }

                else if (strcmp(token[0], "open") == 0)
                {
                    if (token[1] == NULL) //No filename with open.
                    {
                        printf("Please provide the file name with open.\nUse: \n open filename\n");
                    }
                    else if (token[2] != NULL) //More the 1 argument.
                    {
                        printf("Incorrect number of parameters.\nUse: \n open filename\n");
                    }
                    else
                    {
                        open(token[1]);
                    }
                }

                else if (strcmp(token[0], "close") == 0)
                {
                    if (token[1] != NULL) //No need of arguments.
                    {
                        printf("Incorrect number of parameters. \nUse: \n close\n");
                    }
                    else if (fd == NULL) //File system is not open.
                    {
                        printf("Please open the file system before doing any operations on it\n");
                    }
                    else
                    {
                        closefs();
                    }

                }

                else if (strcmp(token[0], "df") == 0)
                {
                    if (token[1] != NULL) //No need of arguments.
                    {
                        printf("Incorrect number of parameters. \nUse: \n df\n");
                    }
                    else if (fd == NULL) //File system is not open.
                    {
                        printf("Please open the file system before doing any operations on it\n");
                    }
                    else
                    {
                        printf("%d bytes free.\n", df());
                    }

                }

                else if (strcmp(token[0], "put") == 0)
                {
                    if (token[1] == NULL) //No filename with put
                    {
                        printf("Please provide the file name with put.\nUse: \n put filename\n");
                    }
                    else if (token[2] != NULL) //More the 1 argument.
                    {
                        printf("Incorrect number of parameters.\nUse: \n put filename\n");
                    }
                    else if (fd == NULL) //File system is not open.
                    {
                        printf("Please open the file system before doing any operations on it\n");
                    }
                    else
                    {
                        put(token[1]);
                    }
                }

                else if (strcmp(token[0], "list") == 0)
                {
                    if (fd == NULL) //File system is not open.
                    {
                        printf("Please open the file system before doing any operations on it\n");
                    }
                    else if ((token[1] == NULL) || (token[2] == NULL)) //max 1 argument.
                    {
                        list(token[1]);
                    }
                    else
                    {
                        printf("Incorrect number of parameters.\nUse: \n list\n list <-h>\n");
                    }
                }

                else if (strcmp(token[0], "get") == 0)
                {
                    if (fd == NULL) //File system is not open.
                    {
                        printf("Please open the file system before doing any operations on it\n");
                    }
                    else if (token[1] == NULL) //No file name with get.
                    {
                        printf("Incorrect number of parameters.\nUse: \n get filename\n");
                        printf(" get filename <newfilename>\n");
                    }
                    else if ((token[2] == NULL) || (token[3] == NULL)) //expects 1 or 2 filenames
                    {
                        get(token);
                    }
                    else
                    {
                        printf("Incorrect number of parameters.\nUse: \n get filename\n");
                        printf(" get filename <newfilename>\n");
                    }
                }

                else if (strcmp(token[0], "del") == 0)
                {
                    if (token[1] == NULL) //No file name with del.
                    {
                        printf("Please provide the file name with del.\nUse: \n del filename\n");
                    }
                    else if (token[2] != NULL) //more then 1 argument.
                    {
                        printf("Incorrect number of parameters.\nUse: \n del filename\n");
                    }
                    else if (fd == NULL) //file system is not open
                    {
                        printf("Please open the file system before doing any operations on it\n");
                    }
                    else
                    {
                        del(token[1]);
                    }
                }

                else if (strcmp(token[0], "attrib") == 0)
                {
                    if (token[1] == NULL || token[2] == NULL) // Less then 2 arguments.
                    {
                        printf("Please provide the file name and attribute with attrib.\n");
                        printf("Use:\n attrib +attribute filename\n attrib -attribute filename");
                    }
                    else if (token[3] != NULL) //More then 2 arguments
                    {
                        printf("Incorrect number of parameters.\n");
                        printf("Use:\n attrib +attribute filename\n attrib -attribute filename");
                    }
                    else if (fd == NULL) // File system is not open.
                    {
                        printf("Please open the file system before doing any operations on it\n");
                    }
                    else
                    {
                        attrib(token);
                    }
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


//Main coding for file system starts from here.

//All initializations.
void initializeDirectory()
{
    int i;
    for(i=0;i<NUM_FILES;i++)
    {
        dir[i].valid=0;
        memset(dir[i].name,0,FILE_NAME);
        dir[i].inode=-1;
    }
}

void initializeInodeList()
{
    int i;
    for(i=0;i<NUM_FILES;i++)
    {
        freeInodeList[i]=1;
    }
}

void initializeBlockList()
{
    int i;
    for(i=0;i<NUM_BLOCKS;i++)
    {
        freeBlockList[i]=1;
    }
}

void initializeInodes()
{
    int i;
    for(i=0;i<NUM_FILES;i++)
    {
        int j;
        inodeList[i].attributes=0;
        inodeList[i].time = 0;
        inodeList[i].size=0;
        for(j=0;j<1250;j++)
        {
            inodeList[i].blocks[j]=-1;
        }
    }
}

/*
    Finds file in the directory.
    Input file name.
    output directory index.
*/
int findDirectoryEntry(char * filename)
{
    int ret = -1;
    int i;
    for(i=0;i<NUM_FILES;i++)
    {
        if(dir[i].valid == 1 && (strcmp(filename, dir[i].name) == 0))
        {
            ret = i;
            break;
        }
    }
    return ret;
}

/*
    Finds free directory entry.
    output directory index.
*/
int findFreeDirectory()
{
    int ret = -1;
    int i;
    for(i=0;i<NUM_FILES;i++)
    {
        if(dir[i].valid == 0)
        {
            ret = i;
            dir[i].valid = 1;
            break;
        }
    }
    return ret;
}

/*
    Finds free Inode.
    output directory index.

    Not using this function as directory index and inode index
    will always be same we have only one directory.

    Can be used in next update
*/
int findFreeInode()
{
    int ret = -1;
    int i;
    for(i=0;i<NUM_FILES;i++)
    {
        if(freeInodeList[i] == 1)
        {
            ret = i;
            freeInodeList[i] = 0;
            break;
        }
    }
    return ret;
}

/*
    Finds free Block.
    output free block index.

    total number of free blocks is
    total number of blocks - 132 as first 132 blocks are used to

*/
int findFreeBlock()
{
    int ret = -1;
    int i;
    for(i=0;i<NUM_BLOCKS-132;i++)
    {
        if(freeBlockList[i] == 1)
        {
            ret = i;
            freeBlockList[i] = 0;
            break;
        }
    }
    return ret;
}

/*
    Gives total free storage available.
    output size in int.

*/
int df()
{
    int ret = 0;
    int i;
    for(i=0;i<NUM_BLOCKS-132;i++)
    {
        if(freeBlockList[i] == 1)
        {
            ret = ret + BLOCK_SIZE;
        }
    }
    return ret;
}

/*
    Lists all files in the Directory.
    does not take any input.
    give attribute "-h" to see hidden files.
*/
void list(char* atrb)
{
    int i, flag = 0;
    for(i=0;i<NUM_FILES;i++)
    {
        if(dir[i].valid)
        {
            if (atrb == NULL)
            {
                if (inodeList[i].attributes < 2 )
                {
                    char *tm = ctime(&inodeList[i].time); // Converting time to
                    tm = &tm[4];                          // needed format.
                    tm[strlen(tm)-9] = '\0';
                    printf("%" PRIu32, inodeList[i].size);
                    printf(" %s", tm);
                    printf(" %s\n",  dir[i].name);
                    flag = 1;
                }
            }
            else if (strcmp(atrb, "-h") == 0) // when -h is used with list
            {
                char *tm = ctime(&inodeList[i].time); // Converting time to
                tm = &tm[4];                          // needed format.
                tm[strlen(tm)-9] = '\0';
                printf("%" PRIu32, inodeList[i].size);
                printf(" %s", tm);
                printf(" %s\n",  dir[i].name);
                flag = 1;
            }
            else
            {
                printf("Attributes can only be -h.\n");
            }
        }
    }
    if (flag == 0)
    {
        printf("list: No files found.\n");
    }
}

/*
    Create the file directory. initializes all data.
    input: file system name.
*/
void creatfs(char* filename)
{
    struct stat buf;
    if (stat(filename, &buf) == 0)
    {
        printf("File system %s already exists.\n", filename);
        printf("Use another file system name\n");
    }
    else
    {
        memset(blocks, 0, NUM_BLOCKS*BLOCK_SIZE);

        fd = fopen(filename, "w");

        dir= (struct Directory_Entry*)&blocks[0][0];
        freeInodeList=(uint8_t*)&blocks[1][0];
        freeBlockList=(uint8_t*)&blocks[2][0];

        //Inodes run from blocks 3-131 so add 3 to the index
        inodeList= (struct Inode*)&blocks[3][0];

        initializeDirectory();
        initializeInodeList();
        initializeBlockList();
        initializeInodes();

        fwrite(blocks, BLOCK_SIZE, NUM_BLOCKS, fd);

        fclose(fd);
        fd = NULL;
    }
}

/*
    Opens the file directory if it is present. assigns all pointers.
    input: file system name.
*/
void open(char* filename)
{
    struct stat buf;
    if (stat(filename, &buf) == 0)
    {
        dir= (struct Directory_Entry*)&blocks[0][0];
        freeInodeList=(uint8_t*)&blocks[1][0];
        freeBlockList=(uint8_t*)&blocks[2][0];

        //Inodes run from blocks 3-131 so add 3 to the index
        inodeList= (struct Inode*)&blocks[3][0];


        fd = fopen(filename, "w+");
        fread(blocks, BLOCK_SIZE, NUM_BLOCKS, fd);
    }
    else
    {
        printf("open: File not found\n");
    }
}

/*
    Closes the file directory.
    no input or output.
*/
void closefs()
{
    if (fd == NULL)
    {
        printf("No file is open to close.\n");
        printf("If you want to test close function, please open a file then close.\n");
    }
    else
    {
        fwrite(blocks, BLOCK_SIZE, NUM_BLOCKS, fd);
        fclose(fd);
        fd = NULL;
    }
}

/*
    Copies file from current directory to file system.
    Input: filename
*/
int put(char* filename)
{
    struct stat buf;
    if (stat(filename, &buf) == 0) //checking if file present or not.
    {
        int nameLen = strlen(filename);
        if (nameLen > 32)
        {
            printf("File name length can not be greater the 32 characters.\n");
        }
        else
        {
            int i;
            for (i=0;i<nameLen;i++)
            {
                if ((filename[i] >= 'a' && filename[i] <= 'z') || // name can have only alphanumeric
                    (filename[i] >= 'A' && filename[i] <= 'Z') || // or ".".
                    (filename[i] == '.') || (filename[i] >= '0' && filename[i] <= '9'))
                {
                    continue;
                }
                else
                {
                    printf("file names should only be alphanumeric with “.”.\n");
                    return -1;
                }
            }
        }

        int copy_size   = buf.st_size;
        if (copy_size > 10240000)
        {
            printf("File size is greater than 10MB, can't put it in our file system.\n");
            return -1;
        }
        if (copy_size > df())
        {
            printf("Available disk size is less then file size. ");
            printf("Can not put the file in file system\n");
            return -1;
        }

        int directoryIndex = findDirectoryEntry(filename);
        if (directoryIndex < 0)
        {
            directoryIndex = findFreeDirectory();
            if (directoryIndex < 0)
            {
                printf("There are no free directory entries. File system is full\n");
                return -1;
            }
        }
        else
        {
            printf("File already exists in directory.\n");
            printf("If you want to update this file please del and put.\n");
            return -1;
        }

        freeInodeList[directoryIndex]           = 0; //updating free Inode list
        strncpy(dir[directoryIndex].name, filename, nameLen); //updating name in directory entry.
        dir[directoryIndex].inode               = directoryIndex; //updating inode in the entry.
        inodeList[directoryIndex].time          = time(NULL); //updating changed time in inode data.
        inodeList[directoryIndex].size          = copy_size; //updating size in inode data.
        inodeList[directoryIndex].attributes    = 0; //updating attributes in inode data.

        // We want to copy and write in chunks of BLOCK_SIZE. So to do this
        // we are going to use fseek to move along our file stream in chunks of BLOCK_SIZE.
        // We will copy bytes, increment our file pointer by BLOCK_SIZE and repeat.
        int offset      = 0;

        // We are going to copy and store our file in BLOCK_SIZE chunks instead of one big
        // memory pool. Why? We are simulating the way the file system stores file data in
        // blocks of space on the disk. block_index will keep us pointing to the area of
        // the area that we will read from or write to.
        int block_index = 0;

        // copy_size is initialized to the size of the input file so each loop iteration we
        // will copy BLOCK_SIZE bytes from the file then reduce our copy_size counter by
        // BLOCK_SIZE number of bytes. When copy_size is less than or equal to zero we know
        // we have copied all the data from the input file.
        FILE *ifp = fopen(filename, "r");
        int bytes, i = 0;

        while( copy_size >= BLOCK_SIZE )
        {
          // Index into the input file by offset number of bytes.  Initially offset is set to
          // zero so we copy BLOCK_SIZE number of bytes from the front of the file.  We
          // then increase the offset by BLOCK_SIZE and continue the process.  This will
          // make us copy from offsets 0, BLOCK_SIZE, 2*BLOCK_SIZE, 3*BLOCK_SIZE, etc.
          fseek( ifp, offset, SEEK_SET );

          int block_index = findFreeBlock();

          // Read BLOCK_SIZE number of bytes from the input file and store them in our
          // data array.
          bytes  = fread( blocks[block_index+132], BLOCK_SIZE, 1, ifp );

          // If bytes == 0 and we haven't reached the end of the file then something is
          // wrong. If 0 is returned and we also have the EOF flag set then that is OK.
          // It means we've reached the end of our input file.
          if( bytes == 0 && !feof( ifp ) )
          {
            printf("An error occured reading from the input file.\n");
            return -1;
          }

          freeBlockList[block_index] = 0; // marking used blocks
          inodeList[directoryIndex].blocks[i] = block_index+132; //adding used blocks in the
          i++;                                                   // sequence to inode data.
          // Clear the EOF file flag.
          clearerr( ifp );

          // Reduce copy_size by the BLOCK_SIZE bytes.
          copy_size -= BLOCK_SIZE;

          // Increase the offset into our input file by BLOCK_SIZE.  This will allow
          // the fseek at the top of the loop to position us to the correct spot.
          offset    += BLOCK_SIZE;

          // Increment the index into the block array
          block_index ++;
        }

        // same process as above for the last block (when data size is less then block size)
        fseek( ifp, offset, SEEK_SET );

        block_index = findFreeBlock();

        bytes  = fread( blocks[block_index+132], copy_size, 1, ifp ); //free blocks starts from 132

        freeBlockList[block_index] = 0;
        inodeList[directoryIndex].blocks[i] = block_index+132; //first 131 blocks are for matadata.


    }
    else
    {
        printf("put error: File not found\n");
        return -1;
    }
    return 0;
}

/*
    Copies file from file system to current directory .
    Input: **char (get filename <newfilename>)
    can give a new name to change the filename in current directory.

*/
int get(char **filename)
{
    int directoryIndex = findDirectoryEntry(filename[1]);
    if (directoryIndex < 0)
    {
        printf("File not found in file system please check the file name\n");
        return -1;
    }

    // Now, open the output file that we are going to write the data to.
    FILE *ofp;
    if (filename[2]==NULL)
    {
        ofp = fopen(filename[1], "w");
        if( ofp == NULL )
        {
          printf("Could not open output file: %s\n", filename[1] );
          perror("Opening output file returned");
          return -1;
        }
    }
    else
    {
        ofp = fopen(filename[2], "w");
        if( ofp == NULL )
        {
          printf("Could not open output file: %s\n", filename[2] );
          perror("Opening output file returned");
          return -1;
        }
    }


    // Initialize our offsets and pointers just we did above when reading from the file.
    int i           = 0;
    int block_index = inodeList[directoryIndex].blocks[i];
    int copy_size   = inodeList[directoryIndex].size;
    int offset      = 0;

    // Using copy_size as a count to determine when we've copied enough bytes to the output file.
    // Each time through the loop, except the last time, we will copy BLOCK_SIZE number of bytes from
    // our stored data to the file fp, then we will increment the offset into the file we are writing to.
    // On the last iteration of the loop, instead of copying BLOCK_SIZE number of bytes we just copy
    // how ever much is remaining ( copy_size % BLOCK_SIZE ).  If we just copied BLOCK_SIZE on the
    // last iteration we'd end up with gibberish at the end of our file.
    while( copy_size > 0 )
    {

      int num_bytes;

      // If the remaining number of bytes we need to copy is less than BLOCK_SIZE then
      // only copy the amount that remains. If we copied BLOCK_SIZE number of bytes we'd
      // end up with garbage at the end of the file.
      if( copy_size < BLOCK_SIZE )
      {
        num_bytes = copy_size;
      }
      else
      {
        num_bytes = BLOCK_SIZE;
      }

      // Write num_bytes number of bytes from our data array into our output file.
      fwrite( blocks[block_index], num_bytes, 1, ofp );

      // Reduce the amount of bytes remaining to copy, increase the offset into the file
      // and increment the block_index to move us to the next data block.
      copy_size -= BLOCK_SIZE;
      offset    += BLOCK_SIZE;
      i++;
      block_index = inodeList[directoryIndex].blocks[i]; //going in the order of blocks.

      // Since we've copied from the point pointed to by our current file pointer, increment
      // offset number of bytes so we will be ready to copy to the next area of our output file.
      fseek( ofp, offset, SEEK_SET );
    }

    // Close the output file, we're done.
    fclose( ofp );

    return 0;
}

/*
    deletes the file from file system.
    Input: filename
*/
int del(char* filename)
{
    int directoryIndex = findDirectoryEntry(filename);
    if (directoryIndex < 0)
    {
        printf("del error: File not found.\n");
        return -1;
    }
    if (inodeList[directoryIndex].attributes == 1 || inodeList[directoryIndex].attributes == 3)
    {
        printf("File cannot be deleted: File is read only.\n");
        return -1;
    }

    freeInodeList[directoryIndex] = 1;  //freeing inode
    dir[directoryIndex].valid     = 0;  //freeing directory entry
    memset(dir[directoryIndex].name,0,FILE_NAME); //initializing name field in directory
    dir[directoryIndex].inode     =-1;  //resetting inode used in directory entry

    int i           = 0;
    while(((int)inodeList[directoryIndex].blocks[i] > 0) && i < 1250) //freeing all used blocks
    {
        freeBlockList[inodeList[directoryIndex].blocks[i]-132]  = 1;
        inodeList[directoryIndex].blocks[i]                     =-1;
        i++;
    }
    return 0;
}

/*
    setting attributes to file in the file system.
    "r" is for read "h" is for hidden
    use "-" to remove attributes and "+" to add.
    attribute values are
    0: default value.
    1: Read only.
    2: hidden.
    3: both read only and hidden
    Input: attribute and filename
*/
int attrib(char **argv)
{
    char *atr = argv[1], *filename = argv[2];

    int directoryIndex = findDirectoryEntry(filename);
    if (directoryIndex < 0)
    {
        printf("File not found in file system please check the file name\n");
        return -1;
    }

    if (strcmp(atr, "-r") == 0)
    {
        if (inodeList[directoryIndex].attributes == 1)
        {
            inodeList[directoryIndex].attributes = 0;
        }
        else if (inodeList[directoryIndex].attributes == 3)
        {
            inodeList[directoryIndex].attributes = 2;
        }
    }
    else if (strcmp(atr, "-h") == 0)
    {
        if (inodeList[directoryIndex].attributes == 2)
        {
            inodeList[directoryIndex].attributes = 0;
        }
        else if (inodeList[directoryIndex].attributes == 3)
        {
            inodeList[directoryIndex].attributes = 1;
        }
    }
    else if (strcmp(atr, "+r") == 0)
    {
        if (inodeList[directoryIndex].attributes == 2)
        {
            inodeList[directoryIndex].attributes = 3;
        }
        else if (inodeList[directoryIndex].attributes == 0)
        {
            inodeList[directoryIndex].attributes = 1;
        }
    }
    else if (strcmp(atr, "+h") == 0)
    {
        if (inodeList[directoryIndex].attributes == 1)
        {
            inodeList[directoryIndex].attributes = 3;
        }
        else if (inodeList[directoryIndex].attributes == 0)
        {
            inodeList[directoryIndex].attributes = 2;
        }
    }
    else
    {
        printf("Attributes can only be -r, -h, +r and +h.\n");
        return -1;
    }
    return 0;
}
