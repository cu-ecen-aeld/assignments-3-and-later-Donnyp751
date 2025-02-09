#include "systemcalls.h"
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{

/*
 * TODO  add your code here
 *  Call the system() function with the command set in the cmd
 *   and return a boolean true if the system() call completed with success
 *   or false() if it returned a failure
*/

    int ret = system(cmd);

    if (ret)
    {
	int error = errno;
	printf("System() failed with errno: %d\n", error);
	return false;
    }

    return true;
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
	printf("Arg[%d]: %s\n", i, command[i]);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    //command[count] = command[count];

/*
 * TODO:
 *   Execute a system command by calling fork, execv(),
 *   and wait instead of system (see LSP page 161).
 *   Use the command[0] as the full path to the command to execute
 *   (first argument to execv), and use the remaining arguments
 *   as second argument to the execv() command.
 *
*/

    int pid = fork();
    int child_status = 0;
    if (pid < 0)
    {
	printf("Failed to fork with error %d\n", errno);
	return false;
    }
    else if (!pid)
    {
	    //child
	    printf("execv() %s \n", command[0]);
	    int status = execv(command[0], command);
	    //will only return if there is an error
        if (status != 0)
	    {
	        printf("Failed to run execv(): %d\n", errno);
	        exit(-1);
	    }
    }
    else
    {
	//parent
	int  wstatus = 0;
	child_status = waitpid(pid, &wstatus, 0);
	printf("After waitpid %d %d %d\n", child_status, pid, wstatus);
	if (child_status < 0 || (WIFEXITED(wstatus) && (WEXITSTATUS(wstatus) != 0)))
	{
	    printf("Failed on wait with cs %d and wstatus %d\n", child_status, wstatus);
	    return false;
	}
	else
	{
	    printf("Child was successful with cs %d and ws %d\n", child_status, wstatus);
	}
    }


    va_end(args);

    return true;
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    //command[count] = command[count];

    int of_fd = open(outputfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (of_fd < 0)
    {
        perror("Failed to open outfile..");
        exit(-1);
    }

    int kidpid;

    switch (kidpid = fork()) {
        case -1: perror("Fork failed"); abort();
        case 0:
            if (dup2(of_fd, 1) < 0)
            {
                perror("dup2 failed on outfile");
                abort();
            }
            close(of_fd);
            int ret = execv(command[0], command);
            if (ret)
            {
                perror("Failed to call execv()");
                abort();
            }
        default:
            close(of_fd);
            int wstatus = 0;
            waitpid(kidpid, &wstatus, 0);
            if ((WIFEXITED(wstatus) && (WEXITSTATUS(wstatus) != 0)))
            {
                perror("waitpid returned error status code\n");
                abort();
            }
    }
    



/*
 * TODO
 *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a refernce,
 *   redirect standard out to a file specified by outputfile.
 *   The rest of the behaviour is same as do_exec()
 *
*/

    va_end(args);

    return true;
}
