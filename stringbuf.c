/*
 * stringbuf.h: interface for the String buffering module.
 *
 * daniel@veillard.com
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h> 
#endif
#include "stringbuf.h"

#include "memory.h"

/*
 * Not the default, too heavy on CPU ...
 */

#ifdef WITH_STRING_BUF

#define CLEANUP_FREQUENCY 2000

/*
 * An hash table for the Strings
 */

#define HASH_SIZE 65536

typedef struct stringHash {
    struct stringHash *next;	/* next clash in the hash table */
    int use;			/* number of references */
    const char *val;		/* the string value */
} stringHash, *stringHashPtr;

stringHashPtr stringHashTable[HASH_SIZE];

/*
 * Hash table initialization.
 */

static int stringHashInitialized = 0;

static void stringHashInitialize(void) {
    int i;

    for (i = 0; i < HASH_SIZE; i++) {
        stringHashTable[i] = NULL;
    }
    stringHashInitialized = 1;
}

/*
 * Compute an hash key
 */

static int stringGetHash(const char *str) {
    unsigned short res = 0;

    while (*str != 0) res += *(str++);

    return((int) (res % HASH_SIZE));
}

/*
 * Cleanup the hash table for unused entries
 */

void stringTableCleanup(void) {
    int key;
    int used = 0, entries = 0;
    stringHashPtr cur;
    stringHashPtr prev;

    for (key =0;key < HASH_SIZE;key++) {
        if (stringHashTable[key] != NULL) used++;
        while (stringHashTable[key] != NULL) {
	    if (stringHashTable[key]->use == 0) {
	        cur = stringHashTable[key]->next;
		stringHashTable[key]->val = NULL;
		stringHashTable[key]->use = 0;
		stringHashTable[key]->next = NULL;
		xmlFree(stringHashTable[key]);
		stringHashTable[key] = cur;
	    } else {
	        entries++;
	        prev = stringHashTable[key];
		cur = stringHashTable[key]->next;
		while (cur != NULL) {
		    if (cur->use == 0) {
		        xmlFree(cur->val);
			prev->next = cur->next;
			cur->next = NULL;
			xmlFree(cur);
			cur = prev->next;
		    } else {
			entries++;
		        prev = cur;
			cur = cur->next;
		    }
		}
		goto next_key;
	    }
	}
next_key:
    }
    printf("String Buffer: %d entries, %d hash\n", entries, used);
}

/*
 * Duplicate an "external" string. Search the string in the
 * list if not found, add it, otherwise increment the counter.
 */
const char *stringAdd(const char *str) {
    int key;
    stringHashPtr cur;
    static int toCleanup = CLEANUP_FREQUENCY;

    if (stringHashInitialized == 0) stringHashInitialize();
    toCleanup--;
    if (toCleanup <= 0) {
        toCleanup = CLEANUP_FREQUENCY;
        stringTableCleanup();
    }

    key = stringGetHash(str);
    cur = stringHashTable[key];
    while (cur != NULL) {
        if (!strcmp(str, cur->val)) {
	    cur->use++;
	    return(cur->val);
	}
	cur = cur->next;
    }
    cur = xmlMalloc(sizeof(stringHash));
    cur->val = xmlStrdup(str);
    cur->use = 1;
    cur->next = stringHashTable[key];
    stringHashTable[key] = cur;

    return(cur->val);
}

/*
 * Free a duplicate of the string. Decrement the counter, if zero,
 * free the string and remove it from the list.
 */
void  stringFree(const char *str) {
    int key;
    stringHashPtr cur;

    key = stringGetHash(str);
    cur = stringHashTable[key];
    while (cur != NULL) {
        if (!strcmp(str, cur->val)) {
	    cur->use--;
	    /* TODO : garbage collect ... */
	    return;
	}
	cur = cur->next;
    }
    printf("stringFree : %s not found !\n", str);
}

#else /* ! WITH_STRING_BUF */
const char *stringAdd(const char *str) {
    return(xmlStrdup(str));
}
void  stringFree(const char *str) {
    xmlFree((char *) str);
}
#endif /* !WITH_STRING_BUF */

