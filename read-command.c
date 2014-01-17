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

struct token_list
{
  enum parse_type comType;
  char *token;
  struct token_list *next_token;
};

typedef struct token_list *tokenlist_t;

// inputs: tail of a linked list, string data
// output: pointer to the tail of the linked list
tokenlist_t insert_at_end (tokenlist_t oldtail, char *data) {
  tokenlist_t newtail = (tokenlist_t)malloc(sizeof(struct token_list)); 

  // Assume data is a null-terminated String
  unsigned strlength = strlen(data);
  newtail->token = (char *)malloc(sizeof(char) * strlength);
  strncpy(newtail->token, data, strlength);

  if (oldtail != NULL)
    oldtail->next_token = newtail;
  newtail->next_token = NULL;
  // printf("Data inserted was %s\n", newtail->token);
  return newtail;
}

// input: tail of a linked list, special character
// output: tail of a linked list
tokenlist_t read_singlespecial (tokenlist_t tail, int c) {
  char token[2];
  memset(token, '\0', sizeof(token));
  sprintf(token, "%c", c);
  tokenlist_t tmp = (tokenlist_t)malloc(sizeof(struct token_list));
  tmp = insert_at_end(tail, token);
  switch (c) {
    case ';':
      tmp->comType = SEMICOLON;
      break;
    case '(':
      tmp->comType = SUBSHELL;
      break;
    case ')':
      tmp->comType = SUBSHELL;
      break;
    case '<':
      tmp->comType = REDIRECT_LESS;
      break;
    case '>':
      tmp->comType = REDIRECT_MORE;
      break;
    default:
      break;
  }
  return tmp;
}

// input: tail of a linked list, character stream
// output: tail of a linked list
tokenlist_t read_ampersand (tokenlist_t tail, void *stream) {
  int c;
  c = getc(stream);
  tokenlist_t tmp = (tokenlist_t)malloc(sizeof(struct token_list));
  if (c == '&') {
    char *token = "&&";
    tmp = insert_at_end(tail, token);
    tmp->comType = AMPERSAND;
  } else {
    // push the char back onto the stream
    ungetc(c, stream);
    char *token = "&";
    tmp = insert_at_end(tail, token);
    tmp->comType = WORD;
  }
  return tmp;
}

// input: tail of a linked list, character stream
// output: tail of a linked list
tokenlist_t read_pipe (tokenlist_t tail, void *stream) {
  int c;
  c = getc(stream);
  tokenlist_t tmp = (tokenlist_t)malloc(sizeof(struct token_list));
  if (c == '|') {
    char *token = "||";
    tmp = insert_at_end(tail, token);
    tmp->comType = OR;
  } else {
    // push the char back onto the stream
    ungetc(c, stream);
    char *token = "|";
    tmp = insert_at_end(tail, token);
    tmp->comType = PIPE;
  }
  return tmp;
}

// input: tail of a linked list, character stream
// output: tail of a linked list
tokenlist_t read_newline (tokenlist_t tail) {
  if (tail == NULL)
	  return NULL;

  if ((tail->comType == OR) || (tail->comType == PIPE) || 
        (tail->comType == AMPERSAND) || (tail->comType == NEWLINE)) {
    return tail;
  } else {
    tokenlist_t tmp = NULL; //(tokenlist_t)malloc(sizeof(struct token_list));
    char *token = "\n";
    tmp = insert_at_end(tail, token);
    tmp->comType = NEWLINE;
    return tmp;
  }
}

// input: tail of a linked list, character stream, read character
// output: tail of a linked list
tokenlist_t read_word (tokenlist_t tail, void *stream, int c) {
  char newtoken[100]; //FIXME: CANNOT BE STATIC
  memset(newtoken, '\0', sizeof(newtoken));
  tokenlist_t tmp = (tokenlist_t)malloc(sizeof(struct token_list));

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
  tmp = insert_at_end(tail, newtoken); 
  tmp->comType = WORD;
  // printf("Storing %s\n", newtoken);
  return tmp;
}

tokenlist_t intoTokens(int (*get_next_byte) (void *),
         void *get_next_byte_argument) {

  // Tokeninzing our commands
  tokenlist_t tokenlist_head = NULL;
  tokenlist_t tokenlist_end = tokenlist_head;

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
struct stack_node
{
  char *stackdata;
  struct stack_node *next_node;
};

struct stack
{
  struct stack_node *topNode;
};

typedef struct stack_node *stacknode_t;
typedef struct stack *stack_t;

// Input: Stack pointer, String data to be new top
// Output: void
void push(stack_t theStack, char* data) {
  stacknode_t newtop = (stacknode_t)malloc(sizeof(struct stack_node));

  // Assume data is a null-terminated String
  unsigned strlength = strlen(data);
  newtop->stackdata = (char *)malloc(sizeof(char) * strlength);
  strncpy(newtop->stackdata, data, strlength);

  if (theStack == NULL)
    theStack = (stack_t)malloc(sizeof(struct stack));
  else
    newtop->next_node = theStack->topNode;
  theStack->topNode = newtop;
  return;
}

// Input: Stack pointer
// Output: Stack Node pointer
stacknode_t pop(stack_t theStack) {
  stacknode_t newtop = NULL;
  stacknode_t oldtop = NULL;

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
  newtop = oldtop->next_node;
  oldtop->next_node = NULL;

  // Updating the stack's top node to be the new top
  theStack->topNode = newtop;
  return oldtop;
}

tokenlist_t inToRPN(tokenlist_t inTokens) {
  tokenlist_t rpnTokens = NULL; // (tokenlist_t)malloc(sizeof(struct token_list));
  tokenlist_t rpnTokens_end = rpnTokens;

  stack_t operatorStack = (stack_t)malloc(sizeof(struct stack));
  memset(operatorStack, 0, sizeof(struct stack));

  stacknode_t readNode = (stacknode_t)malloc(sizeof(struct stack_node));
  memset(readNode, 0, sizeof(struct stack_node));

  char rpn[100]; //FIXME: CANNOT BE STATIC
  memset(rpn, '\0', sizeof(rpn));

  while(inTokens != NULL) {
    switch (inTokens->comType) {
      case WORD:
        rpnTokens_end = insert_at_end(rpnTokens_end, inTokens->token);
        break;
      case NEWLINE:
        if (operatorStack == NULL) {
          rpnTokens_end = insert_at_end(rpnTokens_end, inTokens->token);
          break;
        }
        while (operatorStack->topNode != NULL) {
          readNode = pop(operatorStack);
          rpnTokens_end = insert_at_end(rpnTokens_end, readNode->stackdata);
        }
        rpnTokens_end = insert_at_end(rpnTokens_end, inTokens->token);
        break;
      case SUBSHELL:
        if (strcmp(inTokens->token, "(") == 0) {
          push(operatorStack, inTokens->token);
          break;
        } else {
          readNode = pop(operatorStack);
          if (readNode == NULL)
            break;
          else {
            rpnTokens_end = insert_at_end(rpnTokens_end, readNode->stackdata);

            // pop all the way until you hit the open paren in the stack
            while (strcmp(readNode->stackdata, "(") != 0) {
              readNode = pop(operatorStack);
              if (readNode != NULL)
                rpnTokens_end = insert_at_end(rpnTokens_end, readNode->stackdata);
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
        rpnTokens_end = insert_at_end(rpnTokens_end, inTokens->token);
        break;
      case REDIRECT_LESS:
        rpnTokens_end = insert_at_end(rpnTokens_end, inTokens->token);
        break;
      case PIPE:
      case AMPERSAND:
      case OR:
        readNode = operatorStack->topNode;
        if (readNode == NULL) {
          push(operatorStack, inTokens->token);
        } else if (strcmp(readNode->stackdata, ";") == 0) {
          push(operatorStack, inTokens->token);
        } else if (strcmp(readNode->stackdata, "(") == 0) {
          push(operatorStack, inTokens->token);
        } else {
          readNode = pop(operatorStack);
          rpnTokens_end = insert_at_end(rpnTokens_end, readNode->stackdata);
          push(operatorStack, inTokens->token);
        }
        break;
      case SEMICOLON:
        if (operatorStack == NULL) {
            rpnTokens_end = insert_at_end(rpnTokens_end, inTokens->token);
            break;
          }
          readNode = operatorStack->topNode;
          while ((readNode != NULL) && 
                  (strcmp(readNode->stackdata, "(") != 0)) {
            readNode = pop(operatorStack);
            rpnTokens_end = insert_at_end(rpnTokens_end, readNode->stackdata);
          }
          rpnTokens_end = insert_at_end(rpnTokens_end, inTokens->token);
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

// Command List Implementation
struct command_list
{
  // next command in the linked list
  struct command *next_command;
};

/* FIXME: Define the type 'struct command_stream' here.  This should
   complete the incomplete type declaration in command.h.  */
struct command_stream
{
  // Pointer to the first command in the list 
  struct command *first_command;
};

command_stream_t
make_command_stream (int (*get_next_byte) (void *),
         void *get_next_byte_argument)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */

  // Parsing file to token list
  tokenlist_t tokenlist_head = NULL;
  tokenlist_head = intoTokens(get_next_byte, get_next_byte_argument);

  printf("Finished tokenizing\n");
  
  if (DEBUG) {
    tokenlist_t test = tokenlist_head;
    while(test != NULL) {
      printf("%s ", test->token);
      test = test->next_token;
    }
  }

  // Parsing infix into RPN
  tokenlist_t rpnTokens = NULL;
  rpnTokens = inToRPN(tokenlist_head);

  printf("Finished RPN\n");
  
  if (DEBUG) {
    tokenlist_t rpnTest = rpnTokens;
    while(rpnTest != NULL) {
      printf("%s ", rpnTest->token);
      rpnTest = rpnTest->next_token;
    } 
  } 

  printf("Finished RPN Processing\n");
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
