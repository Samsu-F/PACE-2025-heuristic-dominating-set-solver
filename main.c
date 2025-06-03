#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <string.h>

#include "graph.h"
#include "reduction.h"
#include "greedy.h"



// static bool verify_m(Graph* g)
// {
//     size_t m = 0;
//     for(uint32_t vertices_idx = 0; vertices_idx < g->n; vertices_idx++) {
//         Vertex* v = g->vertices[vertices_idx];
//         m += v->degree;
//     }
//     m /= 2; // each edge was counted twice
//     if(g->m != m) {
//         fprintf(stderr, "verification failed:\n\tm == %zu, g->m == %" PRIu32 "\n", m, g->m);
//         exit(EXIT_FAILURE);
//         return false;
//     }
//     return true;
// }



// bool double_greater_tmp(const double a, const double b)
// {
//     return a > b;
// }



// static void test_pq(Graph* g)
// {
//     PQueue* pq = pq_new(double_greater_tmp);
//     assert(pq);
//     for(uint32_t vertices_idx = 0; vertices_idx < g->n; vertices_idx++) {
//         Vertex* v = g->vertices[vertices_idx];
//         pq_insert(pq, (KeyValPair) {.key = v->id + 100000000, .val = v});
//     }
//     for(uint32_t vertices_idx = 0; vertices_idx < g->n; vertices_idx++) {
//         Vertex* v = g->vertices[vertices_idx];
//         if(v->id % 10 > 0) {
//             pq_decrease_priority(pq, v, pq_get_key(pq, v) - 100000000 + (v->id % 10) * 10000000);
//         }
//     }
//     while(!pq_is_empty(pq)) {
//         KeyValPair kv = pq_pop(pq);
//         Vertex* v = kv.val;
//         fprintf(stderr, "kv.key == %f\tv->id == %" PRIu32 "\tptr %p\n", kv.key, v->id, (void*)v);
//         assert((uint32_t)kv.key % 10000000 == v->id);
//         assert(((v->id % 10 != 0) && (uint32_t)kv.key / 10000000 == v->id % 10) ||
//                ((v->id % 10 == 0) && (uint32_t)kv.key - 100000000 == v->id));
//     }
// }





// // just temporary, not good
// static void clone_dynamic_array(DynamicArray* src, DynamicArray* dst)
// {
//     dst->size = src->size;
//     dst->capacity = src->capacity;
//     dst->vertices = malloc(src->capacity * sizeof(Vertex*));
//     if(dst->vertices == NULL) {
//         exit(EXIT_FAILURE);
//     }
//     memcpy(dst->vertices, src->vertices, src->size * sizeof(Vertex*));
// }



// just some tests so far
int main(int argc, char* argv[])
{
    // fprintf(stderr, "sizeof(Vertex) == %zu\n", sizeof(Vertex));
    srand((unsigned)time(NULL));

    if(argc > 2) {
        fprintf(stderr, "too many arguments\n");
        return EXIT_FAILURE;
    }

    FILE* input_file = NULL;
    bool close_input_file = false;
    if(argc == 2 && false) { // disabled because OPTIL doesn't seem to like this, maybe they call with some parameter?
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


    // clock_t time_parsing_start = clock();
    Graph* g = graph_parse(input_file);
    // clock_t time_parsing_end = clock();

    if(close_input_file) {
        fclose(input_file);
    }
    if(!g) {
        exit(EXIT_FAILURE);
    }

    // test_pq(g);
    // test_weighted_sampling_tree(g);
    // verify_m(g);

    // uint32_t input_n = g->n, input_m = g->m;
    // clock_t time_reduction_start = clock();

    reduce(g, 15.0f, 10.0f); /////////////////////////////////////////////////////////////////////////


    // clock_t time_reduction_end = clock();
    // uint32_t reduced_n = g->n, reduced_m = g->m;

    // clock_t time_greedy_start = clock();
    // DynamicArray ds_greedy = greedy(g);
    // clock_t time_greedy_end = clock();



    // for(uint32_t vertices_idx = 0; vertices_idx < g->n; vertices_idx++)
    //     g->vertices[vertices_idx]->dominated_by_number = dominated_by_numbers[vertices_idx];

    // clock_t time_greedy_rand_start = clock();
    // DynamicArray ds_greedy_rand = greedy_random(g);
    // clock_t time_greedy_rand_end = clock();


    // for(size_t testrun = 0; testrun < 10; testrun++) {
    //     for(uint32_t vertices_idx = 0; vertices_idx < g->n; vertices_idx++)
    //         g->vertices[vertices_idx]->dominated_by_number = dominated_by_numbers[vertices_idx];

    //     DynamicArray ds_tmp = greedy_random(g);
    //     fprintf(stderr, "\nds_tmp.size == %zu\n", ds_tmp.size);
    //     double removal_probability = (double)testrun / 20.0 + 0.02;
    //     for(size_t greedy_repeat = 0; greedy_repeat < 10; greedy_repeat++) {
    //         greedy_random_remove_and_refill(g, &ds_tmp, removal_probability);
    //         fprintf(stderr, "ds_tmp.size == %zu\t\tremoval_probability == %.3f\tgreedy_repeat == %zu\n",
    //                 ds_tmp.size, removal_probability, greedy_repeat);
    //     }
    //     da_free_internals(&ds_tmp);
    // }



    // verify_m(g);
    // graph_print_as_dot(g, false, "Test");

    // print_solution((&g->fixed), &ds_greedy);



    // float ms_parse_input = (float)(1000 * (time_parsing_end - time_parsing_start)) / CLOCKS_PER_SEC;
    // float ms_reduce = (float)(1000 * (time_reduction_end - time_reduction_start)) / CLOCKS_PER_SEC;
    // float ms_greedy = (float)(1000 * (time_greedy_end - time_greedy_start)) / CLOCKS_PER_SEC;
    // float ms_greedy_rand = (float)(1000 * (time_greedy_rand_end - time_greedy_rand_start)) / CLOCKS_PER_SEC;


    // fprintf(stderr, "%s\n", argc == 2 ? argv[1] : "stdin");
    // fprintf(stderr,
    //         "input:   n = %" PRIu32 "\tm = %" PRIu32 "\n"
    //         "reduced: n = %" PRIu32 "\tm = %" PRIu32 "\tfixed.size = %zu\n"
    //         "greedy:        ds.size = %zu\tfixed.size + ds.size = %zu\n"
    //         "greedy random: ds.size = %zu\tfixed.size + ds.size = %zu\n",
    //         input_n, input_m, reduced_n, reduced_m, g->fixed.size, ds_greedy.size,
    //         g->fixed.size + ds_greedy.size, ds_greedy_rand.size, g->fixed.size + ds_greedy_rand.size);
    // fprintf(stderr,
    //         "---\n"
    //         "parse input:    %7.3f ms\n"
    //         "reduce:         %7.3f ms\n"
    //         "greedy:         %7.3f ms\n"
    //         "greedy random:  %7.3f ms\n"
    //         "\n\n",
    //         ms_parse_input, ms_reduce, ms_greedy, ms_greedy_rand);


    // // csv mode
    // fprintf(stderr, "\"%s\",%" PRIu32 ",%" PRIu32 ",%.3f,%.3f,%zu,%zu\n", argc > 1? argv[1] : "stdin", input_n, reduced_n, (double)reduced_n / (double)input_n, ms_reduce, g->fixed.size, ds_greedy.size);



    iterated_greedy_solver(g);

    // free(dominated_by_numbers);
    // free(in_ds);

    graph_free(g);
    // da_free_internals(&ds_greedy);
    // da_free_internals(&ds_greedy_rand);
}
