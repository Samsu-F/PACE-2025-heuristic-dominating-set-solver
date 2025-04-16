#include <stdlib.h>
#include <stdio.h>

#include "graph.h"
#include "reduction.h"



int main(void)
{
    // just some tests so far
    Graph* g = graph_parse_stdin();
    if(!g) {
        exit(1);
    }
    // Vertex* v = g->vertices;
    // for(; v != NULL && v->id != 5; v = v->list_next) {
    // }
    // if(v != NULL) {
    //     fix_and_remove_vertex(g, v);
    // }
    fprintf(stderr, "fixed %zu vertices\n", fix_isol_and_supp_vertices(g));
    graph_print_as_dot(g, false, "Test");
    graph_free(g);
}
