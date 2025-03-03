#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "syscalls.h"
#define MAX_CONTEXT 20
#define NUM_SYSCALLS 373

// int findnum(const char *x){
//     for(int i = 0; i < NUM_SYSCALLS; i++)
//         if(!strcmp(x, numtosys[i]))
//             return i;
//     return -1;
// }

int findNum(const char *x){
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

int main(int argc, char** argv){
    //open the file of syscalls
    // int inCalls[NUM_SYSCALLS][MAX_CONTEXT];
    // int outCalls[NUM_SYSCALLS][MAX_CONTEXT];
    // memset(inCalls, -1, sizeof inCalls);
    // memset(outCalls, -1, sizeof outCalls);
    int adjmat[NUM_SYSCALLS][NUM_SYSCALLS];
    memset(adjmat, 0, sizeof(adjmat));
    int acceptYet = 0;
    int prevCall = -1, curCall = -1, nextCall = -1;
    char callChar[32];
    memset(callChar, 0, sizeof(callChar));
    FILE *graph = fopen("graph.dot", "w");
    fprintf(graph, "digraph graphname {\n");
    for(int j = 1; j < argc; j++){
        FILE *fp = fopen(argv[j], "r");
        while(fscanf(fp, "%s\n", callChar) != EOF){
            int num = findNum(callChar);
            if(num == -1){
                printf("not a syscall: %s", callChar);
                continue;
            }
            else if(!acceptYet && (num == 43 || num == 288))
                acceptYet = num;
            if(acceptYet)
                nextCall = num;
            // printf("%i ", adjmat[curCall][nextCall]);
            if(curCall != -1 && !adjmat[curCall][nextCall]){
                fprintf(graph, "%s -> %s;\n", numtosys[curCall], numtosys[nextCall]);
                adjmat[curCall][nextCall] = 1;
            }
                // for(int i = 0; i < MAX_CONTEXT; i++){
                //     if(inCalls[curCall][i] == prevCall && outCalls[curCall][i] == nextCall)
                //         break;
                //     if(inCalls[curCall][i] == -1){
                //         inCalls[curCall][i] = prevCall;
                //         outCalls[curCall][i] = nextCall;
                //         contextFile[curCall][i] = j;
                //         break;
                //     }
                //     if(i == MAX_CONTEXT - 1)
                //         exit(43);
                // }
            // prevCall = curCall;
            curCall = nextCall;
        }
    }
    fprintf(graph, "}");
    fclose(graph);
}