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
 *
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

/************Function Prototypes******************************************/

/************External Declaration*****************************************/

/**************Implementation***********************************************/

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
  // If prevBlock is NULL, then previous is pageHeader

  // if (prevNode == NULL) {
  //   // PREVNODE IS Freelist header
  //   free_block* currNode = pageHeader->ptr;
  //   if (currNode->size == size) {
  //     pageHeader->ptr = currNode->nextBase;
  //   } else {
  //     int currNodeSize = currNode->size;
  //     free_block* currNodeNext = currNode->nextBase;
  //     currNode = (free_block*)((void *)(currNode)+size); 
      
  //     //currNode = pageHeader->ptr;
  //     pageHeader->ptr = currNode;
  //     currNode->size = currNodeSize - size;
  //     currNode->nextBase = currNodeNext;
  //     //currNode->nextBase = NULL;  WHY?
  //   }
  //} else {
    free_block* currNode = prevNode->nextBase;
    // If size allocated == size available, remove node
    if (currNode->size == size) {
      prevNode->nextBase = currNode->nextBase;
    } else {
      int currNodeSize = currNode->size;
      free_block* currNodeNext = currNode->nextBase;
      currNode = (free_block*)((void *)(currNode)+size); //REALLY NOT SURE
      
      prevNode->nextBase = currNode;
      currNode->nextBase = currNodeNext;
      currNode->size = currNodeSize - size; 
      //prevNode->nextBase->nextBase = NULL;
    }
    D(printf("New free block is at: %x\n", (void*)currNode));
    D(printf("New size of free block: %d\n\n\n", (int)currNode->size));
  //}
  return;
}

void*
kma_malloc(kma_size_t size)
{ 
  //D(if (size == 623) exit(0));

  if (pageHeader == NULL){
    /* If firstFree is undefined, point it to the top of the page */
    kma_page_t* page = get_page();

    *((kma_page_t**)page->ptr) = page;
    D(printf("Here is page: %x\n", (unsigned int)page));

    D(printf("initializing freeList page...ptr: %x\n", (kma_page_t*)page->ptr));

    // PageHeader is placed at beginning of page
    // points to next free block (after what we're allocating)
    pageHeader = (kma_page_t*)page->ptr;
    D(printf("Where is pageHeader pointing???%x\n", (unsigned int)pageHeader->ptr));

    D(printf("PageHeader is at: %x\n", (kma_page_t*)pageHeader));

    //pageHeader->ptr = (free_block*)((void *)page + sizeof(kma_page_t) + size);

    // Set header to beginning of page we're allocating
    free_block* freeList = (free_block*)((void*)(pageHeader) + sizeof(kma_page_t));
    freeList->size = -1;
    D(printf("Freelist is at: %x\n", (kma_page_t*)freeList));

    D(printf("Now allocating block of size: %d\n", (int) size));
    D(printf("----------------------------------\n"));
    D(printf("Start of allocation is: %x\n", ((void *) pageHeader) + sizeof(kma_page_t) + sizeof(free_block)));
    free_block* firstFree = (free_block*)((void*)(freeList) + sizeof(free_block) + size);
    D(printf("New free block is at: %x\n", (void*)firstFree));
    freeList->nextBase = firstFree;
    firstFree->nextBase = NULL;
    firstFree->size = (PAGESIZE - sizeof(kma_page_t) - sizeof(free_block) - size);
    D(printf("New size of free block: %d\n", (int)firstFree->size));
    D(printf("What are we returning?: %x\n\n\n", ((void *) pageHeader) + sizeof(kma_page_t) + sizeof(free_block)));
    //D(exit(0));
    D(printFreeList());
    return (kma_page_t*)(((void *) pageHeader) + sizeof(kma_page_t) + sizeof(free_block));
  } 
  else {
    D(printf("Attempting to allocate block of size: %d\n", (int) size));
    D(printf("----------------------------------\n"));

    free_block* freeList = (free_block*)((void*)(pageHeader) + sizeof(kma_page_t));
    // Now execute cases where pageHeader->ptr is defined
    free_block* node = freeList->nextBase;
    // If prevNode is NULL, assume that previous is pageHeader
    free_block* prevNode = freeList;

    D(printf("Start of freeList: %x\n", freeList));

    while (node->nextBase != NULL) {
      if ((node->size >= size) && (node->size - size > sizeof(free_block))) {
        // CHANGE FREE LIST
        D(printf("Start of allocation is: %x\n", ((void *) node)));
        removeFreeFromList(prevNode, size);
        D(printFreeList());
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
      D(printFreeList());
      return node;
    } else {
      D(printf("MAKING A NEW PAGE\n"));
      kma_page_t* newPage = get_page();
      *((kma_page_t**)newPage->ptr) = newPage;
      D(printf("First page: %x\n", (void*)pageHeader));
      //D(printf("New page: %x\n", (void*)newPage->ptr));
      //pageHeader = (kma_page_t*)page->ptr;

      kma_page_t* newPageHeader = (kma_page_t*)newPage->ptr;
      D(printf("New page: %x\n", (void*)newPage->ptr));
      free_block* newFreeList = (free_block*)((void*)newPageHeader + sizeof(kma_page_t));
      newFreeList->size = -1;
      free_block* newNode = (free_block*)((void*)newFreeList + sizeof(free_block) + size);
      node->nextBase = newNode;

      newNode->nextBase = NULL;
      newNode->size = PAGESIZE - sizeof(kma_page_t) - sizeof(free_block) - size;
      D(printf("Page size: %d\n", (int) node->nextBase->size));
      //node->nextBase = newPage->ptr + sizeof(kma_page_t*);
      D(printFreeList);
      return (free_block*)((void*)newFreeList + sizeof(free_block));
    }
  }
}

free_block* findFreeBlockInsertionPoint(void* ptr){
  free_block* freeList = (free_block*)((void*)pageHeader + sizeof(kma_page_t));
  free_block* node = freeList;
  printf("What the fuck is the node? Is it Null? Please tell me it\'s not null: %x\n", (unsigned int)node);
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

free_block* coalesce(free_block* prevFreeBlock, free_block* newNode) {
  // D(printf("newNode: %x\n", (void*)newNode));
  // D(printf("newNode->size: %x\n", (void*)newNode->size));
  // D(printf("newNode->nextBase: %x\n", (void*)newNode->nextBase));
  if ((void*)prevFreeBlock + prevFreeBlock->size == (void*)newNode && (void*)newNode + newNode->size == (void*)newNode->nextBase) {  // Both previous and next node are free
    D(printf("Size of block in coalesce: %d\n", prevFreeBlock->size));
    D(printf("Size of block in coalesce: %d\n", newNode->size));
    D(printf("Size of block in coalesce: %d\n", newNode->nextBase->size));
    prevFreeBlock->size = prevFreeBlock->size + newNode->size + newNode->nextBase->size;
    prevFreeBlock->nextBase = newNode->nextBase->nextBase;
    return prevFreeBlock;
  } else if ((void*)prevFreeBlock + prevFreeBlock->size == (void*)newNode) { // Free block before it
    prevFreeBlock->size = prevFreeBlock->size + newNode->size;
    prevFreeBlock->nextBase = newNode->nextBase;
    return prevFreeBlock;
  } else if ((void*)newNode + newNode->size == (void*)newNode->nextBase) { // Free block after it
    D(printf("here\n"));
    newNode->size = newNode->size + newNode->nextBase->size;
    newNode->nextBase = newNode->nextBase->nextBase;
    return newNode;
  }
  return newNode; 
} 

kma_page_t* findNextPage(free_block* currNode) {
  if (currNode->nextBase != NULL) {
    return BASEADDR(currNode->nextBase);
  }
  else return NULL;
}

void kma_free(void* ptr, kma_size_t size) {
  
  //D(exit(0));
  D(printf("Attempting to free block of size: %d\n", (int) size));
  D(printf("------------------------------------\n"));
  free_block* prevFreeBlock = NULL;
  free_block* startOfFreeMemory = (free_block*)((void*)pageHeader + sizeof(kma_page_t));

  if (pageHeader==NULL) // Something went wrong
    return;

  // if (pageHeader->ptr == NULL) { //nothing
  //   pageHeader->ptr = (free_block*) ptr;
  //   startOfFreeMemory = pageHeader->ptr;
  // } else {

    free_block* newNode = NULL;
    free_block* firstFreeBlock = (free_block*)(startOfFreeMemory->nextBase);
    D(printf("First free node is: %x\n", (void*) firstFreeBlock));
    

    // if (firstFreeBlock > (free_block*) ptr){
    //   newNode = (free_block*) ptr; 
    //   newNode->nextBase = firstFreeBlock;
    //   newNode->size = size;
    //   pageHeader->ptr = newNode;
    //   startOfFreeMemory = coalesce(newNode, firstFreeBlock);
    // }
    //else{
      prevFreeBlock = findFreeBlockInsertionPoint(ptr);
      D(printf("Block insertion point: %x\n", (void*) prevFreeBlock));


       //D(printFreeList(startOfFreeMemory));
      // Insert into list
      newNode = (free_block*) ptr;
      newNode->nextBase = prevFreeBlock->nextBase;
      prevFreeBlock->nextBase = newNode;

      newNode->size = size;
       D(printFreeList());
      free_block* coalescedBlock = coalesce(prevFreeBlock, newNode);
      D(printf("Size after coalescing: %d\n", (int) coalescedBlock->size));
   // }
  //} 
      //D(printFreeList(coalescedBlock));
      //D(exit(0));
  // Handle possible free pages
  if (coalescedBlock->size == PAGESIZE - sizeof(kma_page_t) - sizeof(free_block)) {
    D(printf("NOW ATTEMPTING TO FREE A PAGE\n"));
    // Find the beginning of current page
    //free_block* coalescedCopy = coalescedBlock;
    kma_page_t* pageToFree = BASEADDR(coalescedBlock);
    pageToFree = *((kma_page_t**)pageToFree);
    D(printf("WHAT\n"));
    D(printf("address of coalesced block: %x\n", (void*)coalescedBlock));
    D(printf("beginning of page to free: %x\n", pageToFree));

    // Find out if this is first page
    if (pageToFree->ptr == pageHeader) {
      D(printf("uhhh\n"));
      // Need to reset first block to that of next page
      // Also check if this is only page
      if (firstFreeBlock->nextBase == NULL) {
        free_page(pageToFree);
        pageHeader = NULL;
        return;
      } else {
        kma_page_t* nextPage = findNextPage(coalescedBlock);
        free_block* newFreeHeader = (free_block*)((void*)nextPage + sizeof(kma_page_t));
        newFreeHeader->nextBase = coalescedBlock->nextBase;
        pageHeader = (kma_page_t*) nextPage->ptr;
      }
    } else {
      D(printf("Here\n"));
      // Still need to adjust pointer
      prevFreeBlock = findFreeBlockInsertionPoint(coalescedBlock);
      D(printf("Previous free block: %d\n", (int) prevFreeBlock->size));
      prevFreeBlock->nextBase = coalescedBlock->nextBase;
      //D(printf("Next base size: %d\n", (int)coalescedBlock->nextBase->size));
    }


    // free_block* newFirstFree = firstFreeBlock->nextBase;

    // free_block* newPageHeader = BASEADDR(newFirstFree);
    // if (BASEADDR(firstFreeBlock) == pageHeader) {
    //   free_block* newStartOfList = (free_block*)((void*)newPageHeader + sizeof(kma_page_t));
    //   newStartOfList->nextBase = newFirstFree;
    // }

    free_page(pageToFree);
    D(printf("got here\n"));
  }
}

#endif // KMA_RM
