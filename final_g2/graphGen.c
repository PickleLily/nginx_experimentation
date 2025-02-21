#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// --------------------SET UP---------------------------------------------------------
#define MAX_SUBNODES 100
#define MAX_SUBEDGES 1000
#define MAX_SUBGRAPHS 100

// Structures for Nodes and Edges
typedef struct Node {
    char PID[64];         // PID
    char args[256];       // Name of the object (e.g., file path, socket info, subprocesses info?)
    int fd;               // The file descriptor
    int graphNum;         //Unique subgraph
    char shape[128];
} Node;

typedef struct Edge {
    char from[256];       // name of the source node 
    char to[256];         // name of the destination node
    int graphNum;         //Unique subgraph
    char syscall[64];     // The system call connecting the nodes
    char edgeType[128];   //"dashed", "dotted" ,"solid", "invis", "bold"
} Edge;

// Subgraph representation
typedef struct Subgraph {
    int graphNum;           //Unique subgraph number
    int currentfd;          //fd of the root accept4 node - not sure if we even need - shpuld this be current fd?
    int masterPID_ID;       //the Node ID of the process that starts interactions?
    Node* nodes[MAX_SUBNODES];
    Edge* edges[MAX_SUBEDGES];
    int node_count;
    int edge_count;
} Subgraph;

// Global graph ID
int graphNum = 0;

// Global graph Reference
Subgraph* graphs[MAX_SUBGRAPHS];

// ---------------------Functions --------------------------------------------------------
//  TODO --> add "regular line" status??

void add_edge(int from, int to, const char *syscall) {
    //get current subgraph
    int subgraphID = graphNum;
    Subgraph* graph = graphs[graphNum];
    Edge **edges = graph->edges; // A list of edge pointers

    int lengthEdges = graph->edge_count;
    
    //check if edge exists
    for (int i = 0; i < lengthEdges; i++) {
        // Ella's notes here: (Thank you AI gods) because edges is a pointer to a collection of pointers
        // We have to dereference each pointer we work with
        // Sorry for using so many pointers
        // We don't need to derefence on strcmp because we're comparing it to another string pointer
        if (*edges[i]->from == from && *edges[i]->to == to && strcmp(edges[i]->syscall, syscall) == 0) {
            return; // Duplicate edge found, do not add
        }
    }	

    //check max edges
    if (lengthEdges >= MAX_SUBEDGES) {
        fprintf(stderr, "Error: Maximum edges exceeded.\n");
        exit(1);
    }

    graph->edge_count++;
    // Set temp values to avoid casting issues
    // May be able to improve this
    char tempFrom[256];
    snprintf(tempFrom, sizeof(from), "%d", from);
    strncpy(edges[lengthEdges]->from, tempFrom, sizeof(*edges[lengthEdges]->from)-1);
    char tempTo[256];
    snprintf(tempTo, sizeof(from), "%d", from);
    strncpy(edges[lengthEdges]->to , tempTo, sizeof(*edges[lengthEdges]->from)-1);
    strncpy(edges[lengthEdges]->syscall, syscall, strlen(syscall));
    strncpy(edges[lengthEdges]->edgeType, "solid", strlen("solid"));
}

int find_or_add_node(const char *args, char PID[], int *node_count) {  
    //get current subgraph
    int subgraphID = graphNum;
    Subgraph* graph = graphs[graphNum];
    Node **nodes = graph->nodes;
    int nodeCount = graph->node_count;

    for (int i = 0; i < nodeCount; i++) {  
        //if we see a matching argument break out of the loop...
        if (strcmp(nodes[i]->args, args) == 0) {
            return nodes[i]->fd;
        }
    }

    if (nodeCount >= MAX_SUBNODES) {
        fprintf(stderr, "Error: Maximum nodes exceeded.\n");
        exit(1);
    }

    nodes[nodeCount]->fd = *node_count;
    strncpy(nodes[nodeCount]->PID, PID, sizeof(nodes[nodeCount]->PID) - 1);
    strncpy(nodes[nodeCount]->args, args, sizeof(nodes[nodeCount]->args) - 1);
    graph->node_count++;
    
    return nodes[nodeCount]->fd;
}


int getSubgraphFD(int currentFD){
    //go through the list of graphs globally
    for(int i = 0; i < length(graphs); i ++){
        Subgraph *g = graphs[i];

        //check the currentfd of each subgraph and return that graphs graphNUM
        if(currentFD == g->currentfd){
            return g->graphNum;
        }
    }
    return -1;
}

// When supplied with fd of accept4 call, make new split graph
void makeSubgraph(int fd, char *socketTuple) {
    // Take global graph num, current fd of accept4, and arg information to build the Subgraph
    // Predefine the two sockets


    // TODO --> change this to use strstr()?  & match with "->"

    char socket1[56];
    char socket2[56];
    char *end = strchr(socketTuple, '-');
    if (end) {
        size_t length = end - socketTuple;
            strncpy(socket1, socketTuple, length);
            socket1[length] = '\0'; // Null-terminate the extracted socket
    }
    //Skip over ->
    char *start = end + 2;
    end = strchr(socketTuple, '\0');
    if (end) {
        size_t length = end - start;
            strncpy(socket2, start, length);
            socket2[length] = '\0'; // Null-terminate the extracted socket
    }

    // Initialize subgraph
    Subgraph* subgraph = (Subgraph*)malloc(sizeof(Subgraph));
    subgraph->graphNum = graphNum;
    subgraph->currentfd = fd;
    subgraph->node_count = 0;
    subgraph->edge_count = 0;

    // Make remote node pointer
    Node* remote = (Node*)malloc(sizeof(Node));
    strncpy(remote->args, socket1, sizeof(socket1)-1);
    remote->fd = fd;
    remote->graphNum = graphNum;
    strncpy(remote->shape, "diamond", 7);
    subgraph->nodes[subgraph->node_count] = remote;
    subgraph->node_count++;

    // Make local node
    Node* local = (Node*)malloc(sizeof(Node));
    strncpy(local->args, socket2, sizeof(socket2)-1);
    local->fd = fd;
    local->graphNum = graphNum;
    strncpy(local->shape, "diamond", 7);
    subgraph->nodes[subgraph->node_count] = local;
    subgraph->node_count++;
    
    //  Connect two
    Edge* edge = (Edge*)malloc(sizeof(Edge));
    strncpy(edge->from, socket1, sizeof(socket1));
    strncpy(edge->to, socket2, sizeof(socket2));
    edge->graphNum = graphNum;
    strncpy(edge->syscall, "accept4", 7);
    subgraph->edges[subgraph->edge_count] = edge;
    subgraph->edge_count++;

    // Add to global list
    graphs[graphNum] = subgraph;
}

// Helper method to parse file argument with "<f>"
void parseFileName(const char *args, char *outputArgs) {
    char *start = strstr(args, "<f>");
    if (start) {
        start += 3; // Skip past "<f>"
        char *end = strchr(start, ')');
        if (end) {
            size_t length = end - start;
            strncpy(outputArgs, start, length);
            outputArgs[length] = '\0'; // Null-terminate the extracted path
        } else {
            strncpy(outputArgs, "Unknown File", 255);
        }
    } else {
        strncpy(outputArgs, args, 255); // If no "<f>", use the entire fd string
    }
}


// Helper method to parse socket tuple
void parseNetworkTuple(const char *args, char *outputArgs) {
    char *start = strstr(args, "tuple=");
    if (start) {
        start += 6; //Skip past tuple=
        char *end = strchr(start, ' ');
        if (end) {
            size_t length = end - start;
            strncpy(outputArgs, start, length);
            outputArgs[length] = '\0'; // Null-terminates extracted tuple
            } else {
                strncpy(outputArgs, "Unknown tuple", 255);
            }
    } else {
        strncpy(outputArgs, args, 255); //If no tuple use entire fd string? -> may want to remove
    }
}

// returns the FD as a int (aka fd=13<...>)
int formatFD(char *fdString) {
    if(strcmp(fdString, "<NA>") == 0){
        return -1;
    }
    long int output;
    output = strtol(fdString, NULL, 10);
    return output;
}

void parseLine(char line[], char *FD, char *syscall, char *args, char *ret, char *PID)
{
    // ignore timestamp, Information and process name (%*s)
    // store the file desc. (the number), syscall name, arugment string, all return values, and the PID string!!
    sscanf(line, "%*s %*s %*s FD:%[^,]  Syscall:%s Args:%[^,] Return:%[^,] PID:%[^\n]", FD, syscall, args, ret, PID);
    return;
}

bool parseSyscall(char syscall[], char returnValues[], char arguments[], char FD[]){

	if(strcmp(syscall, "rt_sigaction") == 0 || strcmp(syscall, "rt_sigprocmask") == 0 || strcmp(syscall, "brk") == 0 || strcmp(syscall, "munmap") == 0)
	{
		return false;
	}
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
	else if(strcmp(syscall, "mmap") == 0 && strcmp(FD, "<NA>") == 0)
	{
		return false;
	}
	else if(strcmp(syscall, "close") == 0 && (strcmp(returnValues, "0 ") == 0 || strcmp(arguments, "") == 0))
	{
		return false;
	}
    else if(strcmp(syscall, "access") == 0 && strcmp(arguments, "mode=0") == 0)
    {
		return false;
	}
    else if(strcmp(syscall, "accept4") == 0 && strcmp(returnValues, "<NA>") == 0 )
    {
        return false;    
    }
	// This is a system call that HAS information...
	else
	{
		return true;
	}
}


main(){
    FILE *file = fopen("events.txt", "r");
    if (!file) {
        perror("Failed to open events file");
        return 1;
    }

    char line[1024];
    bool logging = false;

    //GET THE FULL LINE OF information...
    while (fgets(line, sizeof(line), file)) {
        
        //store the arguments from the line
        char fdString[4], syscall[64], args[512], ret[5], PID[5];
        parseLine(line, fdString, syscall, args, ret, PID);

        //make the FD an int
        int FD = formatFD(fdString);

        // if we have a file interacted with
        if(FD != -1){
            // if it is accept4
            if(strcmp(syscall, "accept4") == 0) 
            {
                logging = true; //start logging -> we may be able to set this to just graphNum != 0
                parseNetworkTuple(args, args);
                makeSubgraph(FD, args);
                graphNum +=1;
                continue;
            } 
            // if it is any other syscall
            else
            {
                if(logging){
                    for(int i = 0; i < length(graphs)-1; i++) {
                        if(graphs[i]->currentfd == FD){
                            addEdge();
                            
                        }
                    }
                }
            }
        }

    }

}