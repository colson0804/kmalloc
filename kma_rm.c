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

static free_block* firstFree = NULL;

/************Function Prototypes******************************************/

/************External Declaration*****************************************/

/**************Implementation***********************************************/

void* findNextFreeBlock(size){
  if (firstFree->nextBase == NULL){
    /* If firstFree is undefined, point it to the top of the page */
     kma_page_t* page = get_page();

    *((kma_page_t**)page->ptr) = page;
    
    if ((size + sizeof(kma_page_t*)) > page->size) {
    // requested size larger than page
      free_page(page);
      return NULL;
    }
    // Set aside free block that we need to allocate
    free_block* newBlock;
    newBlock = (free_block*) page;
    newBlock->size = page->size;  // Should be page size
    newBlock->nextBase = NULL;
    //firstFree = (free_block*) {.size = -1, .nextBase = newBlock};
    firstFree->nextBase = newBlock;
    // Return pointer to top of page, w/ all of relevant info
    return firstFree;
  }

  free_block* node = firstFree;

  // if (node->nextBase == NULL) {
  //   return firstFree;
  // } 

  while(node->nextBase != NULL){
    if (node->nextBase->size < size) {
      return node;
    }
    else
      node = node->nextBase;
  }

  node->nextBase = (free_block*) get_page();

  *((kma_page_t**)((kma_page_t*)node)->ptr) = (kma_page_t*)node;
  
  if ((size + sizeof(kma_page_t*)) > node->size) { // requested size too large
    free_page((kma_page_t*)node);
    return NULL;
  }
  free_block* newBlock = NULL;
  newBlock->nextBase = NULL;
  newBlock->size = size-8;
  node->nextBase = newBlock;

  return node;
}

void*
kma_malloc(kma_size_t size)
{ 
  free_block* nextIsFree;

  printf("Malloc for size: %d", size);

  nextIsFree = findNextFreeBlock(size);
  
  if (nextIsFree == NULL)
    return NULL;

  // if(nextIsFree->nextBase == NULL) {
  //   // Not technically a 'next' free page, but we need 
  //   // to adjust firstFree
  //   firstFree->nextBase += size/4;
  //   firstFree->size = firstFree->size - size;
  //   return firstFree;
  // }
  if(nextIsFree->nextBase->size == size) {
    nextIsFree->nextBase = nextIsFree->nextBase->nextBase;
  } else {
    nextIsFree->nextBase->size = nextIsFree->nextBase->size - size;
    nextIsFree->nextBase = nextIsFree->nextBase + (size/4);
  }

  // if (nextIsFree == firstFree) {
  //   // If the block we are allocating is the first free block,
  //     // we should change the pointer
  //   firstFree = 
  // }

  return nextIsFree->nextBase;
}

free_block* findFreeBlockInsertionPoint(void* ptr){
  free_block* node = firstFree;
  free_block* ptrToFree = ptr;

  if (firstFree ==  NULL){
    return firstFree;
  }

  while ((node !=NULL) && (node->nextBase < ptrToFree)){
    node = node->nextBase;
  }

  return node;
}

void
kma_free(void* ptr, kma_size_t size)
{
  free_block* block = NULL;
  free_block* blockBefore;

  blockBefore = findFreeBlockInsertionPoint(ptr);

  /* add to list here*/
  block->nextBase = blockBefore->nextBase;
  blockBefore->nextBase = block;
  block->size = size - 8;

  if(blockBefore+8+size == ptr){
    /*theres a free block right before it*/
    blockBefore->size += size;
    blockBefore->nextBase = block->nextBase;
  } 
  else if(ptr+8+size == blockBefore->nextBase){
    /*theres a free block right after it*/
    block->size += block->nextBase->size + 8;
    block->nextBase = block->nextBase->nextBase;
  }
  else if((ptr+8+size == blockBefore->nextBase) && (blockBefore+8+size == ptr)){
    /* its between two free blocks*/
    blockBefore->nextBase = block->nextBase->nextBase;
    blockBefore->size += size + 8 + block->nextBase->size + 8; 
  }

  if(block->size + 8 == sizeof(kma_page_t*)){
    /*Its the size of one page*/
    blockBefore->nextBase = block->nextBase;
    free_page(*((kma_page_t**)(block - sizeof(kma_page_t*))));
  }

}

#endif // KMA_RM
