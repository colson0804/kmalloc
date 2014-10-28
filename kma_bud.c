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


#define DEBUG 0

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

 kma_page_t* pageHeader = NULL;
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

void initializeFreeList() {
  kma_page_t* freeListPage;
  freeListPage = get_page();
  *((kma_page_t**)freeListPage->ptr) = freeListPage;
  
  D(printf("initializing freeList page...ptr: %x\n", (kma_page_t*)freeListPage->ptr));
  
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
  
  //D(printf("checking freeList for spot to add\n"));
  int i;
   for (i=7; i >= 0; i--) {
  //D(printf("comparing freeList[%d] (of size %d) to size: %d\n", i, freeList[i].size, sizeOfBlock));
  if (freeList[i].size == sizeOfBlock) {
    D(printf("adding node to freeList[%d], at size %d\n", i, freeList[i].size));
    currNode->nextFree = freeList[i].nextFree;
    freeList[i].nextFree = currNode;
    return;
  }
  }
  D(printf("it didnt find it??\n"));
}

kma_page_t* initializePage(kma_size_t size) {
  // Create a new page
  kma_page_t* page = get_page();

  *((kma_page_t**)page->ptr) = page;
  D(printf("\nno more space, initializing new page...ptr: %x\n", (kma_page_t*)page->ptr));

  kma_page_t* frontOfPage = (kma_page_t*)page->ptr;
  kma_page_t* startOfBitmap = (kma_page_t*)((void*)frontOfPage+roundToPowerOfTwo(sizeof(kma_page_t)));
  D(printf("startOfBitmap is: %x\n", startOfBitmap);) 
  addBitMap(startOfBitmap);  

  free_block* node = (free_block*)((void*)startOfBitmap + 32);
  int i;
  D(printf("node is at beginning of free space\n"));
  for (i=0; i < 7; i++) {
      D(printf("i is: %d\tsize is: %d\tnode is at %x\n", i,(64*power(2, i)), node));
      addToFreeList(node, 64*power(2,i));
      node = (free_block*)((void*)node + (int)(64*power(2,i)));
  }
  D(printf("finished initializing page...\n\n"));
  return frontOfPage;
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

void setBitMap(free_block* currNode, kma_size_t size){
  D(printf("start of setBitmap\n"));
  void* startOfPage = BASEADDR(currNode);
  unsigned char* bitmap = (unsigned char*)(startOfPage+ roundToPowerOfTwo(sizeof(kma_page_t)));
  D(printf("currNode is: %x\tstartOfPage: %x\tbitmap: %x\n", currNode, startOfPage, bitmap));

  int diff = (int)((void*)currNode - startOfPage);
  int blockOffset = diff/32;
  int numBits = size/32;  // Should be fine since we're only passing powers of two-

  int j;
  for(j = 0; j < numBits; j++){
    set_nth_bit(bitmap, blockOffset + j);
  }
}

void* splitNode(kma_size_t sizeOfBlock, free_block* blockPointer, free_block* freeList, int index){
  void* halfwayPoint;
  D(printf("INDEX IS: %d\n", index));
  D(printf("splitting node of size %d - going for size %d\n", freeList[index].size, sizeOfBlock));
  if (index < 0){
    return blockPointer;
  }
  else if (sizeOfBlock == freeList[index].size){
  D(printf("they're equal! returning from splitNode\n"));
  return blockPointer;
  }
  else{
    index = index -1;
    halfwayPoint = (void*)((void*)blockPointer + freeList[index].size);
    D(printf("BLOCK POINTER IS: %x, HALFWAY IS: %x, SIZE IS: %d\n", blockPointer, halfwayPoint, freeList[index].size));
    D(printf("not equal...blockPointer is %x and halfwayPoint is %x\n", blockPointer, halfwayPoint));
    addToFreeList(halfwayPoint, freeList[index].size);
    return splitNode(sizeOfBlock, blockPointer, freeList, index);
  }
}

free_block* allocateSpace(kma_size_t size) {
  // Find smallest possible block this size can fill
  free_block* freeList = (free_block*)((void*)pageHeader + sizeof(kma_page_t));
  kma_size_t sizeOfBlock = roundToPowerOfTwo(size);
  D(printf("allocating space...freeList is at %x\tsizeOfBlock is %d\n", freeList, sizeOfBlock));
        int i;
  for (i=0; i<8; i++) {

    if (sizeOfBlock == freeList[i].size && freeList[i].nextFree != NULL) {
    // Remove free node from list
    D(printf("freeList of exactly  size %d available at %x\n", freeList[i].size, freeList[i].nextFree));
    free_block* blockToAllocate = freeList[i].nextFree;
    freeList[i].nextFree = blockToAllocate->nextFree;
    setBitMap(blockToAllocate, sizeOfBlock);
    return blockToAllocate;
            }
            else if (sizeOfBlock < freeList[i].size && freeList[i].nextFree != NULL) {
      // Remove free node from list
      D(printf("free block of size %d available at %x\n", freeList[i].size, freeList[i].nextFree));
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
  D(printf("\n\n====BEGINNING MALLOC OF SIZE %d====\n\n", size));
  // Initialize free-list and bitmap
  if (!pageHeader) {
  initializeFreeList();
  }
  free_block* freeList = (free_block*)((void*)(pageHeader) + sizeof(kma_page_t));

  kma_page_t* allocatedPointer = (kma_page_t*)allocateSpace(size); 
  if (allocatedPointer != NULL) {
   return allocatedPointer; //also bitmap size
  } else {
   kma_page_t* newPage = initializePage(size);   
   // Actually fill the space
   allocatedPointer = (kma_page_t*) allocateSpace(size);
   D(printf("allocatedPointer is: %x\n", allocatedPointer));
   return allocatedPointer; //also bitmap size
  }
}

void clearBitMap(free_block* currNode, kma_size_t size){
  D(printf("clearing bitmap\n"));
  void* startOfPage = BASEADDR(currNode);
  unsigned char* bitmap = (unsigned char*)(startOfPage+ 32);

  int diff = (int)((void*)currNode - startOfPage);
  int blockOffset = diff/32;
  int numBits = size/32;

  int j;
  for(j = 0; j < numBits; j++){
    clear_nth_bit(bitmap, blockOffset + j);
  }
}

void coalesce(void* ptr, kma_size_t size){
  int sizeOfBlock = roundToPowerOfTwo(size);
  D(printf("checking for coalescing of size %d\n", sizeOfBlock));
  void* startOfPage = BASEADDR(ptr);
  unsigned char* bitmap = (unsigned char*)(startOfPage+32);

  int diff = (int)((void*)ptr - startOfPage);
  int blockOffset = diff/32;
  int numBits = sizeOfBlock/32;
  /*
    CHECK FOR BUDDY:
    if the offset is a multiple of the size, its the second buddy.
    if the offset is not a multiple of the size, its the first.
  */
   void* buddy = ptr;
   if (blockOffset % sizeOfBlock == 0){
    buddy += sizeOfBlock;
   }
   // else buddy -= size???
   int isBuddyFree = 1;
   int i;
   for(i=0; i < numBits; i++){
      // Why are we looking at blockOffset? Shouldn't we look at loc. of buddy?
      // i.e. buddyOffset?
     if (get_nth_bit(bitmap, blockOffset+i) == 1){
       isBuddyFree = 0;
     }
   }

   free_block* freeList = (free_block*)((void*)(pageHeader) + sizeof(kma_page_t));
   free_block* listNode = NULL;
   free_block* prevListNode = NULL;
   if (isBuddyFree){
    // Probably doesn't need to be a loop since we know size of blocks we're coalescing, but eh.
    int i;
     for (i=0;i < 8;i++){
      prevListNode = &freeList[i];
      listNode = freeList[i].nextFree;
      if(sizeOfBlock == listNode->size){
          while(listNode != NULL){
            if((void*)listNode == buddy){
              // Probably should look for either buddy or node we've freed,
                // then remove both of them. It looks like we're just removing buddy
              prevListNode->nextFree = listNode->nextFree;
              addToFreeList(listNode, sizeOfBlock*2);
              coalesce(ptr, sizeOfBlock*2);
              return;
            }
          }
          //IT WASNT IN THE LIST....
       }
     }
   }

}

void checkForFreePage(void* ptr){
  //PAGE FREEING
  //if the whole bitmap (besides first 3) is 0, free the page.
  //if there are no more free pages, free the pageHeader
  void* startOfPage = BASEADDR(ptr);
  unsigned char* bitmap = (unsigned char*)(startOfPage+ sizeof(kma_page_t));
  
  int isPageEmpty = 1;
  //not sure about 256....might be 64?
  int i;
  for (i = 3; i < 256; i++){
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
      }
    }
    free_page(startOfPage);
    if (isOnlyPage){
      free_page(pageHeader);
    }
  }
}


void kma_free(void* ptr, kma_size_t size) {
  D(printf("\n\n====BEGINNING FREE OF SIZE %d at ptr %x====\n\n", size, ptr));
  addToFreeList(ptr, size);
  clearBitMap(ptr, size);
  coalesce(ptr, size);
  checkForFreePage(ptr);

}

#endif // KMA_BUD
