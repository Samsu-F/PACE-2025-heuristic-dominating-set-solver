#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <string.h>
#include <signal.h>

#include "graph.h"
#include "reduction.h"
#include "greedy.h"
#include "weighted_sampling_tree.h" // for debugging and testing



// DynamicArray* g_sigterm_handler_ds = NULL;
// DynamicArray* g_sigterm_handler_fixed = NULL;
static bool g_sigterm_received = false;



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
        // fprintf(stderr, "sample with replacement: index %zu\tid == %" PRIu32 "\n", index,
        //         g->vertices[index]->id);
    }
    for(size_t i = 0; i < g->n - 1; i++) {
        size_t index = wst_sample_without_replacement(wst);
        // fprintf(stderr, "sample without replacement: index %zu\tid == %" PRIu32 "\t\t\tg->n == %" PRIu32 ",\ti == %zu\n",
        //         index, g->vertices[index]->id, g->n, i);
    }
    // fprintf(stderr, "expecting an assertion fail now:\n");
    // size_t index = wst_sample_without_replacement(wst);
    // fprintf(stderr, "no error occurred, this should not happen.\n");


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



static void print_solution(Graph* g, size_t ds_size)
{
    // fprintf(stderr, "print_solution called\n");
    printf("%zu\n", g->fixed.size + ds_size);
    for(size_t fixed_idx = 0; fixed_idx < g->fixed.size; fixed_idx++) {
        Vertex* v = g->fixed.vertices[fixed_idx];
        printf("%" PRIu32 "\n", v->id);
    }
    size_t ds_vertices_found_in_g = 0;
    for(size_t i_vertices = 0; i_vertices < g->n; i_vertices++) {
        Vertex* v = g->vertices[i_vertices];
        if(v->is_in_ds) {
            printf("%" PRIu32 "\n", v->id);
            ds_vertices_found_in_g++;
        }
    }
    fflush(stdout);
    assert(ds_vertices_found_in_g == ds_size);
}



// just temporary, not good
static void clone_dynamic_array(DynamicArray* src, DynamicArray* dst)
{
    dst->size = src->size;
    dst->capacity = src->capacity;
    dst->vertices = malloc(src->capacity * sizeof(Vertex*));
    if(dst->vertices == NULL) {
        exit(EXIT_FAILURE);
    }
    memcpy(dst->vertices, src->vertices, src->size * sizeof(Vertex*));
}



// just temporary, not good
void sigterm_handler(int sig)
{
    // fprintf(stderr, "sigterm handler called\n");
    g_sigterm_received = true;
    // print_solution(g_sigterm_handler_fixed, g_sigterm_handler_ds);
    // exit(EXIT_SUCCESS);
}



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
    // verify_m(g);

    // uint32_t input_n = g->n, input_m = g->m;
    // clock_t time_reduction_start = clock();

    reduce(g, 15.0f, 10.0f); /////////////////////////////////////////////////////////////////////////


    // clock_t time_reduction_end = clock();
    // uint32_t reduced_n = g->n, reduced_m = g->m;

    uint32_t* dominated_by_numbers = malloc(g->n * sizeof(uint32_t));
    bool* in_ds = calloc(g->n, sizeof(bool));
    if(!dominated_by_numbers || !in_ds)
        exit(1);
    for(uint32_t vertices_idx = 0; vertices_idx < g->n; vertices_idx++) {
        dominated_by_numbers[vertices_idx] = g->vertices[vertices_idx]->dominated_by_number;
        assert(!(g->vertices[vertices_idx]->is_in_ds));
    }

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



    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



    // if(signal(SIGTERM, sigterm_handler) == SIG_ERR) {
    //     perror("registering sigterm handler failed");
    //     return EXIT_FAILURE;
    // }
    // // g_sigterm_handler_fixed = &(g->fixed);


    {
        struct sigaction sa;
        sa.sa_handler = sigterm_handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART; // 0 or SA_RESTART if syscalls should be restarted
        if(sigaction(SIGTERM, &sa, NULL) == -1) {
            perror("sigaction failed");
            exit(EXIT_FAILURE);
        }
    }


    {
        // for(uint32_t vertices_idx = 0; vertices_idx < g->n; vertices_idx++) { // restore state after reduction for testing
        //     g->vertices[vertices_idx]->dominated_by_number = dominated_by_numbers[vertices_idx];
        //     g->vertices[vertices_idx]->is_in_pq = false;
        // }

        // DynamicArray ds = greedy_random(g);
        size_t current_best_ds_size = greedy(g);
        // g_sigterm_handler_ds = &current_best_ds;
        for(uint32_t vertices_idx = 0; vertices_idx < g->n; vertices_idx++) {
            dominated_by_numbers[vertices_idx] = g->vertices[vertices_idx]->dominated_by_number;
            in_ds[vertices_idx] = g->vertices[vertices_idx]->is_in_ds;
        }
        assert(fprintf(stderr, "\nafter initial greedy: current_best_ds_size == %zu\n", current_best_ds_size));
        double removal_probability = 0.05;
        for(size_t greedy_repeat = 0; true; greedy_repeat++) {
            // greedy_random_remove_and_refill(g, &ds, removal_probability);
            size_t new_ds_size = greedy_remove_and_refill(g, removal_probability, current_best_ds_size);
            if(new_ds_size <= current_best_ds_size) {
                assert(fprintf(stderr, "%s new_ds_size == %zu\tprev best: %zu\t\tremoval_probability == %.3f\tgreedy_repeat == %zu\n",
                               new_ds_size < current_best_ds_size ? "IMPROVEMENT:" : "EQUAL:      ", new_ds_size,
                               current_best_ds_size, removal_probability, greedy_repeat));
                for(uint32_t vertices_idx = 0; vertices_idx < g->n; vertices_idx++) {
                    dominated_by_numbers[vertices_idx] = g->vertices[vertices_idx]->dominated_by_number;
                    in_ds[vertices_idx] = g->vertices[vertices_idx]->is_in_ds;
                }
                current_best_ds_size = new_ds_size;
            }
            else { // restore previous state
                assert(fprintf(stderr, "worse:       new_ds_size == %zu\tprev best: %zu\t\tremoval_probability == %.3f\tgreedy_repeat == %zu\n",
                               new_ds_size, current_best_ds_size, removal_probability, greedy_repeat));
                for(uint32_t vertices_idx = 0; vertices_idx < g->n; vertices_idx++) {
                    g->vertices[vertices_idx]->dominated_by_number = dominated_by_numbers[vertices_idx];
                    g->vertices[vertices_idx]->is_in_ds = in_ds[vertices_idx];
                }
            }
            if(g_sigterm_received) {
                fprintf(stderr, "g_sigterm_received == true\tgreedy_repeat == %zu, final ds size == %zu\n",
                        greedy_repeat, current_best_ds_size);
                fflush(stderr);
                print_solution(g, current_best_ds_size);
                break;
            }
        }
    }



    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



    free(dominated_by_numbers);
    free(in_ds);

    graph_free(g);
    // da_free_internals(&ds_greedy);
    // da_free_internals(&ds_greedy_rand);
}
