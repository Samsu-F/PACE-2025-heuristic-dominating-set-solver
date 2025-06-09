#ifndef _GREEDY_H
#define _GREEDY_H

#include "graph.h"
#include "pqueue.h"



// runs iterated greedy algorithm on the graph until a sigterm signal is received.
// v->is_in_ds must be set to false for all vertices before calling this function.
// returns the number of vertices in the dominating set.
size_t iterated_greedy_solver(Graph* g);



#endif
