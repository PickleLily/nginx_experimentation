//#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "graphMaker.h"
#define MAX_CONTEXT 20
#define NUM_SYSCALLS 373

int findNum(char *x){
    int min = 0;
    int max = NUM_SYSCALLS-1;
    while(min <= max){
        int mid = min + (max - min) / 2;
        //Calculate if our syscall is smaller or greater than middle syscall
        int eval = strcmp(x, syscallNames[mid]);

        //See if current string is correct
        if(eval == 0){
            return syscallNumbers[mid];
        }
        //Binary search
        //If our syscall is smaller we want to look at left side
        if(eval < 0){
            max = mid - 1;
        }
        //If our syscall is larger we want to look at right side
        else {
            min = mid + 1;
        }
    }
    //On error  
    return -1; 
}
typedef struct Node {
    int sysCallNum; //The system call number for this specific call
    struct Node* parent; //Points backwards to last call
    struct Node** children; //Points to an array of all potential following syscalls
    size_t numChildren; //The number of following syscalls
} Node;

Node* makeNode(int syscall) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->sysCallNum = syscall;
    newNode->parent = NULL;
    newNode->children = NULL;
    newNode->numChildren = 0;
    return newNode;
}

void addNode(Node* parent, Node* child){
    if(parent == NULL || child == NULL){return;}
    parent->numChildren++; //Increment number of following nodes for parent
    //Realloc (free and malloc) the old pointer to pointers array with updated size
    parent->children = (Node**)realloc(parent->children, parent->numChildren * sizeof(Node>
    parent->children[parent->numChildren - 1] = child; //Adds the child node to the end of>
    child->parent = parent;
}

typedef struct Node {
    int sysCallNum; //The system call number for this specific call
    struct Node* parent; //Points backwards to last call
    struct Node** children; //Points to an array of all potential following syscalls
    size_t numChildren; //The number of following syscalls
} Node;

Node* makeNode(int syscall) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->sysCallNum = syscall;
    newNode->parent = NULL;
    newNode->children = NULL;
    newNode->numChildren = 0;
    return newNode;
}

void addNode(Node* parent, Node* child){
    if(parent == NULL || child == NULL){return;}
    parent->numChildren++; //Increment number of following nodes for parent
    //Realloc (free and malloc) the old pointer to pointers array with updated size
    parent->children = (Node**)realloc(parent->children, parent->numChildren * sizeof(Node>
    parent->children[parent->numChildren - 1] = child; //Adds the child node to the end of>
    child->parent = parent;
}

Node* setCurrentNode(int syscall, Node* current){
    Node* temp = current;
    //Check if up
    while(temp != NULL){
        //In the occassion the node we are looking at is the syscall
        //Return that node
        if(temp->sysCallNum == syscall){
            return temp;
        } else {
            //Check if child
            for(size_t i = 0; i < current->numChildren; i++){
                if(current->children[i]->sysCallNum == syscall){
                    return current->children[i];
                }
            }
        temp = temp->parent;
        }
    }
    //If nothing is found return the same node we started on
    return current;
}


void printGraph(FILE* output, Node* current, int layer, int i){
    for(int i = 0; i < layer; i++){
        fprintf(output, "    ");
    }

    fprintf(output, "%d\n", current->sysCallNum);
    for(size_t i = 0; i < current->numChildren; i++){
        printGraph(output, current->children[i], layer + 1, i+1);
    }
}

bool compareNodes(Node* one, Node* two){
    if(one == NULL || two == NULL){
        return false;
    }

    return(one->sysCallNum == two->sysCallNum && one->parent == two->parent);
}

int main(){



    FILE *fptr;
    FILE *fptr2;
    fptr = fopen("parserOutput.txt", "r");
    fptr2 = fopen("graphOutput.txt", "w");
    char *currentLine;

    Node* root = NULL;
    Node* current = NULL;

    if(fptr == NULL || fptr2 == NULL){
        printf("Error, could not open output");
        exit(1);
    }
    
     currentLine = malloc(MAX_CONTEXT * sizeof(char));
    while(fgets(currentLine, MAX_CONTEXT-1, fptr) != NULL){
        //Temp fix for \n
        char *temp = strstr(currentLine, "\n");
        (*temp) = '\0';

        //Get current num
        int currentSysNum = findNum(currentLine);
        if(currentSysNum == -1){
            printf("%s, %d\n", currentLine, findNum(currentLine));
        } else {

            //Fill root node
            if(root == NULL){
                root = makeNode(currentSysNum);
                current = root;
            }
        //every node after root
            else {
                if(currentSysNum != current->sysCallNum){
                    if(setCurrentNode(currentSysNum, current) != current){
                        current = setCurrentNode(currentSysNum, current);
                    }
                    else {
                        Node* newNode = makeNode(currentSysNum);
                        addNode(current, newNode);
                        current = newNode;
                    }
                }
            }
        }
    }
    printGraph(fptr2, root, 0, 1);
}


    currentLine = malloc(MAX_CONTEXT * sizeof(char));

