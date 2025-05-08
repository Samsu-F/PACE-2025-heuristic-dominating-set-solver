#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>

#include "graph.h"
#include "reduction.h"
#include "greedy.h"



static bool verify_n_m_fixed(Graph* g)
{
    size_t n = 0;
    size_t m = 0;
    size_t count_fixed = 0;

    for(Vertex* v = g->vertices; v != NULL; v = v->list_next) {
        n++;
        m += v->degree;
    }
    for(Vertex* v = g->fixed; v != NULL; v = v->list_next) {
        count_fixed++;
    }
    m /= 2; // each edge was counted twice
    if((g->n != n) || (g->m != m) || (g->count_fixed != count_fixed)) {
        fprintf(stderr, "verification failed:\n");
        if(g->n != n)
            fprintf(stderr, "\tn == %zu, g->n == %zu\n", n, g->n);
        if(g->m != m)
            fprintf(stderr, "\tm == %zu, g->m == %zu\n", m, g->m);
        if(g->count_fixed != count_fixed)
            fprintf(stderr, "\tcount_fixed == %zu, g->count_fixed == %zu\n", count_fixed, g->count_fixed);
        exit(1);
        return false;
    }
    return true;
}



bool size_t_greater_tmp(const size_t a, const size_t b)
{
    return a > b;
}



static void test_pq(Graph* g)
{
    PQueue* pq = pq_new(size_t_greater_tmp);
    assert(pq);
    for(Vertex* v = g->vertices; v != NULL; v = v->list_next) {
        pq_insert(pq, (KeyValPair) {.key = v->id + 100000000, .val = v});
    }
    for(Vertex* v = g->vertices; v != NULL; v = v->list_next) {
        if(v->id % 10 > 0) {
            pq_decrease_priority(pq, v, pq_get_key(pq, v) - 100000000 + (v->id % 10) * 10000000);
        }
    }
    while(!pq_is_empty(pq)) {
        KeyValPair kv = pq_pop(pq);
        Vertex* v = kv.val;
        fprintf(stderr, "kv.key == %zu\tv->id == %zu\tptr %p\n", kv.key, v->id, (void*)v);
        assert(kv.key % 10000000 == v->id);
        assert(((v->id % 10 != 0) && kv.key / 10000000 == v->id % 10) ||
               ((v->id % 10 == 0) && kv.key - 100000000 == v->id));
    }
}





static void print_solution(Graph* g, VertexArray* va){
    printf("%zu\n", g->count_fixed + va->size);
    for(Vertex* v = g->fixed; v != NULL; v = v->list_next) {
        printf("%zu\n", v->id);
    }
    for(size_t i = 0; i < va->size; i++){
        printf("%zu\n", va->arr[i]->id);
    }
}





// just some tests so far
int main(int argc, char* argv[])
{
    fprintf(stderr, "sizeof(Vertex) == %zu\n", sizeof(Vertex));

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


    clock_t time_parsing_start = clock();
    Graph* g = graph_parse(input_file);
    clock_t time_parsing_end = clock();

    if(close_input_file) {
        fclose(input_file);
    }
    if(!g) {
        exit(1);
    }

    // test_pq(g);
    verify_n_m_fixed(g);

    size_t input_n = g->n, input_m = g->m;
    clock_t time_reduction_start = clock();

    reduce(g, 15.0f, 10.0f); /////////////////////////////////////////////////////////////////////////

    clock_t time_reduction_end = clock();
    size_t reduced_n = g->n, reduced_m = g->m;

    clock_t time_greedy_start = clock();
    VertexArray ds = greedy(g);
    clock_t time_greedy_end = clock();



    verify_n_m_fixed(g);
    // graph_print_as_dot(g, false, "Test");

    print_solution(g, &ds);



    float ms_parse_input = (float)(1000 * (time_parsing_end - time_parsing_start)) / CLOCKS_PER_SEC;
    float ms_reduce = (float)(1000 * (time_reduction_end - time_reduction_start)) / CLOCKS_PER_SEC;
    float ms_greedy = (float)(1000 * (time_greedy_end - time_greedy_start)) / CLOCKS_PER_SEC;


    fprintf(stderr, "%s\n", argc == 2 ? argv[1] : "stdin");
    fprintf(stderr,
            "input:   n = %zu\tm = %zu\n"
            "reduced: n = %zu\tm = %zu\tcount_fixed = %zu\n"
            "greedy: ds.size = %zu\tcount_fixed + ds.size = %zu\t\tds.allocated_size = %zu\n",
            input_n, input_m, reduced_n, reduced_m, g->count_fixed, ds.size, g->count_fixed + ds.size, ds.allocated_size);
    fprintf(stderr,
            "---\n"
            "parse input:    %7.3f ms\n"
            "reduce:         %7.3f ms\n"
            "greedy:         %7.3f ms\n"
            "\n\n",
            ms_parse_input, ms_reduce, ms_greedy);


    // // csv mode
    // fprintf(stderr, "%zu,%zu,%.3f,%.3f\n", input_n, reduced_n, (double)reduced_n / (double)input_n, ms_reduce);


    graph_free(g);
    free(ds.arr);
}
