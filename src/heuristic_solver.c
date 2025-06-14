#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <string.h>

#include "graph.h"
#include "reduction.h"
#include "greedy.h"
#include "debug_log.h"



static void _print_solution(Graph* g, size_t ds_size)
{
    assert(g != NULL && g->vertices != NULL && g->fixed.vertices != NULL);
    printf("%zu\n", g->fixed.size + ds_size);
    for(size_t fixed_idx = 0; fixed_idx < g->fixed.size; fixed_idx++) {
        Vertex* v = g->fixed.vertices[fixed_idx];
        printf("%" PRIu32 "\n", v->id);
    }
#ifndef NDEBUG
    size_t ds_vertices_found_in_g = 0; // this variable is just for an assertion
#endif
    for(size_t i_vertices = 0; i_vertices < g->n; i_vertices++) {
        Vertex* v = g->vertices[i_vertices];
        if(v->is_in_ds) {
            printf("%" PRIu32 "\n", v->id);
#ifndef NDEBUG
            ds_vertices_found_in_g++;
#endif
        }
    }
    fflush(stdout);
    assert(ds_vertices_found_in_g == ds_size);
}



int main(void)
{
    Graph* g = graph_parse(stdin);
    if(!g) {
        exit(EXIT_FAILURE);
    }

    debug_log("starting reduction with g->n == %" PRIu32 ", g->m == %" PRIu32 "\n", g->n, g->m);
    reduce(g, 13.0, 7.5); // 7.5 seconds to try all reduction rules including rule 2, then 5.5 more seconds to try rule 1 reductions
    debug_log("finished reduction with g->n == %" PRIu32 ", g->m == %" PRIu32 ", g->fixed.size == %zu\n",
              g->n, g->m, g->fixed.size);

    if(g->n <= 3) {
        if(g->n != 0) { // although extremely unlikely, it is possible that the whole graph can be reduced but the time budget ran out just before the last reduction step
            reduce(g, 1.0, 1.0);
        }
        _print_solution(g, 0);
        graph_free(g);
        return EXIT_SUCCESS;
    }

    size_t ds_size = iterated_greedy_solver(g);
    _print_solution(g, ds_size);
    graph_free(g);
    return EXIT_SUCCESS;
}
