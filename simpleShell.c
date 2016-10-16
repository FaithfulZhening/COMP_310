#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>

#define BUFSIZE 10
#define BUFFER 1024
#define MAXLENGTH 100

//
// This code is given for illustration purposes. You need not include or follow this
// strictly. Feel free to writer better or bug free code. This example code block does not
// worry about deallocating memory. You need to ensure memory is allocated and deallocated
// properly so that your shell works without leaking memory.
//
int getcmd(char *prompt, char *args[], int *background) {
	int length, i = 0;
	char *token, *loc;
	char *line = NULL;
	size_t linecap = 0;
	printf("%s", prompt);
	//pid_t rStatus
	length = getline(&line, &linecap, stdin);
	if (length <= 0) {
		exit(-1);
	}
	// Check if background is specified..
	if ((loc = index(line, '&')) != NULL) {
		*background = 1;
		*loc = ' ';
	} else
		*background = 0;

	while ((token = strsep(&line, " \t\n")) != NULL) {
		int j;
		for (j = 0; j < strlen(token); j++)
			if (token[j] <= 32)
				token[j] = '\0';
		if (strlen(token) > 0)
			args[i++] = token;
	}
	free(line);
	return i;
}

/**
   exchange the contents of two **array
**/
void exchange(char **array1, char **array2) {
	int length = sizeof(*array2);
	int i;
	for (i = 0; i < length; i++) {
		array1[i] = array2[i];
	}
}

/**
   find the length of *args[]
**/
int argLength(char *args[]) {
	int i;
	for (i = 0; i < BUFFER; i++) {
		if (args[i] == '\0') {
			return i;
		}
	}
	return -1;
} 

/**
   return 0 we need redirection, return 1 otherwise
**/
int checkRedir(char *args[], int count) {
	if (count <= 2) {
		return 1;
	}
	if (!strcmp(args[count - 2], ">")) {
		return 0;
	}
	return 1;
} 

/**
   return 0 we need piping, return 1 otherwise
   put the part before '|' into args1
   put the part after '|' into args2
**/
int checkPipe(char *args[], int count, char *args1[], char *args2[]) {
	int check = 0; //check becomes the index if a '|' is encountered
	int rval = 1;  // return 0 if we need pipe, return 1 otherwise
	int i;
	for (i = 0; i < count - 1; i++) {
		if (!strcmp(args[i], "|")) {
			check = i;
			rval = 0;
			args1[i] = '\0';
		}
		if (check == 0) {
			args1[i] = args[i];
		}
		if (check != 0) {
			args2[i - check] = args[i + 1];
		}
	}
	args2[count - check - 1] = '\0';
	return rval;
}

void piping(char *char1[], char *char2[]) {
	int fd[2];
	pipe(fd);
	pid_t pipChild = fork();
	if (pipChild == 0) {
		close(fd[0]);
		dup2(fd[1], STDOUT_FILENO);
		close(fd[1]);
		execvp(char1[0], char1);
		exit(0);
	} else {
		pid_t pipchild2 = fork();
		if (pipchild2 == 0) {
			close(fd[1]);
			dup2(fd[0], STDIN_FILENO);
			close(fd[0]);
			execvp(char2[0], char2);
			exit(0);
		}
		close(fd[1]);
		close(fd[0]);
		return;
	}
}

int main(void) {
	char *args[20];
	int bg;
	pid_t childPid;
	char **commands[11]; //this is where we store commands
	int size_c = 1;  //this is the counter for commands
	char *token;  // this is string after '!'
	int num, command_index;  //  this will be the number in history command
	char **background[MAXLENGTH]; // this is where stores the background process
	void exchange(char **array1, char **array2); // this the the function for exchanging two **arrays
	pid_t pids[MAXLENGTH];	// this is where stores the background process pid
	int bgCounts = 0;
	int argLength(char *args[]);

	//initialize commands array
	int i;
	for (i = 0; i < 11; i++) {
		commands[i] = malloc(MAXLENGTH * sizeof(char));
	}

	//initialize background array
	for (i = 0; i < MAXLENGTH; i++) {
		background[i] = malloc(MAXLENGTH * sizeof(char));
	}

	while (1) {
		bg = 0;
		int cnt = getcmd("\n>>", args, &bg);
		/* the steps can be..:
		 (1) fork a child process using fork()
		 (2) the child process will invoke execvp()
		 (3) if background is not specified, the parent will wait,
		 otherwise parent starts the next command... */

		// "clean" the args
		args[cnt] = '\0';

		/* we implement the commands like a queue. If we have
		 more than 10 commands, the first one will be replaced
		 by the second one and so on.
		 So we construst a command array with size 11, the 10
		 most recently enterened command is put in slot 1~10 and
		 the newest command is put in slot 11*/
		if ((size_c) > 11) {
			for (i = 0; i < 10; i++) {
				exchange(commands[i], commands[i + 1]);
			}
			exchange(commands[10], args);
		} else {
			exchange(commands[size_c - 1], args);
		}


		//increment size_c every time
		size_c++;
		/*we check if the input command is ! plus a number,
		 thhi can be done by looking at args[0]*/

		//==================check if history command starts here============
		if ((args[0]!='\0') && (args[0][0] == '!')) {
			// check if the command only contains !n
			if (args[1] != '\0') {
				//dont do anything
			} else {
				token = strtok(args[0], "!");
				/* convert the string to a number
				 and check if it is valid*/
				if (token == NULL) {
					// don't do anything
				} else {
					num = atoi(token);
					int check;
					while (*token) {
						if (isdigit(*token++) == 0) {
							check = 0;
						} else
							check = 1;
					}
					if (check) {
						if (num != 0) {
							if ((num > size_c - 2) || (num < size_c - 11)
									|| (num < 1)) {
								printf("no command found in history\n");
							} else {
								if (size_c <= 11) {
									//command_index is position of the command we want
									command_index = num - 1;
									//size_c-2 is the position of the newest command
									exchange(commands[size_c - 2],commands[command_index]);
								} else {
									command_index = num - size_c + 11;
									/* if it is a valid histroy command ,put it into commands[10] which
									 stores the newest command*/
									exchange(commands[10],commands[command_index]);
								}	
								// if it is a valid history command, we put it into the execute array
								exchange(args, commands[command_index]);
							}
						}
					}
				}
			}
		}
		//===================check if history command ends here================


		//===================check if built-in commands here ==================
		if (args[0]!='\0'){
			//if the command is cd...
			if (!strcmp(args[0], "cd")) {
				if (args[2] != '\0') {
					printf("incorrect number of arguments\n");
				} else if (chdir(args[1]) != 0) {
					printf("not successful\n");
				}
			}
			//if the commnad is exit....
			if ((!strcmp(args[0], "exit")) && (args[1] == '\0')) {
				exit(0);
				free(commands);
			}
		
			// if the command is job...
			if (!strcmp(args[0], "jobs") && (args[1] == '\0')) {
				if (bgCounts == 0) {
					printf("There is no background process\n");
				} else {
					pid_t process_id;
					int status;
					int i;
					for (i = 0; i < bgCounts; i++) {
						process_id = pids[i];
						// check if program is still running
						pid_t running = waitpid(process_id, &status,WNOHANG);
						if (running == 0) {
							printf("Process_id [%d]  ", process_id);
							int length = argLength(background[i]);
							int j;
							for (j = 0; j < length; j++) {
								printf("%s ", background[i][j]);
							}
							printf("\n");
						}
					}
				}
			}

			// if the command is fg......
			if (!strcmp(args[0], "fg") && (args[2] == '\0')) {
				int i;
				for (i = 0; i < bgCounts; i++) {
					int status;
					if (pids[i] == atoi(args[1])) {
						printf("Now bring %d to foreground\n", pids[i]);
						waitpid(pids[i], &status, 0);
						printf("Finished \n");
					}
				}
			}
		}
		//==============piping======================
		int redi = checkRedir(args, cnt);
		char *char1[MAXLENGTH];
		char *char2[MAXLENGTH];
		int pipeCheck = checkPipe(args, cnt, char1, char2);
		if (pipeCheck == 0) piping(char1, char2);

		//================fork=================
		childPid = fork();
		if (childPid == 0 && pipeCheck == 1) {
			// if the command is pwd....
			if ((!strcmp(args[0], "pwd")) && (args[1] == '\0')) {
				char cwd[BUFFER];
				if (getcwd(cwd, sizeof(cwd)) != NULL) {
					printf("%s\n", cwd);
					exit(0);
				} else {
					printf("Not a valid argument\n");
					exit(0);
				}
			} else {
				if ((redi == 1) && (pipeCheck == 1)) {
					execvp(args[0], args);
					exit(0);
				} else if (redi == 0) {
					close(1);
					FILE *fp = fopen(args[cnt - 1], "w");
					fclose(fp);
					open(args[cnt - 1], O_RDWR);
					args[cnt - 2] = '\0';
					execvp(args[0], args);
					exit(0);
				}

			}


		}

		//==================parent===============================
		else {
			/*check if background is specified
			 if bg is 0,the child needs to run in the foreground
			 else we run the child in background*/
			if (bg == 0) {
				int wstatus;
				waitpid(childPid, &wstatus, 0);
			} else {
				//store childPid into pids array, index is bgCounts
				pids[bgCounts] = childPid;
				//store the arguments into the background array
				exchange(background[bgCounts], args);
				bgCounts++;
			}
		}
	}
}


