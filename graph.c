#include "graph.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>



typedef struct Edge {
    Vertex* a;
    Vertex* b;
} Edge;



// free graph, all of its vertices and their internals
// g must not be NULL
void graph_free(Graph* g)
{
    assert(g);
    for(uint32_t vertices_idx = 0; vertices_idx < g->n; vertices_idx++) {
        Vertex* v = g->vertices[vertices_idx];
        if(v->neighbors != NULL) {
            free(v->neighbors);
        }
        free(v);
    }
    free(g->vertices);
    g->vertices = NULL;
    for(size_t fixed_idx = 0; fixed_idx < g->fixed.size; fixed_idx++) {
        Vertex* v = g->fixed.vertices[fixed_idx];
        assert(v->neighbors == NULL);
        free(v);
    }
    da_free_internals(&(g->fixed));
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
    if(!da_init(&(g->fixed), 128)) { // initial_capacity = 128 is arbitrary but seems reasonable
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
    uint32_t n, m;
    fscanf(file, "%" SCNuMAX "%" SCNuMAX "\n", &tmp_n, &tmp_m); // n and m
    if((tmp_n > (uintmax_t)UINT32_MAX) || (tmp_m > (uintmax_t)UINT32_MAX)) {
        fprintf(stderr, "graph_parse_stdin: uint32_t is not large enough to hold number of vertices/edges.");
        exit(EXIT_FAILURE);
    }
    n = (uint32_t)tmp_n;
    m = (uint32_t)tmp_m;
    g->n = n;
    g->m = m;

    // make a temporary array in order to efficiently access the vertices by their id
    Vertex** tmp_vertex_arr_by_id = calloc(n + 1, sizeof(Vertex*));
    g->vertices = malloc(n * sizeof(Vertex*));
    Edge* edges = calloc(m, sizeof(Edge));               // temporary array for the edges
    uint32_t* degrees = calloc(n + 1, sizeof(uint32_t)); // temporary array to keep track of the degrees
    if(tmp_vertex_arr_by_id == NULL || g->vertices == NULL || edges == NULL || degrees == NULL) {
        perror("graph_parse_stdin: allocating array failed");
        exit(EXIT_FAILURE);
    }

    for(uint32_t id = 1; id < n + 1; id++) {
        if(!(tmp_vertex_arr_by_id[id] = calloc(1, sizeof(Vertex)))) {
            perror("graph_parse_stdin: allocating memory for a single Vertex failed");
            exit(EXIT_FAILURE);
        }
        tmp_vertex_arr_by_id[id]->id = id; // initialize all non-zero data of the vertex
    }

    // continue parsing
    for(uint32_t i = 0; i < m; i++) {
        uint32_t u_id, v_id;
        if(fscanf(file, "\t%" SCNu32 " %" SCNu32 "\n", &u_id, &v_id) != 2) {
            assert(false);
        }
        edges[i].a = tmp_vertex_arr_by_id[u_id];
        edges[i].b = tmp_vertex_arr_by_id[v_id];
        degrees[u_id]++;
        degrees[v_id]++;
    }
    for(uint32_t id = 1; id <= n; id++) {
        if(degrees[id] > 0) {
            if(!(tmp_vertex_arr_by_id[id]->neighbors = malloc((size_t)degrees[id] * sizeof(Vertex*)))) {
                perror("graph_parse_stdin: allocating neighbors array of a vertex failed");
                exit(EXIT_FAILURE);
            }
        }
    }
    for(uint32_t i = 0; i < m; i++) {
        _graph_add_edge(edges[i].a, edges[i].b);
    }

    memcpy(g->vertices, &(tmp_vertex_arr_by_id[1]), (size_t)n * sizeof(Vertex*));
    free(tmp_vertex_arr_by_id);
    free(edges);
    free(degrees);
    return g;
}



// debug function
// graph_name is optional and can be NULL
// dominated vertices will green, fixed verticed will be cyan, and removed vertices will be red
void graph_print_as_dot(Graph* g, const bool include_fixed, const char* graph_name)
{
    assert(g);
    printf("graph %s {", graph_name ? graph_name : "G");
    for(uint32_t vertices_idx = 0; vertices_idx < g->n; vertices_idx++) {
        Vertex* v = g->vertices[vertices_idx];
        printf("\n\t%" PRIu32 "", v->id);
        if(v->dominated_by_number > 0) {
            printf("[style=filled, fillcolor=green]");
        }
    }
    for(size_t fixed_idx = 0; include_fixed && fixed_idx < g->fixed.size; fixed_idx++) {
        Vertex* v = g->fixed.vertices[fixed_idx];
        printf("\n\t%" PRIu32 "[style=filled, fillcolor=cyan]", v->id);
    }
    for(uint32_t vertices_idx = 0; vertices_idx < g->n; vertices_idx++) {
        Vertex* v = g->vertices[vertices_idx];
        for(uint32_t i = 0; i < v->degree; i++) {
            Vertex* u = v->neighbors[i];
            if(u->id >= v->id) { // doesnt matter if we check for >= or <=, we just don't want both directions to be printed
                printf("\n\t%" PRIu32 " -- %" PRIu32 "", v->id, u->id);
            }
        }
    }
    for(size_t fixed_idx = 0; include_fixed && fixed_idx < g->fixed.size; fixed_idx++) {
        Vertex* v = g->fixed.vertices[fixed_idx];
        for(uint32_t i = 0; i < v->degree; i++) {
            Vertex* u = v->neighbors[i];
            if(u->id >= v->id) { // doesnt matter if we check for >= or <=, we just don't want both directions to be printed
                printf("\n\t%" PRIu32 " -- %" PRIu32 "", v->id, u->id);
            }
        }
    }
    printf("\n}\n");
}
