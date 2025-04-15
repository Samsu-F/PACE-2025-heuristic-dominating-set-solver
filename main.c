#include <stdlib.h>

#include "graph.h"



int main(void)
{
    Graph* g = graph_parse_stdin();
    if(!g) {
        exit(1);
    }
    graph_print_as_dot(g, "InputGraph");
}
