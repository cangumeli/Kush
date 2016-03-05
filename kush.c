/**
 * KUSH shell interface program
 */

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>


#define MAX_LINE       80 /* 80 chars per line, per command, should be enough. */

int parseCommand(char inputBuffer[], char *args[],int *background);
int execute(char *args[]);
int generic_execute(char *args[]);

int main(void)
{
  char inputBuffer[MAX_LINE]; 	        /* buffer to hold the command entered */
  int background;             	        /* equals 1 if a command is followed by '&' */
  char *args[MAX_LINE/2 + 1];	        /* command line (of 80) has max of 40 arguments */
  pid_t child;            		/* process id of the child process */
  int status;           		/* result from execv system call*/
  int shouldrun = 1;

  int i, upper;
  printf("Kush ucuyor...\n");
  while (shouldrun) {            		/* Program terminates normally inside setup */
    background = 0;

    shouldrun = parseCommand(inputBuffer,args,&background);       /* get next command */

    if (strncmp(inputBuffer, "exit", 4) == 0)
      shouldrun = 0;     /* Exiting from kush*/

    if (shouldrun) {
      /*
	After reading user input, the steps are
	(1) Fork a child process using fork()
	(2) the child process will invoke execv()
	(3) if command included &, parent will invoke wait()
       */
      if (strcmp(args[0], "cd") == 0) {
	if(chdir(args[1]) == -1)  printf("Directory does not exist: %s\n", args[1]);
      } else { //child process required
	child = fork();
	if (child == 0) {
	  if (generic_execute(args) == -1) {
	    printf("Error: Unknown command!\n");
	  }
	  exit(0); //exit child.
	} else {
	  //printf("%d\n", background);
	  if (!background)
	      waitpid(child);
	}
      }
    }

  }
  return 0;
}

/**
 * The parseCommand function below will not return any value, but it will just: read
 * in the next command line; separate it into distinct arguments (using blanks as
 * delimiters), and set the args array entries to point to the beginning of what
 * will become null-terminated, C-style strings.
 */

int parseCommand(char inputBuffer[], char *args[],int *background)
{
    int length,		/* # of characters in the command line */
      i,		/* loop index for accessing inputBuffer array */
      start,		/* index where beginning of next command parameter is */
      ct,	        /* index of where to place the next parameter into args[] */
      command_number;	/* index of requested command number */

    ct = 0;

    /* read what the user enters on the command line */
    do {
	  printf("kush>");
	  fflush(stdout);
	  length = read(STDIN_FILENO,inputBuffer,MAX_LINE);
    }
    while (inputBuffer[0] == '\n'); /* swallow newline characters */

    /**
     *  0 is the system predefined file descriptor for stdin (standard input),
     *  which is the user's screen in this case. inputBuffer by itself is the
     *  same as &inputBuffer[0], i.e. the starting address of where to store
     *  the command that is read, and length holds the number of characters
     *  read in. inputBuffer is not a null terminated C-string.
     */
    start = -1;
    if (length == 0)
      exit(0);            /* ^d was entered, end of user command stream */

    /**
     * the <control><d> signal interrupted the read system call
     * if the process is in the read() system call, read returns -1
     * However, if this occurs, errno is set to EINTR. We can check this  value
     * and disregard the -1 value
     */

    if ( (length < 0) && (errno != EINTR) ) {
      perror("error reading the command");
      exit(-1);           /* terminate with error code of -1 */
    }

    /**
     * Parse the contents of inputBuffer
     */

    for (i=0;i<length;i++) {
      /* examine every character in the inputBuffer */

      switch (inputBuffer[i]){
      case ' ':
      case '\t' :               /* argument separators */
	if(start != -1){
	  args[ct] = &inputBuffer[start];    /* set up pointer */
	  ct++;
	}
	inputBuffer[i] = '\0'; /* add a null char; make a C string */
	start = -1;
	break;

      case '\n':                 /* should be the final char examined */
	if (start != -1){
	  args[ct] = &inputBuffer[start];
	  ct++;
	}
	inputBuffer[i] = '\0';
	args[ct] = NULL; /* no more arguments to this command */
	break;

      default :             /* some other character */
	if (start == -1)
	  start = i;
	if (inputBuffer[i] == '&') {
	  *background  = 1;
	  inputBuffer[i-1] = '\0';
	}
      } /* end of switch */
    }    /* end of for */

    /**
     * If we get &, don't enter it in the args array
     */

    if (*background)
      args[--ct] = NULL;

    args[ct] = NULL; /* just in case the input line was > 80 */

    return 1;

} /* end of parseCommand routine */

int execute(char *args[])
{
  char *command = args[0];

  if (strcmp(command, "cd") == 0) {
    printf("\nError: not implemented yet.\n");
  }
  else if (strcmp(command, "clear") == 0) {
    char *params[] = {"/usr/bin/clear", NULL};
    execv(params[0], params);
  }
  else if (strcmp(command, "env") == 0) {
    char *params[] = {"/usr/bin/env", NULL};
    execv(params[0], params);
  }
  else if (strcmp(command, "echo") == 0) {
    char *params[] = {"/bin/echo", args[1], NULL};
    execv(params[0], params);
  }

  else {
    return 1;
  }
  return 0;

}

int generic_execute(char *args[])
{
  char buf[400];
  FILE *cf;
  char *token;

  if(strcmp(args[0], "trash") == 0){
    if(strcmp(args[1], "-r") == 0){
      char rembash[100];
      sprintf(rembash, "crontab -l | grep -v -w '%s'| crontab -", args[2]);
      system(rembash);
      system("crontab -l > op.txt");
      return 1;
    }
    char *cr_args[] = {"/usr/bin/crontab", "op.txt", NULL};
    FILE *fp;


    fp = fopen ("op.txt", "a");
    fprintf(fp, "%s %s * * * cd %s ; rm -rf *\n", strtok(NULL, "."), strtok(args[1], "."), args[2]);
    fclose(fp);

    execv(cr_args[0], cr_args);
    //0	2	*	*	*	cd <directory> ; rm -rf *
    return 1;
  }

  if((strcmp(args[0], "crontab") == 0)&&(strcmp(args[1], "-r") == 0))
    remove("op.txt");

  //read the path
  cf = popen("echo $PATH", "r");
  fgets(buf, sizeof(buf), cf);
  pclose(cf);

  buf[strlen(buf)-1] = '\0'; //erase the newline

  token = strtok(buf, ":");
  while(token != NULL) {
    char exp_cmd[50]; //stores path+command
    sprintf(exp_cmd, "%s/%s", token, args[0]);

    //if execution fail, continue; else return
    if(execv(exp_cmd, args) != -1)
      return 1;

    token = strtok(NULL, ":"); //update destination to check
  }
  return -1;
}
