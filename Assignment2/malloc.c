#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define ALIGN4(s)         (((((s) - 1) >> 2) << 2) + 4)  //making s next multiple of 4
#define BLOCK_DATA(b)      ((b) + 1)
#define BLOCK_HEADER(ptr)   ((struct _block *)(ptr) - 1)


static int atexit_registered = 0;
static int num_mallocs       = 0;
static int num_frees         = 0;
static int num_reuses        = 0;
static int num_grows         = 0;
static int num_splits        = 0;
static int num_coalesces     = 0;
static int num_blocks        = 0;
static int num_requested     = 0;
static int max_heap          = 0;

/*
 *  \brief printStatistics
 *
 *  \param none
 *
 *  \Prints the heap statistics upon process exit.  Registered
 *  via atexit()
 *
 *  \return none
 */
void printStatistics( void )
{
    printf("\nheap management statistics\n");
    printf("mallocs:\t%d\n", num_mallocs );
    printf("frees:\t\t%d\n", num_frees );
    printf("reuses:\t\t%d\n", num_reuses );
    printf("grows:\t\t%d\n", num_grows );
    printf("splits:\t\t%d\n", num_splits );
    printf("coalesces:\t%d\n", num_coalesces );
    printf("blocks:\t\t%d\n", num_blocks );
    printf("requested:\t%d\n", num_requested );
    printf("max heap:\t%d\n", max_heap );
}

struct _block
{
    size_t  size;         /* Size of the allocated _block of memory in bytes */
    struct _block *prev;  /* Pointer to the previous _block of allcated memory   */
    struct _block *next;  /* Pointer to the next _block of allcated memory   */
    bool   free;          /* Is this _block free?                     */
    char   padding[3];
};

struct _block *freeList = NULL, *last =NULL; /* Free list to track the _blocks available */

/*
 * \brief findFreeBlock
 *
 * \param last pointer to the linked list of free _blocks
 * \param size size of the _block needed in bytes
 *
 * \return a _block that fits the request or NULL if no free _block matches
 *
 * \Uses First Fit, Next Fit, Best Fit or Worst Fit to find the free _block
 */
struct _block *findFreeBlock(size_t size)
{
    printf("in findFreeBlock\n");
    struct _block *curr = freeList;

#if defined FIT && FIT == 0
    /* First fit */
    printf("in first fit\n");
    while (curr && !(curr->free && curr->size >= size))
    {
        last = curr;
        curr  = curr->next;
    }
#endif

#if defined BEST && BEST == 0
    /* Best fit searches all free blocks and assigns
    min size block that is greater then requested size*/
    printf("in BEST fit\n");
    struct _block *best = NULL;
    size_t best_size = (size_t)-1;
    printf("best : %p\n",best);
    printf("curr : %p\n",curr);
    while (curr)
    {
        printf("curr : %p\n",curr);
        if (curr->free && (curr->size >= size) && (curr->size < best_size))
        {
            printf("in best if: %p\n",best);
            best = curr;
            printf("best : %p\n",best);
            best_size = best->size;
            printf("bs : %zu\n",best_size);
        }
        if (curr->next)
        {
            curr = curr->next;
        }
        else
        {
            return(best);
        }
    }
#endif

#if defined WORST && WORST == 0
    /* Worst fit - searches all free blocks and assigns
    max size block that is greater then requested size*/

    struct _block *worst = NULL;
    size_t worst_size = (size_t)0;
    while (curr)
    {
        if (curr->free && curr->size >= size && curr->size > worst_size)
        {
            worst = curr;
            worst_size = worst->size;
        }
        if (curr->next)
        {
            curr = curr->next;
        }
        else
        {
            return(worst);
        }
    }
#endif

#if defined NEXT && NEXT == 0
    /* Next fit */
    printf("in next fit\n");
    if (last)
    {
        if (last->next)  // start search from last assigned memory
        {
            curr = last->next;
        }
        else
        {
            curr = freeList;
        }
    }
    else // return NULL if heap is not assigned.
    {
        return(NULL);
    }
    printf("last : %p\n",last);
    printf("curr : %p\n",curr);
    while (!(curr->free && curr->size >= size))
    {
        printf("curr : %p\n",curr);
        printf("size : %zu\n",curr->size);
        printf("size of block : %zu\n",sizeof(struct _block));
        printf("curr + 100 : %p\n", curr+100);
        printf("curr block data : %p\n",BLOCK_DATA(curr));
        printf("curr block data + 100 : %p\n",BLOCK_DATA(curr)+100);
        printf("curr + 100 : %p\n", ((void *)curr) +1032);
        if (curr->next)
        {
            printf("currNext : %p\n",curr->next);
            curr  = curr->next;
        }
        else
        {
            curr = freeList;
        }
        if (curr == last) // return NULL if free memory not found after a cycle.
        {
            return(NULL);
        }
    }
#endif
    printf("leaving findFreeBlock\n");
    return curr;
}

/*
 * \brief growheap
 *
 * Given a requested size of memory, use sbrk() to dynamically
 * increase the data segment of the calling process.  Updates
 * the free list with the newly allocated memory.
 *
 * \param size size in bytes to request from the OS
 *
 * \return returns the newly allocated _block, NULL if failed
 */
struct _block *growHeap(size_t size)
{
    printf("in growheap\n");
    /* Request more space from OS */
    struct _block *curr = (struct _block *)sbrk(0);
    struct _block *prev = (struct _block *)sbrk(sizeof(struct _block) + size);

    assert(curr == prev);

    /* OS allocation failed */
    if (curr == (struct _block *)-1)
    {
        return NULL;
    }

    /* Update freeList if not set */
    if (freeList == NULL)
    {
        freeList = curr;
    }

    /* Attach new _block to prev _block */
    if (last)
    {
        last->next = curr;
    }

    /* Update _block metadata */
    curr->size = size;
    curr->next = NULL;
    curr->free = false;
    curr->prev = last;
    printf("leaving growHeap\n");
    num_grows++;
    num_blocks++;
    max_heap += (sizeof(struct _block) + size);
    return curr;
}

/*
 * \brief split
 *
 * Given a struct _block  and size, splits the block and makes struct _block as size 'size'
    and creates new block for the remaining size.
 *
 * \param curr - struct _block address that needs to be split
 * \param size - size in bytes that needs to be allocated to curr.
 *  size should be less then then curr->size - sizeof(struct _block)
 *
 * \return returns the newly allocated _block, NULL if failed
 */
void split(struct _block *curr, size_t size)
{
    printf("curr : %p\n",curr);
    printf("Csize : %zu\n",curr->size);
    struct _block *next = (((void *)curr) + (sizeof(struct _block) + size));
    next->size = (curr->size - (sizeof(struct _block) + size));
    next->prev = curr;
    next->free = true;
    curr->size = size;
    curr->next = next;

    printf("curr : %p\n",curr);
    printf("Csize : %zu\n",curr->size);
    printf("next : %p\n",next);
    printf("Nsize : %zu\n",next->size);
    printf("Cn : %p\n",curr->next);
    printf("Np : %p\n",next->prev);
    num_splits++;
    num_blocks++;
}

/*
 * \brief malloc
 *
 * finds a free _block of heap memory for the calling process.
 * if there is no free _block that satisfies the request then grows the
 * heap and returns a new _block
 *
 * \param size size of the requested memory in bytes
 *
 * \return returns the requested memory allocation to the calling process
 * or NULL if failed
 */
void *malloc(size_t size)
{
    printf("in malloc\n");
    num_requested += size;
    if( atexit_registered == 0 )
    {
        printf("in atexit\n");
        atexit_registered = 1;
        atexit( printStatistics );
    }

    /* Align to multiple of 4 */
    printf("aligning\n");
    size = ALIGN4(size);

    /* Handle 0 size */
    if (size == 0)
    {
        printf("rturning null 1\n");
        return NULL;
    }

    /* Look for free _block */
    printf("findfreeblock\n");
    struct _block *next = findFreeBlock(size);

    /* Could not find free _block, so grow heap */
    printf("afterfreeblock\n");
    if (next == NULL)
    {
        next = growHeap(size);
    }
    else
    {
        // split the block to requested size if the found free block is bigger then the requested size
        if ((next->size) > (sizeof(struct _block) + size))
        {
            split(next,size);
        }
        num_reuses++;
    }

    /* Could not find free _block or grow heap, so just return NULL */
    if (next == NULL)
    {
        printf("rturning null 2\n");
        return NULL;
    }

    /* Mark _block as in use */
    next->free = false;

    /* Return data address associated with _block */
    printf("returning block %p\n", next);
    last = next;
    num_mallocs++;
    return BLOCK_DATA(next);
}

/*
 * \brief free
 *
 * frees the memory _block pointed to by pointer. if the _block is adjacent
 * to another _block then coalesces (combines) them
 *
 * \param ptr the heap memory to free
 *
 * \return none
 */
void free(void *ptr)
{
    if (ptr == NULL)
    {
        return;
    }

    /* Make _block as free */
    struct _block *curr = BLOCK_HEADER(ptr);
    assert(curr->free == 0);
    if (curr->prev)
    {
        if (curr->prev->free) //if previous block is free Coalesce current block with it
        {
            printf("prev");
            printf("prevf : %p\n",curr);
            printf("Psize : %zu\n",curr->size);
            curr->prev->next = curr->next;
            if (curr->next)
            {
                curr->next->prev = curr->prev;
            }
            curr->prev->size += (sizeof(struct _block) + curr->size);
            curr = curr->prev;
            num_coalesces++;
            num_blocks--;
        }

        printf("prevf : %p\n",curr->prev);
        printf("Psize : %zu\n",curr->size);
        printf("Cfree : %d\n",curr->free);
    }
    if (curr->next)
    {
        if (curr->next->free)  //if next block is free Coalesce it with current block
        {
            curr->next = curr->next->next;
            if (curr->prev)
            {
                curr->prev->next = curr->next;
            }
            curr->size += sizeof(struct _block) + curr->next->size;
            num_coalesces++;
            num_blocks--;
        }
        printf("next : %p\n",curr->next);
        printf("Nsize : %zu\n",curr->next->size);
    }
    curr->free = true;
    if (curr == last)
    {
        last = curr->next;
    }
    printf("curr : %p\n",curr);
    printf("Csize : %zu\n",curr->size);
    num_frees++;

    printf("leaving free by freeing %p\n",curr);

    /* TODO: Coalesce free _blocks if needed */
}

/*
 * \brief calloc
 *
 * The function allocates memory for an array of nmemb elements of size bytes each
 * The memory is set to zero.
 * then calloc() returns either NULL.
 *
 * \param nmemb - nmemb elements to be allocated
 * \param size - size in bytes of each element.
 *
 * \return returns a pointer to the allocated memory, NULL if failed
 */
void *calloc(size_t nmemb, size_t size)
{
    void *ptr = malloc(nmemb*size);
    memset(ptr, 0, nmemb*size);
    return (ptr);
}



/*The realloc() .      Unless ptr is NULL, it must have been returned by an earlier call to malloc(), calloc() or realloc(). If the area pointed to was moved, a free(ptr) is done.
* \brief calloc
*
* function changes the size of the memory block pointed to by ptr to size bytes.
* The contents will be unchanged in the range from the start of the region up to the minimum of the old and new sizes.
* If the new size is larger than the old size, the added memory will not be initialized.
* If ptr is NULL, then the call is equivalent to malloc(size), for all values of size;
* if size is equal to zero, and ptr is not NULL, then the call is equivalent to free(ptr).
*
*/
void *realloc(void *ptr, size_t size)
{
    struct _block *curr;
    if (ptr)
    {
        curr = BLOCK_HEADER(ptr);
        assert(curr->free);  //won't reallocate unused block.
        if (size == 0)
        {
            free(ptr);
        }
        else if ((curr->size) > (sizeof(struct _block) + size))
        {
            split(curr, size);
        }
        else if (curr->next)
        {
            // if next block is free and can accommodate requested size merge it with current block.
            if (curr->next->free && curr->next->size >= size - (curr->size + sizeof(struct _block)))
            {
                curr->next = curr->next->next;
                if (curr->next)
                {
                    curr->next->prev = curr->prev;
                }
                curr->size += (curr->next->size + sizeof(struct _block));

                // if merged block is bigger then requested size split it.
                if ((curr->size) > (sizeof(struct _block) + size))
                {
                    split(curr, size);
                }
            }
            else //next block is not free. free the block and assign a new block of requested size.
            {
                free(ptr);
                curr = malloc(size);
            }
        }
        else //next block is NULL. free the block and assign a new block of requested size.
        {
            free(ptr);
            curr = malloc(size);
        }
    }
    else //if requested ptr is NULL assign a new block with requested size.
    {
        curr = malloc(size);
    }
    return BLOCK_DATA(curr);
}

/* vim: set expandtab sts=3 sw=3 ts=6 ft=cpp: --------------------------------*/
