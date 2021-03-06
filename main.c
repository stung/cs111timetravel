// UCLA CS 111 Lab 1 main program

#include <errno.h>
#include <error.h>
#include <getopt.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>

#include "command.h"

static char const *program_name;
static char const *script_name;

static void
usage (void)
{
  error (1, 0, "usage: %s [-pt] SCRIPT-FILE", program_name);
}

static int
get_next_byte (void *stream)
{
  return getc (stream);
}

// FIXME: Include a dependency graph argument here
void execute_command_parallel (command_stream_t command_stream) {
  command_t currCommand;
  pid_t child;

  while ((currCommand = read_command_stream(command_stream))) {
    child = fork();
    if (child == 0) { // Child Process
      execute_command(currCommand, 0);
      break;
    } else if (child > 0) { // Parent Process
      continue;
    } else { // error on fork
      error(1, 0, "Cannot create parallel process!");
    }
  }
  return;
}

int
main (int argc, char **argv)
{
  int opt;
  int command_number = 1;
  int print_tree = 0;
  int time_travel = 0;
  program_name = argv[0];

  for (;;)
    switch (getopt (argc, argv, "pt"))
      {
      case 'p': print_tree = 1; break;
      case 't': time_travel = 1; break;
      default: usage (); break;
      case -1: goto options_exhausted;
      }
 options_exhausted:;

  // There must be exactly one file argument.
  if (optind != argc - 1)
    usage ();

  script_name = argv[optind];
  FILE *script_stream = fopen (script_name, "r");
  if (! script_stream)
    error (1, errno, "%s: cannot open", script_name);
  command_stream_t command_stream =
    make_command_stream (get_next_byte, script_stream);

  command_t last_command = NULL;
  command_t command;

  // parallelizing the entire stream was not completed
  int time_travel_stream = 0;

  while ((command = read_command_stream (command_stream)))
    {
      if (time_travel_stream) {
        last_command = command;
        execute_command_parallel(command_stream);
        break;
      } else if (print_tree) {
    	  printf ("# %d\n", command_number++);
    	  print_command (command);
    	} else {
    	  last_command = command;
    	  execute_command (command, time_travel);
    	}
    }

  return print_tree || !last_command ? 0 : command_status (last_command);
}
