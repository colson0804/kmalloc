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

void removeFreeFromList(free_block* prevNode, kma_size_t size) {
  // If prevBlock is NULL, then previous is pageHeader

  if (prevNode == NULL) {
    // PREVNODE IS PAGE HEADER
    free_block* currNode = pageHeader->ptr;
    if (currNode->size == size) {
      pageHeader->ptr = currNode->nextBase;
    } else {
      pageHeader->ptr += size;
      currNode->size = currNode->size - size;
    }
  } else {
    free_block* currNode = prevNode->nextBase;
    // If size allocated == size available, remove node
    if (currNode->size == size) {
      prevNode->nextBase = currNode->nextBase;
    } else {
      prevNode->nextBase += size; //REALLY NOT SURE
      currNode->size = currNode->size - size;
    }
  }
  return;
}

void*
kma_malloc(kma_size_t size)
{ 
  if (pageHeader == NULL){
    /* If firstFree is undefined, point it to the top of the page */
    kma_page_t* page = get_page();

    *((kma_page_t**)page->ptr) = page;
    
    if ((size + sizeof(kma_page_t*)) > page->size) {
    // requested size larger than page
      free_page(page);
      return NULL;
    }
    // PageHeader is placed at beginning of page
      // points to next free block (after what we're allocating)
    pageHeader = page;

    pageHeader->ptr = (kma_page_t*)(page+(sizeof(kma_page_t) + size)*(sizeof(kma_page_t*)));

    // Set new freeblock to end of block we're allocating
    free_block* freeBlock = (free_block*) pageHeader->ptr;
    freeBlock->size = page->size - sizeof(kma_page_t) - size;
    freeBlock->nextBase = NULL;
    return page + sizeof(kma_page_t);
    //return freeBlock;
    // After this, check if this slot is right after pageHeader
  } else {

    // Now execute cases where firstFree has been defined
    free_block* node = pageHeader->ptr;
    // If prevNode is NULL, assume that previous is pageHeader
    free_block* prevNode = NULL;

    while (node->nextBase != NULL) {
      if (node->size >= size) { // + 8???
        // CHANGE FREE LIST
        removeFreeFromList(prevNode, size);
        return node;
      } else {
        prevNode = node;
        node = node->nextBase;
      }
    }
    // We're at the end of the free list
    // Check if there's room to allocate a new block
      // else create a new page
    if(node->size >= size) {
      // Allocate this space
      // CHANGE FREE LIST
      removeFreeFromList(prevNode, size);
      return node;
    } else {
      kma_page_t* newPage = get_page();
      *((kma_page_t**)newPage->ptr) = newPage;
      if ((size + sizeof(kma_page_t*)) > newPage->size) {
        // Requested size larger than page
        free_page(newPage);
        return NULL;
      } 
      node->nextBase = newPage->ptr + sizeof(kma_page_t*);
      return newPage->ptr;
    }
  }
}

free_block* findFreeBlockInsertionPoint(void* ptr){
  free_block* node = pageHeader->ptr;
  
  while (node) {
    if (node->nextBase == NULL || node->nextBase >= (free_block*) ptr) {
      return node;
    } else {
      node = node->nextBase;
    }
  }
  return NULL;
}

free_block* coalesce(free_block* prevFreeBlock, free_block* newNode) {
  
  if (prevFreeBlock + prevFreeBlock->size == newNode && newNode + newNode->size == newNode->nextBase) {  // Both previous and next node are free
    prevFreeBlock->size = prevFreeBlock->size + newNode->size + newNode->nextBase->size;
    prevFreeBlock->nextBase = newNode->nextBase->nextBase;
    return prevFreeBlock;
  } else if (prevFreeBlock + prevFreeBlock->size == newNode) { // Free block before it
    prevFreeBlock->size = prevFreeBlock->size + newNode->size;
    prevFreeBlock->nextBase = newNode->nextBase;
    return prevFreeBlock;
  } else if (newNode + newNode->size == newNode->nextBase) { // Free block after it
    newNode->size = newNode->size + newNode->nextBase->size;
    newNode->nextBase = newNode->nextBase->nextBase;
    return newNode;
  }
  return newNode; 
} 

void kma_free(void* ptr, kma_size_t size) {
  
  free_block* prevFreeBlock = NULL;
  free_block* startOfFreeMemory = NULL;

  if (pageHeader==NULL) // Something went wrong
    return;

  if (pageHeader->ptr == NULL) {
    pageHeader->ptr = (free_block*) ptr;
    startOfFreeMemory = pageHeader->ptr;
  } else {
    prevFreeBlock = findFreeBlockInsertionPoint(ptr);

    // Insert into list
    free_block* newNode = (free_block*) ptr;
    newNode->nextBase = prevFreeBlock->nextBase;
    prevFreeBlock->nextBase = newNode;

    newNode->size = size;

    startOfFreeMemory = coalesce(prevFreeBlock, newNode);
  }
  // Handle possible free pages
  if (startOfFreeMemory->size == PAGESIZE - sizeof(kma_page_t*)) {
    if (pageHeader->ptr == startOfFreeMemory) {
      pageHeader = BASEADDR(startOfFreeMemory->nextBase);
      pageHeader->ptr = startOfFreeMemory->nextBase;
    } else {
      free_block* node = pageHeader->ptr;
      while (node->nextBase != startOfFreeMemory) {
        node = node->nextBase;
      }
      node->nextBase = startOfFreeMemory->nextBase;
    }
    free_page((kma_page_t*) BASEADDR(startOfFreeMemory));
  }
}

#endif // KMA_RM
