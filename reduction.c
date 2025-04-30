#include "reduction.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <stdio.h> // DEBUG



// Comparison function for qsort
static int _compare_Vertex_ptrs(const void* a, const void* b)
{
    const Vertex* x = *(const Vertex* const*)a;
    const Vertex* y = *(const Vertex* const*)b;
    return (x > y) - (x < y);
}



static void _sort_neighbors(Vertex* v)
{
    assert(v->degree > 0);
    if(!(v->neighbors_array_is_sorted)) {
        qsort(v->neighbors, v->degree, sizeof(Vertex*), _compare_Vertex_ptrs);
        v->neighbors_array_is_sorted = true;
    }
}



// removes all edges of v in both directions, and frees the then empty neighbors array of v
static void _remove_edges(Graph* g, Vertex* v)
{
    assert(v != NULL);
    for(size_t i_v = 0; i_v < v->degree; i_v++) {
        Vertex* u = v->neighbors[i_v];
        for(size_t i_u = 0; i_u < u->degree; i_u++) {
            if(u->neighbors[i_u] == v) {
                u->neighbors[i_u] = u->neighbors[u->degree - 1]; // move the last arr elem here
                u->degree--;                                     // shorten the array by one
                u->neighbors_array_is_sorted = false;
                break;
            }
            assert(i_u < u->degree - 1); // assert there are more edges if we didn't find it already
            // this assertion would fail if v has an edge to any u which does not have an edge to v
        }
    }
    g->m -= v->degree;
    v->degree = 0;
    if(v->neighbors != NULL) {
        free(v->neighbors);
        v->neighbors = NULL;
    }
}



// Removes the vertex from the graph, including deleting its edges and updating g->n and g->m.
// This function does however not free v and does not delete it from the doubly linked vertex list, so
// one can call this function on v and still iterate on the list using v->list_next and v->list_prev normally afterwards.
static void _mark_vertex_removed(Graph* g, Vertex* v)
{
    assert(v != NULL);
    g->n--;
    v->status = REMOVED;
    _remove_edges(g, v);
}



// does the pointer stuff to delete v from the vertices list and frees it afterwards
// does not update g->n
// must be called on a vertex only if it has beed marked removed using _mark_vertex_removed before
static void _delete_and_free_vertex(Graph* g, Vertex* v)
{
    assert(g != NULL && v != NULL);
    if(v->status != REMOVED) {
        assert(false);
        _mark_vertex_removed(g, v);
    }
    if(v->list_prev == NULL) { // if it is the head of the list
        g->vertices = v->list_next;
    }
    else {                                      // not the list head
        v->list_prev->list_next = v->list_next; // may be NULL
    }
    if(v->list_next != NULL) { // if it is not the last list element
        v->list_next->list_prev = v->list_prev;
    }
    assert(v->neighbors == NULL);
    free(v);
}



static void _mark_neighbors_dominated(Vertex* v)
{
    assert(v != NULL);
    for(size_t i = 0; i < v->degree; i++) {
        Vertex* u = v->neighbors[i];
        u->status = DOMINATED;
    }
}



// checks if conditions of at least one of the "extra rules" on page 22 of the paper applies.
// returning false does not necessarily mean it cannot be removed, but that the simple rules do not
// imply that it is redundant.
// u: the vertex to check if has become redundant and can also be removed
static bool _is_redundant(Vertex* u)
{
    assert(u && u->status == DOMINATED);
    size_t count_undominated_neighbors = 0;
    Vertex* undominated_neighbors[4];
    for(size_t i = 0; i < u->degree; i++) {
        if(u->neighbors[i]->status == UNDOMINATED) {
            if(count_undominated_neighbors >= 3) { // if this one is the 4th undominated neighbor that was found
                return false; // there is no check for the case count_undominated_neighbors >= 4 later, so no need to continue
            }
            undominated_neighbors[count_undominated_neighbors++] = u->neighbors[i];
        }
    }

    if(count_undominated_neighbors <= 1) { // extra rule (1): delete a white vertex of degree zero or one
        return true;
    }
    // extra rule (2): delete a white vertex of degree two if its neighbors are at
    // distance at most two from each other (after the the removal of v)
    else if(count_undominated_neighbors == 2) {
        Vertex* x = undominated_neighbors[0];
        Vertex* y = undominated_neighbors[1];
        _sort_neighbors(x);
        _sort_neighbors(y);
        size_t i_x = 0, i_y = 0;
        while(i_x < x->degree && i_y < y->degree) {
            if((x->neighbors[i_x] == y) || (y->neighbors[i_y] == x)) { // if x and y are direct neighbors
                return true;
            }
            if(x->neighbors[i_x] < y->neighbors[i_y]) {
                i_x++;
            }
            else if(x->neighbors[i_x] > y->neighbors[i_y]) {
                i_y++;
            }
            else { // if x->neighbors[i_x] == y->neighbors[i_y]
                if(x->neighbors[i_x] != u) {
                    return true; // x and y have a common neighbor that is not u
                }
                i_x++; // if the common neighbor is u
                i_y++;
            }
        }
    }
    // extra rule (3): delete a white vertex of degree three if the subgraph induced by its neighbors is connected.
    else if(count_undominated_neighbors == 3) {
        bool neighbors_connected[3] = {false, false, false}; // to mark which of the vertices in undominated_neighbors is connected to at least one of the others
        for(size_t i_neigh = 0; i_neigh < 3; i_neigh++) {
            Vertex* x = undominated_neighbors[i_neigh];
            for(size_t i_x = 0; i_x < x->degree; i_x++) {
                assert(x->neighbors[i_x] != x); // no self loop
                if(x->neighbors[i_x] == undominated_neighbors[0])
                    neighbors_connected[0] = true;
                if(x->neighbors[i_x] == undominated_neighbors[1])
                    neighbors_connected[1] = true;
                if(x->neighbors[i_x] == undominated_neighbors[2])
                    neighbors_connected[2] = true;
            }
        }
        return neighbors_connected[0] && neighbors_connected[1] && neighbors_connected[2];
    }
    return false;
}



// creates a new Vertex holding id and inserts it at the start of g's fixed list
static void _add_id_to_fixed(Graph* g, uint_fast32_t id)
{
    Vertex* v = malloc(sizeof(Vertex));
    if(!v) {
        perror("_add_id_to_fixed: malloc failed");
        exit(1);
    }
    v->id = id;
    v->neighbors = NULL;
    v->degree = 0;
    v->status = DOMINATED;

    v->list_prev = NULL;
    v->list_next = g->fixed;
    if(g->fixed != NULL) {
        g->fixed->list_prev = v;
    }
    g->fixed = v;
}



// v has to be a vertex somewhere in the vertex list g->vertices
// will mark v as removed and may mark some or all neighbors of v as removed, if they become redundant
static void _fix_vertex_and_mark_removed(Graph* g, Vertex* v)
{
    assert(g != NULL && v != NULL);
    _add_id_to_fixed(g, v->id);
    _mark_neighbors_dominated(v);

    if(v->degree != 0) {
        // save the array of neighbors
        size_t count_neighbors = v->degree;
        Vertex** neighbors = malloc(v->degree * sizeof(Vertex*));
        if(!neighbors) {
            perror("rule_1_reduce_vertex: calloc failed");
            exit(1);
        }
        memcpy(neighbors, v->neighbors, v->degree * sizeof(Vertex*));

        _mark_vertex_removed(g, v); // after this point, v->degree == 0 and v->neighbors == NULL

        for(size_t i = 0; i < count_neighbors; i++) {
            if(_is_redundant(neighbors[i])) {
                _mark_vertex_removed(g, neighbors[i]);
            }
        }
        free(neighbors);
    }
    else {
        _mark_vertex_removed(g, v);
    }
}



// iterate over all vertices in the graph, looking for isolated vertices
// and support vertices (vertices that are the only connection of a leaf).
// if one is found, fix and remove it, then continue.
// returns the number of vertices that were fixed and removed (there could
// be vertices that were removed, but only the fixed ones – not redundant ones –
// are counted).
size_t fix_isol_and_supp_vertices(Graph* g)
{
    assert(g != NULL);
    size_t count_fixed = 0;
    bool another_loop = true;
    while(another_loop) { // TODO: even on really sparse graphs, very few additional vertices are found when
        // re-checking everything, so it might not be worth the computation time. But on the other hand, it
        // does not take much computation time and even small improvements now could be very benefitial later.
        another_loop = false;
        Vertex* next;
        for(Vertex* v = g->vertices; v != NULL; v = next) {
            next = v->list_next; // save this pointer before v may be freed
            if(v->status == REMOVED) {
                _delete_and_free_vertex(g, v);
                continue;
            }
            if(v->degree == 0) {
                if(v->status == UNDOMINATED) {
                    // v is an isolated vertex, so next will definitely be safe
                    _fix_vertex_and_mark_removed(g, v);
                    count_fixed++;
                }
                else {
                    assert(false); // if this occurres, then something else went wrong before
                    _mark_vertex_removed(g, v);
                }
                // since deg(v)=0, fixing and removing v does not affect any other vertex, so we can delete and free immediately,
                // potentially avoiding a re-run. Only possible since v->list_next has already been saved.
                _delete_and_free_vertex(g, v);
            }
            else if(v->degree == 1) {
                if(v->status == UNDOMINATED) {
                    _fix_vertex_and_mark_removed(g, v->neighbors[0]);
                    another_loop = true;
                    count_fixed++;
                }
                else {
                    _mark_vertex_removed(g, v);
                    another_loop = true;
                }
            }
        }
    }
    return count_fixed;
}



// helper function for rule_1_reduce_vertex.
// must only be called if the neighbors arrays of both u and v are sorted
// (by pointer, ascending)
// returns true iff u is in N1(v), i.e. iff u has any neighbor that is not
// a neighbor of v.
static bool _is_in_n1_sorted(Vertex* v, Vertex* u)
{
    assert(v != NULL && u != NULL);
    size_t i_u = 0, i_v = 0;
    while(i_u < u->degree) {
        if(u->neighbors[i_u] == v) {
            i_u++;
        }
        else if(i_v >= v->degree) { // reached end of neighbors of v, but still more of u
            return true;
        }
        else if(u->neighbors[i_u] > v->neighbors[i_v]) {
            i_v++;
        }
        else if(u->neighbors[i_u] == v->neighbors[i_v]) {
            i_u++;
            i_v++;
        }
        // if this point is reached, then u->neighbors[i_u] is not a neighbor of v
        else {
            return true;
        }
    }
    return false;
}



// must only be called if both arrays are sorted (by pointer, ascending).
// returns true iff the arrays have no Vertex* in common
static bool _set_intersection_is_empty_sorted(Vertex** arr_a, size_t n_a, Vertex** arr_b, size_t n_b)
{
    assert(arr_a != 0 || n_a == 0);
    assert(arr_b != 0 || n_b == 0);
    size_t i_a = 0, i_b = 0;
    while(i_a < n_a && i_b < n_b) {
        if(arr_a[i_a] < arr_b[i_b]) {
            i_a++;
        }
        else if(arr_a[i_a] > arr_b[i_b]) {
            i_b++;
        }
        else { // arr_a[i_a] == arr_b[i_b]
            return false;
        }
    }
    return true;
}



// function for rule 1 of the paper
// returns true iff it was reduced
bool rule_1_reduce_vertex(Graph* g, Vertex* v)
{
    assert(g != NULL && v != NULL);
    assert(v->status != REMOVED);
    if(v->degree == 0) {
        if(v->status == UNDOMINATED) {
            _fix_vertex_and_mark_removed(g, v);
        }
        else {
            _mark_vertex_removed(g, v);
        }
        return true;
    }
    if(v->degree == 1) { // handling degree == 1 separately is redundant but might result in a slight speed up. TODO: test
        if(v->status == UNDOMINATED) {
            _fix_vertex_and_mark_removed(g, v->neighbors[0]);
        }
        else {
            _mark_vertex_removed(g, v);
        }
        return true;
    }

    // setup
    Vertex** n1 = calloc(v->degree * 3, sizeof(Vertex*));
    if(!n1) {
        perror("rule_1_reduce_vertex: calloc failed");
        exit(1);
    }
    Vertex** n2 = &(n1[v->degree]);
    Vertex** n3 = &(n2[v->degree]);
    size_t count_n1 = 0, count_n2 = 0, count_n3 = 0;
    // dividing neighbors into the three sets
    _sort_neighbors(v);
    for(size_t i = 0; i < v->degree; i++) {
        Vertex* u = v->neighbors[i];
        _sort_neighbors(u);
        if(_is_in_n1_sorted(v, u)) {
            n1[count_n1++] = u;
        }
        else {
            n2[count_n2++] = u; // for now put in n2, decide later if it is n2 or n3
        }
    }
    for(size_t i = 0; i < count_n2; i++) {
        Vertex* u = n2[i];
        // already dominated vertices have to be in N1 or N2
        if(u->status == UNDOMINATED && _set_intersection_is_empty_sorted(u->neighbors, u->degree, n1, count_n1)) {
            n3[count_n3++] = u;
            n2[i] = n2[count_n2 - 1]; // move the last elem here
            count_n2--;
            i--; // stay here to handle the newly moved elem next
        }
    }
    if(count_n3 > 0) {
        // v can be rule-1-reduced, now do it
        for(size_t i = 0; i < count_n2; i++) {
            n2[i]->status = DOMINATED; // not REALLY necessary but for the assertion
            _mark_vertex_removed(g, n2[i]);
        }
        for(size_t i = 0; i < count_n3; i++) {
            n3[i]->status = DOMINATED;
            _mark_vertex_removed(g, n3[i]);
        }
        _fix_vertex_and_mark_removed(g, v);
        free(n1);
        return true;
    }
    free(n1);
    return false;
}



//temporary test
size_t rule_1_reduce(Graph* g)
{
    size_t count_fixed = 0;
    bool another_loop = true;
    while(another_loop) { // TODO: even on really sparse graphs, very few additional vertices are found when
        // re-checking everything, so it might not be worth the computation time. But on the other hand, it
        // does not take much computation time and even small improvements now could be very benefitial later.
        another_loop = false;
        Vertex* next;
        for(Vertex* v = g->vertices; v != NULL; v = next) {
            next = v->list_next;
            if(v->status == REMOVED) {
                _delete_and_free_vertex(g, v);
                continue;
            }
            else if(v->status == DOMINATED && _is_redundant(v)) {
                _mark_vertex_removed(g, v);
            }
            else if(rule_1_reduce_vertex(g, v)) {
                another_loop = true;
                count_fixed++;
            }
        }
    }
    fprintf(stderr, "rule_1_reduce: count_fixed = %zu\n", count_fixed);
    return count_fixed;
}



// TODO: function for rule 2 of the paper
