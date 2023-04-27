/**
 * @file dyn_mem.h
 *
 */

#ifndef DYN_MEM_H
#define DYN_MEM_H

#ifdef __cplusplus
extern "C" {
#endif


/*********************
 *      INCLUDES
 *********************/

/*----------------
 * Dynamic memory
 *----------------*/
#define USE_DYN_MEM     1
#if USE_DYN_MEM != 0
#define DM_AUTO_ZERO   0     /*Automatically fill-zero the allocated memory*/
#define DM_CUSTOM      0     /*1: use custom malloc/free, 0: use malloc/free provided by dyn_mem*/
#if DM_CUSTOM == 0
  #define DM_MEM_SIZE    (4*1024) /*Size memory used by mem_alloc (in bytes)*/
	//#define DM_MEM_SIZE    (64) /*Size memory used by mem_alloc (in bytes)*/
  #define DM_MEM_ATTR                  /*Complier prefix for big array declaration*/
#else /*DM_CUSTOM != 0: Provide custom malloc/free functions*/
  #define DM_CUST_INCLUDE <stdlib.h>   /*Header for the dynamic memory function*/
  #define DM_CUST_ALLOC   malloc       /*Wrapper to malloc*/
  #define DM_CUST_FREE    free         /*Wrapper to free*/
#endif  /*DM_CUSTOM*/
#endif  /*USE_DYN_MEM*/


#if USE_DYN_MEM != 0

#include <stdint.h>
#include <stddef.h>


/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef struct
{
    uint32_t cnt_free;
    uint32_t cnt_used;
    uint32_t size_free;
    uint32_t size_total;
    uint32_t size_free_big;
    uint8_t pct_frag;
    uint8_t pct_used;
}dm_mon_t;


/**********************
 * GLOBAL PROTOTYPES
 **********************/


/**
 * Initiaize the dyn_mem module (work memory and other variables)
 */
void dm_init(void);

/**
 * Allocate a memory dynamically
 * @param size size of the memory to allocate in bytes
 * @return pointer to the allocated memory
 */
void * dm_alloc(uint32_t size);

/**
 * Free an allocated data
 * @param data pointer to an allocated memory
 */
void dm_free(const void * data);

/**
 * Reallocate a memory with a new size. The old content will be kept.
 * @param data pointer to an allocated memory.
 * Its content will be copied to the new memory block and freed
 * @param new_size the desired new size in byte
 * @return pointer to the new memory
 */
void * dm_realloc(void * data_p, uint32_t new_size);

/**
 * Join the adjacent free memory blocks
 */
void dm_defrag(void);

/**
 * Give information about the work memory of dynamic allocation
 * @param mon_p pointer to a dm_mon_p variable,
 *              the result of the analysis will be stored here
 */
void dm_monitor(dm_mon_t * mon_p);

/**
 * Give the size of an allocated memory
 * @param data pointer to an allocated memory
 * @return the size of data memory in bytes
 */
uint32_t dm_get_size(void * data);

/**********************
 *      MACROS
 **********************/

#define dm_assert(p) {if(p == NULL) {while(1);}}

#endif /*USE_DYN_MEM*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*DYN_MEM_H*/

