#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>

#include "graph.h"
#include "reduction.h"
#include "greedy.h"
#include "weighted_sampling_tree.h" // for debugging and testing



static bool verify_m(Graph* g)
{
    size_t m = 0;
    for(uint32_t vertices_idx = 0; vertices_idx < g->n; vertices_idx++) {
        Vertex* v = g->vertices[vertices_idx];
        m += v->degree;
    }
    m /= 2; // each edge was counted twice
    if(g->m != m) {
        fprintf(stderr, "verification failed:\n\tm == %zu, g->m == %" PRIu32 "\n", m, g->m);
        exit(EXIT_FAILURE);
        return false;
    }
    return true;
}



bool uint32_t_greater_tmp(const uint32_t a, const uint32_t b)
{
    return a > b;
}



static void test_pq(Graph* g)
{
    PQueue* pq = pq_new(uint32_t_greater_tmp);
    assert(pq);
    for(uint32_t vertices_idx = 0; vertices_idx < g->n; vertices_idx++) {
        Vertex* v = g->vertices[vertices_idx];
        pq_insert(pq, (KeyValPair) {.key = v->id + 100000000, .val = v});
    }
    for(uint32_t vertices_idx = 0; vertices_idx < g->n; vertices_idx++) {
        Vertex* v = g->vertices[vertices_idx];
        if(v->id % 10 > 0) {
            pq_decrease_priority(pq, v, pq_get_key(pq, v) - 100000000 + (v->id % 10) * 10000000);
        }
    }
    while(!pq_is_empty(pq)) {
        KeyValPair kv = pq_pop(pq);
        Vertex* v = kv.val;
        fprintf(stderr, "kv.key == %" PRIu32 "\tv->id == %" PRIu32 "\tptr %p\n", kv.key, v->id, (void*)v);
        assert(kv.key % 10000000 == v->id);
        assert(((v->id % 10 != 0) && kv.key / 10000000 == v->id % 10) ||
               ((v->id % 10 == 0) && kv.key - 100000000 == v->id));
    }
}



static void test_weighted_sampling_tree(Graph* g)
{
    srand((unsigned)time(NULL));

    double* weights = malloc(g->n * sizeof(double));
    assert(weights);
    for(uint32_t vertices_idx = 0; vertices_idx < g->n; vertices_idx++) {
        Vertex* v = g->vertices[vertices_idx];
        weights[vertices_idx] = (double)v->id;
    }
    WeightedSamplingTree* wst = wst_new(weights, g->n);
    assert(wst);
    for(size_t i = 0; i < 200; i++) {
        size_t index = wst_sample_with_replacement(wst);
        fprintf(stderr, "sample with replacement: index %zu\tid == %" PRIu32 "\n", index,
                g->vertices[index]->id);
    }
    for(size_t i = 0; i < g->n - 1; i++) {
        size_t index = wst_sample_without_replacement(wst);
        fprintf(stderr, "sample without replacement: index %zu\tid == %" PRIu32 "\t\t\tg->n == %" PRIu32 ",\ti == %zu\n",
                index, g->vertices[index]->id, g->n, i);
    }
    fprintf(stderr, "expecting an assertion fail now:\n");
    size_t index = wst_sample_without_replacement(wst);
    fprintf(stderr, "no error occurred, this should not happen.\n");


    // size_t size = 10000;
    // double* weights = malloc(size * sizeof(double));
    // assert(weights);
    // for(size_t i = 0; i < size; i++) {
    //     weights[i] = 2.0 * (double)i;
    // }
    // WeightedSamplingTree* wst = wst_new(weights, size);
    // assert(wst);
    // for(size_t i = 0; i < 20; i++) {
    //     size_t index = wst_sample_with_replacement(wst);
    //     fprintf(stderr, "sample with replacement: index %zu\n", index);
    // }
    // for(size_t i = 0; i < size - 1; i++) {
    //     size_t index = wst_sample_without_replacement(wst);
    //     fprintf(stderr, "sample without replacement: index %zu\n", index);
    // }
    // fprintf(stderr, "expecting an error now:\n");
    // size_t index = wst_sample_without_replacement(wst);
    // fprintf(stderr, "no error occurred, this should not happen.\n");
}



static void print_solution(Graph* g, DynamicArray* da)
{
    printf("%zu\n", g->fixed.size + (size_t)da->size);
    for(size_t fixed_idx = 0; fixed_idx < g->fixed.size; fixed_idx++) {
        Vertex* v = g->fixed.vertices[fixed_idx];
        printf("%" PRIu32 "\n", v->id);
    }
    for(size_t i = 0; i < da->size; i++) {
        printf("%" PRIu32 "\n", da->vertices[i]->id);
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
        exit(EXIT_FAILURE);
    }

    // test_pq(g);
    // test_weighted_sampling_tree(g);
    verify_m(g);

    uint32_t input_n = g->n, input_m = g->m;
    clock_t time_reduction_start = clock();

    reduce(g, 15.0f, 10.0f); /////////////////////////////////////////////////////////////////////////

    clock_t time_reduction_end = clock();
    uint32_t reduced_n = g->n, reduced_m = g->m;

    clock_t time_greedy_start = clock();
    DynamicArray ds = greedy(g);
    clock_t time_greedy_end = clock();



    verify_m(g);
    // graph_print_as_dot(g, false, "Test");

    print_solution(g, &ds);



    float ms_parse_input = (float)(1000 * (time_parsing_end - time_parsing_start)) / CLOCKS_PER_SEC;
    float ms_reduce = (float)(1000 * (time_reduction_end - time_reduction_start)) / CLOCKS_PER_SEC;
    float ms_greedy = (float)(1000 * (time_greedy_end - time_greedy_start)) / CLOCKS_PER_SEC;


    fprintf(stderr, "%s\n", argc == 2 ? argv[1] : "stdin");
    fprintf(stderr,
            "input:   n = %" PRIu32 "\tm = %" PRIu32 "\n"
            "reduced: n = %" PRIu32 "\tm = %" PRIu32 "\tfixed.size = %zu\n"
            "greedy: ds.size = %zu\tfixed.size + ds.size = %zu",
            input_n, input_m, reduced_n, reduced_m, g->fixed.size, ds.size, g->fixed.size + ds.size);
    fprintf(stderr,
            "---\n"
            "parse input:    %7.3f ms\n"
            "reduce:         %7.3f ms\n"
            "greedy:         %7.3f ms\n"
            "\n\n",
            ms_parse_input, ms_reduce, ms_greedy);


    // // csv mode
    // fprintf(stderr, "%" PRIu32 ",%" PRIu32 ",%.3f,%.3f\n", input_n, reduced_n, (double)reduced_n / (double)input_n, ms_reduce);


    graph_free(g);
    da_free_internals(&ds);
}
