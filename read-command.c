// UCLA CS 111 Lab 1 command reading

#include "command.h"
#include "command-internals.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <error.h>
#include <ctype.h>

#define DEBUG 0

enum parse_type
{
  SUBSHELL,
  REDIRECT_LESS,
  REDIRECT_MORE,
  PIPE,
  AMPERSAND,
  OR,
  SEMICOLON,
  NEWLINE,
  WORD,
};

struct token_node
{
  char *token;
  enum parse_type comType;
  struct token_node *next_token;
};

typedef struct token_node *token_node_t;

int isWords(int c) {
	if (isalpha(c) || isdigit(c) || (c == '!') || (c == '%') || (c == '+') 
		|| (c == '-') || (c == '.') || (c == '/') || (c == ':') || (c =='@') 
			|| (c == '^') || (c == '_'))
		return 1;
	else
		return 0;
}

int isSpecialTokens(int c) {
	if ((c == ';') || (c == '|') || (c == '&') || (c == '(') || (c == ')') 
			|| (c == '<') || (c == '>'))
		return 1;
	else
		return 0;
}

// Check if the input char valid
void checkIfValid(int c) {
	if (!(isWords(c) || isSpecialTokens(c) || (c == '\n')
			|| (c == ' ') || (c == '\t') || (c == '#') || (c == EOF))) {
       error(1, 0, "Invalid char detected");
   }
}

// for AND_COMMAND, PIPE_COMMAND, SEQUENCE_COMMAND, PIPE_COMMAND
// check if the previous and the next token valid
void checkIfSimpleOrSubshell(token_node_t token) {
	if (token == NULL)
		error(1, 0, "Invalid input!!!");
	if (!(token->comType == SUBSHELL || token->comType == WORD)) {
		printf("%s\n", token->token);
		error(1, 0, "Invalid input!!!");
	}
}


// inputs: tail of a linked list, string data
// output: pointer to the tail of the linked list
token_node_t insert_at_end (token_node_t oldtail, char *data, enum parse_type comType) {
  token_node_t newtail = (token_node_t)malloc(sizeof(struct token_node)); 

  // Assume data is a null-terminated String
  unsigned strlength = strlen(data);
  newtail->token = (char *)malloc(sizeof(char) * strlength);
  strncpy(newtail->token, data, strlength);

  if (oldtail != NULL)
    oldtail->next_token = newtail;
  newtail->next_token = NULL;
  newtail->comType = comType;
  return newtail;
}

// input: tail of a linked list, special character
// output: tail of a linked list
token_node_t read_singlespecial (token_node_t tail, int c) {
  char token[2];
  memset(token, '\0', sizeof(token));
  sprintf(token, "%c", c);
  switch (c) {
    case ';':
      return insert_at_end(tail, token, SEMICOLON);
    case '(':
    case ')':
      return insert_at_end(tail, token, SUBSHELL);
    case '<':
      return insert_at_end(tail, token, REDIRECT_LESS);
    case '>':
      return insert_at_end(tail, token, REDIRECT_MORE);
    default:
      // Should not be this case
      error(1, 0, "Invalid input!!!");
      break;
  }
  return NULL;
}

// input: tail of a linked list, character stream
// output: tail of a linked list
token_node_t read_ampersand (token_node_t tail, void *stream) {
  int c;
  c = getc(stream);

  if (c == '&') {
    char *token = "&&";
    return insert_at_end(tail, token, AMPERSAND);
  } else {
    // If there is '&' in command, it should be "&&"
    // Any other case is invalid
    error(1, 0, "Invalid input!!!");
  }
  return NULL;
}

// input: tail of a linked list, character stream
// output: tail of a linked list
token_node_t read_pipe (token_node_t tail, void *stream) {
  int c;
  c = getc(stream);

  if (c == '|') {
    char *token = "||";
    return insert_at_end(tail, token, OR);
  } else {
    // push the char back onto the stream
    ungetc(c, stream);
    char *token = "|";
    return insert_at_end(tail, token, PIPE);
  }
}

// input: tail of a linked list, character stream
// output: tail of a linked list
token_node_t read_newline (token_node_t tail) {
  // Tail will be null if there is not yet any meaningful token from the beginning of the file.
  // New line can be ignore in this case
  if (tail == NULL)
	  return NULL;

  if ((tail->comType == OR) || (tail->comType == PIPE) || 
        (tail->comType == AMPERSAND) || (tail->comType == NEWLINE)) {
    // New line is the same as white space in these cases
    // No need to attach '\n' to the token list
    return tail;
  } else {
    char *token = "\n";
    return insert_at_end(tail, token, NEWLINE);
  }
}

// input: tail of a linked list, character stream, read character
// output: tail of a linked list
token_node_t read_word (token_node_t tail, void *stream, int c) {
  char newtoken[100]; //FIXME: CANNOT BE STATIC
  memset(newtoken, '\0', sizeof(newtoken));
  token_node_t tmp = (token_node_t)malloc(sizeof(struct token_node));

  char appendchar[2];
  memset(appendchar, '\0', sizeof(appendchar));
  sprintf(appendchar, "%c", c);
  strcat(newtoken, appendchar);

  int nextchar;
  nextchar = getc(stream);

  // check if it's a comment, ignore them until new line
  if (c == '#') {
	 while(nextchar != '\n') {
        nextchar = getc(stream);
	 }
	 return read_newline(tail);
  }
  
  char shouldExit = 0;
  while (nextchar != EOF && !shouldExit) {
    checkIfValid(nextchar);
    switch (nextchar) {
      case ';':
      case '(':
      case ')':
      case '<':
      case '>':
      case '&':
      case '|':
      case '\n':
    	case ' ':
      case '\t':
      case EOF:
        ungetc(nextchar, stream);
        shouldExit = 1;
        break;
      default:
        sprintf(appendchar, "%c", nextchar);
        strcat(newtoken, appendchar);
        nextchar = getc(stream);
        break;
    }
  }
  if (tail != NULL) {
    if (tail->comType == WORD) {
  	  tail->token = strcat(tail->token, " ");
  	  tail->token = strcat(tail->token, newtoken);
	  return tail;
    }
  }
  tmp = insert_at_end(tail, newtoken, WORD); 
  return tmp;
}

token_node_t intoTokens(int (*get_next_byte) (void *),
         void *get_next_byte_argument)
{

  // Tokeninzing our commands
  token_node_t tokenlist_head = NULL;
  token_node_t tokenlist_end = tokenlist_head;

  int c;
  while((c = get_next_byte(get_next_byte_argument)) != EOF) {
    
    checkIfValid(c);

    switch (c) {
      case ';':
      case '(':
      case ')':
        tokenlist_end = read_singlespecial(tokenlist_end, c);
        break;
      case '<':
      case '>':
        checkIfSimpleOrSubshell(tokenlist_end);
        tokenlist_end = read_singlespecial(tokenlist_end, c);
        break;
      case '&':
        tokenlist_end = read_ampersand(tokenlist_end, get_next_byte_argument);
        break;
      case '|':
        tokenlist_end = read_pipe(tokenlist_end, get_next_byte_argument);
        break;
      case '\n':
	      if (tokenlist_end != NULL)
		      if((tokenlist_end->comType == REDIRECT_LESS) || (tokenlist_end->comType == REDIRECT_MORE))
			      error(1, 0, "Invalid input!!!");
        tokenlist_end = read_newline(tokenlist_end);
        break;
      case ' ':
      case '\t':
    	case EOF:
    	  break;
    	default:
        tokenlist_end = read_word(tokenlist_end, get_next_byte_argument, c);
        break;
    };

    // Set token list head when get the the first token
    if (tokenlist_head == NULL)
      tokenlist_head = tokenlist_end;
  } 
  return tokenlist_head;
}

// STACK IMPLEMENTATION
struct stack
{
  struct token_node *topNode;
};

typedef struct stack *stack_t;

// Input: Stack pointer, String data to be new top
// Output: void
void push(stack_t theStack, char* data, enum parse_type type) {
  token_node_t newtop = (token_node_t)malloc(sizeof(struct token_node));

  // Assume data is a null-terminated String
  unsigned strlength = strlen(data);
  newtop->token = (char *)malloc(sizeof(char) * strlength);
  strncpy(newtop->token, data, strlength);
  newtop->comType = type;

  if (theStack == NULL)
    theStack = (stack_t)malloc(sizeof(struct stack));
  else
    newtop->next_token = theStack->topNode;
  
  theStack->topNode = newtop;
}

// Input: Stack pointer
// Output: Stack Node pointer
token_node_t pop(stack_t theStack) {
  token_node_t newtop = NULL;
  token_node_t oldtop = NULL;

  // Rearranging the new top node
  if (theStack == NULL) {
    //printf("ERROR: Stack is NULL!\n");
    return oldtop;
  }

  // The old top will be the stack's current top node
  oldtop = theStack->topNode;
  if (oldtop == NULL) {
    //printf("ERROR: Nothing in stack!\n");
    return oldtop;
  }

  // The new top is the node below the top node
  // Set the old top's next node pointer to NULL for safety
  newtop = oldtop->next_token;
  oldtop->next_token = NULL;

  // Updating the stack's top node to be the new top
  theStack->topNode = newtop;
  return oldtop;
}

token_node_t inToRPN(token_node_t inTokens) {
  token_node_t rpnTokens = NULL; // (token_node_t)malloc(sizeof(struct token_node));
  token_node_t rpnTokens_end = rpnTokens;

  stack_t operatorStack = (stack_t)malloc(sizeof(struct stack));
  memset(operatorStack, 0, sizeof(struct stack));

  token_node_t readNode = (token_node_t)malloc(sizeof(struct token_node));
  memset(readNode, 0, sizeof(struct token_node));

  char rpn[100]; //FIXME: CANNOT BE STATIC
  memset(rpn, '\0', sizeof(rpn));

  while(inTokens != NULL) {
    switch (inTokens->comType) {
      case WORD:
        rpnTokens_end = insert_at_end(rpnTokens_end, inTokens->token, inTokens->comType);
        break;
      case NEWLINE:
        if (operatorStack == NULL) {
          rpnTokens_end = insert_at_end(rpnTokens_end, inTokens->token, inTokens->comType);
          break;
        }
        while (operatorStack->topNode != NULL) {
          readNode = pop(operatorStack);
		if (strcmp(readNode->token, "(") == 0) {
			error(1, 0, "Invalid input!!!");
		}
          rpnTokens_end = insert_at_end(rpnTokens_end, readNode->token, readNode->comType);
        }
        rpnTokens_end = insert_at_end(rpnTokens_end, inTokens->token, inTokens->comType);
        break;
      case SUBSHELL:
        if (strcmp(inTokens->token, "(") == 0) {
          push(operatorStack, inTokens->token, inTokens->comType);
        } else {
          while ((readNode = pop(operatorStack)) != NULL) {
            rpnTokens_end = insert_at_end(rpnTokens_end, readNode->token, readNode->comType);
            if (strcmp(readNode->token, "(") == 0)
              break;
          }
		if (readNode == NULL) 
			error(1, 0, "Invalid input!!!");
        }
        break;
      case REDIRECT_MORE:
      case REDIRECT_LESS:
        rpnTokens_end = insert_at_end(rpnTokens_end, inTokens->token, inTokens->comType);
        break;
      case PIPE:
	   checkIfSimpleOrSubshell(rpnTokens_end);
	   checkIfSimpleOrSubshell(inTokens->next_token);
        
	   readNode = operatorStack->topNode;
        if (readNode == NULL) {
          push(operatorStack, inTokens->token, inTokens->comType);
        } else if (strcmp(readNode->token, "|") == 0) {
          readNode = pop(operatorStack);
          rpnTokens_end = insert_at_end(rpnTokens_end, readNode->token, readNode->comType);
          push(operatorStack, inTokens->token, inTokens->comType);
        } else {
          push(operatorStack, inTokens->token, inTokens->comType);
	   }
        break;
      case AMPERSAND:
      case OR:
	   checkIfSimpleOrSubshell(rpnTokens_end);
	   checkIfSimpleOrSubshell(inTokens->next_token);
        
	   readNode = operatorStack->topNode;
        if (readNode == NULL) {
          push(operatorStack, inTokens->token, inTokens->comType);
        } else if (strcmp(readNode->token, ";") == 0) {
          push(operatorStack, inTokens->token, inTokens->comType);
        } else if (strcmp(readNode->token, "(") == 0) {
          push(operatorStack, inTokens->token, inTokens->comType);
        } else if (strcmp(readNode->token, "|") == 0) {
          while ((readNode = pop(operatorStack)) != NULL) {
            rpnTokens_end = insert_at_end(rpnTokens_end, readNode->token, readNode->comType);
            if ((strcmp(readNode->token, "(") == 0) || (strcmp(readNode->token, ";") == 0)) {
      		    push(operatorStack, readNode->token, readNode->comType);
              break;
      		  }
          }
		push(operatorStack, inTokens->token, inTokens->comType);
	   } else {
          readNode = pop(operatorStack);
          rpnTokens_end = insert_at_end(rpnTokens_end, readNode->token, readNode->comType);
          push(operatorStack, inTokens->token, inTokens->comType);
        }
        break;
      case SEMICOLON:
	   checkIfSimpleOrSubshell(rpnTokens_end);
	   if (inTokens->next_token->comType != NEWLINE) {
	   	checkIfSimpleOrSubshell(inTokens->next_token);

          readNode = operatorStack->topNode;
          if (readNode == NULL) {
            push(operatorStack, inTokens->token, inTokens->comType);
          } else if (strcmp(readNode->token, "(") == 0) {
            push(operatorStack, inTokens->token, inTokens->comType);
          } else if (strcmp(readNode->token, ";") == 0) {
            readNode = pop(operatorStack);
            rpnTokens_end = insert_at_end(rpnTokens_end, readNode->token, readNode->comType);
            push(operatorStack, inTokens->token, inTokens->comType);
	     } else {
            while ((readNode = pop(operatorStack)) != NULL) {
              rpnTokens_end = insert_at_end(rpnTokens_end, readNode->token, readNode->comType);
              if (strcmp(readNode->token, "(") == 0) {
      	      push(operatorStack, readNode->token, readNode->comType);
                break;
              }
            }
		  push(operatorStack, inTokens->token, inTokens->comType);
	     }
	   }
        break;
      default:
        //printf("Not a valid comType?\n");
        break;
    }

    if (rpnTokens == NULL)
      rpnTokens = rpnTokens_end;

    // Obtain next token
    inTokens = inTokens->next_token;
  }
  return rpnTokens;
}


// RPN to Command Tree Stack
struct ctStack_node
{
  command_t currCommand;
  struct ctStack_node *next_node;
};

struct command_stream
{
  struct ctStack_node *topNode;
};

typedef struct ctStack_node *ctnode_t;

// Input: command stream, command to be new top
// Output: void
void ctPush(command_stream_t inCTStack, command_t data) {
  ctnode_t newtop = (ctnode_t)malloc(sizeof(struct ctStack_node));
  command_t newCommand = (command_t)malloc(sizeof(struct command));

  newCommand->type = data->type;
  newCommand->status = data->status;
  newCommand->input = data->input;
  newCommand->output = data->output;
  newCommand->u = data->u;

  newtop->currCommand = newCommand;

  // If the current stack is empty
  if (inCTStack == NULL)
    inCTStack = (command_stream_t)malloc(sizeof(struct command_stream));
  else
    newtop->next_node = inCTStack->topNode;
  // Stack's new top node will be newtop
  inCTStack->topNode = newtop;
  return;
}

// Input: command stream
// Output: command Node pointer
ctnode_t ctPop(command_stream_t inCTStack) {
  ctnode_t newtop = NULL;
  ctnode_t oldtop = NULL;

  // Rearranging the new top node
  if (inCTStack == NULL) {
    return oldtop;
  }

  // The old top will be the stack's current top node
  oldtop = inCTStack->topNode;
  if (oldtop == NULL) {
    return oldtop;
  }

  // The new top is the node below the top node
  // Set the old top's next node pointer to NULL for safety
  newtop = oldtop->next_node;
  oldtop->next_node = NULL;

  // Updating the stack's top node to be the new top
  inCTStack->topNode = newtop;
  return oldtop;
}

// Dependency Graph Node
struct depNode
{
  command_t depCommand;
  char** inputs;
  char** outputs;
};

typedef struct depNode *depnode_t;

// order of the commands
struct cmdOrder
{
  depnode_t head;
};

typedef struct cmdOrder *corder_t;

// Input: token list of RPN tokens
// Output: command stream of commands
command_stream_t rpnToCommTree(token_node_t inRPNTokens) {
  command_stream_t outCommStream =
        (command_stream_t)malloc(sizeof(struct command_stream));
  memset(outCommStream, 0, sizeof(struct command_stream));
  ctnode_t readNode = (ctnode_t)malloc(sizeof(struct ctStack_node)); 
  memset(readNode, 0, sizeof(struct ctStack_node));

  command_t readData = (command_t)malloc(sizeof(struct command));
  memset(readData, 0, sizeof(struct command));

  // setting up dependencies for time travel
  corder_t cmdList = (corder_t)malloc(sizeof(struct cmdOrder));
  memset(cmdList, 0, sizeof(struct cmdOrder));


  while(inRPNTokens != NULL) {
    switch (inRPNTokens->comType) {
      case WORD:
        readData->type = SIMPLE_COMMAND;
        readData->status = -1;
        readData->input = 0;
        readData->output = 0;

        readData->u.word = (char **)malloc(sizeof(char **) * 20); //FIXME: CANNOT BE STATIC
        
	      char *tok = NULL;
	      tok = strtok(inRPNTokens->token, " ");

        int i = 0, n;
        while(tok) {
		      // Assume null terminated Strings
          unsigned dataLength = strlen(tok);
          
      		*(readData->u.word + i) = (char *)malloc(sizeof(char *) * dataLength);
          memset(*(readData->u.word + i), 0, sizeof(char *) * dataLength); // FIXME: CANNOT BE STATIC

          strncpy(*(readData->u.word + i), tok, dataLength);

          tok = strtok(NULL, " ");
          i++;
        }
        ctPush(outCommStream, readData);
        break;
      case NEWLINE:
        //FIXME
	      break;
      case SUBSHELL:
        // initializing a new command
        readData->type = SUBSHELL_COMMAND;
        readData->status = - 1;
        readData->input = 0;
        readData->output = 0;

        // popping out the command to be subshelled
        ctnode_t subShellCommand = NULL;
        subShellCommand = ctPop(outCommStream);

        // set our new command union command, push it back onto the stack
        readData->u.subshell_command = subShellCommand->currCommand;
	   readData->type = SUBSHELL_COMMAND;
        ctPush(outCommStream, readData);
	      break;
      case REDIRECT_MORE:
        inRPNTokens = inRPNTokens->next_token;
        ctnode_t outputnode = NULL;
	      outputnode = ctPop(outCommStream);
	      outputnode->currCommand->output = inRPNTokens->token; //FIXME: should check if next token is SIMPLE COMMAND
	      ctPush(outCommStream, outputnode->currCommand);

        // output dependency

        break;
      case REDIRECT_LESS:
        inRPNTokens = inRPNTokens->next_token;
        ctnode_t inputnode = NULL;
	      inputnode = ctPop(outCommStream);
	      inputnode->currCommand->input = inRPNTokens->token; //FIXME: should check if next token is SIMPLE COMMAND
	      ctPush(outCommStream, inputnode->currCommand);
        break;
      case PIPE:
      case AMPERSAND:
      case OR:
      case SEMICOLON:
        readData->status = -1;
        readData->input = 0;
        readData->output = 0;

        // subCommands to be written to
        ctnode_t subCommand = NULL;
        int j = 0;
        while (j < 2) {
          subCommand = ctPop(outCommStream);
		readData->u.command[1 - j] = subCommand->currCommand;
          j++;
        }

        // Assigning the command type
        if (inRPNTokens->comType == PIPE) {
          readData->type = PIPE_COMMAND;
        } else if (inRPNTokens->comType == AMPERSAND) {
          readData->type = AND_COMMAND;
        } else if (inRPNTokens->comType == OR) {
          readData->type = OR_COMMAND;
        } else if (inRPNTokens->comType == SEMICOLON) {
          readData->type = SEQUENCE_COMMAND;
        } else {
          error(1, 0, "Invalid input!!!");
        }
        
        ctPush(outCommStream, readData);
        break;
      default:
        error(1, 0, "Invalid input!!!");
        break;
    }
    // Obtain next token
    inRPNTokens = inRPNTokens->next_token;
	 // printf("%s\n", inRPNTokens->token);
  }

  command_stream_t commStream =
        (command_stream_t)malloc(sizeof(struct command_stream));
  memset(commStream, 0, sizeof(struct command_stream));
  ctnode_t tmpNode = (ctnode_t)malloc(sizeof(struct ctStack_node));
  memset(readNode, 0, sizeof(struct ctStack_node));
  
  // Reversing the order of the stack
  while((tmpNode = ctPop(outCommStream)) != NULL) {
  	ctPush(commStream, tmpNode->currCommand);
  }

  return commStream;
}

command_stream_t
make_command_stream (int (*get_next_byte) (void *),
         void *get_next_byte_argument)
{
  // Parsing file to token list
  token_node_t tokenlist_head = NULL;
  tokenlist_head = intoTokens(get_next_byte, get_next_byte_argument);

  
  if (DEBUG) {
    token_node_t test = tokenlist_head;
    while(test != NULL) {
      printf("%s ", test->token);
      test = test->next_token;
    }
  }

  // Parsing infix into RPN
  token_node_t rpnTokens = NULL;
  rpnTokens = inToRPN(tokenlist_head);

  if (DEBUG) {
    token_node_t rpnTest = rpnTokens;
    while(rpnTest != NULL) {
      printf("%s ", rpnTest->token);
      rpnTest = rpnTest->next_token;
    }
  } 

  // RPN Parser to Command Stream
  command_stream_t outStream = NULL;
  outStream = rpnToCommTree(rpnTokens);

  return outStream;
}

command_t
read_command_stream (command_stream_t s)
{
  command_t outCommand = NULL;
  ctnode_t outNode = NULL;
  outNode = ctPop(s);
  if (outNode == NULL)
	  return outCommand;
  outCommand = outNode->currCommand;
  return outCommand;
}
