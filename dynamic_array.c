#include "dynamic_array.h"


#include <assert.h>
#include <stdio.h>


// returns true if successful
// returns false and leaves da unchanged if unsuccessful
bool da_init(DynamicArray* da, size_t initial_capacity)
{
    assert(da != NULL);
    struct Vertex** tmp = calloc(initial_capacity, sizeof(struct Vertex*));
    if(!tmp) {
        return false;
    }
    da->vertices = tmp;
    da->size = 0;
    da->capacity = initial_capacity;
    return true;
}



static void _da_incr_capacity(DynamicArray* da)
{
    assert(da != NULL);
    assert(da->size <= da->capacity && da->size >= da->capacity - 1);
    size_t new_capacity = 2 * da->capacity;
    struct Vertex** new_ptr = realloc(da->vertices, new_capacity * sizeof(struct Vertex*));
    if(!new_ptr) {
        perror("dynamic array: Reallocating to increase capacity failed.\n");
        // careful if you decide to not exit here: da->vertices is still allocated
        exit(EXIT_FAILURE);
    }
    da->vertices = new_ptr;
    da->capacity = new_capacity;
}



void da_add(DynamicArray* da, struct Vertex* new_elem)
{
    assert(da != NULL && new_elem != NULL);
    if(da->size == da->capacity) {
        _da_incr_capacity(da);
    }
    assert(da->size < da->capacity);
    da->vertices[da->size++] = new_elem;
}



void da_free_internals(DynamicArray* da)
{
    assert(da != NULL);
    free(da->vertices);
    da->vertices = NULL;
    da->size = 0;
    da->capacity = 0;
}
