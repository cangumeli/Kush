/**
 * KUSH shell interface program
 */

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <sys/signal.h>

#define MAX_LINE       80 /* 80 chars per line, per command, should be enough. */

int parseCommand(char inputBuffer[], char *args[],int *background);


/*Executes the schedInfo related commands*/
int kernel_execute(char *args[]);


/*The execution strategy for commands in the child processes
Takes parsed arguments array as the input, locates it in the path and
runs it using execv.
Additional strategies are applied for custom commands car and thrash*/
int generic_execute(char *args[]);


/*Implementation of compile_and_run(car) command
Allows compilation and execution of a C,C++,Java or Python
program with a single command.
Called within generic_execute.
*/
int compile_and_run(char *args[]);


/*Edit args and redirects the standard input to the file fd.
Returns 0 for overwrite, 1 for append, -1 for no redirection*/
int setup_redirect(char *args[], int *fd);


/*Closes the redirected file fd if sr_status signals file is opened.*/
int setdown_redirect(int *fd, int sr_status) { if (sr_status >= 0) close(*fd); }


/*Divides arguments array into 2:
 -args becomes arguments before pipe(|)
 -pargs becomes arguments after pipe(|)
Returns 0 if no pipes are found in the command, 
index of | otherwise*/
int pipe_args(char *args[], char *pargs[]);

int kernel_module_pid = -1; /*pid of the last query of schedInfo, -1 if no queries made.*/

int main(void)
{
  char inputBuffer[MAX_LINE]; 	        /* buffer to hold the command entered */
  int background;             	        /* equals 1 if a command is followed by '&' */
  char *args[MAX_LINE/2 + 1];           /* command line (of 80) has max of 40 arguments */
  pid_t child;            		/* process id of the child process */
  int status;           		/* result from execv system call*/
  int shouldrun = 1;

  int i, upper;
  printf("Kush ucuyor...\n");
  while (shouldrun) {
    /* Program terminates normally inside setup */
    background = 0;

    shouldrun = parseCommand(inputBuffer,args,&background);       /* get next command */

    if (strncmp(inputBuffer, "exit", 4) == 0)
      shouldrun = 0;     /* Exiting from kush*/

    if (shouldrun) {
     
      /*EXECUTION OF 'CD' COMMAND*/
      if (strcmp(args[0], "cd") == 0) {
	       if(chdir(args[1]) == -1)
	        printf("Directory does not exist: %s\n", args[1]);
      }
      
      /*EXECUTION OF 'SCHEDINFO' COMMAND*/
      else if (strcmp(args[0], "schedInfo") == 0) {
        chdir("Module");
        kernel_execute(args);
        chdir("..");
      }
     
      /*STANDARD COMMAND EXECUTION WITH A CHILD PROCESS*/
      else {
	//PIPES
	char *pargs[MAX_LINE/2+1];
	int piped = pipe_args(args, pargs);
	int pipe_fd[2];
	
	/*GENERIC EXECUTION STRATEGY FOR NOT-PIPED COMMANDS*/
	if (!piped) {
	  child = fork();
	  if (child == 0) {
	    int fd;
	    int srs = setup_redirect(args, &fd); //set arguments and open file if redirection exists

	    setdown_redirect(&fd, srs); //close the file if opened
	    if (generic_execute(args) == -1)
	      printf("Error: Unknown command!\n");

	    exit(0); //exit child.
	  }
	  else {	   
	    if (!background) //support background processes
	      waitpid(child, &status, 0);
	  }
	}
	
	/*GENERIC EXECUTION STRATEGY FOR PIPED COMMANDS*/
	else { //if the program is piped
	  pid_t child2;
	  pipe(pipe_fd);
	  child = fork();
	  ///First child, executes after |
	  if (child == 0) {
	    //pipe(pipe_fd);
	    int fd, srs;
	     dup2(pipe_fd[0], STDIN_FILENO);
	     close(pipe_fd[1]);
	     //redirection support
	     srs = setup_redirect(pargs, &fd);
	     setdown_redirect(&fd, srs);
	      if (generic_execute(pargs) == -1)
		printf("Error: Unknown command after |\n");
	      exit(0);
	  }
	  else  { //parent
	    child2 = fork();
	    ///Second child (executes before |)
	    if (child2 == 0) {
	      int fd, srs;
	      dup2(pipe_fd[1], STDOUT_FILENO);
	      close(pipe_fd[0]);
	      //redirection support
	      srs = setup_redirect(args, &fd);
	      setdown_redirect(&fd, srs);
	      if (generic_execute(args) == -1)
		printf("Error: Unknown command before |\n");
	      exit(0);
	    }
	    else { //parent
	      wait(NULL); //wait for child 2
	    }
	    ///close pipe
	    close(pipe_fd[0]);
	    close(pipe_fd[1]);
	    if (!background) //background support for pipes
	      wait(NULL); //wait for children after pipes are closed
	  }
	}
      }
    }
  }
  if (kernel_module_pid != -1) {
    char *exit_args[] = {"schedInfo", "-r"};
    kernel_execute(exit_args);
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


//DOCUMENTATION OF THE FOLLOWING FUNCTIONS ARE ABOVE THEIR PROTOTYPES.

int kernel_execute(char *args[])
{

    //sudo insmod schedInfo processID=PID processSPolicy=policy processPrio=priority
  char nargs[MAX_LINE*2];
  int test;

  if(strcmp(args[1], "-r")==0){
    if(kernel_module_pid == -1){
      printf("The kernel module does not exists!\n");
      return -1;
    }
    system("sudo rmmod schedinfo");
    printf("The kernel module is removed!\n");
    kernel_module_pid = -1;
    return 0;
  }

  if (args[1] == NULL ) {
    sprintf(nargs, "sudo insmod schedinfo.ko");
  }else if(args[2] == NULL){
    sprintf(nargs, "sudo insmod schedinfo.ko processID=%s",
	    args[1]);
  }else if(args[3] == NULL){
    sprintf(nargs, "sudo insmod schedinfo.ko processID=%s processSPolicy=%s",
	    args[1], args[2]);
  }else{
    sprintf(nargs, "sudo insmod schedinfo.ko processID=%s processSPolicy=%s processPrio=%s",
	    args[1], args[2], args[3]);
  }

  if(kernel_module_pid == atoi(args[1])){
    printf("The kernel module is aldready loaded!\n");
    return 0;
  }

  if(kernel_module_pid != -1)
    system("sudo rmmod schedinfo");

  test = system(nargs);
  if(test != -1){
    printf("The module is loaded\n");
    kernel_module_pid = atoi(args[1]);
  }else{
    printf("Insmod operation exit code: %d\n", test);
  }
  return 0;
  
}

int generic_execute(char *args[])
{
  char buf[400];
  FILE *cf;
  char *token;
  //THRASH
  if(strcmp(args[0], "trash") == 0){
    if(strcmp(args[1], "-r") == 0){
      char rembash[MAX_LINE*2];
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
  //COMPILE AND RUN( CAR)
  else if (strcmp(args[0], "car") == 0) {
   return  compile_and_run(args);
  }
  
  if((strcmp(args[0], "crontab") == 0)&&(strcmp(args[1], "-r") == 0))
    remove("op.txt");

  //GENERIC EXECUTION THRU PATH
  /*Inspired by 
    http://stackoverflow.com/questions/11840833/getting-output-of-a-system-command-from-stdout-in-c*/
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

int compile_and_run(char *args[])
{
  char filename[MAX_LINE];
  char * token;
  char *name;

  if (args[1] == NULL){
    printf("car: Not enough input arguments\n");
    return -2;
  }

  strcpy(filename, args[1]);
  name = strtok(filename, ".");
  token = strtok(NULL, ".");
  if (token == NULL) {
    printf("car: Input extension is missing\n");
  }
  //PYTHON
  if (strcmp(token, "py") == 0) {
    char *exe_args[] = {"python", args[1], NULL};
    return  generic_execute(exe_args);
  }
  //C and C++
  else if (strcmp(token, "c") == 0 || strcmp(token, "cpp") == 0) {
    char *exe_args[] = {(strlen(token) == 1) ? "gcc" : "g++",
			"-o",
			name,
			args[1],
			NULL};
    pid_t c1 = fork();
    if (c1 == 0) {
      if (generic_execute(exe_args) == -1)
	printf("%s not found in your path\n", exe_args[0]);
      exit(0);
    } else {
      char run_str[MAX_LINE-4];
      wait(NULL);
      sprintf(run_str, "./%s", name);
      system(run_str);
    }
    return 1;
  }
  //JAVA
  else if (strcmp(token, "java") == 0) {
    char *exe_args[] = {"javac", args[1], NULL};
    pid_t ch = fork();
    if (ch == 0) {
      if (generic_execute(exe_args) == -1)
	printf("javac not found in path\n");
      exit(0);
    } else {
      char run_str[MAX_LINE-5];
      wait(NULL);
      printf("%s\n", name);
      sprintf(run_str, "java %s", name);
      system(run_str);
    }
    return 2;
  }
  else {
    printf("Unknown language %s\n", token);
    return -3;
  }
}


int setup_redirect(char *args[], int *fd)
{
  char filename[MAX_LINE];
  int size = 0;

  while (args[size] != NULL) size++;

  if(size < 3) return -1;

  //OVERWRITE
  if (strcmp(args[size-2], ">") == 0) {
    strcpy(filename, args[size-1]);
    // printf("%s\n", filename);
    args[size-1] = args[size-2] = NULL;
    *fd = open(filename, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
    //if(1) return 1;
    dup2(*fd, STDOUT_FILENO);
    return 0;
  }
  
  //APPEND
  if(strcmp(args[size-2], ">>") == 0) {
    strcpy(filename, args[size-1]);
    args[size-1] = args[size-2] = NULL;
    *fd = open(filename, O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
    dup2(*fd, STDOUT_FILENO);

    return 1;
  }
  //NO REDIRECT
  return -1;
}


int pipe_args(char *args[], char *pargs[])
{
  int pipe_index = 0;
  int i=0, j, k;
  while (args[i] != NULL) {
    if (strcmp(args[i], "|") == 0 && i > 0) {
      pipe_index = i;
      break;
    }
    i++;
  }
  if (pipe_index == 0) return 0; //pipe is not found

  args[pipe_index] = NULL;

  j = pipe_index+1;
  k = 0;
  while (args[j] != NULL) {
    pargs[k] = args[j];
    j++;
    k++;
  }
  pargs[k] = NULL;
  return k;
}
