/***************************************************************************
 *  Title: Kernel Memory Allocator
 * -------------------------------------------------------------------------
 *    Purpose: Kernel memory allocator based on the buddy algorithm
 *    Author: Stefan Birrer
 *    Copyright: 2004 Northwestern University
 ***************************************************************************/
/***************************************************************************
 *  ChangeLog:
 * -------------------------------------------------------------------------
 *    Revision 1.2  2009/10/31 21:28:52  jot836
 *    This is the current version of KMA project 3.
 *    It includes:
 *    - the most up-to-date handout (F'09)
 *    - updated skeleton including
 *        file-driven test harness,
 *        trace generator script,
 *        support for evaluating efficiency of algorithm (wasted memory),
 *        gnuplot support for plotting allocation and waste,
 *        set of traces for all students to use (including a makefile and README of the settings),
 *    - different version of the testsuite for use on the submission site, including:
 *        scoreboard Python scripts, which posts the top 5 scores on the course webpage
 *
 *    Revision 1.1  2005/10/24 16:07:09  sbirrer
 *    - skeleton
 *
 *    Revision 1.2  2004/11/05 15:45:56  sbirrer
 *    - added size as a parameter to kma_free
 *
 *    Revision 1.1  2004/11/03 23:04:03  sbirrer
 *    - initial version for the kernel memory allocator project
 *
 ***************************************************************************/
#ifdef KMA_BUD
#define __KMA_IMPL__
#define MINBLOCKSIZE 32
#define BITMAPSIZE PAGESIZE / MINBLOCKSIZE
#define CHAR_BIT 8

/************System include***********************************************/
#include <assert.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

/************Private include**********************************************/
#include "kma_page.h"
#include "kma.h"

/************Defines and Typedefs*****************************************/
/*  #defines and typedefs should have their names in all caps.
 *  Global variables begin with g. Global constants with k. Local
 *  variables should be in all lower case. When initializing
 *  structures and arrays, line everything up in neat columns.
 */

 typedef struct free_list
 {
  int size;
  struct free_list* nextFree;
 } free_block;

/************Global Variables*********************************************/

 kma_page_t* pageHeader = NULL;

/************Function Prototypes******************************************/
  
/************External Declaration*****************************************/

/**************Implementation***********************************************/
kma_page_t* initializeFreeList(kma_size_t size) {
  kma_page_t* freeListPage;
  freeListPage = get_page();
  *((kma_page_t**)freeListPage->ptr) = freeListPage;

  if ((size + sizeof(kma_page_t*)) > freeListPage->size) {
	// requested size larger than page
	free_page(freeListPage);
	return NULL;
  }
  pageHeader = freeListPage;

  // Place free list immediately after header
  free_block* freeList = (free_block*)((void *)(pageHeader) + sizeof(kma_page_t));
  for (int i=0; i < 8; i++) {
	freeList[i].size = 32*pow(2, i);
	freeList[i].nextFree = NULL;
  }
 
  return freeListPage;
}

void set_nth_bit(unsigned char *bitmap, int idx) {
	bitmap[idx / CHAR_BIT] |= 1 << (idx % CHAR_BIT);
}

void clear_nth_bit(unsigned char *bitmap, int idx) {
	bitmap[idx / CHAR_BIT] &= ~(1 << (idx % CHAR_BIT));
}

int get_nth_bit(unsigned char *bitmap, int idx) {
	unsigned char* bitmapClone = bitmap;
	return (bitmapClone[idx / CHAR_BIT] >> (idx % CHAR_BIT)) & 1;
}

void addBitMap(kma_page_t* page) {
  unsigned char bitmap[64] = { 0 };
  void* destination = (void*)((void*)page + 32);

  for (int i = 0; i < 3; i++) {
   set_nth_bit(bitmap, i);
  }
  memcpy(destination, &bitmap, 32);
}

void addToFreeList(free_block* currNode, size_t size) {
  free_block* freeList = (free_block*)((void *)(pageHeader) + sizeof(kma_page_t));
  for (int i=7; i >= 0; i--) {
	if (freeList[i].size == size) {
		currNode->nextFree = freeList[i].nextFree;
		freeList[i].nextFree = currNode;
		return;
	}
  }
}

kma_page_t* initializePage(kma_size_t size) {
	// Create a new page
  kma_page_t* page = get_page();

  *((kma_page_t**)page->ptr) = page;

  if ((size + sizeof(kma_page_t)) > page->size) {
  // requested size larger than page
	free_page(page);
	return NULL;
  }

  addBitMap(page);

  free_block* node = (free_block*)((void*)page + BITMAPSIZE + sizeof(kma_page_t) + 8);
  addToFreeList(node, 32);
  node = (free_block*)((void*)node + 32);

  for (int i=0; i < 6; i++) {
  	addToFreeList(node, 128*pow(2,i));
      node = (free_block*)((void*)node + (int)(128*pow(2,i)));
  }
  return page;
}

kma_size_t roundToPowerOfTwo(kma_size_t size) {
	for (int i=0; i < 8; i++) {
		if (size < 32*pow(2,i)) {
			return 32 * pow(2,i);
		}
	}
	return size;
}



free_block* allocateSpace(kma_size_t size) {
	// Find smallest possible block this size can fill
	free_block* freeList = (free_block*)((void*)pageHeader + sizeof(kma_page_t));
	kma_size_t sizeOfBlock = roundToPowerOfTwo(size);
	for (int i=0; i<8; i++) {
		if (size <= freeList[i].size && freeList[i].nextFree != NULL) {
			// Remove free node from list
			free_block* blockToAllocate = freeList[i].nextFree;
			freeList[i].nextFree = blockToAllocate->nextFree;
			// Split empty node until 
			//splitNode(sizeOfBlock, freeList, i);
			return blockToAllocate;
		}
	}
	return NULL;
}

void* kma_malloc(kma_size_t size)
{
  // Initialize free-list and bitmap
  if (!pageHeader) {
	initializeFreeList(size);
  }
  free_block* freeList = (free_block*)((void*)(pageHeader) + sizeof(kma_page_t));

  for (int i=0; i < 8; i++) {
	if (freeList[i].size > size) {
	  if (freeList[i].nextFree != NULL){
          //MALLOC
          return NULL;
        }
	}
  }

 kma_page_t* page = initializePage(size);
 // Actually fill the space
 allocateSpace(size);

 //malloc

  return page->ptr+sizeof(kma_page_t) ; //also bitmap size
}

void kma_free(void* ptr, kma_size_t size)
{
  ;
}

#endif // KMA_BUD
