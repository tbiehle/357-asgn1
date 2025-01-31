#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
typedef unsigned char BYTE;
#define PAGESIZE 4096

void *startofheap;

struct chunk // memory element
{ 
    void *next, *prev; // next and previous memory elements
    int size; // size of block
    int occ; // 0 if empty; 1 if occupied
};


chunk *get_last_chunk()
{
    if (!startofheap) return NULL;
    chunk *current = (chunk *)startofheap;
    while (current->next) 
        current = (chunk *)current->next;
    return current;
}


BYTE *mymalloc(int size)
{
    static int start = 1;
    void *pb = sbrk(0); // get the current program break

    int total_size = size + sizeof(chunk); // calculate total size of memory segment
    int correct_size = ((total_size + PAGESIZE - 1) / PAGESIZE) * PAGESIZE;

    if (!startofheap) // first time in mymalloc
    {
        start = 0;

        pb = sbrk(correct_size); // increment the program break 

        chunk *c = (chunk *) pb;
        c->next = c->prev = NULL;
        c->size = correct_size;
        c->occ = 1;

        startofheap = c;
        return (BYTE *) (c + 1);
    } 
    else // memory already been allocated, must find space or create it
    {
        chunk *curr = (chunk *) startofheap;
        int min_size = curr->size;
        chunk *c = NULL;
        while (curr) {
            if (curr->occ == 0 && curr->size >= correct_size) 
            { // found suitable chunk
                if (curr->size < min_size)
                    c = curr;
                    min_size = curr->size;
            }
            curr = (chunk *) curr->next;
        }

        if (c) // best fit chunk exists
        {
            c->occ = 1;
            // check if chunk needs to be split
            // step 1: calculate difference between correct_size and the size of the current block.
            int size_diff = c->size - correct_size;
            // step 2: see if we need to split the block, i.e. the difference is bigger than a page size.
            if (size_diff > PAGESIZE) // might need to check if this should be >=
            { // we have too much room, need to split.
                // step 2a: make a new chunk.
                chunk * new_chunk;
                // step 2b: increment the pointer by the size of the block.
                new_chunk = (chunk *)((BYTE *)c + correct_size); // this makes sure it gets incremented by the proper amount
                new_chunk->size = size_diff; // sets the size of the new chunk to the remaining size
                new_chunk->occ = 0; // say the new chunk is available for allocation
                new_chunk->next = c->next; // insert the new chunk into linked list of chunks
                new_chunk->prev = c; 
                if (c->next) { // if current if current chunk has a chunk after it
                    ((chunk *)c->next)->prev = new_chunk; // insert the new chunk in between
                }
                c->next = new_chunk;
                c->size = correct_size;
            }
            return (BYTE *) (c + 1);
        }

        // no chunk is free
        pb = sbrk(correct_size);
        c = (chunk *) pb;
        c->next = NULL;
        c->occ = 1;
        c->size = correct_size;

        // attach block to end of heap
        chunk *last = get_last_chunk();
        last->next = c;
        c->prev = last;

        return (BYTE *) (c + 1);
    }
}


BYTE *myfree(BYTE *addr)
{
    if (!addr) {
        return NULL;
    }

    chunk *fchunk = (chunk *) addr - 1;
    fchunk->occ = 0;

    while (fchunk->next && ((chunk*)fchunk->next)->occ == 0) // continuously merge with next empty chunk
    {
        fchunk->size += ((chunk*)fchunk->next)->size;
        fchunk->next = ((chunk*)fchunk->next)->next;
        if (fchunk->next)
            ((chunk*)fchunk->next)->prev = fchunk;
    }

    while (fchunk->prev && ((chunk*)fchunk->prev)->occ == 0) // continuously merge with previous empty chunk
    {
        chunk * prev_chunk = (chunk*)fchunk->prev;
        prev_chunk->size += fchunk->size;
        prev_chunk->next = ((chunk*)fchunk->next);

        if (fchunk->next)
            ((chunk*)fchunk->next)->prev = prev_chunk;
        
        fchunk = prev_chunk;
    }

    if (!fchunk->next)
    { // chunk is last chunk in list
        chunk *curr = fchunk;
        int size = curr->size;
        if (curr->prev && ((chunk*)curr->prev)->occ == 0) // don't need to iterate since ideally that whole chunk would be merged
        {
            size += ((chunk*)curr->prev)->size;
        } else
        {
            startofheap = NULL;
        }
        sbrk(-size); // release memory back to the system
    }

    return NULL;
}


void analyze()
{
    printf("\n--------------------------------------------------------------\n");
    if(!startofheap)
    {
        printf("no heap, program break on address: %x\n", sbrk(0));
        return;
    }

    chunk* ch = (chunk*)startofheap;
    for (int no=0; ch; ch = (chunk*)ch->next,no++)
    {
        printf("%d | current addr: %p |", no, (void *)ch);
        printf("size: %d | ", ch->size);
        printf("info: %d | ", ch->occ);
        printf("next: %p | ", (void *)ch->next);
        printf("prev: %p", (void *)ch->prev);
        printf(" \n");
    }
    printf("program break on address: %x\n", sbrk(0));
}

int main()
{
    BYTE *a[100];
    analyze(); //50% points

    for(int i=0;i<100;i++)
        a[i]= mymalloc(1000);
    for(int i=0;i<90;i++)
        myfree(a[i]);
    analyze(); //50% of points if this is correct

    myfree(a[95]);
    a[95] = mymalloc(1000);
    analyze(); //25% points, this new chunk should fill the smaller free one (best fit)

    for(int i=90;i<100;i++)
        myfree(a[i]);
    analyze(); // 25% - should be an empty heap now with the start address from the program start
    return 0;
}