// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>

// #define MAX_NODES 1000
// #define MAX_EDGES 2000

// Structures for Nodes and Edges
// typedef struct Node {
//     char PID[64]; // PID
//     char args[1024]; // Name of the object (e.g., file path, socket info)
//     int id; // Unique identifier
// } Node;

// typedef struct Edge {
//     int from;       // ID of the source node
//     int to;         // ID of the destination node
//     char syscall[64]; // The system call connecting the nodes
// } Edge;


// TODO --> is that okay??? ??
// // Global graph representation
// Node nodes[MAX_NODES];
// Edge edges[MAX_EDGES];
// int node_count = 0, edge_count = 0;

// // Global root node ID
// int root_node_id = -1;


// Function to find or add a node
int find_or_add_node(const char *args, char PID[], int *node_count) {  
    for (int i = 0; i < *node_count; i++) {  
        //if we see a matching argument break out of the loop...
        if (strcmp(nodes[i].args, args) == 0) {
            return nodes[i].id;
        }
    }

    if (*node_count >= MAX_NODES) {
        fprintf(stderr, "Error: Maximum nodes exceeded.\n");
        exit(1);
    }

    nodes[*node_count].id = *node_count;
    strncpy(nodes[*node_count].PID, PID, sizeof(nodes[*node_count].PID) - 1);
    strncpy(nodes[*node_count].args, args, sizeof(nodes[*node_count].args) - 1);
    *node_count = *node_count + 1;
    return nodes[*node_count].id;
}


// Function to add an edge
void add_edge(int from, int to, const char *syscall) 
    for (int i = 0; i < edge_count; i++) {
        if (edges[i].from == from && edges[i].to == to && strcmp(edges[i].syscall, syscall) == 0) {
            return; // Duplicate edge found, do not add
        }
    }	
    if (edge_count >= MAX_EDGES) {
        fprintf(stderr, "Error: Maximum edges exceeded.\n");
        exit(1);
    }
    edges[edge_count].from = from;
    edges[edge_count].to = to;
    strncpy(edges[edge_count].syscall, syscall, sizeof(edges[edge_count].syscall) - 1);
    edge_count++;
}

// // Helper function to parse file descriptors with "<f>"
// void parse_file_descriptor(const char *fd, char *file_path) {
//     char *start = strstr(fd, "<f>");
//     if (start) {
//         start += 3; // Skip past "<f>"
//         char *end = strchr(start, ')');
//         if (end) {
//             size_t length = end - start;
//             strncpy(file_path, start, length);
//             file_path[length] = '\0'; // Null-terminate the extracted path
//         } else {
//             strncpy(file_path, "Unknown File", 255);
//         }
//     } else {
//         strncpy(file_path, fd, 255); // If no "<f>", use the entire fd string
//     }
// }

// Main function to parse Falco output and build the graph
int main() {
    FILE *file = fopen("intermediateOutput.txt", "r");
    if (!file) {
        perror("Failed to open events file");
        return 1;
    }

    char line[1024];
    int from_node = -1;
    while (fgets(line, sizeof(line), file)) {

        char syscall[1024] = "", args[1024] = "", PID[1024] = "", filepath[1024] = "";
        sscanf(line, "%s %s %[^\n]", syscall, PID, args);

        if (strstr(args, "fd=")) 
        {
            sscanf(args, "fd=%255[^ ]", filepath);
            parse_file_descriptor(filepath, filepath);  // Extract file path from fd
        }
        else if (strstr(args, "name="))
        {
            sscanf(args, "name=[^ ]", filepath); 
        }
        else if(strstr(args, "path="))
        {
            sscanf(args, "path=[^ ]", filepath);
        }


        int to_node = find_or_add_node(filepath, PID, &node_count);

        // this means that if NO nodes have been added we will not add an edge... only should occur 1x
        if(from_node != -1){
            add_edge(from_node, to_node, syscall);
        }
        // // will not make edge without 2 nodes OR edges that self reference
        // if(from_node != -1 && from_node != to_node){
        //     add_edge(from_node, to_node, syscall);
        // }
        from_node = to_node;
    }

    fclose(file);
    FILE *dot_file = fopen("../graphs/graph2b.dot", "w");
    if (!dot_file) {
        perror("Failed to open DOT file\n");
        return 1;
    }
    
    fprintf(dot_file, "digraph nginx_syscalls {\n");

    for (int i = 0; i < node_count; i++) {
        fprintf(dot_file, "  %d [label=\"PID:%s %s\"];\n", nodes[i].id, nodes[i].PID, nodes[i].args);
    }

    for (int i = 0; i < edge_count; i++) {
        fprintf(dot_file, "  %d -> %d [label=\"%s\"];\n", edges[i].from, edges[i].to, edges[i].syscall);
    }

    fprintf(dot_file, "}\n");
    fclose(dot_file);
    printf("Graph exported to graphs/graph2.dot\n");
    return 0;
}