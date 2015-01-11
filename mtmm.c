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

/* Struct prototypes*/
typedef struct ssuperblock SuperBlock;
typedef struct sCPUHeap MemHeap;


/* Data structures */
typedef struct sblockheader {
	void * raw_mem;
	unsigned int size;

	SuperBlock * parent_super_block;
	struct sblockheader * prev;
	struct sblockheader * next;
} BlockHeader;

typedef struct sblocklist {
	BlockHeader * head;
	BlockHeader * tail;
	int count; /* Number of block currently in the list*/
} BlockList;


typedef struct ssuperblock {

	int num_free_blocks;
	int num_total_blocks;
	BlockList blocks; /* Ordered list of block composing the Superblock*/
	int block_size;
	pthread_mutex_t mutex;
	void * raw_mem;

	MemHeap * parent_heap; /* Parent heap*/
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

/*  Auxiliary Functions  */
int size_to_class (int size) {
	return (int)ceil(log2(size));
}

void * allocate_from_superblock (SuperBlock * source, size_t sz) {

	if (source->num_free_blocks>0) { /* if there is a free block allocate it */
		BlockHeader * temp_block;

		temp_block=source->blocks.head;

		if (source->num_free_blocks>1) { /*If there is more than one */

			source->blocks.tail->next = source->blocks.head->next;// connect tail with new head (next)
			source->blocks.head->prev = source->blocks.tail;// connect new head with tail (prev)
			source->blocks.head = source->blocks.head->next;// update new head

		} else { /* Single Block available */
			source->blocks.head=NULL;
			source->blocks.head=NULL;
		}

		source->blocks.count--;
		source->num_free_blocks--;// decrease num of free blocks on the superblock

		return (temp_block->raw_mem);// return pointer
	}
	return NULL;
}

void return_block_to_superblock (BlockHeader * block, SuperBlock * target) {
	/* TODO implement*/
	/* Use the code  from move_superblock*/
}

int sb_keeps_invariant(SuperBlock * sb) {
	DBG_ENTRY
	int result;
	
	/* TODO implement*/
	
	DBG_EXIT
	return result;
}

SuperBlock * find_thin_sb(SizeClass * ) {
	/* TODO implement*/
	/* find invariant breaking sb*/
}

SuperBlock * add_superblock_to_heap (MemHeap * heap, int class) {
	DBG_ENTRY
	/* variables*/
	void * temp;
	BlockHeader * raw_mem_pos;
	BlockHeader * prev_block_pos;
	int max_blocks;
	int i;
	size_t block_size;
	SuperBlockList * target_list;
	int class_block_size=((int)pow(2,class));

	temp=fetch_os_memory(SUPERBLOCK_SIZE);
	SuperBlock * new_sb=((SuperBlock *) temp);
	block_size=class_block_size+sizeof(BlockHeader);

	/* Init the superblock header*/
	new_sb->blocks.head=NULL;
	new_sb->blocks.tail=NULL;
	new_sb->raw_mem=temp+sizeof(SuperBlock); /* Starting point of blocks */
	new_sb->next=NULL;
	new_sb->prev=NULL;
	new_sb->parent_heap=heap;


	/* Number of block the size of sizeclass with a header in the remaining space after removing the SuperBlock header*/
	max_blocks=( (SUPERBLOCK_SIZE-sizeof(SuperBlock)) / block_size );
	new_sb->blocks.count=max_blocks;

	prev_block_pos=raw_mem_pos=(BlockHeader *)new_sb->raw_mem;

	/* Chop the superblock into blocks */
	for (i=1; i<=max_blocks; i++) {
		((BlockHeader *)raw_mem_pos)->parent_super_block=new_sb;//add the new block as a pointer to block_pos;
		((BlockHeader *)raw_mem_pos)->raw_mem=raw_mem_pos+sizeof(BlockHeader); // set the pointer for the Block raw memory
		((BlockHeader *)raw_mem_pos)->size=class_block_size;
		((BlockHeader *)raw_mem_pos)->next=(BlockHeader *)(raw_mem_pos+block_size);
		((BlockHeader *)raw_mem_pos)->prev=prev_block_pos;

		prev_block_pos=(BlockHeader *)raw_mem_pos;//advance prev_block_pos
		raw_mem_pos += block_size;//increase raw_mem_pos;

	}
	/* Connect to block list struct*/
	new_sb->blocks.head=(BlockHeader *)new_sb->raw_mem;
	new_sb->blocks.tail=(BlockHeader *)raw_mem_pos;//connect the tail;

	/* Connect the superblock to the heap*/
	target_list=&(heap->sizeClasses[class].super_blocks_list);
	if ( target_list->count > 0 ) { 				//if it has blocks
		target_list->tail->next=new_sb;				//add as the next of the tail.
		new_sb->prev=target_list->tail;				//updated the prev pointer
		target_list->tail=new_sb;					//change the tail
	} else {
		target_list->tail=target_list->head=new_sb; //add it as both the head and the tail
	}

	/*update counters*/
	update_heap_stats(heap,SUPERBLOCK_SIZE,0);
	new_sb->num_total_blocks=max_blocks;
	new_sb->block_size=class_block_size;

	DBG_EXIT
	return new_sb;

}

size_t get_block_size(void * ptr) {
	if (ptr!=NULL) {
		return ( ((BlockHeader *)(ptr-sizeof(BlockHeader)))->size );
	}
	return (-1);
}

void move_superblock (MemHeap * source, MemHeap * target, SuperBlock * sb, int class){

	DBG_ENTRY

	SuperBlockList * src_list;
	SuperBlockList * trg_list;
	SuperBlock * temp;


	/*relevant lists to work on */
	src_list=&(source->sizeClasses[class].super_blocks_list);
	trg_list=&(source->sizeClasses[class].super_blocks_list);

	/* Remove from the old list*/
	if (sb->next != NULL)
		(sb->next)->prev=sb->prev;// if we have a next - set its prev to be our prev
	if (sb->prev!= NULL)
		(sb->prev)->next=sb->next;// if we have a prev - set it to be our next
	if (src_list->tail == sb)
		src_list->tail=sb->prev;// if we are the tail - set the tail our prev
	if (src_list->head == sb)
		src_list->head=sb->next;// if we are the head - set the head to be our next

	/* Add it to the new list*/
	sb->prev=trg_list->tail;//set the tail to be our prev
	trg_list->tail=sb;//update the tail to be us
	if (trg_list->head==NULL)
		trg_list->head=sb;//if the head is NULL-we are the head

	/* Update Parent heap */
	sb->parent_heap=target;

	/*update counters*/
	update_heap_stats(source,(-SUPERBLOCK_SIZE),(-(sb->num_total_blocks - sb->num_free_blocks)*(sb->block_size)));
	update_heap_stats(target,SUPERBLOCK_SIZE, (sb->num_total_blocks - sb->num_free_blocks)*(sb->block_size));

	DBG_ENTRY

}

/* Updates stats of Heap - adds the delta, negative value can be passed
 * Assumes that only the locking thread will call this function
 * */
void update_heap_stats(MemHeap * heap, int total_delta, int used_delta) {
	heap->total_used+=used_delta;
	heap->total_size+=total_delta;
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

/* Fetches a superblock from the OS - aborts the program on failure*/
void * fetch_os_memory(size_t sz) {

	DBG_ENTRY
	int fd;
	void *p;
	size_t total_to_alloc;

	fd = open("/dev/zero", O_RDWR);
	if (fd == -1){
		abrt("Memory allocation from OS failed");
	}
	total_to_alloc=sz;

	p = mmap(0, total_to_alloc, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	close(fd);

	if (p == MAP_FAILED){
		abrt("OS failed to allocate memory");
	}
	DBG_EXIT
	return p;
}

void return_os_memory(void * ptr) {
	DBG_ENTRY
	if (ptr != NULL)
	{
		int size = ((BlockHeader *)(ptr - sizeof(BlockHeader))) -> size + sizeof(BlockHeader);
		if (munmap(ptr - sizeof(BlockHeader), size) < 0)
		{
			abrt("Error releasing memory to OS");
		}

	}
	DBG_EXIT
}

/*  Main Functions  */
void * malloc_work (size_t sz) {
	DBG_ENTRY
	pthread_t self_tid;
	SuperBlock * source_sb; /* SuperBlock to take from */
	void *p;

	if (sz>=1) { /* valid size */

		if ( (sz+sizeof(BlockHeader)) >(BLOCK_LIMIT)) { /* sz > S/2, allocate the superblock from the OS and return it. */

			DBG_MSG("Requested size exceeds half super block");

			p=fetch_os_memory(sz+sizeof(BlockHeader));

			((BlockHeader *) p)->size = sz;
			((BlockHeader *) p)->next=NULL;
			((BlockHeader *) p)->prev=NULL;
			((BlockHeader *) p)->parent_super_block=NULL;
			return (p + sizeof(BlockHeader));

		} else {
			int thread_heap = HASH(self_tid); /* 2. i ← hash(the current thread).*/
			int relevant_class=size_to_class(sz);

			/* relevant size class */

			pthread_mutex_lock( &(hoard.mHeaps[thread_heap].sizeClasses.mutex) ); /* 3. Lock heap relevant size class in relevant heap */

			source_sb=scan_heap( &( hoard.mHeaps[thread_heap] ) ,relevant_class);/* 4. Scan heap i’s list of superblocks from most full to least (for the size class corresponding to sz).*/
			if ( source_sb != NULL) {
				DBG_MSG("Locking global heap\n");
				pthread_mutex_lock( &(hoard.mHeaps[GLOBAL_HEAP].sizeClasses[relevant_class].mutex) ); /* Lock global heap */
				source_sb=scan_heap( &( hoard.mHeaps[GLOBAL_HEAP] ) ,relevant_class); /* 6. Check heap 0 (the global heap) for a superblock.*/

			}

			if (source_sb==NULL) {
				DBG_MSG("Unlocking global heap\n");
				pthread_mutex_unlock( &(hoard.mHeaps[GLOBAL_HEAP].sizeClasses.mutex) ); /* release the global heap, we don't need it */
				source_sb=add_superblock_to_heap( &( hoard.mHeaps[thread_heap] ) ,relevant_class); /*8. Allocate S bytes as superblock s and set the owner to heap i.*/
			} else {
				move_superblock( &(hoard.mHeaps[GLOBAL_HEAP].sizeClasses[relevant_class]),
								&(hoard.mHeaps[thread_heap].sizeClasses[relevant_class]) ,
								source_sb, relevant_class); /* 10. Transfer the superblock s to heap i. */
			}

			update_heap_stats(&( hoard.mHeaps[thread_heap] ),0,(-source_sb->block_size));
	/*				Statistics calc
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

	int relevant_class;
	BlockHeader * block_ptr;
	SuperBlock * origin_sb,sb_to_return;	/* Origin superblock*/
	MemHeap * origin_heap;	/* relevant heap */
	SuperBlock * origin_sb; /* superblock from which this was allocated */
	size_t ret_size; 		/* returned size */


	if (ptr!=NULL){
		block_ptr=(BlockHeader *)(ptr-sizeof(BlockHeader));
		if ( ((block_ptr->size)-sizeof(BlockHeader)) > BLOCK_LIMIT) { /*1. If the block is “large” */
			return_os_memory(ptr); /* 2. Free the superblock to the operating system and return. return_os_memory */
		} else {

			/* Resolving of parent structs */

			origin_sb=block_ptr->parent_super_block;
			origin_heap=origin_sb->parent_heap;
			ret_size=get_block_size(ptr);
			relevant_class=size_to_class(ret_size);

			/* Lock the mutex  */
			pthread_mutex_lock( &(origin_heap->sizeClasses[relevant_class].mutex) ); /* 3. Find the superblock s this block comes from and lock it.*/
			
			return_block_to_superblock(block_ptr,origin_sb); /*5. Deallocate the block from the superblock. */
			
			// if relevant sizeclass on the global heap is empty - find a mostly empty block to return
			sb_to_return=find_thin_sb(&(origin_heap->sizeClasses[relevant_class]));
			if (sb_to_return!=NULL){ /*10. Transfer a mostly-empty superblock s1 to heap 0 (the global heap). */
				pthread_mutex_lock( &(hoard.mHeaps[GLOBAL_HEAP].sizeClasses[relevant_class].mutex) ); //lock global heap
				move_superblock(origin_heap, &(hoard.mHeaps[GLOBAL_HEAP]),sb_to_return);
			}
			//call update_heap_stats

			
			find_thin_sb(&(origin_heap->sizeClasses[relevant_class]));

			// update_stats

			/*
			
			4. Lock heap i, the superblock’s owner.
			
			6. u i ← u i − block size. update
			7. s.u ← s.u − block size.
			8. If i = 0, unlock heap i and the superblock and return.
			9. If u i < a i − K ∗ S and u i < (1 − f) ∗ a i,
			
			
			11. u 0 ← u 0 + s1.u, u i ← u i − s1.u
			12. a 0 ← a 0 + S, a i ← a i − S
			13. Unlock heap i and the superblock.
			 */
		}
	}



}

void * realloc (void * ptr, size_t sz) {
	void * temp_dst;
	
	temp_dst=malloc(sz); /* Allocate more space*/
	
	if (temp_dst != NULL) { /* Was it successful? */
	
		if ( memcpy(temp_dst,ptr, get_block_size(ptr)) == temp_dst){ /* Try to copy */
			free(ptr); /* release the old space/*/
			return temp_dst;
		
		}
	}
	return NULL;
}
