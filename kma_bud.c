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

 typedef struct bit 
 {
 	int val: 1;
 } bit;

/************Global Variables*********************************************/

 kma_page_t* pageHeader = NULL;

/************Function Prototypes******************************************/
	
/************External Declaration*****************************************/

/**************Implementation***********************************************/
kma_page_t* initializeFreeList(kma_size_t size) {
	kma_page_t* freeListPage = get_page();
	*((kma_page_t**)freeListPage->ptr) = freeListPage;

	if ((size + sizeof(kma_page_t)) > freeListPage->size) {
		// requested size larger than page
		free_page(freeListPage);
		return NULL;
	}
	pageHeader = freeListPage;

	// Place free list immediately after header
	free_block* freeList = (free_block*)((long)(pageHeader) + sizeof(kma_page_t));
	for (int i=0; i < 8; i++) {
		freeList[i].size = 32*pow(2, i);
		freeList[i].nextFree = NULL;
	}

	return freeListPage;
}

void set_nth_bit(unsigned char *bitmap, int idx)
{
    bitmap[idx / CHAR_BIT] |= 1 << (idx % CHAR_BIT);
}

void clear_nth_bit(unsigned char *bitmap, int idx)
{
    bitmap[idx / CHAR_BIT] &= ~(1 << (idx % CHAR_BIT));
}

int get_nth_bit(unsigned char *bitmap, int idx)
{
    return (bitmap[idx / CHAR_BIT] >> (idx % CHAR_BIT)) & 1;
}

void addBitMap(kma_page_t* page) {
	unsigned char bitmap[32] = { 0 };
	void* destination = (void*)((long)page + 32);

	for (int i = 0; i < 3; i++) {
		set_nth_bit(bitmap, i);
	}
	memcpy(destination, &bitmap, 32);

}

void* kma_malloc(kma_size_t size)
{
  // Initialize free-list and bitmap
  if (!pageHeader) {
  	initializeFreeList(size);
  }
  free_block* freeList = (free_block*)((long)(pageHeader) + sizeof(kma_page_t));

  for (int i=0; i < 8; i++) {
  	if (freeList[i].size > size) {
  		//malloc
  	}
  }

  // Create a new page
  kma_page_t* page = get_page();

  *((kma_page_t**)page->ptr) = page;

  if ((size + sizeof(kma_page_t)) > page->size) {
  // requested size larger than page
    free_page(page);
    return NULL;
  }

 addBitMap(page);

  return NULL;
}

void kma_free(void* ptr, kma_size_t size)
{
  ;
}

#endif // KMA_BUD
