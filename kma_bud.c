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



//#define DEBUG 1

 #ifdef DEBUG
#  define D(x) x
#else
#  define D(x)
#endif

/************System include***********************************************/
#include <assert.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
/************Private include**********************************************/
#include "kma_page.h"
#include "kma.h"
    
/************Defines and Typedefs*****************************************/
/*  #defines and typedefs should have their names in all caps    *  Global variables begin with g. Global constants with k. Local
 *  variables should be in all lower case. When initializing
 *  structures and arrays, line everything up in neat columns.
 */

 typedef struct free_list
 {
  int size;
  struct free_list* nextFree;
 } free_block;

/************Global Variables*********************************************/

 static kma_page_t* pageHeader = NULL;
 size_t totalRequested = 0;
 size_t totalNeeded = 0;
 int mallocCounter = 0;
 int freeCounter = 0;
 time_t worstMallocTime = 0;
 time_t worstFreeTime = 0;

/************Function Prototypes******************************************/
  
/************External Declaration*****************************************/

/**************Implementation***********************************************/
kma_size_t power(int base, int exp) {
  int i;
  int ret = 1;
  if (exp == 0) {
    return 1;
  }
  for (i = 0; i < exp; i++) {
    ret = base * ret;
  }
  return (kma_size_t) ret;
}



kma_size_t roundToPowerOfTwo(kma_size_t size) {
  int i;
  for (i=0; i < 8; i++) {
    if (size <= 32*power(2,i)) {
      return 32 * power(2,i);
    }
  }
  return size;
}

void initializeFreeList() {
  kma_page_t* freeListPage;
  freeListPage = get_page();
  *((kma_page_t**)freeListPage->ptr) = freeListPage;
  
  pageHeader = (kma_page_t*)freeListPage->ptr;

  // Place free list immediately after header
  free_block* freeList = (free_block*)((void *)(pageHeader) + sizeof(kma_page_t));
  int i = 0;
  for (i=0; i < 8; i++) {
  freeList[i].size = MINBLOCKSIZE*power(2, i);
  freeList[i].nextFree = NULL;
  }
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

void addBitMap(void* destination) {
  unsigned char bitmap[32] = { 0 };
  int i = 0;
  for (i = 0; i < 2; i++) {
   set_nth_bit(bitmap, i);
  }
  memcpy(destination, &bitmap, 32);
}

void addToFreeList(free_block* currNode, size_t size) {
  free_block* freeList = (free_block*)((void *)(pageHeader) + sizeof(kma_page_t));
  
  int sizeOfBlock = roundToPowerOfTwo(size);
  int i;
   for (i=7; i >= 0; i--) {

	if (freeList[i].size == sizeOfBlock) {
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

  kma_page_t* frontOfPage = (kma_page_t*)page->ptr;
  kma_page_t* startOfBitmap = (kma_page_t*)((void*)frontOfPage+roundToPowerOfTwo(sizeof(kma_page_t)));
  addBitMap(startOfBitmap);  

  free_block* node = (free_block*)((void*)startOfBitmap + 32);
  int i;
  for (i=0; i < 7; i++) {
      addToFreeList(node, 64*power(2,i));
      node = (free_block*)((void*)node + (int)(64*power(2,i)));
  }
  return frontOfPage;
}

void setBitMap(free_block* currNode, kma_size_t size){
  void* startOfPage = BASEADDR(currNode);
  unsigned char* bitmap = (unsigned char*)(startOfPage+ roundToPowerOfTwo(sizeof(kma_page_t)));

  int offset = (int)((void*)currNode - startOfPage);
  int blockOffset = offset/32;
  int numBits = size/32;	// Should be fine since we're only passing powers of two-


  int j;
  for(j = 0; j < numBits; j++){
    set_nth_bit(bitmap, blockOffset + j);
  }
}

void* splitNode(kma_size_t sizeOfBlock, free_block* blockPointer, free_block* freeList, int index){
  void* halfwayPoint;
  if (index < 0){
    return blockPointer;
  }
  else if (sizeOfBlock == freeList[index].size){
	  return blockPointer;
  }
  else{
    index = index -1;
    halfwayPoint = (void*)((void*)blockPointer + freeList[index].size);
    addToFreeList(halfwayPoint, freeList[index].size);
    return splitNode(sizeOfBlock, blockPointer, freeList, index);
  }
}

free_block* allocateSpace(kma_size_t size) {

	// Find smallest possible block this size can fill
	free_block* freeList = (free_block*)((void*)pageHeader + sizeof(kma_page_t));
	kma_size_t sizeOfBlock = roundToPowerOfTwo(size);
  int i;
	for (i=0; i<8; i++) {

		if (sizeOfBlock == freeList[i].size && freeList[i].nextFree != NULL) {
  		// Remove free node from list
  		free_block* blockToAllocate = freeList[i].nextFree;
  		freeList[i].nextFree = blockToAllocate->nextFree;
  		setBitMap(blockToAllocate, sizeOfBlock);
  		return blockToAllocate;
    } else if (sizeOfBlock < freeList[i].size && freeList[i].nextFree != NULL) {
			// Remove free node from list
			free_block* blockToAllocate = freeList[i].nextFree;
			freeList[i].nextFree = blockToAllocate->nextFree;
			// Split empty node until 
			void* allocationPoint = splitNode(sizeOfBlock, blockToAllocate, freeList, i--);
             		setBitMap(allocationPoint, sizeOfBlock);
                   	return allocationPoint;
		}
	}
	return NULL;
}

void* kma_malloc(kma_size_t size)
{
  // Initialize free-list and bitmap
  mallocCounter++;
  totalRequested += size;
  time_t start = time(NULL);
  time_t end;
  time_t mallocTime;
  totalNeeded = totalNeeded + size + roundToPowerOfTwo(size);
	if(size > 4096){
		kma_page_t* page;
  		page = get_page();
		*((kma_page_t**)page->ptr) = page;
    end = time(NULL);
    mallocTime = end - start;
    if (mallocTime > worstMallocTime) worstMallocTime = mallocTime;
		return page->ptr + sizeof(kma_page_t*);
	}

  if (!pageHeader) {
  initializeFreeList();
  }
	
  kma_page_t* allocatedPointer = (kma_page_t*)allocateSpace(size); 
  if (allocatedPointer != NULL) {

	return allocatedPointer; //also bitmap size
  } else {
	 initializePage(size);	 
	 // Actually fill the space
	 allocatedPointer = (kma_page_t*) allocateSpace(size);

   end = time(NULL);
   mallocTime = end - start;
   if (mallocTime > worstMallocTime) worstMallocTime = mallocTime;
	 return allocatedPointer; //also bitmap size

  }
}

void clearBitMap(free_block* currNode, kma_size_t size){
  int sizeOfBlock = roundToPowerOfTwo(size);
  void* startOfPage = BASEADDR(currNode);
  unsigned char* bitmap = (unsigned char*)(startOfPage+ 32);


  int offset = (int)((void*)currNode - startOfPage);
  int blockOffset = offset/32;
  int numBits = sizeOfBlock/32;

  int j;
  for(j = 0; j < numBits; j++){
    clear_nth_bit(bitmap, blockOffset + j);
  }
}

void coalesce(void* ptr, kma_size_t size){

  int sizeOfBlock = roundToPowerOfTwo(size);

	if(sizeOfBlock == 4096){
		return;
	}

  void* startOfPage = BASEADDR(ptr);
  unsigned char* bitmap = (unsigned char*)(startOfPage+32);

  int offset = (int)((void*)ptr - startOfPage);
  int numBits = sizeOfBlock/32;
  /*
    CHECK FOR BUDDY:
    if the offset is a multiple of the size, its the second buddy.
    if the offset is not a multiple of the size, its the first.
  */
   void* buddyPtr = ptr;


   if (((offset / sizeOfBlock) % 2) == 0){

    buddyPtr += sizeOfBlock;
   }
   else{
    buddyPtr -= sizeOfBlock;
   }

   int isBuddyFree = 1;
   int i;
   int buddyDiff = (int)((void*)buddyPtr - startOfPage);
   int buddyOffset = buddyDiff/32;

   for(i=0; i < numBits; i++){
     if (get_nth_bit(bitmap, buddyOffset+i) == 1){
       isBuddyFree = 0;
     }
   }

   free_block* freeList = (free_block*)((void*)(pageHeader) + sizeof(kma_page_t));
   free_block* listNode = NULL;
   free_block* prevListNode = NULL;
   if (isBuddyFree){

	   int i = ((int)log2(sizeOfBlock)) - 5;

     
      prevListNode = &freeList[i];
      listNode = freeList[i].nextFree;

	    int buddyFreed = 0;
	    int ptrFreed = 0;
      while(listNode != NULL){
        if((void*)listNode == buddyPtr){
      		buddyFreed = 1;
      		prevListNode->nextFree = listNode->nextFree;
      		listNode = listNode->nextFree;
      		if (ptrFreed){
      			break;
      		}
        } else if ((void*)listNode == ptr){
      		ptrFreed = 1;
      		prevListNode->nextFree = listNode->nextFree;
      		listNode = listNode->nextFree;
      		if (buddyFreed){
      			break;
      		}
	     } else{
			     prevListNode = listNode;
	    		 listNode = listNode->nextFree;
		    }
      }

	if (buddyFreed && ptrFreed){
		free_block* startOfFreeBlock;
		if(buddyPtr < ptr){
			startOfFreeBlock = buddyPtr;		
		}
		else{
			startOfFreeBlock = ptr;
		}
	      addToFreeList(startOfFreeBlock, sizeOfBlock*2);
        coalesce(startOfFreeBlock, sizeOfBlock*2);
        return;  
	}
	else {
		return;	
	}

   }

}

void checkForFreePage(void* ptr){
  //PAGE FREEING
  //if the whole bitmap (besides first 3) is 0, free the page.
  //if there are no more free pages, free the pageHeader
  void* startOfPage = BASEADDR(ptr);
  unsigned char* bitmap = (unsigned char*)(startOfPage+ roundToPowerOfTwo(sizeof(kma_page_t)));
  
  int isPageEmpty = 1;
  //not sure about 256....might be 64?
  int i;
  for (i = 2; i < 256; i++){
    if(get_nth_bit(bitmap, i) == 1){
      isPageEmpty = 0;
      break;
    }
  }

  free_block* freeList = (free_block*)((void*)(pageHeader) + sizeof(kma_page_t));
  
  void* endOfPage = BASEADDR(ptr) + PAGESIZE;
  
  //want to check if there are blocks in freeList not on this page
  int isOnlyPage = 1;
  if (isPageEmpty){
    free_block* prevListNode;
    free_block* listNode;
    int i;
    for (i = 0; i<8;i++){
      prevListNode = &freeList[i];
      listNode = freeList[i].nextFree;
      while(listNode != NULL){
        if(((void*)listNode > startOfPage) &&((void*)listNode < endOfPage)){
          prevListNode->nextFree = listNode->nextFree;
        }
        else{
          isOnlyPage = 0;
        }
	prevListNode = listNode;
	listNode = listNode->nextFree;
      }
    }
	kma_page_t* pagePtr = *((kma_page_t**)(startOfPage));

    free_page(pagePtr);
    if (isOnlyPage){
	     kma_page_t* pageHeaderPtr = *((kma_page_t**)(pageHeader));
       free_page(pageHeaderPtr);
	     pageHeader = NULL;
    }
  }
}

void kma_free(void* ptr, kma_size_t size) {
  freeCounter++;
  time_t start = time(NULL);
  time_t end;
  time_t freeTime;

	if (size > 4096){
		kma_page_t* page;  
  	page = *((kma_page_t**)(ptr - sizeof(kma_page_t*))); 
  	free_page(page);
    end = time(NULL);
    freeTime = end - start;
    if (freeTime > worstFreeTime) worstFreeTime = freeTime;

    printf("Total requested: %d\n", (int)totalRequested);
    printf("Total needed: %d\n", (int)totalNeeded);
    printf("Number of mallocs: %d\n", mallocCounter);
    printf("Number of frees: %d\n", freeCounter);
    printf("Worst allocation time: %d\n", (int) worstMallocTime);
    printf("Worst free time: %d\n\n", (int) worstFreeTime);
		return;
	}

  addToFreeList(ptr, size);
  clearBitMap(ptr, size);
  coalesce(ptr, size);
  checkForFreePage(ptr);

  end = time(NULL);
  freeTime = end - start;
  if (freeTime > worstFreeTime) worstFreeTime = freeTime;

  printf("Total requested: %d\n", (int)totalRequested);
  printf("Total needed: %d\n", (int)totalNeeded);
  printf("Number of mallocs: %d\n", mallocCounter);
  printf("Number of frees: %d\n", freeCounter);
  printf("Worst allocation time: %d\n", (int) worstMallocTime);
  printf("Worst free time: %d\n\n", (int) worstFreeTime);
}

#endif // KMA_BUD

