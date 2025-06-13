#include "greedy.h"

#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <time.h>

#include "assert_allow_float_equal.h"
#include "fast_random.h"
#include "debug_log.h"



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



// must only be called if v is currently in the ds.
static inline void _remove_from_ds(Vertex* v)
{
    assert(v->is_in_ds);
    v->dominated_by_number--;
    for(uint32_t i_v = 0; i_v < v->degree; i_v++) {
        v->neighbors[i_v]->dominated_by_number--;
    }
    v->is_in_ds = false;
}



// returns the resulting ds size
static size_t _random_deconstruction(Graph* g, double removal_probability, size_t current_ds_size, fast_random_t* rng)
{
    const uint64_t rand_threshold = (uint64_t)(removal_probability * (double)FAST_RANDOM_MAX);
    for(size_t i_vertices = 0; i_vertices < g->n; i_vertices++) {
        Vertex* v = g->vertices[i_vertices];
        if(v->is_in_ds && fast_random(rng) < rand_threshold) {
            _remove_from_ds(v);
            current_ds_size--;
        }
    }
    return current_ds_size;
}



typedef struct QueueElem {
    struct QueueElem* next;
    Vertex* val;
} QueueElem;

typedef struct Queue {
    QueueElem* head;
    QueueElem* tail;
} Queue;

static inline bool _queue_is_empty(Queue* q)
{
    return q->head == NULL;
}

static inline void _enqueue(Queue* q, Vertex* new_val)
{
    QueueElem* new_q_elem = calloc(1, sizeof(QueueElem));
    if(!new_q_elem) {
        perror("enqueue: calloc failed");
        exit(EXIT_FAILURE);
    }
    new_q_elem->val = new_val;
    if(!_queue_is_empty(q)) {
        assert(q->tail != NULL);
        q->tail->next = new_q_elem;
        q->tail = new_q_elem;
    }
    else {
        q->head = new_q_elem;
        q->tail = new_q_elem;
    }
}

static inline Vertex* _dequeue(Queue* q)
{
    assert(!_queue_is_empty(q));
    Vertex* result = q->head->val;
    QueueElem* to_free = q->head;
    q->head = q->head->next;
    free(to_free);
    return result;
}

static inline void _clear_queue(Queue* q)
{
    while(!_queue_is_empty(q)) {
        QueueElem* to_free = q->head;
        q->head = q->head->next;
        free(to_free);
    }
}


// create a local hole in the ds coverage using breadth-first search
// returns the resulting ds size
static size_t _local_deconstruction(Graph* g, const size_t max_removals, const size_t current_ds_size, fast_random_t* rng)
{
    static uint32_t queued_current_marker = 0;
    queued_current_marker++;
    // the queued field of the vertices is used like a bool. However, to avoid having to reset all
    // queued fields to false, the next local deconstruction run increments queued_current_marker and
    // checks the queued fields against a new value.

    size_t start_index = (size_t)(((__uint128_t)g->n * (__uint128_t)fast_random(rng)) / ((__uint128_t)FAST_RANDOM_MAX + 1));
    assert(start_index < g->n);

    Queue q = {0};
    _enqueue(&q, g->vertices[start_index]);
    size_t count_removed = 0;
    size_t ds_vertices_queued = 0;
    while((!_queue_is_empty(&q)) && count_removed < max_removals) {
        Vertex* v = _dequeue(&q);
        if(v->is_in_ds) {
            _remove_from_ds(v);
            count_removed++;
        }
        // enqueue neighbors of v if not already enqueued / visited
        for(uint32_t i_v = 0; i_v < v->degree && ds_vertices_queued < max_removals; i_v++) {
            Vertex* u = v->neighbors[i_v];
            if(u->queued != queued_current_marker) {
                u->queued = queued_current_marker;
                _enqueue(&q, u);
                if(u->is_in_ds) {
                    ds_vertices_queued++;
                }
            }
        }
    }
    _clear_queue(&q);
    return current_ds_size - count_removed;
}



static void _init_votes(Graph* g)
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

    PQueue* pq = pq_new();
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



static void _sigterm_handler(int sig)
{
    (void)sig; // supress warning for unused parameter
    _g_sigterm_received = true;
}



static void _register_sigterm_handler(void)
{
    struct sigaction sa = {0};
    sa.sa_handler = _sigterm_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART; // restart interrupted syscalls
    if(sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("sigaction failed to register SIGTERM handler");
        exit(EXIT_FAILURE);
    }
    if(sigaction(SIGINT, &sa, NULL) == -1) { // also terminate on Ctrl+C
        // don't care if registering the SIGINT handler fails, it is not necessary but only QOL
    }
}



static inline double _clamp(double x, double min, double max)
{
    return (x < min) ? min : (x > max) ? max : x;
}



// runs iterated greedy algorithm on the graph until a sigterm signal is received.
// v->is_in_ds must be set to false for all vertices before calling this function.
// returns the number of vertices in the dominating set.
size_t iterated_greedy_solver(Graph* g)
{
    _register_sigterm_handler();
    _init_votes(g);
    fast_random_t rng;
    fast_random_init(&rng, (uint64_t)time(NULL));

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


    // TODO: tweak metaheuristic values
    const double score_decay_factor = 0.9; // must be >0 and <1
    const double reward_improvement = 1.0;
    const double reward_equal = 0.0;        // should be >=0 and <=reward_improvement
    const double minimum_probability = 0.2; // the minimal probability for a deconstruction approach to be selected, regardless of how low its score is. Must be >=0 and <=0.5
    double score_local_decon = 0.0;
    double score_random_decon = 1.0; // Testing has shown that random deconstruction is better in the beginning

    size_t ig_iteration = 0;
    for(; !_g_sigterm_received; ig_iteration++) {
        double probability_local_decon = score_local_decon / (score_local_decon + score_random_decon + 1.e-10); // the tiny summand prevents division by 0
        probability_local_decon = _clamp(probability_local_decon, minimum_probability, 1.0 - minimum_probability);
        debug_log("score_local_decon == %.6f  score_random_decon == %.6f  probability_local_decon == %.6f\t",
                  score_local_decon, score_random_decon, probability_local_decon);
        // deconstruct solution
        if(fast_random(&rng) < (uint64_t)(probability_local_decon * (double)FAST_RANDOM_MAX)) {
            debug_log("local deconstruction \t");
            current_ds_size = _local_deconstruction(g, 40, current_ds_size, &rng); // max removals can be tweaked // TODO
            current_ds_size = _greedy_vote_construct(g, current_ds_size);
            double reward = current_ds_size < saved_ds_size  ? reward_improvement :
                            current_ds_size == saved_ds_size ? reward_equal :
                                                               0.0;
            score_local_decon = score_local_decon * score_decay_factor + reward;
        }
        else {
            debug_log("random deconstruction\t");
            current_ds_size = _random_deconstruction(g, 0.006, current_ds_size, &rng); // removal probability can be tweaked // TODO
            current_ds_size = _greedy_vote_construct(g, current_ds_size);
            double reward = current_ds_size < saved_ds_size  ? reward_improvement :
                            current_ds_size == saved_ds_size ? reward_equal :
                                                               0.0;
            score_random_decon = score_random_decon * score_decay_factor + reward;
        }

        if(current_ds_size <= saved_ds_size) {
            debug_log("%s current_ds_size == %zu\tsaved_ds_size == %zu\t\tig_iteration == %zu\n",
                      current_ds_size < saved_ds_size ? "IMPROVEMENT:" : "EQUAL: =    ", current_ds_size,
                      saved_ds_size, ig_iteration);
            for(uint32_t i = 0; i < g->n; i++) { // save the current solution
                dominated_by_numbers[i] = g->vertices[i]->dominated_by_number;
                in_ds[i] = g->vertices[i]->is_in_ds;
            }
            saved_ds_size = current_ds_size;
        }
        else { // restore saved solution
            debug_log("worse:       current_ds_size == %zu\tsaved_ds_size == %zu\t\tig_iteration == %zu\n",
                      current_ds_size, saved_ds_size, ig_iteration);
            for(uint32_t i = 0; i < g->n; i++) {
                g->vertices[i]->dominated_by_number = dominated_by_numbers[i];
                g->vertices[i]->is_in_ds = in_ds[i];
            }
            current_ds_size = saved_ds_size;
        }
    }
    fprintf(stderr, "g_sigterm_received == %d\t\tfinal ds size == %zu\t\tds + fixed == %zu\t\tgreedy iterations == %zu\n",
            _g_sigterm_received, current_ds_size, current_ds_size + g->fixed.size, ig_iteration);
    fflush(stderr);

    free(in_ds);
    free(dominated_by_numbers);
    return current_ds_size;
}
