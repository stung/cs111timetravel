// UCLA CS 111 Lab 1 command reading

#include "command.h"
#include "command-internals.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <error.h>

#define DEBUG 1

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */

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
  token_node_t tmp = (token_node_t)malloc(sizeof(struct token_node));
  //tmp = insert_at_end(tail, token);
  switch (c) {
    case ';':
      tmp = insert_at_end(tail, token, SEMICOLON);
      break;
    case '(':
    case ')':
      tmp = insert_at_end(tail, token, SUBSHELL);
      break;
    case '<':
      tmp = insert_at_end(tail, token, REDIRECT_LESS);
      break;
    case '>':
      tmp = insert_at_end(tail, token, REDIRECT_MORE);
      break;
    default:
      break;
  }
  return tmp;
}

// input: tail of a linked list, character stream
// output: tail of a linked list
token_node_t read_ampersand (token_node_t tail, void *stream) {
  int c;
  c = getc(stream);
  token_node_t tmp = (token_node_t)malloc(sizeof(struct token_node));
  if (c == '&') {
    char *token = "&&";
    tmp = insert_at_end(tail, token, AMPERSAND);
  } else {
    // push the char back onto the stream
    ungetc(c, stream);
    char *token = "&";
    tmp = insert_at_end(tail, token, WORD);
  }
  return tmp;
}

// input: tail of a linked list, character stream
// output: tail of a linked list
token_node_t read_pipe (token_node_t tail, void *stream) {
  int c;
  c = getc(stream);
  token_node_t tmp = (token_node_t)malloc(sizeof(struct token_node));
  if (c == '|') {
    char *token = "||";
    tmp = insert_at_end(tail, token, OR);
  } else {
    // push the char back onto the stream
    ungetc(c, stream);
    char *token = "|";
    tmp = insert_at_end(tail, token, PIPE);
  }
  return tmp;
}

// input: tail of a linked list, character stream
// output: tail of a linked list
token_node_t read_newline (token_node_t tail) {
  if (tail == NULL)
	  return NULL;

  if ((tail->comType == OR) || (tail->comType == PIPE) || 
        (tail->comType == AMPERSAND) || (tail->comType == NEWLINE)) {
    return tail;
  } else {
    token_node_t tmp = NULL; //(token_node_t)malloc(sizeof(struct token_node));
    char *token = "\n";
    tmp = insert_at_end(tail, token, NEWLINE);
    return tmp;
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
  tmp = insert_at_end(tail, newtoken, WORD); 
  return tmp;
}

token_node_t intoTokens(int (*get_next_byte) (void *),
         void *get_next_byte_argument) {

  // Tokeninzing our commands
  token_node_t tokenlist_head = NULL;
  token_node_t tokenlist_end = tokenlist_head;

  int c;
  while((c = get_next_byte(get_next_byte_argument)) != EOF) {
    switch (c) {
      case ';':
      case '(':
      case ')':
      case '<':
      case '>':
        tokenlist_end = read_singlespecial(tokenlist_end, c);
        break;
      case '&':
        tokenlist_end = read_ampersand(tokenlist_end, get_next_byte_argument);
        break;
      case '|':
        tokenlist_end = read_pipe(tokenlist_end, get_next_byte_argument);
        break;
      case '\n':
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
    printf("ERROR: Stack is NULL!\n");
    return oldtop;
  }

  // The old top will be the stack's current top node
  oldtop = theStack->topNode;
  if (oldtop == NULL) {
    printf("ERROR: Nothing in stack!\n");
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
          rpnTokens_end = insert_at_end(rpnTokens_end, readNode->token, readNode->comType);
        }
        rpnTokens_end = insert_at_end(rpnTokens_end, inTokens->token, inTokens->comType);
        break;
      case SUBSHELL:
        if (strcmp(inTokens->token, "(") == 0) {
          push(operatorStack, inTokens->token, inTokens->comType);
          break;
        } else {
          readNode = pop(operatorStack);
          if (readNode == NULL)
            break;
          else {
            rpnTokens_end = insert_at_end(rpnTokens_end, readNode->token, readNode->comType);

            // pop all the way until you hit the open paren in the stack
            while (strcmp(readNode->token, "(") != 0) {
              readNode = pop(operatorStack);
              if (readNode != NULL)
                rpnTokens_end = insert_at_end(rpnTokens_end, readNode->token, readNode->comType);
              else {
                break;
              }
            }
            break;
          }
          break;
        }
        break;
      case REDIRECT_MORE:
        rpnTokens_end = insert_at_end(rpnTokens_end, inTokens->token, inTokens->comType);
        break;
      case REDIRECT_LESS:
        rpnTokens_end = insert_at_end(rpnTokens_end, inTokens->token, inTokens->comType);
        break;
      case PIPE:
      case AMPERSAND:
      case OR:
        readNode = operatorStack->topNode;
        if (readNode == NULL) {
          push(operatorStack, inTokens->token, inTokens->comType);
        } else if (strcmp(readNode->token, ";") == 0) {
          push(operatorStack, inTokens->token, inTokens->comType);
        } else if (strcmp(readNode->token, "(") == 0) {
          push(operatorStack, inTokens->token, inTokens->comType);
        } else {
          readNode = pop(operatorStack);
          rpnTokens_end = insert_at_end(rpnTokens_end, readNode->token, readNode->comType);
          push(operatorStack, inTokens->token, inTokens->comType);
        }
        break;
      case SEMICOLON:
        if (operatorStack == NULL) {
            rpnTokens_end = insert_at_end(rpnTokens_end, inTokens->token, inTokens->comType);
            break;
          }
          readNode = operatorStack->topNode;
          // Stop when stack is empty or when you hit a subshell
          while ((readNode != NULL) && 
                  (strcmp(readNode->token, "(") != 0)) {
            readNode = pop(operatorStack);
            rpnTokens_end = insert_at_end(rpnTokens_end, readNode->token, readNode->comType);
          }
          rpnTokens_end = insert_at_end(rpnTokens_end, inTokens->token, inTokens->comType);
        break;
      default:
        printf("Not a valid comType?\n");
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

/* FIXME: Define the type 'struct command_stream' here.  This should
   complete the incomplete type declaration in command.h.  */
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
    printf("ERROR: Stack is NULL!\n");
    return oldtop;
  }

  // The old top will be the stack's current top node
  oldtop = inCTStack->topNode;
  if (oldtop == NULL) {
    printf("ERROR: Nothing in stack!\n");
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

// Input: token list of RPN tokens
// Output: command stream of commands
command_stream_t rpnToCommTree(token_node_t inRPNTokens) {
  command_stream_t outCommStream =
        (command_stream_t)malloc(sizeof(struct command_stream));
  memset(outCommStream, 0, sizeof(struct command_stream));
 ctnode_t readNode = (ctnode_t)malloc(sizeof(struct ctStack_node)); memset(readNode, 0, sizeof(struct ctStack_node));

  command_t readData = (command_t)malloc(sizeof(struct command));
  memset(readData, 0, sizeof(struct command));

  while(inRPNTokens != NULL) {
    switch (inRPNTokens->comType) {
      case WORD:
        readData->type = SIMPLE_COMMAND;
        readData->status = -1;
        readData->input = 0;
        readData->output = 0;

        int i = 0;
        readData->u.word = (char **)malloc(sizeof(char **) * 20); //FIXME: CANNOT BE STATIC

        while(inRPNTokens->comType == WORD) {
          *(readData->u.word + i) = (char *)malloc(sizeof(char *) * 50);
          memset(*(readData->u.word + i), 0, sizeof(char*) * 50); // FIXME: CANNOT BE STATIC

          // Assume null terminated Strings
          unsigned dataLength = strlen(inRPNTokens->token);
          strncpy(*(readData->u.word + i), inRPNTokens->token, dataLength);

          inRPNTokens = inRPNTokens->next_token;
          i++;
        }
        ctPush(outCommStream, readData);
        break;
      case NEWLINE:
        // if (operatorStack == NULL) {
        //   rpnTokens_end = insert_at_end(rpnTokens_end, inTokens->token);
        //   break;
        // }
        // while (operatorStack->topNode != NULL) {
        //   readNode = pop(operatorStack);
        //   rpnTokens_end = insert_at_end(rpnTokens_end, readNode->stackdata);
        // }
        // rpnTokens_end = insert_at_end(rpnTokens_end, inTokens->token);
        break;
      case SUBSHELL:
        // if (strcmp(inTokens->token, "(") == 0) {
        //   push(operatorStack, inTokens->token);
        //   break;
        // } else {
        //   readNode = pop(operatorStack);
        //   if (readNode == NULL)
        //     break;
        //   else {
        //     rpnTokens_end = insert_at_end(rpnTokens_end, readNode->stackdata);

        //     // pop all the way until you hit the open paren in the stack
        //     while (strcmp(readNode->stackdata, "(") != 0) {
        //       readNode = pop(operatorStack);
        //       if (readNode != NULL)
        //         rpnTokens_end = insert_at_end(rpnTokens_end, readNode->stackdata);
        //       else {
        //         break;
        //       }
        //     }
        //     break;
        //   }
        //   break;
        // }
        break;
      case REDIRECT_MORE:
        // rpnTokens_end = insert_at_end(rpnTokens_end, inTokens->token);
        break;
      case REDIRECT_LESS:
        // rpnTokens_end = insert_at_end(rpnTokens_end, inTokens->token);
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
          readData->u.command[j] = subCommand->currCommand;
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
          printf("ERROR: Should not be here");
        }
        
        ctPush(outCommStream, readData);
        break;
      default:
        printf("Not a valid comType\n");
        break;
    }
    // Obtain next token
    inRPNTokens = inRPNTokens->next_token;
  }
  return outCommStream;
}



command_stream_t
make_command_stream (int (*get_next_byte) (void *),
         void *get_next_byte_argument)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */

  // Parsing file to token list
  token_node_t tokenlist_head = NULL;
  tokenlist_head = intoTokens(get_next_byte, get_next_byte_argument);

  printf("Finished tokenizing\n");
  
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

  printf("Finished RPN\n");
  
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

  printf("Finished RPN to Command Stream\n");
  // error (1, 0, "command reading not yet implemented");
  return 0;
}

command_t
read_command_stream (command_stream_t s)
{
  s;
  /* FIXME: Replace this with your implementation too.  */

  error (1, 0, "command reading not yet implemented");
  return 0;
}
