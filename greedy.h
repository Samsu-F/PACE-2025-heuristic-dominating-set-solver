#ifndef _GREEDY_H
#define _GREEDY_H

#include "graph.h"
#include "pqueue.h"
#include "dynamic_array.h"



size_t greedy(Graph* g);

size_t greedy_remove_and_refill(Graph* g, double removal_probability, size_t current_ds_size);



#endif
