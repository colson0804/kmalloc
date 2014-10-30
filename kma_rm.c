/***************************************************************************
 *  Title: Kernel Memory Allocator
 * -------------------------------------------------------------------------
 *    Purpose: Kernel memory allocator based on the resource map algorithm
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
  ***************************************************************************/
#ifdef KMA_RM
#define __KMA_IMPL__

//#define DEBUG 0

 #ifdef DEBUG
#  define D(x) x
#else
#  define D(x)
#endif

/************System include***********************************************/
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>

/************Private include**********************************************/
#include "kma_page.h"
#include "kma.h"

/************Defines and Typedefs*****************************************/
/*  #defines and typedefs should have their names in all caps.
 *  Global variables begin with g. Global constants with k. Local
 *  variables should be in all lower case. When initializing
 *  structures and arrays, line everything up in neat columns.
 */

typedef struct resource_map
{
  int size;
  struct resource_map* nextBase;
} free_block;

/************Global Variables*********************************************/

static kma_page_t* pageHeader = NULL;
size_t totalRequested = 0;
size_t totalNeeded = 0;
int mallocCounter = 0;
int freeCounter = 0;
int worstMallocTime = 0;
int worstFreeTime = 0;

/************Function Prototypes******************************************/

/************External Declaration*****************************************/

/**************Implementation***********************************************/

// Print free nodes for debugging
void printFreeList() {
  free_block* startOfList = (void*)pageHeader + sizeof(kma_page_t);
  free_block* node = startOfList->nextBase;
  while(node != NULL) {
    printf("Loc: %x\t", (unsigned int)node);
    printf("Size: %d\n", (int)node->size);
    node = node->nextBase;
  }
}

void removeFreeFromList(free_block* prevNode, kma_size_t size) {

    free_block* currNode = prevNode->nextBase;
    // If size allocated == size available, remove node
    if (currNode->size == size) {
      prevNode->nextBase = currNode->nextBase;
    } else {  // Otherwise, adjust size and header location
      int currNodeSize = currNode->size;
      free_block* currNodeNext = currNode->nextBase;
      currNode = (free_block*)((void *)(currNode)+size);
      
      prevNode->nextBase = currNode;
      currNode->nextBase = currNodeNext;
      currNode->size = currNodeSize - size; 
    }
    
  return;
}


free_block* findFreeBlockInsertionPoint(void* ptr){
  // Basically locates the free block that occurs directly before what we're adding
  // so we can update free list
  free_block* freeList = (free_block*)((void*)pageHeader + sizeof(kma_page_t));
  free_block* node = freeList;

  while (node) {
    if (node->nextBase == NULL){
      return node;
    }
    else if(node->nextBase >= (free_block*) ptr){ 
      return node;
    } else {
      node = node->nextBase;
    }
  }
  return NULL;
}

void*
kma_malloc(kma_size_t size)
{ 
  mallocCounter++;
  struct timeval start, end;
  int mallocTime;
  gettimeofday(&start, NULL);
  totalRequested += size;
  totalNeeded += size;

  if (pageHeader == NULL){
    /* If firstFree is undefined, point it to the top of the page */
    kma_page_t* page = get_page();
    totalNeeded += sizeof(kma_page_t);

    *((kma_page_t**)page->ptr) = page;

    // PageHeader is placed at beginning of page
    // points to next free block (after what we're allocating)
    pageHeader = (kma_page_t*)page->ptr;

    // Set header to beginning of page we're allocating
    free_block* freeList = (free_block*)((void*)(pageHeader) + sizeof(kma_page_t));
    freeList->size = -1;
    free_block* firstFree = (free_block*)((void*)(freeList) + sizeof(free_block) + size);
    freeList->nextBase = firstFree;
    firstFree->nextBase = NULL;
    firstFree->size = (PAGESIZE - sizeof(kma_page_t) - sizeof(free_block) - size);
    gettimeofday(&end, NULL);
    mallocTime = end.tv_usec - start.tv_usec;
    if (mallocTime > worstMallocTime) worstMallocTime = mallocTime;
    return (kma_page_t*)(((void *) pageHeader) + sizeof(kma_page_t) + sizeof(free_block));
  } 
  else {
    free_block* freeList = (free_block*)((void*)(pageHeader) + sizeof(kma_page_t));
    // Now execute cases where pageHeader->ptr is defined
    free_block* node = freeList->nextBase;
    // If prevNode is NULL, assume that previous is pageHeader
    free_block* prevNode = freeList;

    while (node->nextBase != NULL) {
      if ((node->size >= size) && (node->size - size > sizeof(free_block))) {
        // CHANGE FREE LIST
        removeFreeFromList(prevNode, size);
        gettimeofday(&end, NULL);
        mallocTime = end.tv_usec - start.tv_usec;
        if (mallocTime > worstMallocTime) worstMallocTime = mallocTime;
        return node;
      } else {
        prevNode = node;
        node = node->nextBase;
      }
    }

    // We're at the end of the free list
    // Check if there's room to allocate a new block
      // else create a new page
    if((node->size >= size)  && (node->size - size > sizeof(free_block))) {
      // Allocate this space
      // CHANGE FREE LIST
      removeFreeFromList(prevNode, size);
      gettimeofday(&end, NULL);
      mallocTime = end.tv_usec - start.tv_usec;
      if (mallocTime > worstMallocTime) worstMallocTime = mallocTime;
      return node;
    } else {
      kma_page_t* newPage = get_page();
      totalNeeded += sizeof(kma_page_t);
      *((kma_page_t**)newPage->ptr) = newPage;

      kma_page_t* newPageHeader = (kma_page_t*)newPage->ptr;

      free_block* newFreeList = (free_block*)((void*)newPageHeader + sizeof(kma_page_t));
      newFreeList->size = -1;
      free_block* newNode = (free_block*)((void*)newFreeList + sizeof(free_block) + size);

  	free_block* newPrevFree = findFreeBlockInsertionPoint((void*)newNode);

  	newNode->nextBase = newPrevFree->nextBase;
  	newPrevFree->nextBase = newNode;

      newNode->size = PAGESIZE - sizeof(kma_page_t) - sizeof(free_block) - size;

      gettimeofday(&end, NULL);
      mallocTime = end.tv_usec - start.tv_usec;
      if (mallocTime > worstMallocTime) worstMallocTime = mallocTime;
      return (free_block*)((void*)newFreeList + sizeof(free_block));

    }
  }
}


free_block* coalesce(free_block* prevFreeBlock, free_block* newNode) {

  if ((void*)prevFreeBlock + prevFreeBlock->size == (void*)newNode && (void*)newNode + newNode->size == (void*)newNode->nextBase) {  // Both previous and next node are free
    prevFreeBlock->size = prevFreeBlock->size + newNode->size + newNode->nextBase->size;
    prevFreeBlock->nextBase = newNode->nextBase->nextBase;
    return prevFreeBlock;
  } else if ((void*)prevFreeBlock + prevFreeBlock->size == (void*)newNode) { // Free block before it
    prevFreeBlock->size = prevFreeBlock->size + newNode->size;
    prevFreeBlock->nextBase = newNode->nextBase;
    return prevFreeBlock;
  } else if ((void*)newNode + newNode->size == (void*)newNode->nextBase) { // Free block after it
    newNode->size = newNode->size + newNode->nextBase->size;
    newNode->nextBase = newNode->nextBase->nextBase;
    return newNode;
  }
  return newNode; 
} 

kma_page_t* findNextPage(free_block* currNode) {
  if (currNode->nextBase != NULL) {
    return (kma_page_t*)BASEADDR(currNode->nextBase);
  }
  else return NULL;
}

void kma_free(void* ptr, kma_size_t size) {

  freeCounter++;
  free_block* prevFreeBlock = NULL;
  free_block* startOfFreeMemory = (free_block*)((void*)pageHeader + sizeof(kma_page_t));
  struct timeval start, end;
  int freeTime;
  gettimeofday(&start, NULL);

    free_block* newNode = NULL;
    free_block* firstFreeBlock = startOfFreeMemory->nextBase;

      prevFreeBlock = findFreeBlockInsertionPoint(ptr);

      // Insert into list
      newNode = (free_block*) ptr;
      newNode->nextBase = prevFreeBlock->nextBase;
      prevFreeBlock->nextBase = newNode;

      newNode->size = size;
      free_block* coalescedBlock = coalesce(prevFreeBlock, newNode);

  // Handle possible free pages
  if (coalescedBlock->size == PAGESIZE - sizeof(kma_page_t) - sizeof(free_block)) {
    // Find the beginning of current page
    kma_page_t* pageToFree = BASEADDR(coalescedBlock);
    pageToFree = *((kma_page_t**)pageToFree);

    // Find out if this is first page
    if (pageToFree->ptr == pageHeader) {
      // Need to reset first block to that of next page
      // Also check if this is only page
      if (firstFreeBlock->nextBase == NULL) {
        free_page(pageToFree);
        pageHeader = NULL;
        gettimeofday(&end, NULL);
        freeTime = end.tv_usec - start.tv_usec;
        if (freeTime > worstFreeTime) worstFreeTime = freeTime;
        printf("Total requested: %d\n", (int)totalRequested);
        printf("Total needed: %d\n", (int)totalNeeded);
        printf("Number of mallocs: %d\n", mallocCounter);
        printf("Number of frees: %d\n", freeCounter);
        printf("Worst allocation time (microseconds)): %d\n", (int) worstMallocTime);
        printf("Worst free time (microseconds)): %d\n\n", (int) worstFreeTime);
        return;
      } else {
        kma_page_t* nextPage = findNextPage(coalescedBlock);
        free_block* newFreeHeader = (free_block*)((void*)nextPage + sizeof(kma_page_t));
        newFreeHeader->nextBase = coalescedBlock->nextBase;
        pageHeader = (kma_page_t*) nextPage;
      }
    } else {
      // Still need to adjust pointer
      prevFreeBlock = findFreeBlockInsertionPoint(coalescedBlock);
      prevFreeBlock->nextBase = coalescedBlock->nextBase;
    }

    free_page(pageToFree);
    
  }

  gettimeofday(&end, NULL);
  freeTime = end.tv_usec - start.tv_usec;
  if (freeTime > worstFreeTime) worstFreeTime = freeTime;
  
  printf("Total requested: %d\n", (int)totalRequested);
  printf("Total needed: %d\n", (int)totalNeeded);
  printf("Number of mallocs: %d\n", mallocCounter);
  printf("Number of frees: %d\n", freeCounter);
  printf("Worst allocation time (microseconds)): %d\n", (int) worstMallocTime);
  printf("Worst free time (microseconds): %d\n\n", (int) worstFreeTime);
}

#endif // KMA_RM
