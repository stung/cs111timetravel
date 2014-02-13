// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"

#include <error.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */

struct commandIONode {
	command_t c;
	char** input;
	char** output;
	struct commandIONode *next;
	struct commandIONode *prev;
};

struct commandIOList {
	struct commandIONode *head;
	struct commandIONode *tail;
};

typedef struct commandIONode *ionode_t;
typedef struct commandIOList *iolist_t;

struct DependencyGraph {
    int** reqMatrix;
    int* numDeps;
    int numCommands;
    iolist_t commandList;
};

typedef struct DependencyGraph *depG_t;

void addIOList (iolist_t list, command_t c) {
  ionode_t new_node = (ionode_t)malloc(sizeof(struct commandIONode)); 

  new_node->c = c;
  if (list->head == NULL)
    list->head = new_node;
  
  if (list->tail == NULL)
    list->tail = new_node;
  else {	  
    list->tail->next = new_node;
    new_node->prev = list->tail;
    list->tail = new_node;
  }
}

void
intoIOCommandList(command_t c, iolist_t list) {
  if (c->type != SEQUENCE_COMMAND)
	  addIOList(list, c);
  else {
    if (c->u.command[0] != NULL)
	    intoIOCommandList(c->u.command[0], list);
    if (c->u.command[1] != NULL)
	    intoIOCommandList(c->u.command[1], list);
  }
}

depG_t generateDependecies(command_t c) {
	iolist_t ioList = (iolist_t)malloc(sizeof(struct commandIOList));
	depG_t depGraph = (depG_t)malloc(sizeof(struct DependencyGraph));

  intoIOCommandList(c, ioList);
  depGraph->commandList = ioList;
  return depGraph;
}

int
command_status (command_t c)
{
  return c->status;
}

void
execute_command (command_t c, int time_travel)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */

  pid_t child; // keeping track of the child process
  int status; // exit status
  int fd[2]; // array for file descriptors
  depG_t depGraph = NULL; // dependency graph pointer

  switch (c->type) {
  	case AND_COMMAND:
  	  // executes first command. If succeeds, executes second command
  	  // if first command fails, stop there
  	  execute_command(c->u.command[0], time_travel);
  	  if (c->u.command[0]->status == 0) { // first command succeeded
  	  	execute_command(c->u.command[1], time_travel);
  	  	c->status = c->u.command[1]->status; // sets status to second command
  	  } else {
  	  	c->status = c->u.command[0]->status; // sets status to first command
  	  }
  	  break;
  	case OR_COMMAND:
  	  // executes first command. If fails, executes second command
  	  // if first command succeeds, stop there
  	  execute_command(c->u.command[0], time_travel);
  	  if (c->u.command[0]->status != 0) { // first command failed
  	  	execute_command(c->u.command[1], time_travel);
  	  	c->status = c->u.command[1]->status; // sets status to second command
  	  } else {
  	  	c->status = c->u.command[0]->status; // sets status to first command
  	  }
  	  break;
  	case SEQUENCE_COMMAND:
      if (time_travel) {
  	   printf("time travel mode, start generate dependency\n");
       depGraph = generateDependecies(c);
       ionode_t commHead = depGraph->commandList->head;
       while (commHead != NULL) {
        child = fork();
        if (child == 0) { // child process
          execute_command(commHead->c, 0);
          break;
        } else if (child > 0) { // parent process
          commHead = commHead->next;
          continue;
        } else {
          error(1, 0, "Cannot generate child process!");
        }
       }
        /*
	   depgG_t deps = generateDependecies(c);
        int i = 0;
        for (; i < deps->numCommands; i++) {
          child = fork();
          if (child == 0) { // child process
            // FIXME: execute_command(), which command?
          } else if (child > 0) { // parent process
            if (deps->numDeps[i] == 0) {
              continue;
            } else {
              waitpid(child, &status, 0); // wait for child to complete
            }
          } else {
            error(1, 0, "Cannot generate child process!");
          }
        }
        */
      } else {
    	  // execute both commands sequentially
    	  execute_command(c->u.command[0], 0);
    	  execute_command(c->u.command[1], 0);
    	  c->status = c->u.command[1]->status; // sets status to second command
      }
  	  break;
  	case PIPE_COMMAND:
  	  // redirects output of cmd1 into cmd2
  	  if (pipe(fd) == -1)
  	  	error(1, 0, "Cannot create pipe!");

  	  child = fork();
  	  if (child == 0) { // child writes to the pipe
  	  	close(fd[0]); // close the read end of the pipe

  	  	// writing to pipe instead of STDOUT
  	  	if (dup2(fd[1], STDOUT_FILENO) == -1)
  	  		error(1, 0, "Cannot redirect output!");
  	  	execute_command(c->u.command[0], time_travel);
  	  	close(fd[1]); // close write end of the pipe
  	  	exit(c->u.command[0]->status);
  	  } else if (child > 0) { // parent reads the pipe
  	  	close(fd[1]); // close write end of the pipe
  	  	waitpid(child, &status, 0); // wait for child to exit

  	  	printf("Status is %d, child status is %d\n", 
  	  		status, c->u.command[0]->status);

  	  	c->u.command[0]->status = status;

  	  	// reading from pipe instead of STDIN
  	  	if (dup2(fd[0], STDIN_FILENO) == -1)
  	  		error(1, 0, "Cannot redirect input!");
  	  	execute_command(c->u.command[1], time_travel);
  	  	close(fd[0]); // close read end of the pipe
  	  	c->status = c->u.command[1]->status;
  	  } else
  	  	error(1, 0, "Cannot create child process!");
  	  break;
  	case SIMPLE_COMMAND:
  	  // allow the kernel to execute the command
  	  child = fork();
  	  if (child == 0) { // child process
  	  	// handle redirects
  	  	int fd_in;
  	  	int fd_out;
  	  	if (c->input) {
  	  		// eg. cmd < file
  	  		if ((fd_in = open(c->input, O_RDONLY, 0666)) == -1)
  	  			error(1, 0, "Cannot open input file!");
  	  		if (dup2(fd_in, STDIN_FILENO) == -1)
  	  			error(1, 0, "Cannot do input redirect!");
  	  	}
  	  	if (c->output) {
  	  		// eg. cmd > file
  	  		if ((fd_out = open(c->output, O_WRONLY|O_CREAT|O_TRUNC, 0666)) == -1)
  	  			error(1, 0, "Cannot open output file!");
  	  		if (dup2(fd_out, STDOUT_FILENO) == -1)
  	  			error(1, 0, "Cannot do output redirect!");
  	  	}

  	  	// handle execution
  	  	execvp(c->u.word[0], c->u.word); // includes original command in argv array
  	  	error(1, 0, "Cannot execute command!"); // if we go beyond the execvp
  	  } else if (child > 0) { // parent process, pid of the child process returned
  	  	// wait for child process
  	  	waitpid(child, &status, 0);

  	  	// read and store the exit status
  	  	c->status = status;
  	  } else {
  	  	error(1, 0, "Cannot create child process!");
  	  }
  	  break;
  	case SUBSHELL_COMMAND:
  	  execute_command(c->u.subshell_command, 0);
  	  break;
  	default:
  	  break;
  }
}
