#include "graph.h"
 
W3PiGraph mygraph;

#if defined(__AIESIM__) || defined(__X86SIM__)

int main(void) 
{
    int N_iter = 7;

    mygraph.init();
    mygraph.run(N_iter);
    mygraph.end();

    return 0;
}

#endif