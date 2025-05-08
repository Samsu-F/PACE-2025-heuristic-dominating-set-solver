#include "reduction.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include <stdio.h> // DEBUG



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
    assert(v->status != REMOVED); // wouldn't be a problem but it's a sign something went wrong
    if(v->status != REMOVED) {
        g->n--;
        v->status = REMOVED;
        _remove_edges(g, v);
    }
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



// returns true iff (set Intersection of N[u] over all u in vertices) \ {ignore_v, ignore_w} is non_empty.
// ignore_v and ignore_w must not be in vertices.
// may change neighbor tags in the neighborhood of all u in vertices, and those of ignore_v and ignore_w.
// ignore_v and ignore_w may be NULL if no or only one vertex needs to be ignored
static bool _common_neighbor_exists(Vertex** vertices, size_t arr_size, Vertex* ignore_v, Vertex* ignore_w)
{
    assert(vertices != NULL);
    if(arr_size <= 1) {
        return true;
    }
    Vertex* u0 = vertices[0];
    u0->neighbor_tag = u0->id;
    for(size_t i = 0; i < u0->degree; i++) {
        u0->neighbors[i]->neighbor_tag = u0->id;
    }
    if(ignore_v != NULL) { // disqualify v and w
        ignore_v->neighbor_tag = 0;
    }
    if(ignore_w != NULL) {
        ignore_w->neighbor_tag = 0;
    }
    size_t prev_id = u0->id;
    for(size_t i_vertices = 1; i_vertices < arr_size; i_vertices++) {
        Vertex* u = vertices[i_vertices];
        bool common_neighbor_found = false;
        for(size_t i_u = 0; i_u < u->degree; i_u++) {
            if(u->neighbors[i_u]->neighbor_tag == prev_id) { // neighbor shared with all previous u in vertices (including u0)
                common_neighbor_found = true;
                u->neighbors[i_u]->neighbor_tag = u->id;
            }
            else {
                u->neighbors[i_u]->neighbor_tag = 0;
            }
        }
        if(u->neighbor_tag == prev_id) { // u is itself a shared neighbor with all previous u in vertices (including u0)
            common_neighbor_found = true;
            u->neighbor_tag = u->id;
        }
        else {
            u->neighbor_tag = 0;
        }
        prev_id = u->id;
        if(!(common_neighbor_found)) {
            return false;
        }
    }
    return true;
}



// checks if conditions of at least one of the "extra rules" on page 22 of the paper applies.
// returning false does not necessarily mean it cannot be removed, but that the simple rules do not
// imply that it is redundant.
// u: the vertex to check if has become redundant and can also be removed
static bool _is_redundant(Vertex* u)
{
    assert(u && u->status == DOMINATED);
    size_t count_undominated_neighbors = 0;
    Vertex** undominated_neighbors = malloc(u->degree * sizeof(Vertex*));
    if(!undominated_neighbors) {
        perror("_is_redundant: malloc failed");
        exit(1);
    }
    for(size_t i = 0; i < u->degree; i++) {
        if(u->neighbors[i]->status == UNDOMINATED) {
            undominated_neighbors[count_undominated_neighbors++] = u->neighbors[i];
        }
    }
    bool result = _common_neighbor_exists(undominated_neighbors, count_undominated_neighbors, u, NULL);
    free(undominated_neighbors);
    return result;
}



// creates a new Vertex holding id and inserts it at the start of g's fixed list
static void _add_id_to_fixed(Graph* g, size_t id)
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
    g->count_fixed++;
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
            perror("_fix_vertex_and_mark_removed: malloc failed");
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



// v and w have to be somewhere in the vertex list g->vertices
// will mark v and w as removed and may mark some or all neighbors of v or w as removed, if they become redundant after removing both v and w
static void _fix_vertices_and_mark_removed(Graph* g, Vertex* v, Vertex* w)
{
    assert(g != NULL && v != NULL && w != NULL);
    _add_id_to_fixed(g, v->id);
    _add_id_to_fixed(g, w->id);
    _mark_neighbors_dominated(v);
    _mark_neighbors_dominated(w);

        // save the array of neighbors
        size_t count_neighbors = v->degree + w->degree;
        Vertex** neighbors = malloc(count_neighbors * sizeof(Vertex*));
        if(!neighbors) {
            perror("_fix_vertices_and_mark_removed: malloc failed");
            exit(1);
        }
        memcpy(neighbors, v->neighbors, v->degree * sizeof(Vertex*));
        memcpy(&(neighbors[v->degree]), w->neighbors, w->degree * sizeof(Vertex*));

        _mark_vertex_removed(g, v); // after this point, v->degree == 0 and v->neighbors == NULL
        _mark_vertex_removed(g, w); // after this point, w->degree == 0 and w->neighbors == NULL

        for(size_t i = 0; i < count_neighbors; i++) {
            if(neighbors[i]->status != REMOVED && _is_redundant(neighbors[i])) {
                _mark_vertex_removed(g, neighbors[i]);
            }
        }
        free(neighbors);
}



// helper function for _rule_1_reduce_vertex.
// must only be called if the neighbor tags of v and any neighbor of v were set to v->id
// returns non-zero iff u is in N1(v), i.e. iff u has any neighbor that is not
// a neighbor of v.
// returns 1 iff u is strictly in N1, and returns 2 iff u may be put in N2 because
// every neighbor of u that is not a neighbor of v is already dominated.
// However, if this returns 2 for u, then u must NOT be put in N3 because it may still get
// dominated from the outside neighbor later.
static int _is_in_n1_rule1(const size_t v_id, const Vertex* const u)
{
    assert(u != NULL);
    bool dominated_outside_neighbor_found = false;
    for(size_t i = 0; i < u->degree; i++) {
        if(u->neighbors[i]->neighbor_tag != v_id) {
            if(u->neighbors[i]->status == UNDOMINATED) {
                return true;
            }
            else {
                dominated_outside_neighbor_found = true;
            }
        }
    }
    if(dominated_outside_neighbor_found) {
        return 2;
    }
    return 0;
}



// helper function for _rule_2_reduce_vertices.
// must only be called if the neighbor tags of v and any neighbor of v were set to v->id, and likewise for w.
// returns non-zero iff u is in N1(v, w), i.e. iff u has any neighbor that is neither
// a neighbor of v nor a neighbor of w.
// returns 1 iff u is strictly in N1, and returns 2 iff u may be put in N2 because
// every neighbor of u that is not a neighbor of v or w is already dominated.
// However, if this returns 2 for u, then u must NOT be put in N3 because it may still get
// dominated from the outside neighbor later.
static int _is_in_n1_rule2(const size_t v_id, const size_t w_id, const Vertex* const u)
{
    assert(u != NULL);
    assert(v_id != w_id && u->id != v_id && u->id != w_id);
    bool dominated_outside_neighbor_found = false;
    for(size_t i = 0; i < u->degree; i++) {
        if(u->neighbors[i]->neighbor_tag != v_id && u->neighbors[i]->neighbor_tag != w_id) {
            if(u->neighbors[i]->status == UNDOMINATED) {
                return true;
            }
            else {
                dominated_outside_neighbor_found = true;
            }
        }
    }
    if(dominated_outside_neighbor_found) {
        return 2;
    }
    return 0;
}



// helper function for _rule_1_reduce_vertex.
// must only be called if x->neighbor_tag of any neighbor x of v is v_id iff x in N1(v).
// v->neighbor_tag must be set to 0 before calling this function.
// returns true iff u is in N2(v), i.e. iff u has any neighbor that is in N1(v).
static bool _is_in_n2_rule1(const size_t v_id, const Vertex* const u)
{
    assert(u != NULL);
    if(u->status != UNDOMINATED) {
        return true; // only undominated vertices can be in N3
    }
    for(size_t i = 0; i < u->degree; i++) {
        if(u->neighbors[i]->neighbor_tag == v_id) { // if u has any neighbor that is in N1
            assert(u->neighbors[i]->id != v_id); // assert the N1 neighbor found is in fact not v itself
            return true;
        }
    }
    return false;
}



// helper function for _rule_2_reduce_vertices.
// must only be called if x->neighbor_tag of any x in N(v, w) is in {v_id, w_id} iff x in N1(v, w).
// v->neighbor_tag and w->neighbor_tag must be set to 0 before calling this function.
// returns true iff u is in N2(v, w), i.e. iff u has any neighbor that is in N1(v, w).
static bool _is_in_n2_rule2(const size_t v_id, const size_t w_id, const Vertex* const u)
{
    assert(u != NULL);
    assert(v_id != w_id && u->id != v_id && u->id != w_id);
    if(u->status != UNDOMINATED) {
        return true; // only undominated vertices can be in N3
    }
    for(size_t i = 0; i < u->degree; i++) {
        // if u has any neighbor that is in N1(v, w)
        if(u->neighbors[i]->neighbor_tag == v_id || u->neighbors[i]->neighbor_tag == w_id) {
            // assert the N1 neighbor found is in fact not v or w itself
            assert(u->neighbors[i]->id != v_id && u->neighbors[i]->id != w_id);
            return true;
        }
    }
    return false;
}



// function for rule 1 of the paper
// returns true iff it was reduced
static bool _rule_1_reduce_vertex(Graph* g, Vertex* v)
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
    Vertex** n2_only = malloc(2 * v->degree * sizeof(Vertex*)); // block allocation for n2_only and n2_n3_mixed
    if(!n2_only) {
        perror("_rule_1_reduce_vertex: malloc failed");
        exit(1);
    }
    Vertex** n2_n3_mixed = &(n2_only[v->degree]);
    size_t count_n2_only = 0, count_n2_n3_mixed = 0; // the number of elements in the arrays

    v->neighbor_tag = v->id;
    for(size_t i = 0; i < v->degree; i++) {
        Vertex* u = v->neighbors[i];
        u->neighbor_tag = v->id;
    }
    for(size_t i_v = 0; i_v < v->degree; i_v++) {
        Vertex* u = v->neighbors[i_v];
        switch(_is_in_n1_rule1(v->id, u)) {
            case 0:
                // not in N1 but still unknown if N2 or N3
                n2_n3_mixed[count_n2_n3_mixed++] = u; // for now put it there, decide later if it is N2 or N3
                break;
            case 2:
                n2_only[count_n2_only++] = u; // may be put in N2 but must not be put in N3
                break;
            default: // in N1, nothing to do
                break;
        }
    }
    // now split N2 and N3
    // first tag N2 and N3 differently from N1
    for(size_t i = 0; i < count_n2_only; i++) {
        Vertex* u = n2_only[i];
        u->neighbor_tag = u->id;
    }
    for(size_t i = 0; i < count_n2_n3_mixed; i++) {
        Vertex* u = n2_n3_mixed[i];
        u->neighbor_tag = u->id;
    }
    v->neighbor_tag = 0;
    bool reduce = false; // the flag that saves if we reduce at the end
    if((count_n2_only + count_n2_n3_mixed == v->degree) && v->status == UNDOMINATED) {
        // v needs to be dominated by itself or a neighbor, but all neighbors are at best equally good or worse choices than v
        reduce = true;
    }
    else {
        // check if N3 is not empty
        for(size_t i = 0; i < count_n2_n3_mixed; i++) {
            Vertex* u = n2_n3_mixed[i];
            if(!(_is_in_n2_rule1(v->id, u))) {
                // we found a vertex that is in N3 ==> N3 is not empty
                reduce = true;
                break; // no need to continue, we only need to know if N3 is empty
            }
        }
    }


    if(reduce) {
        // v can be rule-1-reduced, now do it
        for(size_t i = 0; i < count_n2_only; i++) {
            _mark_vertex_removed(g, n2_only[i]);
        }
        for(size_t i = 0; i < count_n2_n3_mixed; i++) {
            _mark_vertex_removed(g, n2_n3_mixed[i]);
        }
        _fix_vertex_and_mark_removed(g, v);
        free(n2_only);
        return true;
    }
    free(n2_only);
    return false;
}



// returns true iff vertices is a subset of N(v)
// undefined result if v itself is in vertices
// may change neighbor tags in the neighborhood of v
static bool _is_subset_of_neighborhood(Vertex** vertices, size_t arr_size, Vertex* v)
{
    assert(vertices != NULL && v != NULL);
    for(size_t i = 0; i < v->degree; i++) {
        v->neighbors[i]->neighbor_tag = v->id;
    }
    for(size_t i = 0; i < arr_size; i++) {
        if(vertices[i]->neighbor_tag != v->id) {
            return false;
        }
    }
    return true;
}



static bool _rule_2_reduce_vertices(Graph* g, Vertex* v, Vertex* w)
{
    bool result = false;
    assert(g != NULL && v != NULL && w != NULL);
    assert(v->status != REMOVED && w->status != REMOVED);
    assert(v != w && v->id != w->id);
    // assert(v->degree >= 2 && w->degree >= 2);  ///// DEBUG, put it back ///// DEBUG, put it back ///// DEBUG, put it back ///// DEBUG, put it back ///// DEBUG, put it back ///// DEBUG, put it back /////

    // setup
    Vertex** n2 = malloc(2 * (v->degree + w->degree) * sizeof(Vertex*)); // block allocation for n2 and n3
    if(!n2) {
        perror("_rule_2_reduce_vertices: malloc failed");
        exit(1);
    }
    Vertex** n3 = &(n2[v->degree + w->degree]);
    size_t count_n2 = 0, count_n3 = 0; // the number of elements in the arrays

    // tag all vertices in N[v,w]
    w->neighbor_tag = w->id; // tag w 's neighbors first, then those of v
    for(size_t i = 0; i < w->degree; i++) {
        w->neighbors[i]->neighbor_tag = w->id;
    }
    v->neighbor_tag = v->id;
    for(size_t i = 0; i < v->degree; i++) {
        v->neighbors[i]->neighbor_tag = v->id;
    }
    bool v_and_w_are_adjacent = (w->neighbor_tag == v->id);

    size_t count_n1 = 0;
    for(size_t i = 0; i < v->degree; i++) {
        Vertex* u = v->neighbors[i];
        if(u == v || u == w) {
            continue;
        }
        switch(_is_in_n1_rule2(v->id, w->id, u)) {
            case 0:
                // not in N1 but still unknown if N2 or N3
                n3[count_n3++] = u; // for now put it in n3, decide later if it is N2 or N3
                break;
            case 2:
                n2[count_n2++] = u; // may be put in N2 but must not be put in N3
                break;
            default: // in N1
                count_n1++;
                break;
        }
    }
    for(size_t i = 0; i < w->degree; i++) {
        Vertex* u = w->neighbors[i];
        if(u == v || u == w || u->neighbor_tag != w->id) { // if u's tag is v's id, it is a neighbor of both v and w and was already tested
            continue;
        }
        switch(_is_in_n1_rule2(v->id, w->id, u)) {
            case 0:
                // not in N1 but still unknown if N2 or N3
                n3[count_n3++] = u; // for now put it in n3, decide later if it is N2 or N3
                break;
            case 2:
                n2[count_n2++] = u; // may be put in N2 but must not be put in N3
                break;
            default: // in N1
                count_n1++;
                break;
        }
    }


    // now split N2 and N3
    // first tag N2 and N3 differently from N1
    for(size_t i = 0; i < count_n2; i++) {
        Vertex* u = n2[i];
        u->neighbor_tag = u->id;
    }
    for(size_t i = 0; i < count_n3; i++) {
        Vertex* u = n3[i];
        u->neighbor_tag = u->id;
    }
    v->neighbor_tag = 0;
    w->neighbor_tag = 0;
    for(size_t i = 0; i < count_n3; i++) {
        Vertex* u = n3[i];
        if(_is_in_n2_rule2(v->id, w->id, u)) {
            n2[count_n2++] = u;
            n3[i] = n3[--count_n3]; // move the last elem here
            i--;                    // stay here to handle the newly moved elem next
        }
    }
    v->neighbor_tag = v->id; // ensure it is a valid value at the end
    w->neighbor_tag = w->id;

    // for rule 2, N3(v, w) being non-empty does not imply that we can reduce, we need to do some more testing
    if(count_n3 > 0 && !(_common_neighbor_exists(n3, count_n3, v, w))) {
        bool v_alone_dominates_n3 = _is_subset_of_neighborhood(n3, count_n3, v);
        bool w_alone_dominates_n3 = _is_subset_of_neighborhood(n3, count_n3, w);

        bool remove_n3 = false;
        bool remove_n2_v = false; // whether the intersection of N2(v,w) and N(v) should be removed
        bool remove_n2_w = false; // whether the intersection of N2(v,w) and N(w) should be removed
        if(v_alone_dominates_n3 && w_alone_dominates_n3) { // case 1.1 of the paper
            // TODO: test if it is worth it to do anything here
            assert(fprintf(stderr, "rule 2 case 1.1 found, v->id == %zu,\tw->id == %zu\t==> do nothing\t\tcount_n2 == %zu, count_n3 == %zu\n",
                           v->id, w->id, count_n2, count_n3));
        }
        else if(v_alone_dominates_n3) { // case 1.2
            assert(!w_alone_dominates_n3);
            assert(fprintf(stderr, "rule 2 case 1.2 found, v->id == %zu,\tw->id == %zu\t==> fix v\n",
                           v->id, w->id));
            remove_n3 = true;
            remove_n2_v = true;
            _fix_vertex_and_mark_removed(g, v);
        }
        else if(w_alone_dominates_n3) { // case 1.3
            assert(fprintf(stderr, "rule 2 case 1.3 found, v->id == %zu,\tw->id == %zu\t==> fix w\n",
                           v->id, w->id));
            assert(!v_alone_dominates_n3);
            remove_n3 = true;
            remove_n2_w = true;
            _fix_vertex_and_mark_removed(g, w);
        }
        else { // case 2: neither v alone nor w alone dominates N3
            assert(fprintf(stderr, "rule 2 case 2 found, v->id == %zu,\tw->id == %zu\t==> fix v and w\n",
                           v->id, w->id));
            assert((!v_alone_dominates_n3) && (!w_alone_dominates_n3));
            remove_n3 = true;
            remove_n2_v = true;
            remove_n2_w = true;
            _fix_vertices_and_mark_removed(g, v, w);
        }
        if(count_n1 == 0 && (!v_and_w_are_adjacent)) { // intentionally not an else if
            // case I found myself, not from the paper. Isolated component consisting of just this neighborhood N[v, w]
            if(v->status == UNDOMINATED && w->status == UNDOMINATED) {
                remove_n3 = true;
                remove_n2_v = true;
                remove_n2_w = true;
                _fix_vertices_and_mark_removed(g, v, w);
            }
            else if(v->status == UNDOMINATED) {
                _fix_vertex_and_mark_removed(g, v);
                remove_n2_v = true;
            }
            else if(w->status == UNDOMINATED) {
                remove_n2_w = true;
                _fix_vertex_and_mark_removed(g, w);
            }
        }


        if(remove_n3) {
            for(size_t i = 0; i < count_n3; i++) {
                if(n3[i]->status != REMOVED) {
                    _mark_vertex_removed(g, n3[i]);
                }
            }
        }
        if(remove_n2_v) {
            for(size_t i = 0; i < v->degree; i++) {
                v->neighbors[i]->neighbor_tag = v->id;
            }
            for(size_t i = 0; i < count_n2; i++) {
                if(n2[i]->status != REMOVED && n2[i]->neighbor_tag == v->id) {
                    _mark_vertex_removed(g, n2[i]);
                }
            }
        }
        if(remove_n2_w) {
            for(size_t i = 0; i < w->degree; i++) {
                v->neighbors[i]->neighbor_tag = w->id;
            }
            for(size_t i = 0; i < count_n2; i++) {
                if(n2[i]->status != REMOVED && n2[i]->neighbor_tag == w->id) {
                    _mark_vertex_removed(g, n2[i]);
                }
            }
        }
        result = remove_n3 || remove_n2_v || remove_n2_w;
    }
    free(n2);
    return result;
}



void reduce(Graph* g, float time_budget_total, float time_budget_rule2)
{
    assert(time_budget_total >= time_budget_rule2);
    const clock_t start_time = clock();
    const clock_t deadline_total = start_time + (clock_t)(time_budget_total * CLOCKS_PER_SEC);
    const clock_t deadline_rule2 = start_time + (clock_t)(time_budget_rule2 * CLOCKS_PER_SEC);
    const clock_t deadline_redundant = start_time + (clock_t)(1.1 * time_budget_total * CLOCKS_PER_SEC);
    size_t loop_iteration = 0;
    bool time_remaining_total = true, time_remaining_rule2 = true, time_remaining_redundant = true;

    bool another_loop = true;
    while(another_loop) { // TODO: even on really sparse graphs, very few additional vertices are found when
        // re-checking everything, so it might not be worth the computation time. But on the other hand, it
        // does not take much computation time and even small improvements now could be very benefitial later.
        another_loop = false;
        Vertex* next;
        for(Vertex* v = g->vertices; v != NULL; v = next) {
            next = v->list_next;

            if((loop_iteration++ % 256) == 0) {
                clock_t current_time = clock();
                time_remaining_total = current_time < deadline_total;
                time_remaining_rule2 = current_time < deadline_rule2;
                time_remaining_redundant = current_time < deadline_redundant;
            }

            if(v->status == REMOVED) {
                _delete_and_free_vertex(g, v);
                continue;
            }
            if(!time_remaining_redundant) {
                continue;
            }
            else if(v->status == DOMINATED && _is_redundant(v)) {
                _mark_vertex_removed(g, v);
                another_loop = true;
                continue;
            }
            if(!time_remaining_total) {
                continue;
            }
            else if(_rule_1_reduce_vertex(g, v)) {
                another_loop = true;
                continue;
            }

            if(time_remaining_rule2) {
                // TODO: I think this is inefficient but every other way of doing it that I have tried so far was slower
                for(size_t i = 0; v->status != REMOVED && i < v->degree;) {
                    Vertex* u1 = v->neighbors[i++];
                    if(u1->status != REMOVED && _rule_2_reduce_vertices(g, v, u1)) {
                        another_loop = true;
                        i--; // stay at this index
                        continue;
                    }
                    for(size_t j = i; v->status != REMOVED && j < v->degree; j++) {
                        Vertex* u2 = v->neighbors[j];
                        assert(u1 != u2 && u1 != v && u2 != v);
                        if(u1->status != REMOVED && u2->status != REMOVED &&
                           _rule_2_reduce_vertices(g, u1, u2)) {
                            another_loop = true;
                            i = 0;
                            break;
                        }
                    }
                }
            }
        }
    }
}
