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

/* Debug tools*/
#define DBG_MSG {printf("\n[%d]: %s):", __LINE__, __FUNCTION__);printf
#define DBG_ENTRY printf("\n[%d]: --> %s", __LINE__,__FUNCTION__);
#define DBG_EXIT printf("\n[%d]: <-- %s", __LINE__,__FUNCTION__);

#define hash2(A) ((A)%2)
#define abrt(X) perror(X); exit(0);
#define NUM_SIZE_CLASSES 16
#define CPU_COUNT 2
#define GLOBAL_HEAP CPU_COUNT+1

void * malloc_init (size_t sz);
static void * (*real_malloc)(size_t)=malloc_init; /*pointer to the real malloc to be used*/
/*static int debug=0;*/

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
	int count;
} BlockList;

typedef struct ssuperblock {
	int num_blocks;
	int num_used_blocks;
	BlockList blocks;

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
	size_t totat_size;
	size_t total_used;
	SuperBlockList super_blocks_list;

} SizeClass;

typedef struct sCPUHeap {
/*
* @brief
* CPUid – to hold the CPU ID
* mutex - Mutex to lock the heap
* sizeClasses - array of size classes
* */
	unsigned int CPUid;
	SizeClass sizeClasses[NUM_SIZE_CLASSES];

} MemHeap;

static struct sHoard {
	MemHeap mHeaps[CPU_COUNT+1]; // 2 CPUs, and the last one is the global
} hoard;

/*
 * This function should only run once.
 * It initializes the structs and then replaces the real_malloc pointer to be "malloc_work" - for the next calls
 */



void * malloc_work (size_t sz) {

	if (sz<1) { /* valid size */
		if (sz>(SUPERBLOCK_SIZE/2)) { /* sz > S/2, allocate the superblock from the OS and return it. */
			int fd;
			void *p;
			fd = open("/dev/zero", O_RDWR);
			if (fd == -1){
				perror("Memory allocation from OS failed");
				exit (0);
			}
			p = mmap(0, sz + sizeof(BlockHeader), PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
			close(fd);

			if (p == MAP_FAILED){
				perror(NULL);
				return 0;
			}

			((BlockHeader *) p)->size = sz;
			((BlockHeader *) p)->next=NULL;
			((BlockHeader *) p)->prev=NULL;
			((BlockHeader *) p)->parent_super_block=NULL;
			return (p + sizeof(BlockHeader));

		} else {


		/* to allocate for a size class (SUPERBLOCK_SIZE/sizeclass)*sizeof(header)+SUPERBLOCK_SIZE */

		}

		int curr_cpu = 	/*TID*/0%CPU_COUNT;
		int relevant_class; /*log of request size in base 2 +1 */
		pthread_mutex_lock( &(hoard.mHeaps[curr_cpu].sizeClasses[relevant_class].mutex) ); /* Lock the relevant size class - wait for it while (EBUSY == pthread_mutex_lock(&mutex) {}*/
		/*
			2. i ← hash(the current thread).
			3. Lock heap i.  <------ Use a while statement -
			4. Scan heap i’s list of superblocks from most full to least (for the size class corresponding to sz).
			5. If there is no superblock with free space,
			6. Check heap 0 (the global heap) for a superblock.
			7. If there is none,
			8. Allocate S bytes as superblock s and set the owner to heap i.
			9. Else,
			10. Transfer the superblock s to heap i.
			11. u 0 ← u 0 − s.u
			12. u i ← u i + s.u
			13. a 0 ← a 0 − S
			14. a i ← a i + S
			15. u i ← u i + sz.
			16. s.u ← s.u + sz.
			17. Unlock heap i.
			18. Return a block from the superblock.
		 */



	}

	return NULL;/*cannot satisfy request*/

}

void * malloc_init (size_t sz) {

	int i=0;
	for (i=0; i<=CPU_COUNT;i++) { /* Init the CPU heaps*/
		hoard.mHeaps[i].CPUid=i;

		if (pthread_mutex_init(&(hoard.mHeaps[i].mutex), NULL)) { /* Init the heap mutex*/
			perror("Initialization failed.\n"); /* If mutex init fails */
			exit (0);
		}

		int j;
		for (j=0; j<NUM_SIZE_CLASSES; j++) { /* Init the SizeClass and the superblock list*/
			hoard.mHeaps[i].sizeClasses[j].mSize=(int)pow(2,j);
			hoard.mHeaps[i].sizeClasses[j].total_used=0;
			hoard.mHeaps[i].sizeClasses[j].totat_size=0;
			hoard.mHeaps[i].sizeClasses[j].super_blocks_list.count=0;
			hoard.mHeaps[i].sizeClasses[j].super_blocks_list.head=NULL;
			hoard.mHeaps[i].sizeClasses[j].super_blocks_list.tail=NULL;
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
