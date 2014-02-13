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
#include <string.h>

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

struct childNode {
  pid_t child;
  struct childNode *next;
};

struct childList {
  struct childNode *head;
  struct childNode *tail;
};

typedef struct childNode *cnode_t;
typedef struct childList *clist_t;

void addChildList (clist_t list, pid_t child) {
  cnode_t new_node = (cnode_t)malloc(sizeof(struct childNode));

  new_node->child = child;
  if (list->head == NULL)
    list->head = new_node;
  if (list->tail == NULL)
    list->tail = new_node;
  else {
    list->tail->next = new_node;
    list->tail = new_node;
  }
}

// adds to the doubly linked sequence command list
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

// traverses the command tree
// adds individual sequence commands in order of execution
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

// traverses the sequence command node
// finds all input/outputs and adds them to the node data members
void findCommandIO(ionode_t node, command_t c) {
  if (c->input != 0) {
	  int i = 0;
	  while(*(node->input + i) != NULL) {
		  i++;
	  }
	  unsigned dataLength = strlen(c->input);
	  *(node->input + i) = (char *)malloc(sizeof(char *) * dataLength);
	  strncpy(*(node->input + i), c->input, dataLength);
  }
  if (c->output != 0) {
	  int j = 0;
	  while(*(node->output + j) != NULL) {
		  j++;
	  }
	  unsigned dataLength = strlen(c->output);
	  *(node->output + j) = (char *)malloc(sizeof(char *) * dataLength);
	  strncpy(*(node->output + j), c->output, dataLength);
  }
  if (c->type == AND_COMMAND || c->type == OR_COMMAND || c->type == PIPE_COMMAND) {
	  findCommandIO(node, c->u.command[0]);
	  findCommandIO(node, c->u.command[1]);
  } else if (c->type == SUBSHELL_COMMAND) {
	  findCommandIO(node, c->u.subshell_command);
  }
}

// searches for dependencies
int findOutputFile(ionode_t n, char* fileName, int index) {
	if (n == NULL)
    return -1;

  int i = 0;
	while (*(n->output + i) != NULL) {
		if (strcmp(*(n->output + i), fileName) == 0)
			return index;
		i++;
	}
	if (n->prev == NULL)
		return -1;
	else
		return findOutputFile(n->prev, fileName, index - 1);
}

// updates numDeps according to reqMatrix
void updateNumDeps(depG_t g, int index) {
	int depTotal = 0;
	int tmp;
	for (tmp = 0; tmp < index; tmp++)
		if (g->reqMatrix[index][tmp] == 1)
			depTotal++;
	g->numDeps[index] = depTotal;
}

// initially sets reqMatrix
void setDep(depG_t g, ionode_t n, int index) {
	int i = 0;
	while (*(n->input + i) != NULL) {
		int depIndex = findOutputFile(n->prev, *(n->input + i), index - 1);
		if (depIndex != -1)
			g->reqMatrix[index][depIndex] = 1;
		i++;
	}
	int j = 0;
	while (*(n->output + j) != NULL) {
		int depIndex = findOutputFile(n->prev, *(n->output + j), index - 1);
		if (depIndex != -1)
			g->reqMatrix[index][depIndex] = 1;
		j++;
	}
	updateNumDeps(g, index);
}

// generates the dependency graph for command c
depG_t generateDependecies(command_t c) {
	iolist_t ioList = (iolist_t)malloc(sizeof(struct commandIOList));
	depG_t depGraph = (depG_t)malloc(sizeof(struct DependencyGraph));

  intoIOCommandList(c, ioList);
  depGraph->commandList = ioList;
	
	ionode_t tmpNode = ioList->head;
	int num = 0;
	while(tmpNode != NULL) {
	  num++;
	  tmpNode->input = (char **)malloc(sizeof(char **) * 20); //FIXME: CANNOT BE STATIC
	  tmpNode->output = (char **)malloc(sizeof(char **) * 20); //FIXME: CANNOT BE STATIC
	  findCommandIO(tmpNode, tmpNode->c);
	  tmpNode = tmpNode->next;
	}
	depGraph->numCommands = num;
	depGraph->numDeps = (int *)calloc(num, sizeof(int));

	depGraph->reqMatrix = (int **)malloc(num * sizeof(int*));
	int i;
	for(i = 0; i < num; i++) 
		depGraph->reqMatrix[i] = (int *)calloc(num, sizeof(int));

	ionode_t node = ioList->head;
	int j = 0;
	while(j < num) {
		setDep(depGraph, node, j);
		node = node->next;
		j++;
	}

     return depGraph;
}

// runs all of the commands until empty
void runDependencies(depG_t g, int* run_array, command_t* comm_array) {
  pid_t child;
  int status;
  ionode_t commHead = g->commandList->head;
  clist_t child_list = (clist_t)malloc(sizeof(struct childList));

  int i;
  int j;
  int isAllRun = 1;
  for(i = 0; i < g->numCommands; i++) {
    if (g->numDeps[i] == 0) {
      if (run_array[i] == 0) {
        isAllRun = 0;
        run_array[i] = 1;
        child = fork();
        if (child == 0) { // child process
          execute_command(comm_array[i], 0);
          return;
        } else if (child > 0) { // parent process
          addChildList(child_list, child);
          for (j = 0; j < g->numCommands; j++) {
            g->reqMatrix[j][i] = 0;
          }
          continue;
        } else {
          error(1, 0, "Cannot generate child process!");
        }
      }
    }
  }

  cnode_t nodeHead = child_list->head;
  while(nodeHead != NULL) {
    waitpid(nodeHead->child, &status, 0);
    nodeHead = nodeHead->next;
  }

  for (i = 0; i < g->numCommands; i++) {
    updateNumDeps(g, i);
  }

  if (!isAllRun) {
    runDependencies(g, run_array, comm_array);
  }
  return;
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
       depGraph = generateDependecies(c);
       int num = depGraph->numCommands;
       ionode_t commHead = depGraph->commandList->head;

       command_t* comm_array = (command_t *)calloc(num, sizeof(command_t));
       int *run_array = (int *)calloc(num, sizeof(int));
       int i;
       for (i = 0; i < num; i++) {
        comm_array[i] = commHead->c;
        commHead = commHead->next;
       }

       runDependencies(depGraph, run_array, comm_array);

       /*
       ionode_t lastComm;
       pid_t lastChild;
       char isLastComm = 0;

       while (commHead != NULL) {
        child = fork();
        if (child == 0) { // child process
          execute_command(commHead->c, 0);
          c->status = commHead->c->status;
          break;
        } else if (child > 0) { // parent process
          lastChild = child;
          lastComm = commHead;
          commHead = commHead->next;
          if (commHead == NULL)
            isLastComm = 1;
          continue;
        } else {
          error(1, 0, "Cannot generate child process!");
        }
       }
       if (isLastComm) {
         // wait for child process
         waitpid(lastChild, &status, 0);
         c->status = status;
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
