#include <stdlib.h>

int main(){
    system("dot -Tpng graph1a.dot -o graph1a.png");
    system("dot -Tpng graph1b.dot -o graph1b.png");
    system("dot -Tpng graph2.dot -o graph2.png");
}