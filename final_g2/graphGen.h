// graphGen.h
#ifndef GRAPHGEN_H
#define GRAPHGEN_H

// Structs 
// Structures for Nodes and Edges
typedef struct Node {
    char PID[64];         // PID
    char args[256];       // Name of the object (e.g., file path, socket info, subprocesses info?)
    int fd;               // The file descriptor
    // int graphNum;         //Unique subgraph
    char shape[128];
    int nodeID;
} Node;

typedef struct Edge {
    int from;       // name of the source node 
    int to;         // name of the destination node
    int graphNum;         //Unique subgraph
    char syscall[64];     // The system call connecting the nodes
    char edgeType[128];   //"dashed", "dotted" ,"solid", "invis", "bold"
} Edge;

// Subgraph representation + functions
typedef struct Subgraph {
    int graphNum;           //Unique subgraph number
    int currentfd;          //fd of the root accept4 node - not sure if we even need - shpuld this be current fd?
    int masterPID_ID;       //the Node ID of the process that starts interactions?
    Node* nodes[100];       // Hardcoded here
    Edge* edges[1000];      // Hardcoded here
    int node_count;
    int edge_count;
} Subgraph;

int getSubgraphFD(int currentFD);
Subgraph* initialize_subgraph(int fd, char *PID);
void makeSubgraph(int fd, char *socketTuple, char *PID);

// Functions
void add_edge(int from, int to, const char *syscall);
void update_edge(int edge, char *newcall);
int find_or_add_node(int fileDescriptor, const char *args, char PID[], char shape[]);
int getSubgraphFD(int currentFD);
int getNodeFD(int currentFD);
void printOutput();
int formatFD(char *fdString);
void parseArgs(const char *args, char *output);
void parseLine(char line[], char *FD, char *syscall, char *args, char *ret, char *PID);
bool parseSyscall(char syscall[], char returnValues[], char arguments[], char FD[]);
void printSubgraphMetadata();
void createDOT(char* setting);


#endif /* GRAPHGEN_H */
