#include <sys/types.h>
#include <sys/syscall.h>
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <unistd.h>
#include <assert.h>
#include <pthread.h>

#include "memlib.h"
#include "malloc.h"

name_t myname = {
     /* team name to be displayed on webpage */
     "TeamPineapple",
     /* Full name of first team member */
     "Spongebob Squarepants",
     /* Email address of first team member */
     "demke@cs.toronto.edu",
     /* Full name of second team member */
     "Patrick Star",
     /* Email address of second team member */
     "patrick.star16384@gmail.com"
};


typedef ptrdiff_t vaddr_t;

static
void
fill_deadbeef(void *vptr, size_t len)
{
	u_int32_t *ptr = vptr;
	size_t i;

	for (i=0; i<len/sizeof(u_int32_t); i++) {
		ptr[i] = 0xdeadbeef;
	}
}


#undef  SLOW	/* consistency checks */
#undef SLOWER	/* lots of consistency checks */

//#define DEBUG
<<<<<<< HEAD
#define SIG_SB 's'
#define SIG_BIG 'B'

#define NUM_HEAPS 8

=======
#define NUM_HEAPS 8
>>>>>>> 47651c8c6b73915e591667e525f2794050e0fc43
#define PAGE_SIZE  4096
#if defined(__GNUC__)
#if __LP64__
#define PAGE_FRAME 0xfffffffffffff000
#else
#define PAGE_FRAME 0xfffff000
#endif /* __X86_64__ */
#endif /* __GNUC__ */

<<<<<<< HEAD
#define NSIZES 8
static const size_t sizes[NSIZES] = {16, 32, 64, 128, 256, 512, 1024, 2048 };

#define SMALLEST_SUBPAGE_SIZE 16
=======
#define NSIZES 9
static const size_t sizes[NSIZES] = { 8, 16, 32, 64, 128, 256, 512, 1024, 2048 };

#define SMALLEST_SUBPAGE_SIZE 8
>>>>>>> 47651c8c6b73915e591667e525f2794050e0fc43
#define LARGEST_SUBPAGE_SIZE 2048

////////////////////////////////////////

struct freelist {
	struct freelist *next;
};

struct pageref {
	struct pageref *next;
	struct freelist *flist;
	vaddr_t pageaddr_and_blocktype;
	int nfree;
<<<<<<< HEAD
	int heapid;
};

struct big_freelist {
	char signature;
=======
};

struct big_freelist {
>>>>>>> 47651c8c6b73915e591667e525f2794050e0fc43
	int npages;
	struct big_freelist *next;
};

<<<<<<< HEAD
struct page_backref {
	char signature;
	struct pageref *pr;
};
=======
>>>>>>> 47651c8c6b73915e591667e525f2794050e0fc43
struct heap {
	pthread_mutex_t lock;
	struct pageref *sizebases[NSIZES];
};
#define INVALID_OFFSET   (0xffff)

#define PR_PAGEADDR(pr)  ((pr)->pageaddr_and_blocktype & PAGE_FRAME)
#define PR_BLOCKTYPE(pr) ((pr)->pageaddr_and_blocktype & ~PAGE_FRAME)
#define MKPAB(pa, blk)   (((pa)&PAGE_FRAME) | ((blk) & ~PAGE_FRAME))

static struct pageref *fresh_refs; /* static global, initially 0 */
static struct pageref *recycled_refs;
static struct heap heaps[NUM_HEAPS];
//static struct pageref *sizebases[NSIZES];
static struct big_freelist *bigchunks;

pthread_mutex_t big_lock;

#ifdef DEBUG
# define DEBUG_PRINT(x) printf x
#else
# define DEBUG_PRINT(x) do {} while (0)
#endif

int getheapid() {
	return (syscall(__NR_gettid) % NUM_HEAPS); 
}

static
struct pageref *
allocpageref(void)
{
	struct pageref *ref;

	/* Use a pageref that already has a page allocated,
	 * if there are any.
	 */
	if (recycled_refs) {
		ref = recycled_refs;
		recycled_refs = recycled_refs->next;
		return ref;
	}

	/* No recycled_refs, use fresh one, if there are any */
	if (fresh_refs) {
		ref = fresh_refs;
		fresh_refs = fresh_refs->next;
		return ref;
	}

	/* All out of pagerefs.  Initialize a new page worth by
	 * getting a page with mem_sbrk()
	 */

	ref = (struct pageref *)mem_sbrk(PAGE_SIZE);
	if (ref) {
		bzero(ref, PAGE_SIZE);
		fresh_refs = ref+1;
		struct pageref *tmp = fresh_refs;
		int nrefs = PAGE_SIZE / sizeof(struct pageref) - 1;
		int i;
		for (i = 0; i < nrefs-1; i++) {
			tmp->next = tmp+1;
			tmp = tmp->next;
		}
		tmp->next = NULL;
	}
	return ref;

}

static
void
freepageref(struct pageref *p)
{
	p->next = recycled_refs;
	recycled_refs = p;
}


////////////////////////////////////////

/* SLOWER implies SLOW */
#ifdef SLOWER
#ifndef SLOW
#define SLOW
#endif
#endif

#ifdef SLOW
static
void
checksubpage(struct pageref *pr)
{
	vaddr_t prpage, fla;
	struct freelist *fl;
	int blktype;
	int nfree=0;

	if (pr->flist == NULL) {
		assert(pr->nfree==0);
		return;
	}

	prpage = PR_PAGEADDR(pr);
	blktype = PR_BLOCKTYPE(pr);
	

	for (fl=pr->flist; fl != NULL; fl = fl->next) {
		fla = (vaddr_t)fl;
		assert(fla >= prpage && fla < prpage + PAGE_SIZE);
		assert((fla-prpage) % sizes[blktype] == 0);
		nfree++;
	}
	assert(nfree==pr->nfree);
}
#else
#define checksubpage(pr) ((void)(pr))
#endif

#ifdef SLOWER
static
void
checksubpages(void)
{
	struct pageref *pr;
	int i;
	unsigned sc=0;

	for (i=0; i<NSIZES; i++) {
		for (pr = sizebases[i]; pr != NULL; pr = pr->next) {
			checksubpage(pr);
			sc++;
		}
	}

}
#else
#define checksubpages() 
#endif

////////////////////////////////////////

static
void
remove_lists(struct pageref *pr, int blktype, int heapid)
{
	struct pageref **guy;

	assert(blktype>=0 && blktype<NSIZES);

	for (guy = &heaps[heapid].sizebases[blktype]; *guy; guy = &(*guy)->next) {
		checksubpage(*guy);
		if (*guy == pr) {
			*guy = pr->next;
			break;
		}
	}

}

static
inline
int blocktype(size_t sz)
{
	unsigned i;
	for (i=0; i<NSIZES; i++) {
		if (sz <= sizes[i]) {
			return i;
		}
	}

	printf("Subpage allocator cannot handle allocation of size %lu\n", 
	      (unsigned long)sz);
	exit(1);

	// keep compiler happy
	return 0;
}

static
void *
subpage_kmalloc(size_t sz)
{
	unsigned blktype;	// index into sizes[] that we're using
	struct pageref *pr;	// pageref for page we're allocating from
	vaddr_t prpage;		// PR_PAGEADDR(pr)
	vaddr_t fla;		// free list entry address
	struct freelist *fl;	// free list entry
	void *retptr;		// our result

	volatile int i;

	int heapid;


	blktype = blocktype(sz);
	sz = sizes[blktype];


	checksubpages();
	
	heapid = getheapid();

	pthread_mutex_lock(&heaps[heapid].lock);

	for (pr = heaps[heapid].sizebases[blktype]; pr != NULL; pr = pr->next) {

		/* check for corruption */
		assert(PR_BLOCKTYPE(pr) == blktype);
		checksubpage(pr);

		if (pr->nfree > 0) {

		doalloc: /* comes here after getting a whole fresh page */

			prpage = PR_PAGEADDR(pr);
			fl = pr->flist;

			retptr = pr->flist;
			pr->flist = pr->flist->next;
			pr->nfree--;

			if (pr->flist != NULL) {
				assert(pr->nfree > 0);
				fla = (vaddr_t)fl;
				assert(fla - prpage < PAGE_SIZE);
			}
			else {
				assert(pr->nfree == 0);
			}

			checksubpages();
			pthread_mutex_unlock(&heaps[heapid].lock);
			return retptr;
		}
	}

	/*
	 * No page of the right size available.
	 * Make a new one.
	 */
	pthread_mutex_lock(&big_lock);
	pr = allocpageref();
	pthread_mutex_unlock(&big_lock);
	if (pr==NULL) {
		/* Couldn't allocate accounting space for the new page. */
		printf("malloc: Subpage allocator couldn't get pageref\n"); 
		return NULL;
	}

	prpage = PR_PAGEADDR(pr);
	if (prpage == 0) {
		pthread_mutex_lock(&big_lock);
		prpage = (vaddr_t)mem_sbrk(PAGE_SIZE);
		pthread_mutex_unlock(&big_lock);
		if (prpage==0) {
			/* Out of memory. */
			freepageref(pr);
			printf("malloc: Subpage allocator couldn't get a page\n"); 
			return NULL;
		}
	}

	pr->pageaddr_and_blocktype = MKPAB(prpage, blktype);
<<<<<<< HEAD
	pr->nfree = (PAGE_SIZE / sizes[blktype]) - 1;
	pr->heapid = heapid;
	/* Store back pointer ref*/
	struct page_backref *backref = (struct page_backref *)prpage;
	backref->signature = SIG_SB;
	backref->pr = pr; 

	/* Build freelist */

	fla = (vaddr_t)(char*)(prpage + sizes[blktype]);
=======
	pr->nfree = PAGE_SIZE / sizes[blktype];

	/* Build freelist */

	fla = prpage;
>>>>>>> 47651c8c6b73915e591667e525f2794050e0fc43
	fl = (struct freelist *)fla;
	fl->next = NULL;
	for (i=1; i<pr->nfree; i++) {
		fl = (struct freelist *)(fla + i*sizes[blktype]);
		fl->next = (struct freelist *)(fla + (i-1)*sizes[blktype]);
		assert(fl != fl->next);
	}
	pr->flist = fl;
	assert((vaddr_t)pr->flist == prpage+(pr->nfree-1)*sizes[blktype]);

	pr->next = heaps[heapid].sizebases[blktype];
	heaps[heapid].sizebases[blktype] = pr;


	/* This is kind of cheesy, but avoids duplicating the alloc code. */
	goto doalloc;
}

static
int
subpage_kfree(void *ptr)
{
	int blktype;		// index into sizes[] that we're using
	vaddr_t ptraddr;	// same as ptr
	struct pageref *pr=NULL;// pageref for page we're freeing in
	vaddr_t prpage;		// PR_PAGEADDR(pr)
	vaddr_t offset;		// offset into page
<<<<<<< HEAD
	//int i, j;
	int heapid;
=======
	int i, j;
>>>>>>> 47651c8c6b73915e591667e525f2794050e0fc43

	ptraddr = (vaddr_t)ptr;

	checksubpages();

<<<<<<< HEAD
	struct page_backref *backref = (struct page_backref*)((long unsigned int)ptr & PAGE_FRAME);
	if(backref->signature == SIG_BIG) {
		return -1; //Not  here; present in global pages
	}
	
	pr = backref->pr;
	heapid = pr->heapid;
	prpage = PR_PAGEADDR(pr);
	blktype = PR_BLOCKTYPE(pr);
	/* Nasty search to find the page that this block came from */
	/*int heapid = getheapid();
=======
	/* Nasty search to find the page that this block came from */
	int heapid = getheapid();
>>>>>>> 47651c8c6b73915e591667e525f2794050e0fc43
	//DEBUG_PRINT(("In subpage_free\n"));
	for(j = heapid; j != heapid; j = (j + 1) % NUM_HEAPS) {
		pthread_mutex_lock(&heaps[j].lock);
		//DEBUG_PRINT(("Takes lock\n"));
		for (i=0; i < NSIZES && pr==NULL; i++) {
			for (pr = heaps[j].sizebases[i]; pr; pr = pr->next) {
				prpage = PR_PAGEADDR(pr);
				blktype = PR_BLOCKTYPE(pr);

<<<<<<< HEAD
				// check for corruption
=======
				/* check for corruption */
>>>>>>> 47651c8c6b73915e591667e525f2794050e0fc43
				assert(blktype>=0 && blktype<NSIZES);
				checksubpage(pr);

				if (ptraddr >= prpage && ptraddr < prpage + PAGE_SIZE) {
					break;
				}
			}
		}

		if(pr != NULL) {
			break; //got a page, j holds lock
		}
		//else continue search
		pthread_mutex_unlock(&heaps[j].lock);
		//DEBUG_PRINT(("Does unlock\n"));
	}

	if (pr == NULL) {
<<<<<<< HEAD
		// Not on any of our pages - not a subpage allocation
		return -1;
	}*/

	pthread_mutex_lock(&heaps[heapid].lock);
=======
		/* Not on any of our pages - not a subpage allocation */
		return -1;
	}
>>>>>>> 47651c8c6b73915e591667e525f2794050e0fc43

	offset = ptraddr - prpage;

	/* Check for proper positioning and alignment */
	if (offset >= PAGE_SIZE || offset % sizes[blktype] != 0) {
		printf("kfree: subpage free of invalid addr %p\n", ptr);
		exit(1);
	}

	/*
	 * Clear the block to 0xdeadbeef to make it easier to detect
	 * uses of dangling pointers.
	 */
	fill_deadbeef(ptr, sizes[blktype]);

	/*
	 * We probably ought to check for free twice by seeing if the block
	 * is already on the free list. But that's expensive, so we don't.
	 */
	((struct freelist *)ptr)->next = pr->flist;
	pr->flist = (struct freelist *)ptr;
	pr->nfree++;

	assert(pr->nfree <= PAGE_SIZE / sizes[blktype]);
<<<<<<< HEAD
	if (pr->nfree == PAGE_SIZE / sizes[blktype] - 1) {
		/* Whole page is free. */
		remove_lists(pr, blktype, heapid);
=======
	if (pr->nfree == PAGE_SIZE / sizes[blktype]) {
		/* Whole page is free. */
		remove_lists(pr, blktype, j);
>>>>>>> 47651c8c6b73915e591667e525f2794050e0fc43
		pthread_mutex_lock(&big_lock);
		freepageref(pr);
		pthread_mutex_unlock(&big_lock);
	}

	checksubpages();

<<<<<<< HEAD
	pthread_mutex_unlock(&heaps[heapid].lock);
=======
	pthread_mutex_unlock(&heaps[j].lock);
>>>>>>> 47651c8c6b73915e591667e525f2794050e0fc43
	return 0;
}

static void *big_kmalloc(int sz)
{
	/* Handle requests bigger than LARGEST_SUBPAGE_SIZE 
	 * We simply round up to the nearest page-sized multiple
	 * after adding some overhead space to hold the number of 
	 * pages.
	 */
	
	void *result = NULL;

	sz += SMALLEST_SUBPAGE_SIZE;
	/* Round up to a whole number of pages. */
	int npages = (sz + PAGE_SIZE - 1)/PAGE_SIZE;

	/* Check if we happen to have a chunk of the right size already */
	struct big_freelist *tmp = bigchunks;
	struct big_freelist *prev = NULL;
	while (tmp != NULL) {
		if (tmp->npages > npages) {
			/* Carve the block in two pieces */
			tmp->npages -= npages;
<<<<<<< HEAD
			struct big_freelist *hdr_ptr = (struct big_freelist *)((char *)tmp+(tmp->npages*PAGE_SIZE));
			hdr_ptr->npages = npages;
=======
			int *hdr_ptr = (int *)((char *)tmp+(tmp->npages*PAGE_SIZE));
			*hdr_ptr = npages;
>>>>>>> 47651c8c6b73915e591667e525f2794050e0fc43
			result = (void *)((char *)hdr_ptr + SMALLEST_SUBPAGE_SIZE);
			break;
		} else if (tmp->npages == npages) {
			/* Remove block from freelist */
			if (prev) {
				prev->next = tmp->next;
			} else {
				bigchunks = tmp->next;
			}
<<<<<<< HEAD
			struct big_freelist *hdr_ptr = tmp;
			assert(hdr_ptr->npages == npages);
=======
			int *hdr_ptr = (int *)tmp;
			assert(*hdr_ptr == npages);
>>>>>>> 47651c8c6b73915e591667e525f2794050e0fc43
			result = (void *)((char *)hdr_ptr + SMALLEST_SUBPAGE_SIZE);
			break;
		} else {
			prev = tmp;
			tmp = tmp->next;
		}
	}

	if (result == NULL) {
		/* Nothing suitable in freelist... grab space with mem_sbrk */
<<<<<<< HEAD
		struct big_freelist *hdr_ptr = (struct big_freelist *)mem_sbrk(npages*PAGE_SIZE);
		if (hdr_ptr != NULL) {
			hdr_ptr->signature = SIG_BIG;
			hdr_ptr->npages = npages;
=======
		int *hdr_ptr = (int *)mem_sbrk(npages*PAGE_SIZE);
		if (hdr_ptr != NULL) {
			*hdr_ptr = npages;
>>>>>>> 47651c8c6b73915e591667e525f2794050e0fc43
			result = (void *)((char *)hdr_ptr + SMALLEST_SUBPAGE_SIZE);
		}
	}

	return result;
}

static void big_kfree(void *ptr)
{
	/* Coalescing is unlikely to do much good (other page allocations
	 * for small objects are likely to prevent big chunks from fitting
	 * together), so we don't bother trying.
	 */
<<<<<<< HEAD
	DEBUG_PRINT(("In big free\n"));
	struct big_freelist *hdr_ptr = (struct big_freelist *)((char *)ptr - SMALLEST_SUBPAGE_SIZE);
	//int npages = *hdr_ptr;

	struct big_freelist *newfree = hdr_ptr;
	assert(newfree->npages == hdr_ptr->npages);
	assert(newfree->signature == SIG_BIG);
=======

	int *hdr_ptr = (int *)((char *)ptr - SMALLEST_SUBPAGE_SIZE);
	//int npages = *hdr_ptr;

	struct big_freelist *newfree = (struct big_freelist *) hdr_ptr;
	assert(newfree->npages == *hdr_ptr);
>>>>>>> 47651c8c6b73915e591667e525f2794050e0fc43
	newfree->next = bigchunks;
	bigchunks = newfree;
}

//
////////////////////////////////////////////////////////////


int mm_init(void)
{
	int i = 0;

	for (i = 0; i < NUM_HEAPS; i++) {
		pthread_mutex_init(&heaps[i].lock, NULL);
	}
	if (dseg_lo == NULL && dseg_hi == NULL) {
		return mem_init();
	}
	return 0;
}

void *
mm_malloc(size_t sz)
{
	void *result;
	
	DEBUG_PRINT(("Entered mm malloc %d bytes\n", sz));
<<<<<<< HEAD
=======
	//pthread_mutex_lock(&malloc_lock);
>>>>>>> 47651c8c6b73915e591667e525f2794050e0fc43

	if (sz>=LARGEST_SUBPAGE_SIZE) {
		pthread_mutex_lock(&big_lock);
		result = big_kmalloc(sz);
		pthread_mutex_unlock(&big_lock);
	} else {
		result = subpage_kmalloc(sz);
	}

<<<<<<< HEAD

	//DEBUG_PRINT(("Return mm malloc %d\n", sz));
=======
	//pthread_mutex_unlock(&malloc_lock);

	DEBUG_PRINT(("Return mm malloc %d\n", sz));
>>>>>>> 47651c8c6b73915e591667e525f2794050e0fc43
	return result;
}

void
mm_free(void *ptr)
{
	/*
	 * Try subpage first; if that fails, assume it's a big allocation.
	 */
	DEBUG_PRINT(("Entered mm free\n"));
	if (ptr == NULL) {
		return;
	} else {
<<<<<<< HEAD
=======
	  //pthread_mutex_lock(&malloc_lock);
>>>>>>> 47651c8c6b73915e591667e525f2794050e0fc43
	  if (subpage_kfree(ptr)) {
		  pthread_mutex_lock(&big_lock);
		  big_kfree(ptr);
		  pthread_mutex_unlock(&big_lock);
	  }
<<<<<<< HEAD
=======
	  //pthread_mutex_unlock(&malloc_lock);
>>>>>>> 47651c8c6b73915e591667e525f2794050e0fc43
	}
	DEBUG_PRINT(("Return mm free\n"));
}

