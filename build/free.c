#include <stdlib.h>

#define container_of(ptr, type, member) ({          \
    const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
    (type *)( (char *)__mptr - offsetof(type,member) );})
    
struct malloc_chunk {
        
    INTERNAL_SIZE_T      prev_size;  /* Size of previous chunk (if free).  */
    INTERNAL_SIZE_T      size;       /* Size in bytes, including overhead. */
        
    struct malloc_chunk* fd;         /* double links -- used only if free. */
    struct malloc_chunk* bk;
        
    /* Only used for large blocks: pointer to next larger size.  */
    struct malloc_chunk* fd_nextsize; /* double links -- used only if free. */
    struct malloc_chunk* bk_nextsize;
};

int main()
{
    int *p = (int *) malloc(1024);
    free(p);
    return 0;
}
