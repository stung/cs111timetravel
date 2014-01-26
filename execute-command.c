// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"

#include <error.h>

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */

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
  if (time_travel) {
  	error(1, 0, "Time travel not implemented yet");
  }

  switch (c->type) {
  	case AND_COMMAND:
  	  execute_command(c->u.command[0], time_travel);
  	  if (c->u.command[0]->status == 0) { // first command succeeded
  	  	execute_command(c->u.command[1], time_travel);
  	  	c->status = c->u.command[1]->status; // sets status to second command
  	  } else {
  	  	c->status = c->u.command[0]->status; // sets status to first command
  	  }
  	  break;
  	case OR_COMMAND:
  	  execute_command(c->u.command[0], time_travel);
  	  if (c->u.command[0]->status != 0) { // first command failed
  	  	execute_command(c->u.command[1], time_travel);
  	  	c->status = c->u.command[1]->status; // sets status to second command
  	  } else {
  	  	c->status = c->u.command[0]->status; // sets status to first command
  	  }
  	  break;
  	case SEQUENCE_COMMAND:
  	case PIPE_COMMAND:
  	case SIMPLE_COMMAND:
  	case SUBSHELL_COMMAND:
  	default:
  		break;
  }
}
