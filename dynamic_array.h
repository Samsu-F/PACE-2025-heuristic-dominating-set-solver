#ifndef _DYNAMICARRAY_H
#define _DYNAMICARRAY_H

#include <stdlib.h>
#include <stdbool.h>



struct Vertex;



typedef struct {
    struct Vertex** vertices;
    size_t size;
    size_t capacity;
} DynamicArray;



// returns true if successful
// returns false and leaves da unchanged if unsuccessful
bool da_init(DynamicArray* da, size_t initial_capacity);



void da_add(DynamicArray* da, struct Vertex* new_elem);



void da_free_internals(DynamicArray* da);



#endif