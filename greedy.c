#include "greedy.h"

#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>

#include "assert_allow_float_equal.h"



static bool _g_sigterm_received = false;



// return the new ds size
static size_t _make_minimal(Graph* g, size_t current_ds_size)
{
    assert(g != NULL);
    for(size_t i_vertices = 0; i_vertices < g->n; i_vertices++) {
        Vertex* v = g->vertices[i_vertices];
        if(v->is_in_ds && v->dominated_by_number > 1) {
            bool v_redundant = true;
            for(uint32_t i_v = 0; i_v < v->degree; i_v++) {
                if(v->neighbors[i_v]->dominated_by_number < 2) {
                    assert(v->neighbors[i_v]->dominated_by_number >= 1); // otherwise ds would not be a dominating set
                    v_redundant = false;
                    break;
                }
            }
            if(v_redundant) {
                v->is_in_ds = false;
                current_ds_size--;
                v->dominated_by_number--;
                for(uint32_t i_v = 0; i_v < v->degree; i_v++) {
                    v->neighbors[i_v]->dominated_by_number--;
                }
            }
        }
    }
    return current_ds_size;
}



// comparison function for the priority queue
static bool _double_greater(const double a, const double b)
{
    return a > b;
}



// returns a random double x, with +0.0 <= x <= 1.0, uniform distribution
static double _random(void)
{
    return rand() / (double)RAND_MAX; // TODO: use a better rng. RAND_MAX is only guaranteed to be at least 32767 and rand() is also not that fast.
}



// returns the resulting ds size
static size_t _remove_randomly_from_ds(Graph* g, double removal_probability, size_t current_ds_size)
{
    for(size_t i_vertices = 0; i_vertices < g->n; i_vertices++) {
        Vertex* v = g->vertices[i_vertices];
        if(v->is_in_ds) {
            if((_random() < removal_probability)) {
                v->dominated_by_number--;
                for(uint32_t i_v = 0; i_v < v->degree; i_v++) {
                    v->neighbors[i_v]->dominated_by_number--;
                }
                v->is_in_ds = false;
                current_ds_size--;
            }
        }
    }
    return current_ds_size;
}



// TODO: use more efficient queue implementation specifically designed for the use case
typedef struct Queue {
    struct Queue* next;
    Vertex* val;
} Queue;



// TODO: refactor
// returns the resulting ds size
static size_t _local_deconstruction(Graph* g, const size_t current_ds_size)
{
    for(size_t i_vertices = 0; i_vertices < g->n; i_vertices++) { // inefficient, TODO
        g->vertices[i_vertices]->queued = false;
    }

    size_t start_index = (size_t)(_random() * g->n);
    start_index = start_index >= g->n ? g->n - 1 : start_index;

    Queue* q_head = calloc(1, sizeof(Queue));
    if(!q_head) {
        exit(42);
    }
    q_head->val = g->vertices[start_index];
    Queue* q_tail = q_head;

    size_t count_removed = 0;
    while(q_head != NULL && count_removed < 25) {
        Vertex* v = q_head->val;

        if(v->is_in_ds) {
            v->dominated_by_number--;
            for(uint32_t i_v = 0; i_v < v->degree; i_v++) {
                v->neighbors[i_v]->dominated_by_number--;
            }
            v->is_in_ds = false;
            count_removed++;
        }

        for(uint32_t i_v = 0; i_v < v->degree; i_v++) {
            Vertex* u = v->neighbors[i_v];
            if(!u->queued) {
                u->queued = true;
                Queue* new_q_elem = calloc(1, sizeof(Queue));
                if(!new_q_elem)
                    exit(42);
                new_q_elem->val = u;
                q_tail->next = new_q_elem;
                q_tail = new_q_elem;
            }
        }

        Queue* to_free = q_head;
        q_head = q_head->next;
        free(to_free);
    }

    while(q_head != NULL) {
        Queue* to_free = q_head;
        q_head = q_head->next;
        free(to_free);
    }

    return current_ds_size - count_removed;
}



void init_votes(Graph* g)
{
    assert(g != NULL);
    for(size_t vertices_idx = 0; vertices_idx < g->n; vertices_idx++) {
        Vertex* v = g->vertices[vertices_idx];
        v->vote = 1.0 / (double)(v->degree + 1);
    }
}



static size_t _greedy_vote_construct(Graph* g, size_t current_ds_size)
{
    uint32_t undominated_vertices = 0; // the total number of undominated vertices remaining in the graph

    PQueue* pq = pq_new(_double_greater);
    if(!pq) {
        perror("greedy: pq_new failed");
        exit(EXIT_FAILURE);
    }
    for(uint32_t vertices_idx = 0; vertices_idx < g->n; vertices_idx++) {
        Vertex* v = g->vertices[vertices_idx];
        double weight = 0.0; // aka votes received
        if(v->dominated_by_number == 0) {
            undominated_vertices++;
            weight = v->vote;
        }
        for(uint32_t i = 0; i < v->degree; i++) {
            Vertex* u = v->neighbors[i];
            if(u->dominated_by_number == 0) {
                weight += u->vote;
            }
        }
        v->is_in_pq = false;
        if(weight > 0.0) {
            pq_insert(pq, (KeyValPair) {.key = weight, .val = v});
        }
    }


    while(undominated_vertices > 0) {
        assert(!pq_is_empty(pq));
        KeyValPair kv = pq_pop(pq);
        Vertex* v = kv.val;
        assert(!v->is_in_ds);
        v->is_in_ds = true;
        current_ds_size++;
        double v_is_newly_dominated = 0.0;
        v->dominated_by_number++;
        if(v->dominated_by_number == 1) {
            v_is_newly_dominated = 1.0;
            undominated_vertices--;
        }

        for(uint32_t i_v = 0; i_v < v->degree; i_v++) {
            Vertex* u1 = v->neighbors[i_v];
            u1->dominated_by_number++;
            double delta_weight_u1 = v_is_newly_dominated * v->vote;
            if(u1->dominated_by_number == 1) { // if v is the first one to dominate u1
                delta_weight_u1 += u1->vote;   // u1 no longer votes for itself
                undominated_vertices--;
                for(uint32_t i_u1 = 0; i_u1 < u1->degree; i_u1++) {
                    Vertex* u2 = u1->neighbors[i_u1];
                    // because u1 is now dominated, u2 no longer receives u1's vote
                    if(u2->is_in_pq) {
                        pq_decrease_priority(pq, u2, pq_get_key(pq, u2) - u1->vote);
                    }
                }
            }
            if(u1->is_in_pq && delta_weight_u1 > 0) {
                pq_decrease_priority(pq, u1, pq_get_key(pq, u1) - delta_weight_u1);
            }
        }
    }
    pq_free(pq);
    current_ds_size = _make_minimal(g, current_ds_size);
    return current_ds_size;
}



// just temporary, not good
static void _sigterm_handler(int sig)
{
    (void)sig;
    _g_sigterm_received = true;
}



static void _register_sigterm_handler(void)
{
    struct sigaction sa;
    sa.sa_handler = _sigterm_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART; // 0 or SA_RESTART if syscalls should be restarted
    if(sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("sigaction failed");
        exit(EXIT_FAILURE);
    }
}



// runs iterated greedy algorithm on the graph until a sigterm signal is received.
// v->is_in_ds must be set to false for all vertices before calling this function.
// returns the number of vertices in the dominating set.
size_t iterated_greedy_solver(Graph* g)
{
    const double removal_probability = 0.05; // can be tweaked
    _register_sigterm_handler();
    init_votes(g);
    // create two array that are use to save and restore the best solution found so far:
    bool* in_ds = calloc(g->n, sizeof(bool));
    uint32_t* dominated_by_numbers = malloc(g->n * sizeof(uint32_t));
    if(in_ds == NULL || dominated_by_numbers == NULL) {
        exit(1);
    }

    size_t current_ds_size = _greedy_vote_construct(g, 0); // get initial solution
    for(uint32_t i = 0; i < g->n; i++) {                   // save the initial solution
        dominated_by_numbers[i] = g->vertices[i]->dominated_by_number;
        in_ds[i] = g->vertices[i]->is_in_ds;
    }
    size_t saved_ds_size = current_ds_size; // the size of the ds saved in dominated_by_numbers and in_ds

    size_t ig_iteration = 0;
    for(; !_g_sigterm_received; ig_iteration++) {
        // deconstruct solution
        if(ig_iteration % 3 == 0) {
            assert(fprintf(stderr, "local deconstruction \t"));
            current_ds_size = _local_deconstruction(g, current_ds_size);
        }
        else {
            assert(fprintf(stderr, "random deconstruction\t"));
            current_ds_size = _remove_randomly_from_ds(g, removal_probability, current_ds_size);
        }
        // reconstruct
        current_ds_size = _greedy_vote_construct(g, current_ds_size);

        if(current_ds_size <= saved_ds_size) {
            assert(fprintf(stderr, "%s current_ds_size == %zu\tsaved_ds_size == %zu\t\tig_iteration == %zu\n",
                           current_ds_size < saved_ds_size ? "IMPROVEMENT:" : "EQUAL: =    ", current_ds_size,
                           saved_ds_size, ig_iteration));
            for(uint32_t i = 0; i < g->n; i++) { // save the current solution
                dominated_by_numbers[i] = g->vertices[i]->dominated_by_number;
                in_ds[i] = g->vertices[i]->is_in_ds;
            }
            saved_ds_size = current_ds_size;
        }
        else { // restore saved solution
            assert(fprintf(stderr, "worse:       current_ds_size == %zu\tsaved_ds_size == %zu\t\tig_iteration == %zu\n",
                           current_ds_size, saved_ds_size, ig_iteration));
            for(uint32_t i = 0; i < g->n; i++) {
                g->vertices[i]->dominated_by_number = dominated_by_numbers[i];
                g->vertices[i]->is_in_ds = in_ds[i];
            }
            current_ds_size = saved_ds_size;
        }
    }
    fprintf(stderr, "g_sigterm_received == %d\t\tfinal ds size == %zu   \tgreedy iterations == %zu\n",
            _g_sigterm_received, current_ds_size, ig_iteration);
    fflush(stderr);

    free(in_ds);
    free(dominated_by_numbers);
    return current_ds_size;
}
