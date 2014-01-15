// UCLA CS 111 Lab 1 command reading

#include "command.h"
#include "command-internals.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <error.h>

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */
struct token_list
{
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
  printf("Data inserted was %s\n", newtail->token);
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
  } else {
    // push the char back onto the stream
    ungetc(c, stream);
    char *token = "&";
    tmp = insert_at_end(tail, token);
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
  } else {
    // push the char back onto the stream
    ungetc(c, stream);
    char *token = "|";
    tmp = insert_at_end(tail, token);
  }
  return tmp;
}

// input: tail of a linked list, character stream
// output: tail of a linked list
tokenlist_t read_newline (tokenlist_t tail) {
  printf("Grabbing a newline\n");
  tokenlist_t tmp = (tokenlist_t)malloc(sizeof(struct token_list));
  char *token = "\n";
  tmp = insert_at_end(tail, token);
  return tmp;
}

// input: tail of a linked list, character stream, read character
// output: tail of a linked list
tokenlist_t read_word (tokenlist_t tail, void *stream, int c) {
  // printf("entering word case\n");
  /* if (c == ' ' || c == '\t') {
    // printf("finish this simple word\n");
    return tail;
  } else if (c == '\n') {
    // printf("storing this newline\n");
    ungetc(c, stream);
    return tail;
  } else {
    */
  char newtoken[100]; //FIXME: CANNOT BE STATIC
  memset(newtoken, '\0', sizeof(newtoken));
  tokenlist_t tmp = (tokenlist_t)malloc(sizeof(struct token_list));

  char appendchar[2];
  memset(appendchar, '\0', sizeof(appendchar));
  sprintf(appendchar, "%c", c);
  strcat(newtoken, appendchar);

  int nextchar;
  nextchar = getc(stream);
  char newlineTerm = 0;
  while (nextchar != EOF) {
    if (nextchar == ' ' || nextchar == '\t') {
      break;
    } else if (nextchar == '\n') {
      // printf("storing a newline\n");
      //ungetc(c, stream);
      newlineTerm = 1;
      break;
    } else {
      sprintf(appendchar, "%c", nextchar);
      strcat(newtoken, appendchar);
      nextchar = getc(stream);
    }
  }
  tmp = insert_at_end(tail, newtoken); 
  // printf("Storing %s\n", newtoken);
  //free(newtoken);
  if (newlineTerm)
    return read_newline(tmp);
  return tmp;
}

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

  tokenlist_t tokenlist_head = NULL;
  tokenlist_t tokenlist_end = NULL;

  int c;

do {
    c = get_next_byte(get_next_byte_argument);

    switch (c) {
      case ';':
        tokenlist_end = read_singlespecial(tokenlist_end, c);
        break;
      case '(':
        tokenlist_end = read_singlespecial(tokenlist_end, c);
        break;
      case ')':
        tokenlist_end = read_singlespecial(tokenlist_end, c);
        break;
      case '<':
        tokenlist_end = read_singlespecial(tokenlist_end, c);
        break;
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
        break;
      case '\t':
        break;
	 case EOF:
	   break;
	 default:
        tokenlist_end = read_word(tokenlist_end, get_next_byte_argument, c);
        break;
    };

    if (tokenlist_head == NULL)
      tokenlist_head = tokenlist_end;
  } while (c != EOF);

  printf("Finished tokenizing\n");
  
  while(tokenlist_head != NULL) {
    printf("Checking the linked list");
    printf("%s\n", tokenlist_head->token);
    tokenlist_head = tokenlist_head->next_token;
  }
  
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
