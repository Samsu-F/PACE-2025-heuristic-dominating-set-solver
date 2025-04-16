#ifndef _REDUCTION_H
#define _REDUCTION_H

#include "graph.h"


// implementation of the (slightly modified) reduction rules presented in
// J. Alber, M. R. Fellows, R. Niedermeier. Polynomial Time Data Reduction for Dominating Set
// arXiv:cs/0207066v1



// v has to be a vertex somewhere in the vertex list g->vertices
void fix_and_remove_vertex(Graph* g, Vertex* v);



// iterate over all vertices in the graph, looking for isolated vertices
// and support vertices (vertices that are the only connection of a leaf).
// if one is found, fix and remove it, then continue.
// returns the number of vertices that were fixed and removed (there could
// be vertices that were removed, but only the fixed ones – not redundant ones –
// are counted).
size_t fix_isol_and_supp_vertices(Graph* g);



#endif
