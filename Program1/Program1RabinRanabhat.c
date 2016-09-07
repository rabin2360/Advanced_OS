/*
 ============================================================================
 Name        : Program1.c
 Author      : Rabin Ranabhat
 Version     : 1.0
 Copyright   : ---------------
 Description : Custom linux shell written in C
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/wait.h>
//to read write the file
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAX_COMMAND_SIZE 256
#define NAME_SIZE 100
#define MY_FIFO "myfifo"
#define OUTPUTFILE "outputFile"

//global variables to implement history
int tokenCount;
int historyCount;
char * historyBuffer[MAX_COMMAND_SIZE];

void executeCenter(char []);

//parsing the input string and converts the words in the input string to elements in the array
char ** parseString(char * inputString)
{
	char ** tokenArray = malloc(MAX_COMMAND_SIZE * sizeof(char));
	int i = 0;
	char * tokens;

	//tokenization
	tokens = strtok(inputString, " ");

	//breaks the input string into tokens using space as a delimiter
	while(tokens != NULL)
	{
		tokenArray[i++] = tokens;
		tokens = strtok(NULL, " ");
	}

	tokenCount = i;
	return tokenArray;
}

bool commandHasProcessSubstitution(char * parsedCommand[], int length)
{
	bool hasProcessSubCommand = false;

	for(int i = 0; i<(length -1); i++)
	{
		if(strcmp(parsedCommand[i], "<(") == 0)
		{
			hasProcessSubCommand = true;
			break;
		}
	}

	return hasProcessSubCommand;
}

//determines the presence of '|' in the command
bool commandHasPipeCharacter(char *parsedCommand[], int length)
{
	bool hasPipeCharacter = false;

	//searching for pipe character
	for(int i = 0; i<(length-1); i++)
	{
		if(strcmp(parsedCommand[i], "|") == 0)
		{
			hasPipeCharacter = true;
			break;
		}

	}

	return hasPipeCharacter;
}

//determines the presence of '&' in the command
bool commandHasAmpersand(char * command)
{
	bool hasAmpersand = false;
	int stringLength = strlen(command);

	//checking if the process has to run in the background, decimal for & = 38
	if(command[stringLength -1] == 38)
	{
		hasAmpersand = true;
	}

	return hasAmpersand;
}

//determines the presence of '<', '>' in the command
bool commandHasIORedirection(char * parsedCommand[], int length)
{
	bool hasIORedirection = false;

	for(int i = 0; i< (length -1); i++)
	{
		if((strcmp(parsedCommand[i], ">") == 0) || (strcmp(parsedCommand[i], "<") == 0))
		{
			hasIORedirection = true;
			break;
		}
	}

	return hasIORedirection;
}

//processes I/O redirection
void processIORedirection(char * parsedCommand[], int length)
{
	int in, out;
	int pidStatus;
	pid_t pid;
	int redirectionOption = 0;

	int i;
	//determines the redirection option
	for(i = 0; i< (length -1); i++)
	{
		if((strcmp(parsedCommand[i], ">") == 0) || (strcmp(parsedCommand[i], "<") == 0))
		{
			//redirect output to a file
			if(strcmp(parsedCommand[i], ">") == 0)
			{
				redirectionOption = 1;
			}
			//redirect input from a file
			else if(strcmp(parsedCommand[i], "<") == 0)
			{
				redirectionOption = 2;
			}
			//if neither
			else
			{
				redirectionOption = 0;
			}

			break;
		}
	}


	//separating the parsedCommands to first and second command
	char ** firstCommand = malloc(MAX_COMMAND_SIZE*sizeof(char));
	char ** secondCommand = malloc(MAX_COMMAND_SIZE*sizeof(char));

	//first command
	int j;
	for(j = 0; j<i; j++)
	{
		firstCommand[j] = parsedCommand[j];
	}

	firstCommand[j] = '\0';

	//second command
	int k = 0;
	for(int k = i+1; k<(tokenCount); k++)
	{
		secondCommand[k-(i+1)] = parsedCommand[k];
	}

	secondCommand[k-(i+1)] = '\0';

	pid = fork();

	if(pid == 0)
	{
		//child process
		if(redirectionOption ==1)
		{
			//redirect output i.e. >
			out = open(secondCommand[0], O_CREAT | O_TRUNC | O_WRONLY, 0666);

			//replace the standard output with appropriate file
			dup2(out, STDOUT_FILENO);
			close(out);

		}
		else if(redirectionOption ==2)
		{
			//redirect input i.e. <
			in = open(secondCommand[0], O_RDONLY, 0666);

			//replace standard input with appropriate file
			dup2(in, STDIN_FILENO);
			close(in);
		}

		execvp(firstCommand[0], firstCommand);
		printf("error executing the command\n");
	}
	else
	{
		//parent process
		waitpid(pid, &pidStatus, 0);
	}

	free(firstCommand);
	free(secondCommand);
}

//determine if the command is 'fc'
bool commandHasFCInput(char * command)
{
	bool hasFCCommand = false;

	if(strcmp(command, "fc") == 0)
	{
		hasFCCommand = true;
	}

	return hasFCCommand;
}

//process 'fc' command. Restricted version
void processFCCommand(char * parsedCommand[], int length)
{
	char temp [MAX_COMMAND_SIZE];
	int start = atoi(parsedCommand[1]);
	int end = atoi(parsedCommand[2]);

	if(end>=start)
	//when n2>=n1
	{
		//the start and end values need to be positive
		if(historyCount  == 0 || end >historyCount || end >=MAX_COMMAND_SIZE || start<= 0 || end <= 0)
		//error conditions
		{
			printf("fc failed because:\n"
					"- either, there is no history\n"
					"- or, start and end values cannot be negative\n"
					"- or, the end count is greater than the total number of recorded items\n"
					"- or, the end count is greater than the size of the history buffer - %d\n",(MAX_COMMAND_SIZE-1));
		}
		else
		//listing the history and executing the commands
		{
			for(int i = (start-1); i<end; i++)
			{
				printf("%d: %s\n", i+1,historyBuffer[i]);

				strcpy(temp, historyBuffer[i]);

				executeCenter(temp);
			}
		}

	}
	//when n1>n2
	else
	{
		printf("fc command error. \nfc usage format: fc -n1 -n2 (n1 is smaller than or equal to n2)\n");
	}

}

//show history
void showHistory()
{
	if(historyCount > 0)
	{

		int i = 0;
		while(historyBuffer[i] !=NULL)
		{
			printf("%d: %s\n",i+1, historyBuffer[i]);
			i++;
		}
	}
	else
	{
		printf("There are currently no items in history\n");
	}

}

//adds the commands entered to history
void addToHistory(char historyElement[])
{
	char *command = malloc(MAX_COMMAND_SIZE*sizeof(char));

	int j = 0;

	//copying the history element to to the command array
	for(j = 0; j<strlen(historyElement); j++)
	{
		command[j] = historyElement[j];
	}

	//adding NULL to the end of the command string
	command[j]='\0';

	//adding element to the history buffer
	historyBuffer[historyCount++] = command;

	//making the list circular
	historyCount = historyCount % (MAX_COMMAND_SIZE-1);

	//free(command);
}

//processes the commands with pipe character - needs some work
void processPipeCharacter(char * parsedCommand[], int length)
{
	int i = 0;
	int k = 0;
	int j = 0;
	int pipefd[2];
	int pipefd1[2];

	pid_t childpid;
	int pidStatus;
	int numberOfCommands = 0;

	char * command[MAX_COMMAND_SIZE];

	//creating the pipes
	//pipe(pipefd);
	//pipe(pipefd1);

	while(parsedCommand[i]!=NULL)
	{
		if(strcmp(parsedCommand[i], "|") == 0)
		{
			numberOfCommands++;
		}
		//printf("parsedCommand: %s\n", parsedCommand[i]);
		i++;
	}
	numberOfCommands++;

	//printf("number of commands: %d\n", numberOfCommands);
	for(i = 0; i<numberOfCommands;i++)
	{

		k = 0;
		while(strcmp(parsedCommand[j], "|") != 0)
		{
			//printf("%s\n", parsedCommand[j]);
			command[k] = parsedCommand[j];
			j++;
			k++;

			if(parsedCommand[j] ==NULL)
			{
				k++;
				break;
			}

		}

		//increment j to skip over '|'
		j++;

		//adding null to the end of the command for execvp
		command[k] = NULL;

		if(i % 2  == 0)
		{
			pipe(pipefd);
		}
		else
		{
			pipe(pipefd1);
		}

			//creating the child process
			childpid = fork();

			if(childpid == 0)
			{
				//child process execution
				if(i == 0)
				{
					//standard output is changed to the pipefd[1]
					dup2(pipefd[1],STDOUT_FILENO);
					execvp(command[0], command);

					printf("Processing failed. Please check the command(s) entered.\n");
					exit(EXIT_FAILURE);
				}
				//last element in the sequence of piped command
				//determining the input for the last element in the command chain
				else if(i == (numberOfCommands-1))
				{
					//if even numbered command uses pipefd for file input
					if(numberOfCommands % 2 == 0)
					{
						dup2(pipefd[0], STDIN_FILENO);
					}
					//odd number command uses pipefd1 for file input
					else
					{
						dup2(pipefd1[0], STDIN_FILENO);
					}

					execvp(command[0], command);


					printf("Processing failed. Please check the command(s) entered.\n");
					exit(EXIT_FAILURE);
				}
				//determining the input and output for elements in the middle of the command chain
				else
				{
					//if even numbered command,
					//pipefd1 feeds the input and output is written to pipefd is where out put is stored
					if(i % 2  == 0)
					{
						dup2(pipefd1[0], STDIN_FILENO);
						dup2(pipefd[1], STDOUT_FILENO);
					}
					//if the command is odd,
					//pipefd feeds the input and pipefd1 is where output is sent to
					else
					{
						dup2(pipefd[0], STDIN_FILENO);
						dup2(pipefd1[1], STDOUT_FILENO);
					}

					execvp(command[0],command);
				}
			}


			//closing the pipe accordingly
			if(i == 0)
			{
				close(pipefd[1]);
			}
			else if(i == (numberOfCommands -1))
			{
				if(numberOfCommands %2 == 0)
				{
					close(pipefd[0]);
				}
				else
				{
					close(pipefd1[0]);
				}

			}
			else
			{
				if(i % 2 == 0)
				{
					close(pipefd1[0]);
					close(pipefd[1]);
				}else
				{
					close(pipefd[0]);
					close(pipefd1[1]);
				}
			}

			//parent waits for child execution to finish
			waitpid(childpid, &pidStatus, 0);
		}

}

//process substitution
void processSubstitution(char * parsedInput[], int length)
{
	pid_t childPid;
	int writeFifo;
	int readFifo;
	int pidStatus;
	int redirectionOption;

	int i;
	//determines the redirection option
	for(i = 0; i< (length -1); i++)
	{
		if((strcmp(parsedInput[i], ">(") == 0) || (strcmp(parsedInput[i], "<(") == 0))
		{
			//redirect output to a file
			if(strcmp(parsedInput[i], ">(") == 0)
			{
				redirectionOption = 1;
			}
			//redirect input from a file
			else if(strcmp(parsedInput[i], "<(") == 0)
			{
				redirectionOption = 2;
			}
			//if neither
			else
			{
				redirectionOption = 0;
			}

			break;
		}
	}


	//separating the parsedCommands to first and second command
	char ** firstCommand = malloc(MAX_COMMAND_SIZE*sizeof(char));
	char ** secondCommand = malloc(MAX_COMMAND_SIZE*sizeof(char));

	//first command
	int j;
	for(j = 0; j<i; j++)
	{
		firstCommand[j] = parsedInput[j];
	}

	firstCommand[j] = '\0';

	//second command
	int k = 0;
	for(int k = i+1; k<(tokenCount-1); k++)
	{
		secondCommand[k-(i+1)] = parsedInput[k];
	}

	secondCommand[k-(i+1)] = '\0';


	//creating named pipe
	if(mkfifo(MY_FIFO, 0666)<0)
	{
		perror("mkfifo error\n");
	}

	//since only two commands are being processes at the moment
	for(int i = 0; i<2; i++)
	{
		childPid = fork();

		if(childPid == 0)
		{
			if(i == 0)
			{
				writeFifo = open(MY_FIFO, O_WRONLY | O_NONBLOCK);
				dup2(writeFifo, STDOUT_FILENO);
				close(writeFifo);
				execvp(secondCommand[0], secondCommand);

			}
			else
			{
				readFifo = open(MY_FIFO, O_RDONLY | O_NONBLOCK);
				dup2(readFifo, STDIN_FILENO);
				close(readFifo);
				execvp(firstCommand[0], firstCommand);
			}
		}

		waitpid(childPid, &pidStatus, 0);
	}

	//unlinking the named pipe
	unlink(MY_FIFO);
}

//appending the command "tee -a Filename" to the inputValue
void addTeeCommand(char inputCommand [], char outputFileName [])
{

	//variables
	char ** parsedCommand = malloc(MAX_COMMAND_SIZE*sizeof(char));
	char *teeCommand[]= {"|", "tee", "-a", outputFileName};

	//parsing input value
	parsedCommand = parseString(inputCommand);

	//appending the "tee -a fileName" command to the parsed command
	for(int i = 0; i<4; i++)
	{
		parsedCommand[tokenCount++] = teeCommand[i];
	}

	//processing the parsed command
	processPipeCharacter(parsedCommand, tokenCount);

	free(parsedCommand);
}

void customCommandHeaderStatement()
{
	//print statements when in record mode
	printf("!!!!!!!!!!!Record mode!!!!!!!!!!!\n");
	printf("Enter stop to exit record mode.\n");

}

//for custom command that will output the execution of the command to the console window and an output file
void customCommand(char * parsedInput[], int length)
{
	bool recordCommands = true;
	char inputString[MAX_COMMAND_SIZE];
	//char outputFileName[NAME_SIZE] = OUTPUTFILE;
	char * pos;

	customCommandHeaderStatement();


	while(recordCommands)
	{
		//this string is appended before the inputString to echo the entered command to the output file
		char inputValue[] = "echo ";

		printf("> ");

		//program exits when cntrl-D pressed
				if(fgets(inputString, sizeof(inputString), stdin) == NULL)
				{
					printf("Exiting...\n");
					recordCommands = false;
					break;
				}

				//empty strings not allowed
				if(strlen(inputString) > 1)
				{
					//getting rid of the newline character
					if((pos = strchr(inputString, '\n')) != NULL)
					{
						*pos = '\0';
					}

					//exit when the entered command is "quit"
					if(strcmp(inputString,"stop") == 0)
					{
						printf("Exiting record...\n");
						recordCommands = false;
						break;
					}

					//this part echoes the entered command to the specified file
					strcat(inputValue, inputString);
					//printf("inputValue:%s\n ",inputValue);

					addTeeCommand(inputValue, OUTPUTFILE);

					//this part sends the output of running the command to the specified file
					addTeeCommand(inputString, OUTPUTFILE);

					strcpy(inputString, "echo -----------------------------------------------------");

					//this part just prints the ------- at the end of command execution
					addTeeCommand(inputString, OUTPUTFILE);


				}//empty strings are not printed

	}//infinite loop in "record mode"

}//end of the method

//determines the command execution course
void executeCenter(char inputString [])
{

	//variables
	pid_t pid;
	int pidStatus;

	bool hasAmpersand = false;
	bool runInBackground = false;
	bool hasPipeCharacter = false;
	bool hasIORedirection = false;
	bool hasFCCommand = false;
	bool hasProcessSubCommand = false;

	//add to history
	addToHistory(inputString);

	//parse the input
	char ** parsedInput = parseString(inputString);

	//check if the process needs to run in the background
	hasAmpersand = commandHasAmpersand(parsedInput[0]);
	hasPipeCharacter = commandHasPipeCharacter(parsedInput, tokenCount);
	hasIORedirection = commandHasIORedirection(parsedInput, tokenCount);
	hasFCCommand = commandHasFCInput(parsedInput[0]);
	hasProcessSubCommand = commandHasProcessSubstitution(parsedInput, tokenCount);


	if(hasPipeCharacter)
	{
		processPipeCharacter(parsedInput, tokenCount);
	}
	else if(hasIORedirection)
	{
		processIORedirection(parsedInput, tokenCount);
	}
	else if(hasFCCommand)
	{
		if(tokenCount == 3)
		{
			processFCCommand(parsedInput, tokenCount);
		}
		else
		{
			printf("fc command error.\nfc usage format: fc -n1 -n2 (n1 and n2 are positive integers.)\n");
		}
	}
	//command entered is history
	else if (strcmp(parsedInput[0], "history") == 0)
	{
		showHistory();
	}
	//has process substitution command
	else if(hasProcessSubCommand)
	{
		processSubstitution(parsedInput, tokenCount);
	}
	//custom record command - puts the shell in the record mode
	else if(strcmp(parsedInput[0], "record") == 0)
	{
		customCommand(parsedInput, tokenCount);
	}
	//if none of the others - determination is made whether to run the program in background or foreground
	else
	{
		runInBackground = false;
		//is hasAmpersand then the program is run in the background
		if(hasAmpersand)
		{
			runInBackground = true;
			char * temp = parsedInput[0];
			temp[strlen(temp)-1] = 0;
			parsedInput[0] = temp;
		}


		//fork a child and send the command
		pid = fork();

		//child process
		if(pid == 0)
		{
			execvp(parsedInput[0], parsedInput);

			printf("Command execution failed.\n");
			exit(EXIT_FAILURE);
		}
		//parent process
		else if(pid < 0)
		{
			printf("Child process could not be created.\n");
		}
		else
		{
			//determine foreground or background
			if(runInBackground)
			{
				//fork a child and run in background
				while(waitpid(pid, &pidStatus, WNOHANG)>0)
				{
					printf("Child died.\n");
				}
			}
			else
			{
				//parents waiting for the child
				waitpid(pid, &pidStatus, 0);

			}//parent process - foreground and background

		}//parent process else statement

	}//final else part (if none of the if conditions are true)

}//end of method

//welcome message for the terminal
void welcomeMessage()
{
	printf("Program1\n");
	printf("=======================\n");
}

//running program in interactive mode
void interactiveMode()
{
	//variables
	char inputString[MAX_COMMAND_SIZE];
	char hostname[NAME_SIZE];
	int getHostError;
	char *pos;

	//getting the host name
	getHostError = gethostname(hostname, sizeof(hostname));

	//error while getting the host name
	if(getHostError == -1)
	{
		printf("Could not get the host name. Exiting now.\n");
		exit(1);
	}

	welcomeMessage();

	//interactive shell
	while(1)
	{

		printf("%s~Program1> ", hostname);

		//program exits when cntrl-D pressed
		if(fgets(inputString, sizeof(inputString), stdin) == NULL)
		{
			printf("Exiting...\n");
			exit(EXIT_SUCCESS);
		}

		//empty strings not allowed
		if(strlen(inputString) > 1)
		{
			//getting rid of the newline character
			if((pos = strchr(inputString, '\n')) != NULL)
			{
				*pos = '\0';
			}

			//exit when the entered command is "quit"
			if(strcmp(inputString,"quit") == 0)
			{
				printf("Exiting...\n");
				exit(EXIT_SUCCESS);
			}

			//execute the command
			executeCenter(inputString);

		}//making determination that the input string is not empty
	}//infinite loop in interactive mode
}

//running program in batch mode
void batchMode(char * fileName)
{
	FILE *fp;
	char buff[1028];
	char inputArray [MAX_COMMAND_SIZE][MAX_COMMAND_SIZE];

	fp = fopen(fileName, "r");

	if(fp == NULL)
	{
		perror("Error while opening the file\n");
		exit(EXIT_FAILURE);
	}

	//tracks the number of commands read from the file
	int size = 0;
	char * pos;

	printf("File Read:\n");

	while(fgets(buff, sizeof(buff), (FILE*)fp)!=NULL)
	{
		//getting rid of the newline character
		if((pos = strchr(buff, '\n')) != NULL)
		{
			*pos = '\0';
		}

		printf("%s\n",buff);
		strcpy(inputArray[size], buff);

		size++;
	}

	printf("\n");


	//iterates through the list of commands read and executes them
	for(int i = 0; i<size; i++)
	{
		executeCenter(inputArray[i]);
		printf("\n");
	}

	fclose(fp);
}

//driver
int main(int argc, char* argv[]) {

	//decide interactive or batch mode
	if(argc>1)
	{
		//batch mode
		batchMode(argv[1]);
	}
	else
	{
		//interactive mode
		interactiveMode();
	}

	return EXIT_SUCCESS;
}

