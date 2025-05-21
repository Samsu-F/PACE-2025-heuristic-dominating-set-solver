#ifndef _GREEDY_H
#define _GREEDY_H

#include "graph.h"
#include "pqueue.h"
#include "dynamic_array.h"



DynamicArray greedy(Graph* g);

DynamicArray greedy_random(Graph* g);


void greedy_random_remove_and_refill(Graph* g, DynamicArray* ds, double removal_probability);



#endif
