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
tokenlist_t insert_at_end (tokenlist_t tail, char *data) {
  tokenlist_t tmp = (tokenlist_t)malloc(sizeof(struct token_list)); 

  // Assume data is a null-terminated String
  unsigned strlength = strlen(data);
  tmp->token = (char *)malloc(sizeof(char) * strlength);
  strncpy(tmp->token, data, strlength);

  if (tail != NULL)
    tail->next_token = tmp;
  tmp->next_token = NULL;
  return tmp;
}

// input: tail of a linked list
// output: tail of a linked list
tokenlist_t read_pipe (tokenlist_t tail) {
  printf("entering | case\n");
  char *token = "|";
  tail = insert_at_end(tail, token);
  return tail;
}

// input: tail of a linked list, character stream
// output: tail of a linked list
tokenlist_t read_ampersand (tokenlist_t tail, void *stream) {
  printf("entering && case\n");
  int c;
  c = getc(stream);
  if (c == '&') {
    char *token = "&&";
    tail = insert_at_end(tail, token);
  } else {
    // push the char back onto the stream
    ungetc(c, stream);
    char *token = "&";
    tail = insert_at_end(tail, token);
  }
  return tail;
}

// inputs: tail of a linked list, character stream
// output: pointer to the tail of the linked list
/*tokenlist_t tokenizer (tokenlist_t tail, void *stream) {
  int c;
  while (c != EOF) {
    c = get_next_byte(stream);
    switch (c) {
      case '|':
        tail = read_pipe(tail);
        break;
      default:
        break;
    };
  }
}*/

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

  /*
  // Determining the size of the string
  struct stat st;
  int statcheck = stat(get_next_byte_argument, &st);
  // printf("Stat status is %d\n", statcheck);
  if (!statcheck) {
    perror("");
  }
  unsigned size = st.st_size;
  printf("Size of fullstring is %u\n", size);

  // Stringbuilder
  char* fullstring; 
  char nextchar[2]; // converting the int to a null-terminated string
  fullstring = (char *) malloc(sizeof(char) * (size + 2));

  while(c != EOF) {
    c = get_next_byte(get_next_byte_argument);
    if (c != EOF) {
      memset(nextchar, '\0', sizeof(nextchar));
      memset(nextchar, c, 1);
      strcat(fullstring, nextchar);
    }
  }

  // prints out the full string
  printf("%s\n", fullstring);
  */
  tokenlist_t tokenlist_head = (tokenlist_t)malloc(sizeof(struct token_list));
  tokenlist_t tokenlist_end = tokenlist_head;

  int c;
  c = get_next_byte(get_next_byte_argument);

  while (c != EOF) {
    c = get_next_byte(get_next_byte_argument);
    switch (c) {
      case '|':
        tail = read_pipe(tail);
        break;
      default:
        break;
    };
  }

  /*
  // Array of tokens
  char **tokenlist; // FIXME: THIS ALSO CANNOT BE STATIC
  tokenlist = (char **) malloc(size + 2);
  int tokenPos = 0;
  printf("Length of string is %d\n", (int) strlen(fullstring));
  printf("Size of fullstring is %d\n", size);
  */
  /*
  for (i = 0; i < strlen(fullstring); i++) {
    switch (fullstring[i]) {
      case '&':
        printf("entering & case\n");
        if (fullstring[i + 1] == '&') {
          char *strdata = "&&";
          tokenlist_end = insert_at_end(tokenlist_end, strdata);
          i++;
        }
        break;
      default:
        break;
    };
  }
  */

  while(tokenlist_head != NULL) {
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
