#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "graphGen.h"
// --------------------TODO Area---------------------------------------------------------
/*
    TODO --> the master PID is being set as the PID passed in... SHOULD we create one for each network tuple?
        Should we use the incoming IP addr. as our MASTER PID




*/
// --------------------SET UP---------------------------------------------------------
#define MAX_SUBNODES 100
#define MAX_SUBEDGES 1000
#define MAX_SUBGRAPHS 100

// Global graph ID
int graphNum = -1;

// Global graph Reference
Subgraph* graphs[MAX_SUBGRAPHS];

//global from & to in network tuple --> update this for each line we track
int globalFrom = 2; //default to be the Partent process ID (will always be 2)
int globalTo = -1;

// ---------------------Functions --------------------------------------------------------

void add_edge(int from, int to, const char *syscall) {
    //get current subgraph
    int subgraphID = graphNum;
    Subgraph* graph = graphs[graphNum];

    //check if edge exists
    for (int i = 0; i < graph->edge_count; i++) {
        // Ella's (AI GODS) tips: If ever pulling an exact value out of a pointer we need to derefernce
        // Now that these are also chars we dont need to worry about it
        if (graph->edges[i]->from == from && graph->edges[i]->to == to && strcmp(graph->edges[i]->syscall, syscall) == 0) {
            return; // Duplicate edge found, do not add
        }
    }	

    //check max edges
    if (graph->edge_count >= MAX_SUBEDGES) {
        fprintf(stderr, "Error: Maximum edges exceeded.\n");
        exit(1);
    }

    Edge* newEdge = (Edge*)malloc(sizeof(Edge));
    newEdge->from = from;
    newEdge->to = to;
    newEdge->graphNum = graphNum;
    strncpy(newEdge->syscall, syscall, strlen(syscall));
    strncpy(newEdge->edgeType, "solid", strlen("solid"));
    graph->edges[graph->edge_count] = newEdge;
    graph->edge_count++;
}

void update_edge(int edge, const char *newcall) {
    int subgraphID = graphNum;
    Subgraph* graph = graphs[graphNum];

    //Update edge (denoted by edgenum for now) with new syscall
    Edge* temp = graph->edges[edge];
    strncpy(temp->syscall, newcall, sizeof(newcall));
    strncpy(temp->edgeType, "solid", sizeof("solid"));
}

// TODO --> this is not secure...
int find_or_add_node(const char *args, char PID[], char shape[]) {  
    //get current subgraph
    int subgraphID = graphNum;
    Subgraph* graph = graphs[graphNum];
    
    int nodeCount = graph->node_count;

    for (int i = 0; i < nodeCount; i++) {  
        //if we see a matching argument break out of the loop...
        if (strcmp(graph->nodes[i]->args, args) == 0) {
            return graph->nodes[i]->fd;
        }
    }

    if (nodeCount >= MAX_SUBNODES) {
        fprintf(stderr, "Error: Maximum nodes exceeded.\n");
        exit(1);
    }

    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->graphNum = graphNum;
    newNode->nodeID = graphs[graphNum]->node_count;
    strncpy(newNode->args, args, sizeof(newNode->args) - 1);
    strncpy(newNode->PID, PID, sizeof(newNode->PID) - 1);
    strncpy(newNode->shape, "ellipse", sizeof("ellipse"));
    graph->nodes[graph->node_count] = newNode;
    graph->node_count++;  

    return newNode->nodeID;
}

int getSubgraphFD(int currentFD){
    //go through the list of graphs globally
    for(int i = 0; i < graphNum; i ++){
        Subgraph *g = graphs[i];

        //check the currentfd of each subgraph and return that graphs graphNUM
        if(currentFD == g->currentfd){
            return g->graphNum;
        }
    }
    return -1;
}

Subgraph* initialize_subgraph(int fd, char *PID){
    Subgraph* subgraph = (Subgraph*)malloc(sizeof(Subgraph));
    subgraph->graphNum = graphNum;
    subgraph->currentfd = fd;
    subgraph->node_count = 0;
    subgraph->edge_count = 0;
    subgraph->masterPID_ID = atoi(PID);
    return subgraph;
}

// When supplied with fd of accept4 call, make new split graph
void makeSubgraph(int fd, char *socketTuple, char *PID) {
    // Take global graph num, current fd of accept4, and arg information to build the Subgraph
    // Predefine the two sockets
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
    Subgraph* subgraph = initialize_subgraph(fd, PID);
    int currentGraphNum = graphNum;
    graphs[currentGraphNum] = subgraph;

    int numNodes = graphs[currentGraphNum]->node_count;
    int numEdges = graphs[currentGraphNum]->edge_count;

    // Make remote node pointer
    Node* remote = (Node*)malloc(sizeof(Node));
    strncpy(remote->args, socket1, sizeof(socket1)-1);
    remote->fd = fd;
    remote->graphNum = graphNum;
    strncpy(remote->shape, "diamond", sizeof("diamond"));
    subgraph->nodes[subgraph->node_count] = remote;
    remote->nodeID = graphs[graphNum]->node_count;
    subgraph->node_count++;

    // Make local node
    Node* local = (Node*)malloc(sizeof(Node));
    strncpy(local->args, socket2, sizeof(socket2)-1);
    local->fd = fd;
    local->graphNum = graphNum;
    strncpy(local->shape, "diamond", sizeof("diamond"));
    subgraph->nodes[subgraph->node_count] = local;
    local->nodeID = graphs[graphNum]->node_count;
    subgraph->node_count++;
    
    //  Connect two
    Edge* networkedge = (Edge*)malloc(sizeof(Edge));
    networkedge->from = remote->nodeID;
    networkedge->to = local->nodeID;

    networkedge->graphNum = graphNum;
    strncpy(networkedge->syscall, "accept4", sizeof("accept4"));
    subgraph->edges[subgraph->edge_count] = networkedge;
    strncpy(networkedge->edgeType, "solid", sizeof("solid"));
    subgraph->edge_count++;

    // Make PID node
    Node* pid = (Node*)malloc(sizeof(Node));
    strncpy(pid->args, PID, sizeof(PID));
    pid->fd = fd;
    strncpy(pid->shape, "rectangle", sizeof("rectangle"));
    subgraph->nodes[subgraph->node_count] = pid;
    pid->nodeID = subgraph->node_count;
    subgraph->node_count++;

    // Connect PID node
    Edge* pidedge = (Edge*)malloc(sizeof(Edge));
    pidedge->from = local->nodeID;
    pidedge->to = pid->nodeID;

    pidedge->graphNum = graphNum;
    strncpy(pidedge->syscall, "", 1);
    subgraph->edges[subgraph->edge_count] = pidedge;
    strncpy(pidedge->edgeType, "dashed", sizeof("dashed"));
    subgraph->edge_count++;

    // Add to global list
    graphs[graphNum] = subgraph;
}

void printOutput() {
    int i, j = 0;
    for(int i = 0 ; i < graphNum; i ++){

        for(int j = 0 ; j < graphs[i]->node_count; j ++){
            printf("node:%s\n", graphs[i]->nodes[j]->args);
        }
        for(int j = 0 ; j < graphs[i]->edge_count; j ++){
            printf("edge: %s from:%d to:%d\n",graphs[i]->edges[j]->syscall, graphs[i]->edges[j]->from, graphs[i]->edges[j]->to);
        }
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

// Helper method to parse arg information
void parseArgs(const char *args, char *output) {
    char *res = strstr(args, "res");
    if (res) {
        strncpy(output, "Unknown tuple", 255);
    } else {
        char *start = strstr(args, ">");
        if (start) {
            start += 1; //Skip past <f>=
            char *end = strchr(start, ')');
            if (end) {
                size_t length = end - start;
                strncpy(output, start, length);
                output[length] = '\0'; // Null-terminates extracted tuple
                } else {
                    strncpy(output, "Unknown tuple", 255);
                }
            } else {
                strncpy(output, "Unknown tuple", 255); //If no tuple use entire fd string? -> may want to remove
            }
    }
}

void parseLine(char line[], char *FD, char *syscall, char *args, char *ret, char *PID)
{
    // ignore timestamp, Information and process name (%*s)
    // store the file desc. (the number), syscall name, arugment string, all return values, and the PID string!!
    char time[64];
    char type[64];
    char program[64];
    sscanf(line, "%s %s Name:%s FD:%[^ ] Syscall:%[^ ] Args:%[^,], Return:%[^,], PID:%[^\n]", time, type, program, FD, syscall, args, ret, PID);
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

void printSubgraphMetadata(){
    printf("%d subgraphs created\n", graphNum);
    for(int i = 0; i < graphNum; i++){
        printf("graph %d Master PID: %d\n",i,graphs[graphNum]->masterPID_ID);
        printf("nodes: %d    edges: %d\n\n",graphs[graphNum]->node_count, graphs[graphNum]->edge_count);
    }
}

// Helper method to parse socket tuple
// TODO Have this make all graphs as subgraphs within same larger file
void createDOT(char* setting){

    //Delimited by setting
    if(strcmp("individual", setting) == 0){

        for(int i = 0; i <= graphNum; i++){ //for every subgraph
        
            // open new dot file with unique name
            char path[1024];
            sprintf(path, "./graphs/graph%d.dot", i);
            printf("%s", path);
            FILE *dot_file = fopen(path, "w");
    
            if (!dot_file) {
                perror("Failed to open DOT file\n");
                return;
            }
    
            //print the setup info:
            fprintf(dot_file, "digraph nginx_syscalls {\n");
    
            // add all of the nodes
            
            for(int j = 0; j < graphs[i]->node_count; j++){
                Node* n = graphs[i]->nodes[j];
                fprintf(dot_file, "  %d [label=\"%s\" shape=%s];\n", j, graphs[i]->nodes[j]->args, graphs[i]->nodes[j]->shape);
            }
    
            Edge** e = graphs[i]->edges;
            // add all of the edges
            for(int j = 0; j < graphs[i]->edge_count; j++){
                fprintf(dot_file, "  %d -> %d [label=\"%s\"];\n", graphs[i]->edges[j]->from, graphs[i]->edges[j]->to, graphs[i]->edges[j]->syscall);
            }
    
            //print the shutdown info:
            fprintf(dot_file, "}\n");
            fclose(dot_file);
            printf("Graph exported to %s\n", path);
    
        }

    } else if(strcmp("together", setting) == 0) {
        // Open dot file
        // open new dot file with unique name - generated randomly
        int randomVal = rand();
        char path[1024];
        sprintf(path, "./graphs/graph-%d.dot",randomVal);
        printf("Created graph %s", path);
        FILE *dot_file = fopen(path, "w");

        if (!dot_file) {
            perror("Failed to open DOT file\n");
            return;
        }

        // After new dotfile has been successfully made:
        //print the setup info:
        fprintf(dot_file, "digraph nginx_syscalls {\n");

        for(int i = 0; i <= graphNum; i++){ //for every subgraph

            // Init subgraph
            fprintf(dot_file, "subgraph cluster_%d {\n", i);
        
            // add all of the nodes
            for(int j = 0; j < graphs[i]->node_count; j++){
                Node* n = graphs[i]->nodes[j];
                fprintf(dot_file, "  %d%d [label=\"%s\" shape=%s];\n", j, i, graphs[i]->nodes[j]->args, graphs[i]->nodes[j]->shape);
            }
    
            Edge** e = graphs[i]->edges;
            // add all of the edges
            for(int j = 0; j < graphs[i]->edge_count; j++){
                fprintf(dot_file, "  %d%d -> %d%d [label=\"%s\" style=%s];\n", graphs[i]->edges[j]->from, i, graphs[i]->edges[j]->to, i, graphs[i]->edges[j]->syscall, graphs[i]->edges[j]->edgeType);
            }

            // Close subgraph
            fprintf(dot_file, "}\n");
            // Insert another enter for readability
            fprintf(dot_file, "\n");
        }

        fprintf(dot_file, "}\n");
    } else if(strcmp("overlaid", setting) == 0) {
        // Open dot file
        // open new dot file with unique name - generated randomly
        int randomVal = rand();
        char path[1024];
        sprintf(path, "./graphs/graph-%d.dot",randomVal);
        printf("Created graph %s", path);
        FILE *dot_file = fopen(path, "w");

        if (!dot_file) {
            perror("Failed to open DOT file\n");
            return;
        }

        // After new dotfile has been successfully made:
        //print the setup info:
        fprintf(dot_file, "digraph nginx_syscalls {\n");

        for(int i = 0; i <= graphNum; i++){ //for every subgraph

            // Init subgraph
            fprintf(dot_file, "subgraph cluster_%d {\n", i);
        
            // add all of the nodes
            for(int j = 0; j < graphs[i]->node_count; j++){
                Node* n = graphs[i]->nodes[j];
                fprintf(dot_file, "  %d [label=\"%s\" shape=%s];\n", j, graphs[i]->nodes[j]->args, graphs[i]->nodes[j]->shape);
            }
    
            Edge** e = graphs[i]->edges;
            // add all of the edges
            for(int j = 0; j < graphs[i]->edge_count; j++){
                fprintf(dot_file, "  %d -> %d [label=\"%s\" style=%s];\n", graphs[i]->edges[j]->from, graphs[i]->edges[j]->to, graphs[i]->edges[j]->syscall, graphs[i]->edges[j]->edgeType);
            }

            // Close subgraph
            fprintf(dot_file, "}\n");
            // Insert another enter for readability
            fprintf(dot_file, "\n");
        }

        fprintf(dot_file, "}\n");
    } else {
        createDOT("individual");
    }
}

void parseNetworkTuple(const char *arguments, char *from, char *to){
    //look for the end of the arrow signifying a connection between two IP's
    char *start = strstr(arguments, ">");
    if(start){
        int socket2start = start + 1;
        int socket2end = length(&arguments);

        int socket1start = 0;
        int socket1end = start - 1;
        strncpy(from, arguments[socket1start]);
        strncpy(to);
    }
    return;
}


int main(){

    FILE *file = fopen("events.txt", "r");
    if (!file) {
        perror("Failed to open events file");
        return 1;
    }

    char line[1024];
    int number_of_subgraph_nodes = 0;

    //GET THE FULL LINE OF information...
    while (fgets(line, sizeof(line), file)) {
        
        //store the arguments from the line
        char fdString[4], syscall[64], args[1024], ret[64], PID[64];
        parseLine(line, fdString, syscall, args, ret, PID);

        //make the FD an int
        int FD = formatFD(fdString);

        // if we have a file interacted with
        if(FD != -1){
            parseArgs(args, args);

            // if it is accept4
            if(strcmp(syscall, "accept4") == 0) 
            {
                graphNum +=1;                
                makeSubgraph(FD, args, PID);
                number_of_subgraph_nodes = 0;
            } 
            // if it is any other syscall
            else
            {
                if(graphNum >= 0){
                    // Increment nodes
                    number_of_subgraph_nodes+=1;

                    if(strcmp("Unknown tuple", args) != 0) {
                        if(strcmp("recvfrom", syscall) == 0) {
                            update_edge(1, syscall);
                        } else {
                            char from[128];
                            char to[128];
                            parseNetworkTuple(args, from, to);
                            int destinationNode = find_or_add_node(to, PID, "ellipse");
                            int originNode = find_or_add_node(from, PID, "ellipse");

                                add_edge(originNode, destinationNode, syscall);
                        }
                    }
                }
            }
        }
    }
    printSubgraphMetadata();
    createDOT("together");
}