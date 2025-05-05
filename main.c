#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "graph.h"
#include "reduction.h"



int main(int argc, char* argv[])
{
    if(argc > 2) {
        fprintf(stderr, "too many arguments\n");
        return EXIT_FAILURE;
    }

    FILE* input_file = NULL;
    bool close_input_file = false;
    if(argc == 2) {
        input_file = fopen(argv[1], "r");
        if(!input_file) {
            perror("Failed to open file");
            return EXIT_FAILURE;
        }
        close_input_file = true;
    }
    else {
        input_file = stdin;
    }

    // just some tests so far

    clock_t time_parsing_start = clock();
    Graph* g = graph_parse(input_file);
    clock_t time_parsing_end = clock();

    if(close_input_file) {
        fclose(input_file);
    }
    if(!g) {
        exit(1);
    }
    fprintf(stderr, "%s\n", argc == 2 ? argv[1] : "stdin");
    fprintf(stderr, "input:   n = %zu\tm = %zu\n", g->n, g->m);
    clock_t time_reduction_start = clock();

    // fprintf(stderr, "simple: count_fixed = %zu\n", fix_isol_and_supp_vertices(g));
    reduce(g);

    clock_t time_reduction_end = clock();
    fprintf(stderr, "reduced: n = %zu\tm = %zu\n", g->n, g->m);

    graph_print_as_dot(g, false, "Test");
    graph_free(g);


    float ms_parse_input = (float)(1000 * (time_parsing_end - time_parsing_start)) / CLOCKS_PER_SEC;
    float ms_reduce = (float)(1000 * (time_reduction_end - time_reduction_start)) / CLOCKS_PER_SEC;
    fprintf(stderr,
            "---\n"
            "parse input:    %7.3f ms\n"
            "reduce:         %7.3f ms\n\n",
            ms_parse_input, ms_reduce);
}
