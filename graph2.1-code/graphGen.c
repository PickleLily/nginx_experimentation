#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SUBNODES 100
#define MAX_SUBEDGES 1000
#define MAX_SUBGRAPHS 100

// Structures for Nodes and Edges
typedef struct Node {
    char name[256]; // Name of the object (e.g., file path, socket info, subprocesses info?)
    int fd; // The file descriptor
    int graphNum; //Unique subgraph
    char shape[128];
} Node;

typedef struct Edge {
    char from[256];       // name of the source node 
    char to[256];         // name of the destination node
    int graphNum; //Unique subgraph
    char syscall[64]; // The system call connecting the nodes
} Edge;

// Subgraph representation
typedef struct Subgraph {
    int graphNum; //Unique subgraph number
    int currentfd; //fd of the root accept4 node - not sure if we even need - shpuld this be current fd?
    Node* nodes[MAX_SUBNODES];
    Edge* edges[MAX_SUBEDGES];
    int node_count;
    int edge_count;
} Subgraph;

// Global graph ID
int graphNum = 0;

// Global graph references
Subgraph* graphs[MAX_SUBGRAPHS];

// When supplied with fd of accept4 call, make new split graph
void makeSubgraph(int fd, char *socketTuple) {
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
    Subgraph* subgraph = (Subgraph*)malloc(sizeof(Subgraph));
    subgraph->graphNum = graphNum;
    subgraph->currentfd = fd;
    subgraph->node_count = 0;
    subgraph->edge_count = 0;

    // Make remote node pointer
    Node* remote = (Node*)malloc(sizeof(Node));
    strncpy(remote->name, socket1, sizeof(socket1)-1);
    remote->fd = fd;
    remote->graphNum = graphNum;
    strncpy(remote->shape, "diamond", 7);
    subgraph->nodes[subgraph->node_count] = remote;
    subgraph->node_count++;

    // Make local node
    Node* local = (Node*)malloc(sizeof(Node));
    strncpy(local->name, socket2, sizeof(socket2)-1);
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

/**
 * We don't need to worry about overwriting/seeing the same pattern multiple times, 
 * as we should document each time?
 * 
 * If we see multiple actions of openat.... we are going to log each one independently
 * We also only care about syscalls with fd's (for now)
 * Going to need cast for fd
 */
// void addNode(Subgraph graph, char *name, int fd, char *shape) {
//     // In occassion node exists terminate
//     for (int i = 0; i < graph.node_count; i++) {
//         if (strcmp(graph.nodes[i].name, name) == 0) {
//             return;
//         }
//     }

//     // Check if max nodes has been reached
//     if (graph.node_count >= MAX_SUBNODES) {
//         fprintf(stderr, "Error: Maximum subgraph nodes exceeded.\n");
//         exit(1);
//     }

//     // Add node *if necessary

//     graph.nodes[graph.node_count].fd = .graph.node_count;
//     strncpy(nodes[node_count].name, name, sizeof(nodes[node_count].name) - 1);
//     return nodes[node_count++].id;
// }

// // Function to add an edge
// void add_edge(int from, int to, const char *syscall) {
//     for (int i = 0; i < edge_count; i++) {
//         if (edges[i].from == from && edges[i].to == to &&
//             strcmp(edges[i].syscall, syscall) == 0) {
//             // Duplicate edge found, do not add
//             return;
//         }
//     }	
//     if (edge_count >= MAX_EDGES) {
//         fprintf(stderr, "Error: Maximum edges exceeded.\n");
//         exit(1);
//     }
//     edges[edge_count].from = from;
//     edges[edge_count].to = to;
//     strncpy(edges[edge_count].syscall, syscall, sizeof(edges[edge_count].syscall) - 1);
//     edge_count++;
// }

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

int formatFD(char *fdString) {
    if(strcmp(fdString, "<NA>") == 0){
        return -1;
    }
    long int output;
    output = strtol(fdString, NULL, 10);
    return output;
}

// Main function to parse Falco output and build the graph
int main() {
    FILE *file = fopen("basicOut.txt", "r");
    if (!file) {
        perror("Failed to open events file");
        return 1;
    }

    // Parse each line into respective subparts
    char line[1024];
    while (fgets(line, sizeof(line), file)) {
        int  localExtFlag;
        char fdString[4], syscall[64], args[512], ret[5], pid[5];
        if (sscanf(line, "%*s %*s %*s FD:%[^ ] Syscall:%s Args:%[^,] Return:%[^ ] PID:%[^\n]", fdString, syscall, args, ret, pid) == 5){ // Syscall:%s Args:%(.*) Return:%[^ ]+ PID:%[^\n]", fd, syscall, args, ret, pid) != 5) {
            continue;
        }

        // Ignore all non-valid fd's by formatting fd field
        int fd = formatFD(fdString);

        //Every time we see "accept4", we assign it to new graph
        if(fd != -1 && strcmp(syscall, "accept4") == 0) {
            parseNetworkTuple(args, args);
            makeSubgraph(fd, args);
            // printf("%s->%s\n", graphs[0]->edges[0]->from, graphs[0]->edges[0]->to);
            graphNum +=1;
            continue;
        } //When we close a graph (accept4), we'll set current fd to -1

        // if file descriptor not null:
        // First identify which graph we're relating to?
        // Do we want to change the current fd or make a list of active fds?
        else if(fd != -1) {
            for(int i = 0; i < length(graphs)-1; i++) {
                if(graphs[i]->currentfd == fd){
                    addEdge();
                }
            }
        }
        // if it is equal to a network socket fd
        // will share an edge with network socket (local) -> subgraph->nodes[1]

        // if it is not equal to the network socket
        // we make new node that is triangle -> or rectangle
        // if (root_node_id == -1 && strcmp(syscall, "accept4") == 0) {
        //     char fd[256] = "Unknown FD";
        //     if (strstr(args, "fd=")) {
        //         sscanf(args, "fd=%255[^ ]", fd); // Extract fd field
        //         parse_file_descriptor(fd, fd);  // Extract file path from fd
        //     }
        //     root_node_id = find_or_add_node(fd, 1); // Use fd as root node
        //     continue;
        // }

    //     // Parse arguments for objects like files or sockets
    //     char object1[256] = "";
    //     char *path_start;
    //     if (path_start = strstr(args, "path=")) {
    //         sscanf(path_start, "path=%255s", object1); // Extract file path
    //     } else if (path_start = strstr(args, "fd=")) {
    //         char fd[256];
    //         sscanf(path_start, "fd=%255[^ ]", fd); // Extract fd field
    //         if (strstr(fd, "<f>")) {
    //             parse_file_descriptor(fd, object1); // Extract file path if "<f>"
    //         } else if ((strstr(fd, "<u>")) || (strstr(fd, "<4t>"))){
    //         localExtFlag = 1;
    //     strncpy(object1, fd, 255);
    //     }else {
    //             continue;
    //         }
    //     }

    //     // Add edges if relevant objects are found
    //     if (strlen(object1) > 0 && root_node_id != -1) {
    //         int to = find_or_add_node(object1, localExtFlag);
    //         add_edge(root_node_id, to, syscall);
    //     }
    // }

    // fclose(file);

    // // Output the graph in DOT format
    // FILE *dot_file = fopen("graph.dot", "w");
    // if (!dot_file) {
    //     perror("Failed to open DOT file");
    //     return 1;
    // }

    // fprintf(dot_file, "digraph nginx_syscalls {\n");
    // for (int i = 0; i < node_count; i++) {
    //     if (nodes[i].isExt) {
    //         fprintf(dot_file, "  %d [label=\"%s\", shape=diamond];\n", nodes[i].id, nodes[i].name);
    //     } else {
    //         fprintf(dot_file, "  %d [label=\"%s\"];\n", nodes[i].id, nodes[i].name);
    //     }
    // }
    // for (int i = 0; i < edge_count; i++) {
    //     fprintf(dot_file, "  %d -> %d [label=\"%s\"];\n",
    //             edges[i].from, edges[i].to, edges[i].syscall);
    // }
    // fprintf(dot_file, "}\n");

    // fclose(dot_file);
    }
    // printf("Graph exported to graph.dot\n");
    return 0;
}
