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

void* findNextFreeBlock(size){
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
    pageHeader->ptr = page + sizeof(kma_page_t*) + size;

    // Set new freeblock to end of block we're allocating
    free_block* freeBlock = (free_block*) pageHeader->ptr;
    freeBlock->size = page->size - sizeof(kma_page_t*) - sizeof(free_block*) - size;
    freeBlock->nextBase = NULL;
    return page + sizeof(kma_page_t*);
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

void*
kma_malloc(kma_size_t size)
{ 
  free_block* BlockToAllocate;

  BlockToAllocate = findNextFreeBlock(size);
  
  // if (BlockToAllocate == NULL)
  //   return NULL;
  printf("Got to here");
  // if (BlockToAllocate->nextBase == NULL) {
  //   // RIGHT???
  //   return pageHeader + sizeof(kma_page_t*);
  // }


  return BlockToAllocate;
}

free_block* findFreeBlockInsertionPoint(void* ptr){
  // kma_page* node = pageHeader;
  // free_block* ptrToFree = ptr;

  // if (pageHeader ==  NULL) {
  //   return pageHeader;
  // }

  // while ((node !=NULL) && (node->nextBase < ptrToFree)) {
  //   node = node->nextBase;
  // }
  free_block* node = NULL;
  return node;
}

void
kma_free(void* ptr, kma_size_t size)
{
  // free_block* block = NULL;
  // free_block* blockBefore;

  // blockBefore = findFreeBlockInsertionPoint(ptr);

  // /* add to list here*/
  // block->nextBase = blockBefore->nextBase;
  // blockBefore->nextBase = block;
  // block->size = size - 8;

  // if(blockBefore+8+size == ptr){
  //   /*theres a free block right before it*/
  //   blockBefore->size += size;
  //   blockBefore->nextBase = block->nextBase;
  // } 
  // else if(ptr+8+size == blockBefore->nextBase){
  //   /*theres a free block right after it*/
  //   block->size += block->nextBase->size + 8;
  //   block->nextBase = block->nextBase->nextBase;
  // }
  // else if((ptr+8+size == blockBefore->nextBase) && (blockBefore+8+size == ptr)){
  //   /* its between two free blocks*/
  //   blockBefore->nextBase = block->nextBase->nextBase;
  //   blockBefore->size += size + 8 + block->nextBase->size + 8; 
  // }

  // if(block->size + 8 == sizeof(kma_page_t*)){
  //   /*Its the size of one page*/
  //   blockBefore->nextBase = block->nextBase;
  //   free_page(*((kma_page_t**)(block - sizeof(kma_page_t*))));
  // }

}

#endif // KMA_RM
