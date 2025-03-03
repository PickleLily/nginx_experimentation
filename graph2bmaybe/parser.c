#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

int root_node_id = -1;
#define MAX_NODES 1000

//input holds all of the information needed for all three graphs generations...
typedef struct input{
	char process_name[1024];
    char FD[1024];
	char syscall_name[1024];
	char args[1024];
    char returnValues[1024];
    char PID[1024];
	int inputID;
}Inputs;

void extract_value(const char *line, const char *key, char *output, size_t output_size) {
    char *pos = strstr(line, key);
    if (pos) {
        pos += strlen(key); // Move past the key
        while (*pos == ' ') pos++; // Skip spaces

        // Extract the next value (single word)
        sscanf(pos, "%255s", output);
    } else {
        output[0] = '\0'; // Set empty string if key is not found
    }
}

void extract_args(const char *line, const char *key, const char *stop, char *output, size_t output_size) {
    char *start = strstr(line, key);
    if (start) {
        start += strlen(key); // Move past "Args:"
        while (*start == ' ') start++; // Skip spaces

        char *end = strstr(start, stop); // Find "Return:"
        size_t len = (end) ? (size_t)(end - start) : strlen(start); // Calculate length up to "Return:" or end of string

        if (len >= output_size) len = output_size - 1; // Ensure it fits within buffer
        strncpy(output, start, len);
        output[len] = '\0'; // Null-terminate
    } else {
        output[0] = '\0'; // Set empty string if key is not found
    }
}

Inputs makeInput(char processName[], char FD[], char syscall[], char args[], char returnValue[], char PID[] ){
	Inputs newInput;
	strcpy(newInput.process_name, processName);
    strcpy(newInput.FD, FD);
	strcpy(newInput.syscall_name, syscall);
	strcpy(newInput.args, args);
    strcpy(newInput.returnValues, returnValue);
    strcpy(newInput.PID, PID);
	return newInput;
}

Inputs* parseInput(int *inputCount){
    FILE *input;
	char line[6*1024+1];
	int node = 0;
    Inputs *il = malloc(sizeof(Inputs) * MAX_NODES);


	//opens the local events.txt file (in same dir)
	input = fopen("events.txt", "r");
    if(!input){
        perror("Failed to open the input file");
		free(il);
    }

	while(fgets(line, sizeof(line), input)){
		char time[1024], type[1024], name[1024], FD[1024], syscall[1024], args[1024], returnVal[1024], PID[1024];
		sscanf(line, "%s %s Name:%s FD:%s Syscall:%s Args:%[^:] :%[^:] :%s", time, type, name, FD, syscall, args, returnVal, PID);

		extract_value(line, "Syscall:", syscall, sizeof(syscall));
		extract_args(line, "Args:", "Return:", args, sizeof(args)); // Extract everything between Args: and Return:
		extract_value(line, "Return:", returnVal, sizeof(returnVal));
		extract_value(line, "PID:", PID, sizeof(PID));

		char parsedArgs[1024];
		// printf("%s\n", args);
		size_t args_len = strlen(args);
    	size_t args_copy_len = (args_len > 7) ? args_len - 7 : 0;
		strncpy(parsedArgs, args, args_copy_len);
		parsedArgs[args_copy_len] = '\0';
		// printf("%s\n\n", parsedArgs);

		char parsedReturn[1024];
		// printf("%s\n", returnVal);
		size_t ret_len = strlen(returnVal);
    	size_t ret_copy_len = (ret_len > 3) ? ret_len - 3 : 0;
		strncpy(parsedReturn, returnVal, ret_copy_len);
		parsedReturn[ret_copy_len] = '\0';
		// printf("%s\n\n", parsedReturn);


        // TODO --> change makeInput()???
        Inputs i = makeInput(name, FD, syscall, parsedArgs, parsedReturn, PID);

		il[node] = i;
		node++;

		//extract root node from first accept4 syscall
		if (root_node_id == -1 && strcmp(syscall, "accept4") == 0) {
			char fd[256] = "Unknown FD";
			if (strstr(args, "fd=")) {
				sscanf(args, "fd=%255[^ ]", fd); // Extract fd field
				// parse_file_descriptor(fd, fd);  // Extract file path from fd
			}
		}
	}
	*inputCount = node;
    return il;
}

bool parseSyscall(char syscall[], char returnValues[], char arguments[], char FD[]){
	//does NOT interact with a file...
	// || strcmp(syscall, "access") == 0
	if(strcmp(syscall, "rt_sigaction") == 0 || strcmp(syscall, "rt_sigprocmask") == 0 || strcmp(syscall, "brk") == 0 || strcmp(syscall, "munmap") == 0)
	{
		return false;
	}
	// chdir with <NA> does not touch a file path...
	else if(strcmp(syscall, "chdir") == 0 && strcmp(arguments, "") == 0)
	{
		return false;
	}
	// open with <NA> does not touch a file file path...
	//&& strcmp(returnValues, "<NA>") == 0
	// TODO --> this is resulting in the removal of the other PIDS BUT not fixing tracking the syscalls???
	else if (strcmp(syscall, "open") == 0 )
	{
		return false; 
	}
	// mmap with <NA> does not touch a file file path...
	else if(strcmp(syscall, "mmap") == 0 && strcmp(FD, "<NA>") == 0)
	{
		return false;
	}
	// close with no arguments is a duplicate call...
	else if(strcmp(syscall, "close") == 0 && (strcmp(returnValues, "0 ") == 0 || strcmp(arguments, "") == 0))
	{
		return false;
	}else if(strcmp(syscall, "access") == 0 && strcmp(arguments, "mode=0") == 0 ){
		return false;
	}
	// This is a system call that HAS information...
	else
	{
		return true;
	}
}

int main(){
	//this is returning just the first system call NOT a list of them all...
	int inputCount = 0;
	Inputs *inputs = parseInput(&inputCount);

    FILE *intermediateOutput = fopen("intermediateOutput.txt", "w");
    printf("IntermediateOutput.txt created...");

	int i = 0;
	bool seenAccept4 = false;
	for(int i = 0; i < inputCount; i++){
		{
			char printable[5096]= "";

			// syscall, return, arg, fd
			bool log = parseSyscall(inputs[i].syscall_name, inputs[i].returnValues, inputs[i].args, inputs[i].FD);
			if(strcmp(inputs[i].syscall_name, "accept4") == 0){
				seenAccept4 = true;
			}

			if(log == true && seenAccept4 == true){
				snprintf(printable , 5096, "%s %s %s", inputs[i].syscall_name, inputs[i].PID, inputs[i].args);
				fprintf(intermediateOutput, "%s\n", printable);
			}
		}	
	}
}
