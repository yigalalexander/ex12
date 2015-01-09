/*
 * mtmm.c - Implementation of mtmm.h, a library for in process threads
 *
 *   Created on: Dec 24, 2014
 *      Student: Yigal Alexander
 *      	 ID: 306914565
 */
#include "mtmm.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <pthread.h>
#include <math.h>
#include <stdlib.h>

/* Debug tools */
#define DBG_MSG printf("\n[%d]: %s):", __LINE__, __FUNCTION__);printf
#define DBG_ENTRY printf("\n[%d]: --> %s", __LINE__,__FUNCTION__);
#define DBG_EXIT printf("\n[%d]: <-- %s", __LINE__,__FUNCTION__);

/* Constants */
#define NUM_SIZE_CLASSES 16
#define CPU_COUNT 2
#define GLOBAL_HEAP CPU_COUNT
#define BLOCK_LIMIT SUPERBLOCK_SIZE/2
#define K 0 /* Should it be 1 */
#define F (1/4)

#define HASH(A) ((A)%CPU_COUNT)
#define abrt(X) perror(X); exit(0);

void * malloc_init (size_t sz); /* Initialization function prototype */
static void * (*real_malloc)(size_t)=malloc_init; /*pointer to the real malloc to be used*/

typedef struct sblockheader {
	int is_used;
	void * addr;
	unsigned int size;

	struct sblockheader * parent_super_block;
	struct sblockheader * prev;
	struct sblockheader * next;
} BlockHeader;

typedef struct sblocklist {
	BlockHeader * head;
	BlockHeader * tail;
	int count; /* Number of block currently in the list*/
} BlockList;


typedef struct ssuperblock {
	int num_blocks;
	int num_free_blocks;
	BlockList blocks;
	pthread_mutex_t mutex;
	void * raw_mem;

	void * heap;
	struct ssuperblock * prev;
	struct ssuperblock * next;
} SuperBlock;

typedef struct ssuperblocklist {
	SuperBlock * head;
	SuperBlock * tail;
	int count;
} SuperBlockList;

typedef struct ssizeclass {

	unsigned int mSize;
	pthread_mutex_t mutex;
	SuperBlockList super_blocks_list;

} SizeClass;

typedef struct sCPUHeap {
/*
* @b	rief
* CPUid – to hold the CPU ID
* mutex - Mutex to lock the heap
* sizeClasses - array of size classes
* */
	unsigned int CPUid;
	size_t total_size;
	size_t total_used;
	SizeClass sizeClasses[NUM_SIZE_CLASSES];

} MemHeap;

static struct sHoard {
	MemHeap mHeaps[CPU_COUNT+1]; // 2 CPUs, and the last one is the global
} hoard;

void * allocate_from_superblock (SuperBlock * source, size_t sz) {
	/* TODO */
	if (curr_sb->num_free_blocks>0) { /* if there is a free block allocate it */
					BlockHeader * temp_block;

					temp_block=curr_sb->blocks.head;

					if (curr_sb->num_free_blocks>1) { /*If there is more than one */

						curr_sb->blocks.tail->next = curr_sb->blocks.head->next;// connect tail with new head (next)
						curr_sb->blocks.head->prev = curr_sb->blocks.tail;// connect new head with tail (prev)
						curr_sb->blocks.head = curr_sb->blocks.head->next;// update new head

					} else { /* Single Block available */
						curr_sb->blocks.head=NULL;
						curr_sb->blocks.head=NULL;
					}

					curr_sb->blocks.count--;
					curr_sb->num_free_blocks--;// decrease num of free blocks on the superblock
					curr_class->total_used = curr_class->total_used + temp_block->size;// update statistics

					return (temp_block->addr);// return pointer

				}

}

SuperBlock * add_super_block_to_heap (MemHeap * heap, int class) {
	/* TODO Add chopping function*/
}

void move_superblock (MemHeap * source, MemHeap * target, SuperBlock * sb){
	/* TODO */
}

void update_stats(MemHeap * heap, int delta) {
	/* TODO */
}

/* Scans a given heap for a suitable SuperBlock */
SuperBlock * scan_heap (MemHeap * heap,int requested_class) {
	SuperBlock * curr_sb;
	SuperBlock * prev_sb;
	SizeClass * curr_class=&( heap->sizeClasses[requested_class] );

	prev_sb=curr_class->super_blocks_list.tail;

	for (curr_sb=curr_class->super_blocks_list.head;
			(curr_sb != curr_class->super_blocks_list.tail) || (curr_sb->num_free_blocks>0); /* We made it to the end of the list or we found a superblock with free blocks*/
			curr_sb=curr_sb->next) {

		prev_sb=curr_sb;
	}

	if (curr_sb->num_free_blocks>0) { /* if there is a free block allocate it */
		return curr_sb;
	}

	return NULL;

}

void * fetch_os_memory(size_t sz, size_t header_size, unsigned int segments) {

	DBG_ENTRY
	int fd;
	void *p;
	size_t total_to_alloc;

	fd = open("/dev/zero", O_RDWR);
	if (fd == -1){
		abrt("Memory allocation from OS failed");
	}
	total_to_alloc=sz+(segments*header_size);

	p = mmap(0, total_to_alloc, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	close(fd);

	if (p == MAP_FAILED){
		abrt("OS failed to allocate memory");
	}
	DBG_EXIT
	return p;
}



void * malloc_work (size_t sz) {
	DBG_ENTRY
	/* NOTES
	 * to allocate for a size class (SUPERBLOCK_SIZE/sizeclass)*sizeof(header)+SUPERBLOCK_SIZE*/


	pthread_t self_tid;
	SuperBlock * source_sb; /* SuperBlock to take from */
	void *p;



	if (sz>=1) { /* valid size */

		if ( (sz+sizeof(BlockHeader)) >(BLOCK_LIMIT)) { /* sz > S/2, allocate the superblock from the OS and return it. */

			DBG_MSG("Requested size exceeds half super block");

			p=fetch_os_memory(sz+sizeof(BlockHeader),0,0);

			((BlockHeader *) p)->size = sz;
			((BlockHeader *) p)->next=NULL;
			((BlockHeader *) p)->prev=NULL;
			((BlockHeader *) p)->parent_super_block=NULL;
			return (p + sizeof(BlockHeader));

		} else {
			/* TODO when getting a super block for chopping  need to init the mutex*/
			int thread_heap = HASH(self_tid); /* 2. i ← hash(the current thread).*/
			int relevant_class=(int)ceil(log2(sz));

			/* relevant size class */

			pthread_mutex_lock( &(hoard.mHeaps[thread_heap].sizeClasses.mutex) ); /* 3. Lock heap relevant size class in relevant heap */

			source_sb=scan_heap( &( hoard.mHeaps[thread_heap] ) ,relevant_class);/* 4. Scan heap i’s list of superblocks from most full to least (for the size class corresponding to sz).*/
			if ( source_sb != NULL) {
				DBG_MSG("Locking global heap\n");
				pthread_mutex_lock( &(hoard.mHeaps[GLOBAL_HEAP].sizeClasses.mutex) ); /* Lock global heap */
				source_sb=scan_heap( &( hoard.mHeaps[GLOBAL_HEAP] ) ,relevant_class); /* 6. Check heap 0 (the global heap) for a superblock.*/

			}

			if (source_sb==NULL) {
				DBG_MSG("Unlocking global heap\n");
				pthread_mutex_unlock( &(hoard.mHeaps[GLOBAL_HEAP].sizeClasses.mutex) ); /* release the global heap, we don't need it */
				source_sb=add_super_block_to_heap( &( hoard.mHeaps[thread_heap] ) ,relevant_class); /*8. Allocate S bytes as superblock s and set the owner to heap i.*/
			} else {
				move_superblock( &(hoard.mHeaps[GLOBAL_HEAP]), hoard.mHeaps[thread_heap] ,source_sb); /* 10. Transfer the superblock s to heap i. */
			}

			update_stats(&( hoard.mHeaps[thread_heap] ),sz); /* TODO should I be calling it like that? */
	/*
					11. u 0 ← u 0 − s.u
					12. u i ← u i + s.u
					13. a 0 ← a 0 − S
					14. a i ← a i + S
					15. u i ← u i + sz.
					16. s.u ← s.u + sz.
			 */
			p=allocate_from_superblock(source_sb,sz);
			pthread_mutex_unlock( &(hoard.mHeaps[thread_heap].sizeClasses.mutex) ); //17. Unlock heap i.
			return p; //18. Return a block from the superblock.
		}
	}

	DBG_EXIT
	return NULL;/*cannot satisfy request*/

}

/*
 * This function should only run once.
 * It initializes the structs and then replaces the real_malloc pointer to be "malloc_work" - for the next calls
 */
void * malloc_init (size_t sz) {

	int idx_cpu=0;
	for (idx_cpu=0; idx_cpu<=CPU_COUNT;idx_cpu++) { /* Init the CPU heaps*/
		hoard.mHeaps[idx_cpu].CPUid=idx_cpu;
		int idx_class;
		for (idx_class=0; idx_class<NUM_SIZE_CLASSES; idx_class++) { /* Init the SizeClass and the superblock list*/
			if (pthread_mutex_init(&(hoard.mHeaps[idx_cpu].sizeClasses[idx_class].mutex), NULL)) { /* Init the heap mutex*/
				abrt("Initialization failed.\n"); /* If mutex init fails */
			}
			hoard.mHeaps[idx_cpu].sizeClasses[idx_class].mSize=(int)pow(2,idx_class);
			hoard.mHeaps[idx_cpu].sizeClasses[idx_class].total_used=0;
			hoard.mHeaps[idx_cpu].sizeClasses[idx_class].total_size=0;
			hoard.mHeaps[idx_cpu].sizeClasses[idx_class].super_blocks_list.count=0;
			hoard.mHeaps[idx_cpu].sizeClasses[idx_class].super_blocks_list.head=NULL;
			hoard.mHeaps[idx_cpu].sizeClasses[idx_class].super_blocks_list.tail=NULL;
		}

	}
	real_malloc=malloc_work;
	return ((*real_malloc)(sz)); /* Run the actual allocation function*/

}

void * malloc (size_t sz) {

	return ((*real_malloc)(sz));

}

void free (void * ptr) {
	/* free with munmap - to use when freeing a large block that was allocated directly with mmap*/
	/*

	The free() function frees the memory space pointed to by ptr, which must have been returned
	by a previous call to malloc(), calloc() or realloc(). Otherwise, or if free(ptr) has already
	been called before, undefined behavior occurs. If ptr is NULL, no operation is performed.


	free (ptr)
	1. If the block is “large”,
	2. Free the superblock to the operating system and return.
	3. Find the superblock s this block comes from and lock it.
	4. Lock heap i, the superblock’s owner.
	5. Deallocate the block from the superblock.
	6. u i ← u i − block size.
	7. s.u ← s.u − block size.
	8. If i = 0, unlock heap i and the superblock and return.
	9. If u i < a i − K ∗ S and u i < (1 − f) ∗ a i,
	10. Transfer a mostly-empty superblock s1
	to heap 0 (the global heap).
	11. u 0 ← u 0 + s1.u, u i ← u i − s1.u
	12. a 0 ← a 0 + S, a i ← a i − S
	13. Unlock heap i and the superblock.
	 */
	if (ptr != NULL)
	{
		int size = ((BlockHeader *)(ptr - sizeof(BlockHeader))) -> size + sizeof(BlockHeader);
		if (munmap(ptr - sizeof(BlockHeader), size) < 0)
		{
			perror(NULL);
		}

	}
	printf("myfree\n");

}

void * realloc (void * ptr, size_t sz) {
	return NULL;
}
