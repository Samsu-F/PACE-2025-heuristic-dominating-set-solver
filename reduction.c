#include "reduction.h"

#include <stdlib.h>
#include <assert.h>

#include <stdio.h> // DEBUG



// does the pointer stuff to remove v from the vertices list
// does not update g->n
// does not free v or alter any of its data apart from list_next and list_prev
static void _remove_from_vertices(Graph* g, Vertex* v)
{
    assert(g != NULL && v != NULL);
    if(v->list_prev == NULL) { // if it is the head of the list
        g->vertices = v->list_next;
    }
    else {                                      // not the list head
        v->list_prev->list_next = v->list_next; // may be NULL
    }
    if(v->list_next != NULL) { // if it is not the last list element
        v->list_next->list_prev = v->list_prev;
    }
}



// does the pointer stuff to move v from the vertices list to the fixed list
// does not update g->n
static void _move_to_fixed(Graph* g, Vertex* v)
{
    assert(g != NULL && v != NULL);
    _remove_from_vertices(g, v);

    // add to list of fixed vertices
    v->list_prev = NULL;
    v->list_next = g->fixed;
    if(g->fixed != NULL) {
        g->fixed->list_prev = v;
    }
    g->fixed = v;
}



static void _mark_neighbors_dominated(Vertex* v)
{
    assert(v != NULL);
    for(size_t i = 0; i < v->degree; i++) {
        Vertex* u = v->neighbors[i];
        u->status = DOMINATED;
    }
}



// removes all edges of v in both directions, and frees the then empty neighbors array of v
static void _remove_edges(Vertex* v)
{
    assert(v != NULL);
    for(size_t i_v = 0; i_v < v->degree; i_v++) {
        Vertex* u = v->neighbors[i_v];
        for(size_t i_u = 0; i_u < u->degree; i_u++) {
            if(u->neighbors[i_u] == v) {
                u->neighbors[i_u] = u->neighbors[u->degree - 1]; // move the last arr elem here
                u->degree--;                                     // shorten the array by one
                break;
            }
            assert(i_u < u->degree - 1); // assert there are more edges if we didn't find it already
            // this assertion would fail if v has an edge to any u which does not have an edge to v
        }
    }
    v->degree = 0;
    free(v->neighbors);
    v->neighbors = NULL;
}



// v has to be a vertex somewhere in the vertex list g->vertices
// v has to be a dominated vertex
static void _remove_redundant_vertex(Graph* g, Vertex* v)
{
    assert(v->status == DOMINATED);
    assert(g != NULL && v != NULL);
    g->n--;
    _remove_from_vertices(g, v);
    g->m -= v->degree;
    _remove_edges(v);
    free(v);
}



// checks if conditions of at least one of the "extra rules" on page 22 of the paper applies.
// returning false does not necessarily mean it cannot be removed, but that the simple rules do not
// imply that it is redundant.
// v: the vertex that will be fixed and removed
// u: the vertex to check if it becomes redundant and can also be removed
static bool _is_redundant_neighbor(Vertex* v, Vertex* u)
{
    // important: this function is called before removing v from the graph, therefore the
    // degree of u is still 1 higher than it will be. Thus, the degrees here are by one greater
    // than the ones in the paper.
    if(u->degree <= 2) { // extra rule (1): delete a white vertex of degree zero or one
        return true;
    }
    // extra rule (2): delete a white vertex of degree two if its neighbors are at
    // distance at most two from each other (after the the removal of v)
    if(u->degree == 3) {
        Vertex* x = u->neighbors[0] != v ? u->neighbors[0] : u->neighbors[1];
        Vertex* y = u->neighbors[2] != v ? u->neighbors[2] : u->neighbors[1];

        // TODO: make the following check for common neighbors more efficient, this is O(n^2)
        for(size_t i_y = 0; i_y < y->degree; i_y++) {
            for(size_t i_x = 0; i_x < x->degree; i_x++) {
                if((x->neighbors[i_x] == y) || (y->neighbors[i_y] == x) || // if x and y are direct neighbors or
                   (x->neighbors[i_x] == y->neighbors[i_y] && x->neighbors[i_x] != u &&
                    x->neighbors[i_x] != u)) { // if they have a common neighbor apart from  u and v
                    return true;
                }
            }
        }
    }
    // extra rule (3): delete a white vertex of degree three if the subgraph induced by its neighbors is connected.
    if(u->degree == 4) {
        Vertex* neighbors[3];
        for(size_t i = 0, i_neigh = 0; i < u->degree; i++) {
            if(u->neighbors[i] != v) {
                neighbors[i_neigh++] = u->neighbors[i];
            }
        } // now neighbors is filled with the neighbors of u that are not v
        bool neighbors_connected[3] = {false, false, false}; // to mark which of the vertices in neighbors is connected to at least one of the others
        for(size_t i_neigh = 0; i_neigh < 3; i_neigh++) {
            Vertex* x = neighbors[i_neigh];
            for(size_t i_x = 0; i_x < x->degree; i_x++) {
                assert(x->neighbors[i_x] != x); // no self loop
                if(x->neighbors[i_x] == neighbors[0])
                    neighbors_connected[0] = true;
                if(x->neighbors[i_x] == neighbors[1])
                    neighbors_connected[1] = true;
                if(x->neighbors[i_x] == neighbors[2])
                    neighbors_connected[2] = true;
            }
        }
        return neighbors_connected[0] && neighbors_connected[1] && neighbors_connected[2];
    }
    return false;
}



// implements the "extra rules" on page 22 of the paper.
// v: the vertex that will be fixed and removed.
// may only be called after marking the neighbors of v as dominated.
static void _remove_redundant_neighbors(Graph* g, Vertex* v)
{
    assert(g != NULL && v != NULL);
    for(size_t i = 0; i < v->degree; i++) {
        for(size_t j = 0; j < v->degree; j++) {
        }
        for(size_t j = 0; j < v->degree; j++) {
        }
        Vertex* u = v->neighbors[i];
        if(_is_redundant_neighbor(v, u)) {
            _remove_redundant_vertex(g, u);
            i--; // removing u causes this array of neighbors of v to change
        }
    }
}



// v has to be a vertex somewhere in the vertex list g->vertices
// will remove v and may remove some or all neighbors of v, if they become redundant
void fix_and_remove_vertex(Graph* g, Vertex* v)
{
    assert(g != NULL && v != NULL);
    g->n--;
    v->status = DOMINATED;
    _mark_neighbors_dominated(v);
    _remove_redundant_neighbors(g, v); // will affect v->degree
    g->m -= v->degree; // must not be done before _remove_redundant_neighbors!
    _remove_edges(v);
    _move_to_fixed(g, v);
}



// helper function for fix_isol_and_supp_vertices:
// beginning from search_start, go backwards until finding a vertex that will
// definitely not be also removed if to_remove is fixed and removed (i.e. a vertex that
// is not a neighbor of to_remove or has degree >= 5).
// returns NULL if no such vertex is found.
static Vertex* _find_safe_list_elem(Vertex* to_remove, Vertex* search_start)
{
    for(Vertex* v = search_start; v != NULL; v = v->list_prev) {
        if(v == to_remove) {
            continue; // definitely not safe, will be removed
        }
        if(v->degree >= 5) {
            return v;
        }
        else {
            bool not_a_neighbor_of_to_remove = true;
            for(size_t i = 0; i < v->degree; i++) {
                if(v->neighbors[i] == to_remove) {
                    not_a_neighbor_of_to_remove = false;
                    break;
                }
            }
            if(not_a_neighbor_of_to_remove) {
                return v;
            }
        }
    }
    return NULL;
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
            next = v->list_next; // save this pointer before v may be removed
            if(v->degree == 0) {
                // v is an isolated vertex, so next will definitely be safe
                fix_and_remove_vertex(g, v);
                count_fixed++;
                // fixing and removing v does not affect any other vertex, so no need to re-run
            }
            else if(v->degree == 1) {
                next = _find_safe_list_elem(v->neighbors[0], v->list_next);
                fix_and_remove_vertex(g, v->neighbors[0]);
                count_fixed++;
                another_loop = true;
            }
        }
    }
    return count_fixed;
}



// Comparison function for qsort
static int _compare_Vertex_ptrs(const void* a, const void* b)
{
    const Vertex* x = *(const Vertex* const*)a;
    const Vertex* y = *(const Vertex* const*)b;
    return (x > y) - (x < y);
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
        if(i_v >= v->degree) { // reached end of neighbors of v, but still more of u
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
        else if(u->neighbors[i_u] == v) {
            i_u++;
        }
        else {
            return true;
        }
    }
    return false;
}



// must only be called if both arrays are sorted (by pointer, ascending).
// returns true iff the arrays have no Vertex* in common, apart from v
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
    if(v->degree == 0) {
        fix_and_remove_vertex(g, v);
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
    qsort(v->neighbors, v->degree, sizeof(Vertex*), _compare_Vertex_ptrs);
    for(size_t i = 0; i < v->degree; i++) {
        Vertex* u = v->neighbors[i];
        qsort(u->neighbors, u->degree, sizeof(Vertex*), _compare_Vertex_ptrs);
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
            _remove_redundant_vertex(g, n2[i]);
        }
        for(size_t i = 0; i < count_n3; i++) {
            n3[i]->status = DOMINATED;
            _remove_redundant_vertex(g, n3[i]);
        }
        fix_and_remove_vertex(g, v);
        free(n1);
        return true;
    }
    free(n1);
    return false;
}



// TODO: function for rule 2 of the paper
