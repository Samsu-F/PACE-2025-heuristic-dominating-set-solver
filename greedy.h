#ifndef _GREEDY_H
#define _GREEDY_H

#include <stdlib.h>
#include <assert.h>

#include "graph.h"
#include "pqueue.h"


// TODO: not sure if this struct is acutally a good idea
typedef struct {
    Vertex** arr;
    uint32_t size;
    uint32_t allocated_size;
} VertexArray;



VertexArray greedy(Graph* g);



#endif
