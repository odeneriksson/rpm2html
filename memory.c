/*
 * memory.c: a memory allocator wrapper.
 *
 * daniel@veillard.com
 */

#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include <libxml/xmlmemory.h>
#include "memory.h"

#ifndef NO_DEBUG_MEMORY

#ifdef debugMalloc
#undef debugMalloc
#endif
#ifdef debugRealloc
#undef debugRealloc
#endif
#ifdef debugStrdup
#undef debugStrdup
#endif
extern void debugMemoryDump(void);
extern int debugInitMemory(void);

/*
 * Each of the blocks allocated begin with a header containing information
 */

#define MEMTAG 0x5aa5

#define MALLOC_TYPE 1
#define REALLOC_TYPE 2
#define STRDUP_TYPE 3

typedef struct memnod {
    unsigned int   mh_tag;
    unsigned int   mh_type;
    unsigned long  mh_number;
    size_t         mh_size;
#ifdef MEM_LIST
   struct memnod *mh_next;
   struct memnod *mh_prev;
#endif
   const char    *mh_file;
   unsigned int   mh_line;
}  MEMHDR;


#ifdef SUN4
#define ALIGN_SIZE  16
#else
#define ALIGN_SIZE  sizeof(double)
#endif
#define HDR_SIZE    sizeof(MEMHDR)
#define RESERVE_SIZE (((HDR_SIZE + (ALIGN_SIZE-1)) \
		      / ALIGN_SIZE ) * ALIGN_SIZE)


#define CLIENT_2_HDR(a) ((MEMHDR *) (((char *) (a)) - RESERVE_SIZE))
#define HDR_2_CLIENT(a)    ((void *) (((char *) (a)) + RESERVE_SIZE))


static int initialized = 0;
static unsigned long  debugMemSize = 0;
static unsigned long  debugMemSizeMeg = 0;
static int block=0;
#ifdef MEM_LIST
static MEMHDR *memlist = NULL;
#endif

void mem_tag_error(void *addr);
#ifdef MEM_LIST
void  debugmem_list_add(MEMHDR *);
void debugmem_list_delete(MEMHDR *);
#endif
#define Mem_Tag_Err(a) mem_tag_error(a);

/******************************
#define TEST_POINT {						\
    unsigned long  meg = debugMemSize >> 20;			\
    if (meg != debugMemSizeMeg) debugMemoryDump();		\
    debugMemSizeMeg = meg;					\
}
 ******************************/

#ifndef TEST_POINT
#define TEST_POINT
#endif

/*
 * FUNCTION	: debugMalloc
 * PURPOSE	: a malloc() equivalent supporting multi-thread access.
 * INPUT	: an int specifying the size in byte to allocate.
 * OUTPUT	: none
 * RESULT	: a pointer to the allocated area or NULL in case of
 *		lack of memory space.
 * HISTORY	: 
 */

void *
debugMallocLoc(int size, const char * file, int line)
{
    MEMHDR *p;
    
    if (!initialized)
	debugInitMemory();
#ifdef DEBUG_MEMORY
    fprintf(stderr, "Malloc(%d)\n",size);
#endif

    TEST_POINT
    
    p = (MEMHDR *) malloc(RESERVE_SIZE+size);

    if (!p) {
       fprintf(stderr, "debugMalloc : Out of free space\n");
       debugMemoryDump();
    }   
    p->mh_tag = MEMTAG;
    p->mh_number = ++block;
    p->mh_size = size;
    p->mh_type = MALLOC_TYPE;
    p->mh_file = file;
    p->mh_line = line;
    debugMemSize += size;
#ifdef MEM_LIST
    debugmem_list_add(p);
#endif

#ifdef DEBUG_MEMORY
    fprintf(stderr, "Malloc(%d) Ok\n",size);
#endif
    

    TEST_POINT

    return(HDR_2_CLIENT(p));
}

void *
debugMalloc(int size)
{
    return(debugMallocLoc(size, "none", 0));
}

/*
 * debugRealloc : realloc() equivalent.
 */
 

/*
 * FUNCTION	: debugRealloc
 * PURPOSE	: a realloc() equivalent supporting multi-thread access.
 * INPUT	: the initial memory block pointer, and the new size.
 * OUTPUT	: none
 * RESULT	: a pointer to the new area or NULL in case of allocation
 *		 error.
 * HISTORY	: 
 */

void *
debugReallocLoc(void *ptr,int size, const char * file, int line)
{
    MEMHDR *p;
    unsigned long number;

    if (!initialized)
	debugInitMemory();
    TEST_POINT

    p = CLIENT_2_HDR(ptr);
    number = p->mh_number;
    if (p->mh_tag != MEMTAG) {
         Mem_Tag_Err(p);
	 goto error;
    }
    p->mh_tag = ~MEMTAG;
    debugMemSize -= p->mh_size;
#ifdef MEM_LIST
    debugmem_list_delete(p);
#endif

    p = (MEMHDR *) realloc(p,RESERVE_SIZE+size);
    if (!p) {
	 goto error;
    }
    p->mh_tag = MEMTAG;
    p->mh_number = number;
    p->mh_type = REALLOC_TYPE;
    p->mh_size = size;
    p->mh_file = file;
    p->mh_line = line;
    debugMemSize += size;
#ifdef MEM_LIST
    debugmem_list_add(p);
#endif

    TEST_POINT

    return(HDR_2_CLIENT(p));
    
error:    
    return(NULL);
}

void *
debugRealloc(void *ptr,int size) {
    return(debugReallocLoc(ptr, size, "none", 0));
}

/*
 * debugFree : free() equivalent with error code !
 */


/*
 * FUNCTION	: debugFree
 * PURPOSE	: an equivalent of free() allowing multi-thread access.
 * INPUT	: the pointer to the memory block allocated using the
 *	 debugMalloc or debugRealloc functions.
 * OUTPUT	: none
 * RESULT	:
 *		  0 : normal result
 *		  EMACHINTER_INVALID_POINTER : the pointer wasn't previously
 *			allocated by debugMalloc or debugRealloc.
 *		  or initialization or internal error.
 * HISTORY	: 
 */

int
debugFree(void *ptr)
{
    MEMHDR *p;
    int ret;

    if (!initialized)
	debugInitMemory();
    TEST_POINT

    p = CLIENT_2_HDR(ptr);
    if (p->mh_tag != MEMTAG) {
         Mem_Tag_Err(p);
         ret = -1;
         goto error;
    }
    p->mh_tag = ~MEMTAG;
    debugMemSize -= p->mh_size;

#ifdef MEM_LIST
    debugmem_list_delete(p);
#endif
    free(p);

    TEST_POINT

    ret = 0;
    return(ret);
    
error:    
    return(ret);
}

/*
 * debugStrdup : strdup() equivalent.
 */


/*
 * FUNCTION	: debugStrdup
 * PURPOSE	: a strdup() equivalent supporting multi-thread access.
 * INPUT	: the null terminated string to duplicate.
 * OUTPUT	: none
 * RESULT	: a T_Char pointer referencing the new string or NULL
 *  if allocation error occurs.
 * HISTORY	: 
 */

char *
debugStrdupLoc(const char *str, const char *file, int line)
{
    char *s;
    size_t size = strlen(str) + 1;
    MEMHDR *p;

    if (!initialized)
	debugInitMemory();
    TEST_POINT

    p = (MEMHDR *) malloc(RESERVE_SIZE+size);
    if (!p) {
        goto error;
    }
    p->mh_tag = MEMTAG;
    p->mh_number = ++block;
    p->mh_size = size;
    p->mh_type = STRDUP_TYPE;
    p->mh_file = file;
    p->mh_line = line;
    debugMemSize += size;
#ifdef MEM_LIST
    debugmem_list_add(p);
#endif
    s = HDR_2_CLIENT(p);
    
    if (s != NULL)
        strcpy(s,str);
    else
        goto error;
    
    TEST_POINT

    return(s);

error:
    return(NULL);
}

char *
debugStrdup(const char *str) {
    return(debugStrdupLoc(str, "none", 0));
}

/*
 * FUNCTION	: debugMemUsed
 * PURPOSE	: returns the amount of memory currenly allocated using
 *  debugMalloc, debugRealloc and debugStrdup.
 * INPUT	: none
 * OUTPUT	: none
 * RESULT	: a 32 bits integer representing the amount of memory allocated.
 * HISTORY	: 
 */

int
debugMemUsed(void) {
    if (!initialized)
	debugInitMemory();
     return(debugMemSize);
}


/*
 * FUNCTION	: debugMemDisplay
 * PURPOSE	: show in-extenso the memory allocated using
 *  debugMalloc, debugRealloc and debugStrdup.
 * INPUT	: a FILE descriptor used as the output file, if NULL
 *  the result is written to the file .memorylist ...
 * OUTPUT	: 
 * RESULT	:
 *		  0 : normal result
 * HISTORY	: 
 */

void
debugMemDisplay(FILE *fp)
{
    if (!initialized)
	debugInitMemory();
#ifdef MEM_LIST
      MEMHDR *p;
      int     idx;
    
      fprintf(fp,"      MEMORY ALLOCATED : %lu\n",debugMemSize);
      fprintf(fp,"BLOCK  NUMBER   SIZE  TYPE\n");
      idx = 0;
      p = memlist;
      while (p) {
	  fprintf(fp,"%-5u  %6lu %6u ",idx++,p->mh_number,p->mh_size);
          switch (p->mh_type) {
             case STRDUP_TYPE:fprintf(fp,"strdup()  in ");break;
             case MALLOC_TYPE:fprintf(fp,"malloc()  in ");break;
            case REALLOC_TYPE:fprintf(fp,"realloc() in ");break;
                      default:fprintf(fp,"   ???    in ");break;
          }
	  if (p->mh_file != NULL) fprintf(fp,"%s(%d)", p->mh_file, p->mh_line);
          if (p->mh_tag != MEMTAG)
	      fprintf(fp,"  INVALID");
          fprintf(fp,"\n");
          p = p->mh_next;
      }
#else
      fprintf(fp,"Memory list not compiled (MEM_LIST not defined !)\n");
#endif
}

#ifdef MEM_LIST

void debugmem_list_add(MEMHDR *p)
{
       p->mh_next = memlist;
       p->mh_prev = NULL;
       if (memlist) memlist->mh_prev = p;
       memlist = p;
#ifdef MEM_LIST_DEBUG
       if (stderr)
       Mem_Display(stderr);
#endif
}

void debugmem_list_delete(MEMHDR *p)
{
       if (p->mh_next)
       p->mh_next->mh_prev = p->mh_prev;
       if (p->mh_prev)
       p->mh_prev->mh_next = p->mh_next;
       else memlist = p->mh_next;
#ifdef MEM_LIST_DEBUG
       if (stderr)
       Mem_Display(stderr);
#endif
}

#endif

/*
 * mem_tag_error : internal error function.
 */
 
void mem_tag_error(void *p)
{
    if (!initialized)
	debugInitMemory();
    fprintf(stderr, "Memory tag error occurs :%p \n\t bye\n", p);
#ifdef MEM_LIST
    if (stderr)
        debugMemDisplay(stderr);
#endif
}

FILE *debugMemoryDumpFile = NULL;

/*
 * FUNCTION	: debugMemoryDump
 * PURPOSE	: saves information concerning memory allocation
 *		  in a ".memdump" file.
 * INPUT	: none
 * OUTPUT	: none
 * RESULT	: none
 * HISTORY	: 
 */

void
debugMemoryDump(void)
{
    FILE *dump;

    if (!initialized)
	debugInitMemory();
    dump = fopen(".memdump", "w");
    if (dump == NULL) debugMemoryDumpFile = stdout;
    else debugMemoryDumpFile = dump;

    debugMemDisplay(debugMemoryDumpFile);

    if (dump != NULL) fclose(dump);
}


/****************************************************************
 *								*
 *		Initialization Routines				*
 *								*
 ****************************************************************/


/*
 * debugInitMemory : initialize the memory allocator.
 */


/*
 * FUNCTION	: debugInitMemory
 * PURPOSE	: initialize the memory allocator.
 * INPUT	: none
 * OUTPUT	: none
 * RESULT	:
 *		  0 : normal result
 * HISTORY	: 
 */

int
debugInitMemory(void)
{
     int ret;
     xmlFreeFunc freeFunc;
     xmlMallocFunc mallocFunc;
     xmlReallocFunc reallocFunc;
     xmlStrdupFunc strdupFunc;
    
#ifdef DEBUG_MEMORY
     fprintf(stderr, "debugInitMemory() Ok\n");
#endif     
     xmlMemSetup(debugFree, debugMalloc, debugRealloc, debugStrdup);
     xmlMemGet(&freeFunc, &mallocFunc, &reallocFunc, &strdupFunc);
     if ((freeFunc != debugFree) ||
	 (mallocFunc != debugMalloc) ||
	 (reallocFunc != debugRealloc) ||
	 (strdupFunc != debugStrdup)) {
	 fprintf(stderr, "debugInitMemory: xml memory allocator problem\n");
	 exit(1);
     }
     initialized = 1;
     ret = 0;
     return(ret);
}

#endif /* ! NO_DEBUG_MEMORY */
