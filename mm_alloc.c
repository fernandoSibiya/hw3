/*
  mm_alloc.c

 Stub implementations of the mm_* routines. Remove this comment and provide
 a summary of your allocator's design here.
 */
#include "mm_alloc.h"
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

#define ASSERT     assert
#define align4(x)  (((((x)-1)>>2)<<2)+4)
#define TRUE 1
#define FALSE 0

s_block_ptr base = NULL;            
s_block_ptr last;
s_block_ptr findBlock(size_t size)
{  
    s_block_ptr p = base;
    for (; p != NULL; p = p->next)
    {
        last = p;
        if (p->free && p->size >= size)
            return p;
    }
    return NULL;
}
// extending break point with sbrk
s_block_ptr extend_heap(s_block_ptr last, size_t s)
{    
    // Current break is address of the new block
    s_block_ptr new_block;
    new_block = (s_block_ptr)sbrk(0);
    int sb = (int)sbrk(s + BLOCK_SIZE);
    if (sb < 0)
    {
        return (NULL);
    }
    new_block->size = s;
    new_block->ptr  = new_block->data;
    new_block->free = TRUE;

    if (base == NULL) // heap is empty
    {
        new_block->prev = NULL;
        base = new_block;
    }
    else
    {
        ASSERT(last->next == NULL);
        new_block->next = NULL;
        new_block->prev = last;
        last->next = new_block;
    }
    return new_block;
}

void split_block(s_block_ptr p, size_t new_size)
{
    s_block_ptr new_block = NULL;
    if (p->size >= new_size + BLOCK_SIZE + 4)
    {
        // making small sized blocks to new_size
        p->size = new_size;

        // define a new block on the address p->data + size
        new_block = (s_block_ptr)(p->data + new_size);
        new_block->size = p->size - new_size - BLOCK_SIZE;
        new_block->ptr = new_block->data;
        new_block->free = TRUE;

        // insert the new block after block p
        new_block->next = p->next;
        new_block->prev = p;
        if (p->next)
            p->next->prev = new_block;
        p->next = new_block;
    }

}

s_block_ptr fusion(s_block_ptr pb)
{
    ASSERT(pb->free == TRUE);
    // merging p and q if p's next block exists and is free

    if (pb->next && pb->next->free)
    {
        pb->size = pb->size + BLOCK_SIZE + pb->next->size;
        if (pb->next->next)
            pb->next->next->prev = pb;
        pb->next = pb->next->next;
    }

    return (pb);
}


s_block_ptr get_block(void *p){
    // If p is a valid block address, convert it to a s_block_ptr
    char *tmp;
    tmp = (char*)p;
    return (s_block_ptr)(tmp - BLOCK_SIZE);
}

static void copyBlock(s_block_ptr src, s_block_ptr dst){
    int *sdata, *ddata;
    size_t i;
    sdata = src->ptr;
    ddata = dst->ptr;
    for (i = 0; i * 4 < src->size && i * 4 < dst->size; i++)
        ddata[i] = sdata[i];
}

//*******************************Interfaces
void* mm_malloc(size_t size){
#ifdef MM_USE_STUBS
    return calloc(1, size);
#else
    // Align the requested size
    size_t s = align4(size);
    s_block_ptr pb;

    // no block allocated yet
    if (base == NULL)
    {
        pb = extend_heap(last,s);
        if (pb == NULL)
            return NULL;
        base = pb;
    }
    // if already allocated blocks
    else
    {
        // find first fitting
        pb = findBlock(s);
        if (pb == NULL)
        {
            pb = extend_heap(last,s);
            if (pb == NULL)
                return NULL;
        }
        else
        {
            if (pb->size - s >= BLOCK_SIZE + 4)
                split_block(pb, s);
        }
    }

    pb->free = FALSE;
    return pb->ptr;
#endif
}

void* mm_realloc(void* ptr, size_t size){
#ifdef MM_USE_STUBS
    return realloc(ptr, size);
#else
    size_t s;
    void *newp;
    s_block_ptr pb, new;
    // exclude some special conditions
    if (ptr == NULL)
        return mm_malloc(size);
    
    if ((pb=get_block(ptr)) != NULL)
    {
        s = align4(size);
        if (pb->size >= s)
        {
            // If space is enough, try to split current block
            if (pb->size - s >= (BLOCK_SIZE + 4))
                split_block(pb, s);
        }
    }
    else
    {
        // Try fusion with next if possible
        if (pb->next && pb->next->free
                && (pb->size + BLOCK_SIZE + pb->next->size) >= s)
        {
            fusion(pb);
            if (pb->size - s >= BLOCK_SIZE + 4)
                split_block(pb, s);
        }
        else
        {
            // no way to get enough space on current node, have to malloc new memory
            newp = mm_malloc(s);
            if (newp == NULL)
                return NULL;
            new = get_block(newp);
            copyBlock(pb, new);
            mm_free(pb);
            return (newp);
        }
        return (pb);
    }
    return (NULL);
#endif
}

void mm_free(void* ptr){
#ifdef MM_USE_STUBS
    free(ptr);
#else
    s_block_ptr pb;
    // If the pointer ptr is valid and get a non-NULL block address
    if ((pb=get_block(ptr)) != NULL)
    {
        pb->free = TRUE;
        // Step backward and fusion two blocks
        if(pb->prev && pb->prev->free)
            fusion(pb->prev);
        // Also try fusion with the next block
        if(pb->next)
            fusion(pb);
        // Else we are the last pointer, free the end of heap
        else
        {
            // If we are also the head pointer, no more block
            if (pb->prev == NULL)
                base = NULL;
            // Else release the end of heap
            else
                pb->prev->next = NULL;
            brk(pb);
        }
    }
    
#endif
}
