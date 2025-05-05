#include "graph.h"

#include <stdlib.h>
#include <assert.h>



typedef struct Edge {
    Vertex* a;
    Vertex* b;
} Edge;



// helper
static void _graph_free_vertex_list(Vertex* list_head)
{
    Vertex* v = list_head;
    while(v != NULL) {
        Vertex* to_free = v;
        v = v->list_next;
        if(to_free->neighbors != NULL) {
            free(to_free->neighbors);
        }
        free(to_free);
    }
}

// free graph, all of its vertices and their internals
// g must not be NULL
void graph_free(Graph* g)
{
    assert(g);
    _graph_free_vertex_list(g->vertices);
    _graph_free_vertex_list(g->fixed);
    g->vertices = NULL;
    g->fixed = NULL;
    free(g);
}



// may only be called if the allocated space of u->neighbors and v->neighbors is large enough to hold the additional edge
static void _graph_add_edge(Vertex* u, Vertex* v)
{
    assert(u != NULL && v != NULL);
    assert(u->neighbors != NULL && v->neighbors != NULL);
    u->neighbors[u->degree++] = v;
    v->neighbors[v->degree++] = u;
}



// caller is responsible for freeing using graph_free(...)
Graph* graph_parse(FILE* file)
{
    Graph* g = calloc(1, sizeof(Graph));
    if(!g) {
        return NULL;
    }

    char c = (char)fgetc(file);
    while(c == 'c') { // while the first char of a line is 'c', skip that entire line
        for(c = (char)fgetc(file); c != '\n'; c = (char)fgetc(file)) { // empty loop
            assert(c != EOF);
        }
        c = (char)fgetc(file); // get first char of next line
    }
    assert(c == 'p'); // first char of the first non-comment line should be a p
    c = (char)fgetc(file);
    assert(c == ' ');
    c = (char)fgetc(file);
    assert(c == 'd');
    c = (char)fgetc(file);
    assert(c == 's');
    c = (char)fgetc(file);
    assert(c == ' ');

    uintmax_t tmp_n, tmp_m;
    size_t n, m;
    fscanf(file, "%" SCNuMAX "%" SCNuMAX "\n", &tmp_n, &tmp_m); // n and m
    if((tmp_n > (uintmax_t)SIZE_MAX) || (tmp_m > (uintmax_t)SIZE_MAX)) {
        fprintf(stderr, "graph_parse_stdin: size_t is not large enough to hold number of vertices/edges.");
        exit(1);
    }
    n = (size_t)tmp_n;
    m = (size_t)tmp_m;
    g->n = n;
    g->m = m;

    // make a temporary array in order to efficiently access the vertices by their id
    Vertex** tmp_vertex_arr = calloc(n + 1, sizeof(Vertex*));
    Edge* edges = calloc(m, sizeof(Edge));           // temporary array for the edges
    size_t* degrees = calloc(n + 1, sizeof(size_t)); // temporary array to keep track of the degrees
    if(tmp_vertex_arr == NULL || edges == NULL || degrees == NULL) {
        perror("graph_parse_stdin: allocating temporary array failed");
        exit(1);
    }
    for(size_t id = 1; id < n + 1; id++) {
        if(!(tmp_vertex_arr[id] = calloc(1, sizeof(Vertex)))) {
            perror("graph_parse_stdin: allocating memory for a single Vertex failed");
            exit(1);
        }
        tmp_vertex_arr[id]->id = id; // initialize all non-zero data of the vertex
        tmp_vertex_arr[id]->status = UNDOMINATED;
    }
    // link them to form a doubly linked list
    g->vertices = tmp_vertex_arr[1];
    for(size_t i = 1; i < n; i++) {
        tmp_vertex_arr[i]->list_next = tmp_vertex_arr[i + 1];
        tmp_vertex_arr[i + 1]->list_prev = tmp_vertex_arr[i];
    }

    // continue parsing
    for(size_t i = 0; i < m; i++) {
        size_t u_id, v_id;
        if(fscanf(file, "\t%zu %zu\n", &u_id, &v_id) != 2) {
            assert(false);
        }
        edges[i].a = tmp_vertex_arr[u_id];
        edges[i].b = tmp_vertex_arr[v_id];
        degrees[u_id]++;
        degrees[v_id]++;
    }
    for(size_t id = 1; id <= n; id++) {
        if(degrees[id] > 0) {
            if(!(tmp_vertex_arr[id]->neighbors = malloc(degrees[id] * sizeof(Vertex*)))) {
                perror("graph_parse_stdin: allocating neighbors array of a vertex failed");
                exit(1);
            }
        }
    }
    for(size_t i = 0; i < m; i++) {
        _graph_add_edge(edges[i].a, edges[i].b);
    }

    free(tmp_vertex_arr);
    free(edges);
    free(degrees);
    return g;
}



// debug function
// graph_name is optional and can be NULL
// dominated vertices will green, fixed verticed will be cyan, and removed vertices will be red
void graph_print_as_dot(Graph* g, bool include_fixed, const char* graph_name)
{
    assert(g);
    printf("graph %s {", graph_name ? graph_name : "G");
    for(Vertex* v = g->vertices; v != NULL; v = v->list_next) {
        printf("\n\t%zu", v->id);
        switch(v->status) {
            case DOMINATED:
                printf("[style=filled, fillcolor=green]");
                break;
            case REMOVED:
                printf("[style=filled, fillcolor=red]");
                break;
            default:
                break;
        }
    }
    for(Vertex* v = g->fixed; include_fixed && v != NULL; v = v->list_next) {
        printf("\n\t%zu[style=filled, fillcolor=cyan]", v->id);
    }
    for(Vertex* v = g->vertices; v != NULL; v = v->list_next) {
        for(size_t i = 0; i < v->degree; i++) {
            Vertex* u = v->neighbors[i];
            if(u->id >= v->id) { // doesnt matter if we check for >= or <=, we just don't want both directions to be printed
                printf("\n\t%zu -- %zu", v->id, u->id);
            }
        }
    }
    for(Vertex* v = g->fixed; include_fixed && v != NULL; v = v->list_next) {
        for(size_t i = 0; i < v->degree; i++) {
            Vertex* u = v->neighbors[i];
            if(u->id >= v->id) { // doesnt matter if we check for >= or <=, we just don't want both directions to be printed
                printf("\n\t%zu -- %zu", v->id, u->id);
            }
        }
    }
    printf("\n}\n");
}
