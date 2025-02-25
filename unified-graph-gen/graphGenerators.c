#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "graphMaker.h"

#define MAX_EDGES 1000
#define MAX_SYSCALLS 373
int node_count = 0, edge_count = 0;
int root_node_id = -1;
//SETUP AREA

//type 1 objects...
typedef struct Edge {
    int parent;       // ID of the source node
    int child;        // ID of the destination node
    char args[1000]; // The system call connecting the nodes
} Edge;


typedef struct Node {
    int syscallID; //The system call number for this specific call
    int nodeID; // unique node identifier
} Node;

Node nodes[MAX_SYSCALLS];
Edge edges[MAX_EDGES];

Node* makeNode(int syscall){
	Node* newNode = (Node*)malloc(sizeof(Node));
	newNode->syscallID = syscall;
	//maybe store the name that we find???
	newNode->parent = NULL;
	newNode->children = NULL;
	newNode->numChildren = 0;
	return newNode;
}


//May want to add arguments? or more details here...
Edge* makeEdge(int parentSyscallID, int childSyscallID){
	Edge* newEdge = (Edge*)malloc(sizeof(Edge));
	newEdge->parent = parentSyscallID;
	newEdge->child = childSyscallID;
	//maybe store the edges??
	return newEdge;
}

//Helper function to find the syscall nuber via the name
int findNum(char *x){
    int min = 0;
    int max = MAX_SYSCALLS-1;
    while(min <= max){
        int mid = min + (max - min) / 2;
        int eval = strcmp(x, syscallNames[mid]);

        if(eval == 0){
            return syscallNumbers[mid];
        }
        if(eval < 0){
            max = mid - 1;
        }else {
            min = mid + 1;
        }
    }
    //On error
    return -1;
}


// Helper function to parse file descriptors with "<f>"
void parse_file_descriptor(const char *fd, char *file_path){
	char *start = strstr(fd, "<f>");
	if (start) {
	       	start += 3; // Skip past "<f>"
	        char *end = strchr(start, ')');
        	if (end) {
	            size_t length = end - start;
        	    strncpy(file_path, start, length);
        	    file_path[length] = '\0'; // Null-terminate the extracted path
       		 } else {
            		strncpy(file_path, "Unknown File", 255);
        	 }
  	} else {
        	strncpy(file_path, fd, 255); // If no "<f>", use the entire fd string
	}
}


//this graph generation with nodes and edges is made from Lilys version of graph generation...
void dotFileGeneration(FILE *file, int node_count, int edge_count, Node nodes[], Edge edges[]){
	fprintf(file, "digraph nginx_syscalls {/n");
	for(int i = 0; i < node_count; i++){
		fprintf(file, "  %d [label=\"%s\"];\n", nodes[i].syscallID, nodes[i].name);
	}

	for(int i = 0; i < edge_count; i++){
		//fprintf(file, "  %d -> %d [label = \"%s\"];\n]", 
		//	edges[i].parent, edges[i].child, edges[i].syscall);
		fprintf(file, "  %d -> %d;\n", edges[i].parent, edges[i].child);

	}

	fprintf(file, "}\n");
	return;
}


// Function to find or add a node
int find_or_add_node(const char *name) {
    for (int i = 0; i < node_count; i++) {
        if (strcmp(nodes[i].name, name) == 0) {
            return nodes[i].id;
        }
    }
    if (node_count >= MAX_NODES) {
        fprintf(stderr, "Error: Maximum nodes exceeded.\n");
        exit(1);
    }
    nodes[node_count].id = node_count;
    strncpy(nodes[node_count].name, name, sizeof(nodes[node_count].name) - 1);
    return nodes[node_count++].id;
}

// Function to add an edge
void add_edge(int from, int to, const char *syscall) {
    if (edge_count >= MAX_EDGES) {
        fprintf(stderr, "Error: Maximum edges exceeded.\n");
        exit(1);
    }
    edges[edge_count].parent = from;
    edges[edge_count].child = to;
    strncpy(edges[edge_count].syscall, syscall, sizeof(edges[edge_count].syscall) - 1);
    edge_count++;
}


//Graph 1a Generation
/*Ellas method
	each instance of a node is a specific invocation of a system call where
	the path that is taken is specific to the syscalls prev. and post.
	we can see the flow of interactions on the site rather than from syscall to syscall*/
void graph1a_generation(FILE *input){
	return;
}


//Graph 1b Generation
/*Lilys method
	this works by creating a graph that links all nodes in an absolute manner
	each instance of a node is the syscall with different edges representing all
	possibilities of syscalls prev. and post that node.*/
void graph1b_generation(FILE *input){
	//parse the input file...
	char line[1024];
	while(fgets(line, sizeof(line), input)){
		char syscall[64], args[512];
		sscanf(line, "Syscall=%s Args=%[^\n]", syscall, args);

		//extract root node from first accept4 syscall
		if (root_node_id == -1 && strcmp(syscall, "accept4") == 0) {
	            char fd[256] = "Unknown FD";
        	    if (strstr(args, "fd=")) {
                	sscanf(args, "fd=%255[^ ]", fd); // Extract fd field
                	parse_file_descriptor(fd, fd);  // Extract file path from fd
            	    }
            	    root_node_id = find_or_add_node(fd); // Use fd as root node
            	    continue;
         	}
		// Parse arguments for objects like files or sockets
        	char object1[256] = "";
        	if (strstr(args, "path=")) {
            		sscanf(args, "path=%255s", object1); // Extract file path
        	} else if (strstr(args, "fd=")) {
            		char fd[256];
	            	sscanf(args, "fd=%255[^ ]", fd); // Extract fd field
        	  	if (strstr(fd, "<f>")) {
              			parse_file_descriptor(fd, object1); // Extract file path if "<f>"
           	    	} else {
                	  	strncpy(object1, fd, 255); // Use fd as-is if not a file
            	    	}
        	}

        	// Add edges if relevant objects are found
        	if (strlen(object1) > 0 && root_node_id != -1) {
            		int to = find_or_add_node(object1);
            		add_edge(root_node_id, to, syscall);
        	}
	}
	return;
}


//Graph 2 Generation
/*Keatons method:
	this type of graph focuses on the arguments and the objects of interaction
	as the nodes with the edges being the system calls that relate to the objects.
*/
void graph2_generation(FILE *input){
	return;
}


//Generate all three graphs

int main(){
	printf("start...\n");

//open all the input and the output files for the graph generation...
	FILE *input;
	FILE *type1a_output;
	FILE *type1b_output;
	FILE *type2_output;

	input = fopen("input.txt", "r");
	//output is .dot file for the graph gen library...
	type1a_output = fopen("graph1a.dot", "w");
	type1b_output = fopen("graph1b.dot", "w");
	type2_output = fopen("graph2.dot", "w");

	if(!type1a_output || !type1b_output || !type2_output){
		perror("Failed to open an output file...\n");
		return -1;
	}
	if(!input){
		perror("Failed to open the input file...\n");
	}


	printf("Generating Type 1a Graph\n");
	type1a_output = graph1a_generation(input);

	printf("Generating Type 2 Graph\n");
	type1b_output = graph1b_generation(input);

	printf("Generating Type 2b Graph\n");
 	type2_output = graph2_generation(input);

	printf("fin.\n");

	//cleanup
	fclose(input);
	fclose(type1a_output);
	fclose(type1b_output);
	fclose(type2_output);

}
