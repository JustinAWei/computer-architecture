#include "heap.h"
#include "debug.h"
#include <stdlib.h>
#include <stdio.h>

long mCount = 0;
long fCount = 0;

typedef struct mem_block {
    int size;
    int fr;
    struct mem_block *next;
    struct mem_block *prev;
} b;

b * head = 0;

void* malloc(size_t bytes) {
    if(bytes == 0) return 0;
    if (head == 0) {
		head = (b*) the_heap;
        head->size = HEAP_SIZE;
        head->fr = 1;
		head->next = 0;
		head->prev = 0;
    }

	// printf("malloc\n");
	// printf("want size %d\n", (int)bytes);
    size_t total_bytes = bytes + (8-(8+bytes%8)) + sizeof(b);

    mCount++;
    b * cur_block = head;
    // loop through memory until we find block >= bytes
    while((long)cur_block != 0) {
        // if block size > bytes, split
 		// printf("block size: %d\tfree: %d\n", cur_block->size, cur_block->fr);
        if(cur_block->fr == 1 && cur_block->size >= total_bytes) {
            // cur_block is now used
            cur_block->fr = 0;

            if (cur_block->size > total_bytes + sizeof(b)) {
				// printf("splitting\n");
                // split the remaining memory off
                int split_size = (int)(cur_block->size - total_bytes);
				// printf("block size: %d, byte: %d, split: %d\n", cur_block->size, (int)bytes, split_size);
                cur_block->size = total_bytes;

                // new memory place
                b* split_block = (b*)((unsigned long)cur_block + total_bytes);
                split_block->fr = 1;
                split_block->size = split_size;

                b* cur_block_next = cur_block->next;
                split_block->next = cur_block_next;

                if((long)cur_block_next != 0) {
                    cur_block_next->prev = split_block;
                }
                split_block->prev = cur_block;

                cur_block->next = split_block;
                // printf("Current after split -- block size: %d\tfree: %d\n", cur_block->size, cur_block->fr);
                // printf("Next after split -- block size: %d\tfree: %d\n", cur_block->next->size, cur_block->next->fr);
            }

            // if its exactly the size we want, just return it
			return (void*) (cur_block + sizeof(b));
        }
	if(cur_block->next == 0) {
		return 0;
	}
        cur_block = cur_block->next;
    }
	// printf("no free mem\n");
    // exit(0);
    return 0;
}

void free(void* p) {
    if(p==0) return;
    fCount++;
    b *cur_block = (b*) p - sizeof(b);
    if(cur_block == 0) return;
	// printf("freeing block -- address: %lx, size %d\n", (long) cur_block, cur_block->size);

    cur_block->fr = 1;

    // merge next
    if((long)(cur_block->next) != 0 && cur_block->next->fr == 1) {
		// printf("next block is -- address: %lx, size: %d\n", (long) cur_block->next, cur_block->next->size);
        cur_block->size += cur_block->next->size;
        cur_block->next = cur_block->next->next;

        if((long)(cur_block->next) != 0) {
            cur_block->next->prev = cur_block;
        }
		// printf("merged cur block -- address: %lx, size %d\n", (long) cur_block, cur_block->size);
    }

    // merge prev
    if((long)cur_block->prev != 0 && cur_block->prev->fr == 1) {
		// printf("prev block is -- address: %lx, size: %d\n", (long) cur_block->prev, cur_block->prev->size);

		cur_block->prev->size += cur_block->size;
        cur_block->prev->next = cur_block->next;

        if((long)(cur_block->next) != 0) {
            cur_block->next->prev = cur_block->prev;
        }
		// printf("merged cur block -- address: %lx, size %d\n", (long) cur_block->prev, cur_block->prev->size);

    }
    // printf("new prev block -- address: %lx, size %d\n", (long) cur_block->prev, cur_block->prev->size);
    // printf("new cur block -- address: %lx, size %d\n", (long) cur_block->prev->prev, cur_block->prev->prev->size);
    // printf("new next block -- address: %lx, size %d\n", (long) cur_block->prev->next, cur_block->prev->next->size);
}
