#include "graph.h"
 
W3PiGraph mygraph;

int main(void) 
{
    int N_iter = 1;

    mygraph.init();
    mygraph.run(N_iter);
    mygraph.end();

    return 0;
}