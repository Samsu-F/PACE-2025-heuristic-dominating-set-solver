#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "graph.h"
#include "reduction.h"



int main(void)
{
    // just some tests so far

    clock_t time_parsing_start = clock();
    Graph* g = graph_parse_stdin();
    clock_t time_parsing_end = clock();
    if(!g) {
        exit(1);
    }

    fprintf(stderr, "  remaining: n = %zu\tm = %zu\n", g->n, g->m);
    clock_t time_reduction_start = clock();

    // fprintf(stderr, "simple_reduce: count_fixed = %zu\n", fix_isol_and_supp_vertices(g));
    rule_1_reduce(g);

    clock_t time_reduction_end = clock();
    fprintf(stderr, "  remaining: n = %zu\tm = %zu\n", g->n, g->m);

    graph_print_as_dot(g, true, "Test");
    graph_free(g);


    float ms_parse_input = (float)(1000 * (time_parsing_end - time_parsing_start)) / CLOCKS_PER_SEC;
    float ms_reduce = (float)(1000 * (time_reduction_end - time_reduction_start)) / CLOCKS_PER_SEC;
    fprintf(stderr,
            "\n\n---\n"
            "parse input:    %7.3f ms\n"
            "reduce:         %7.3f ms\n",
            ms_parse_input, ms_reduce);
}
