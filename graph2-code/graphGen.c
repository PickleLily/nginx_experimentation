#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_NODES 1000
#define MAX_EDGES 2000

// Structures for Nodes and Edges
typedef struct Node {
    char name[256]; // Name of the object (e.g., file path, socket info)
    int id; // Unique identifier
    int isExt;
} Node;

typedef struct Edge {
    int from;       // ID of the source node
    int to;         // ID of the destination node
    char syscall[64]; // The system call connecting the nodes
} Edge;

// Global graph representation
Node nodes[MAX_NODES];
Edge edges[MAX_EDGES];
int node_count = 0, edge_count = 0;

// Global root node ID
int root_node_id = -1;

// Function to find or add a node
int find_or_add_node(const char *name, int is_external) {
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
    nodes[node_count].isExt = is_external;
    strncpy(nodes[node_count].name, name, sizeof(nodes[node_count].name) - 1);
    return nodes[node_count++].id;
}

// Function to add an edge
void add_edge(int from, int to, const char *syscall) {
    for (int i = 0; i < edge_count; i++) {
        if (edges[i].from == from && edges[i].to == to &&
            strcmp(edges[i].syscall, syscall) == 0) {
            // Duplicate edge found, do not add
            return;
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

// Helper function to parse file descriptors with "<f>"
void parse_file_descriptor(const char *fd, char *file_path) {
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

// Main function to parse Falco output and build the graph
int main() {
    FILE *file = fopen("truncData.txt", "r");
    if (!file) {
        perror("Failed to open events file");
        return 1;
    }

    char line[1024];
    while (fgets(line, sizeof(line), file)) {
        char syscall[64], args[512];
	int localExtFlag = 0;
        if (sscanf(line, "%*s %*s %*s Syscall=%s Args=%[^\n]", syscall, args) != 2) {
            continue;
        }

        // Extract root node from the first accept4 syscall
        if (root_node_id == -1 && strcmp(syscall, "accept4") == 0) {
            char fd[256] = "Unknown FD";
            if (strstr(args, "fd=")) {
                sscanf(args, "fd=%255[^ ]", fd); // Extract fd field
                parse_file_descriptor(fd, fd);  // Extract file path from fd
            }
            root_node_id = find_or_add_node(fd, 1); // Use fd as root node
            continue;
        }

        // Parse arguments for objects like files or sockets
        char object1[256] = "";
	char *path_start;
        if (path_start = strstr(args, "path=")) {
            sscanf(path_start, "path=%255s", object1); // Extract file path
        } else if (path_start = strstr(args, "fd=")) {
            char fd[256];
            sscanf(path_start, "fd=%255[^ ]", fd); // Extract fd field
            if (strstr(fd, "<f>")) {
                parse_file_descriptor(fd, object1); // Extract file path if "<f>"
            } else if ((strstr(fd, "<u>")) || (strstr(fd, "<4t>"))){
	        localExtFlag = 1;
		strncpy(object1, fd, 255);
	    }else {
                continue;
            }
        }

        // Add edges if relevant objects are found
        if (strlen(object1) > 0 && root_node_id != -1) {
            int to = find_or_add_node(object1, localExtFlag);
            add_edge(root_node_id, to, syscall);
        }
    }

    fclose(file);

    // Output the graph in DOT format
    FILE *dot_file = fopen("../graphs/graph2.dot", "w");
    if (!dot_file) {
        perror("Failed to open DOT file");
        return 1;
    }

    fprintf(dot_file, "digraph nginx_syscalls {\n");
    for (int i = 0; i < node_count; i++) {
        if (nodes[i].isExt) {
            fprintf(dot_file, "  %d [label=\"%s\", shape=diamond];\n", nodes[i].id, nodes[i].name);
        } else {
            fprintf(dot_file, "  %d [label=\"%s\"];\n", nodes[i].id, nodes[i].name);
        }
    }
    for (int i = 0; i < edge_count; i++) {
        fprintf(dot_file, "  %d -> %d [label=\"%s\"];\n",
                edges[i].from, edges[i].to, edges[i].syscall);
    }
    fprintf(dot_file, "}\n");

    fclose(dot_file);
    printf("Graph exported to graphs/graph2.dot\n");
    return 0;
}
