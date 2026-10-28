/*
 * rpmdata.c : handle the data in the RPM database.
 *
 * See Copyright for the status of this software.
 *
 * $Id: rpmdata.c,v 1.54 2008/11/13 23:56:21 hany Exp $
 */

#include <config.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <ctype.h>
#include <errno.h>

#include <rpm/rpmlib.h>

#include "rpm2html.h"
#include "rpmdata.h"

int rpmNameCmp(const rpmData *a, const rpmData *b);

/* Duplicate a C string using libxml2's allocator.  The returned
 * pointer remains owned by its caller and is released with xmlFree(). */
static char *rpmdataXmlStrdup(const char *value) {
    return (char *) xmlStrdup(BAD_CAST value);
}

/*
 * An hash table for the RPM list
 */

#define HASH_SIZE 4096

rpmDataPtr rpmHashTable[HASH_SIZE];

/*
 * An hash table for the Resources list
 */

rpmRessPtr rpmRessHashTable[HASH_SIZE];

/*
 * global variables shared by rpm2htm code
 */
int rpm2htmlVerbose = 1;
int force = 0;
time_t currentTime;

/*
 * the lists of RPM and resources collected so far.
 */
rpmDataPtr rpmSoftwareList = NULL;
rpmRessPtr ressList = NULL;
rpmRessPtr ressInstalledList = NULL;
rpmArchPtr archList = NULL;
rpmDirPtr dirList = NULL;
rpmSubdirPtr dirTree = NULL;
rpmRealDirPtr treeRoot = NULL;

/*
 * Hash table initialization.
 */

static int rpmHashInitialized = 0;

void rpmHashInitialize(void) {
    int i;

    for (i = 0; i < HASH_SIZE; i++) {
        rpmHashTable[i] = NULL;
        rpmRessHashTable[i] = NULL;
    }
    rpmHashInitialized = 1;
}

/*
 * Compute an RPM hash key
 */

int rpmGetHash(const char *name, const char *version,
	       const char *release) {
    unsigned short res = 0;

    while (*name != 0) res += *(name++);
    while (*version != 0) res += *(version++);
    while (*release != 0) res += *(release++);

    return((int) (res % HASH_SIZE));
}

/*
 * Compute a Resource hash key
 */

int rpmGetRessHash(const char *name) {
    unsigned short res = 0;

    while (*name != 0) res += *(name++);

    return((int) (res % HASH_SIZE));
}

/*
 * Search wether an RPM is available.
 */

rpmDataPtr rpmSearchSoftware(const char *name, const char *version,
           const char *release, const char *arch) {
    rpmDataPtr cur;
    int hash;

    if (!rpmHashInitialized) rpmHashInitialize();

    hash = rpmGetHash(name, version, release);
    cur = rpmHashTable[hash];

    while (cur != NULL) {
        if (!strcmp(name, cur->name) &&
	    !strcmp(version, cur->version) &&
	    !strcmp(release, cur->release)) {
	    /*
	     * this is already in the sofware list, link in as
	     * a new arch support.
	     */
	    while (cur != NULL) {
	        if (!strcmp(arch, cur->arch))
		    return(cur);
		cur = cur->nextArch;
	    }
	    return(NULL);
	}    
        cur = cur->nextHash;
    }
    return(NULL);
}

/*
 * Insert a new RPM, managing the list of various software.
 */

void rpmAddSoftware(rpmDataPtr rpm) {
    rpmDataPtr cur;
    int hash;

    if (!rpmHashInitialized) rpmHashInitialize();

    hash = rpmGetHash(rpm->name, rpm->version, rpm->release);
    cur = rpmHashTable[hash];

    while (cur != NULL) {
        if (!strcmp(rpm->name, cur->name) &&
	    !strcmp(rpm->version, cur->version) &&
	    !strcmp(rpm->release, cur->release)) {
	    /*
	     * this is already in the sofware list, link in as
	     * a new arch support.
	     */
#ifdef DEBUG_HASH
fprintf(stderr, "clash : (%d) %s-%s-%s.%s and %s-%s-%s.%s\n", hash,
        rpm->name, rpm->version, rpm->release, rpm->arch,
        cur->name, cur->version, cur->release, cur->arch);
#endif

	    rpm->nextArch = cur->nextArch;
	    cur->nextArch = rpm;
	    rpm->nextSoft = NULL; /* this one is not in the software list */
	    return;
	}    
        cur = cur->nextHash;
    }

    /*
     * this wasn't found in the software list !
     */
#ifdef DEBUG_HASH
fprintf(stderr, "new : (%d) %s-%s-%s.%s\n", hash,
        rpm->name, rpm->version, rpm->release, rpm->arch);
#endif

    rpm->nextHash = rpmHashTable[hash];
    rpmHashTable[hash] = rpm;
    rpm->nextArch = NULL;
    rpm->nextSoft = rpmSoftwareList;
    rpmSoftwareList = rpm;
}

/*
 * Add a resource to the list if it doesn't exists and add the
 * corresponding RPM as a provider.
 */

rpmRessPtr rpmRessAdd(char *ress, rpmDataPtr rpm, int installed) {
    rpmRessPtr cur;
    int hash;
    
    if (!rpmHashInitialized) rpmHashInitialize();

    hash = rpmGetRessHash(ress);
    cur = rpmRessHashTable[hash];

    /* search for the resource */
    while (cur != NULL) {
        if (!strcmp(ress, cur->name)) goto found;
	cur = cur->nextHash;
    }

    /* not found allocate a new resource block and fill it */
    cur = (rpmRessPtr) xmlMalloc(sizeof(rpmRess));
    if (cur == NULL) {
         fprintf(stderr, "cannot allocate %zu bytes: %s\n", sizeof(rpmRess),
	         strerror(errno));
         return(NULL);
    }
    memset(cur, 0, sizeof(rpmRess));
    cur->max_provider = 8;
    cur->provider = (rpmDataPtr *) xmlMalloc(cur->max_provider *
                                          sizeof(rpmDataPtr));
    if (cur->provider == NULL) {
         fprintf(stderr, "cannot allocate %zu bytes: %s\n", 
	         cur->max_provider * sizeof(rpmDataPtr),
	         strerror(errno));
         return(NULL);
    }
    cur->name = rpmdataXmlStrdup(ress);
    cur->nb_provider = 0;
    cur->stamp = rpm->extra->stamp;
    if (installed) {
	cur->next = ressInstalledList;
	ressInstalledList = cur;
    } else {
	cur->next = ressList;
	ressList = cur;
    }
    cur->nextHash = rpmRessHashTable[hash];
    rpmRessHashTable[hash] = cur;

found:
    /* add the provider to the array */
    if (cur->nb_provider >= cur->max_provider) {
        cur->max_provider *= 2;
	cur->provider = (rpmDataPtr *) xmlRealloc(cur->provider,
				 cur->max_provider * sizeof(rpmDataPtr));
        if (cur->provider == NULL) {
            fprintf(stderr, "rpmRessAdd : ran out of memory !\n");
	    exit(1);
	}
    }
    cur->provider[cur->nb_provider++] = rpm;
    if (rpm->extra->stamp > cur->stamp) 
        cur->stamp = rpm->extra->stamp;
    return(cur);
}

/*
 * Add a resource to the list if it doesn't exists and add the
 * corresponding RPM as a requester.
 */

rpmRessPtr rpmRequAdd(char *requ, char * requv, rpm_dep_flag requf, rpmDataPtr rpm, int installed) { 
    rpmRessPtr cur;
    int hash;
    
    if (!rpmHashInitialized) rpmHashInitialize();

    hash = rpmGetRessHash(requ);
    cur = rpmRessHashTable[hash];
    
    /* search for the resource */
    while (cur != NULL) {
        if (!strcmp(requ, cur->name)){
	    goto found;
	}
	cur = cur->nextHash;
    }

    /* not found allocate a new resource block and fill it */
    cur = (rpmRessPtr) xmlMalloc(sizeof(rpmRess));
    if (cur == NULL) {
         fprintf(stderr, "cannot allocate %zu bytes: %s\n", sizeof(rpmRess),
	         strerror(errno));
         return(NULL);
    }
    memset(cur, 0, sizeof(rpmRess));
    cur->max_provider = 8;
    cur->provider = (rpmDataPtr *) xmlMalloc(cur->max_provider *
                                          sizeof(rpmDataPtr));
    if (cur->provider == NULL) {
         fprintf(stderr, "cannot allocate %zu bytes: %s\n", 
	         cur->max_provider * sizeof(rpmDataPtr),
	         strerror(errno));
         return(NULL);
    }
    cur->name = rpmdataXmlStrdup(requ);
    if (requv != NULL) 
	cur->version = rpmdataXmlStrdup(requv);
    cur->flag = requf;
    cur->nb_provider = 0;
    cur->stamp = rpm->extra->stamp;
    if (installed) {
	cur->next = ressInstalledList;
	ressInstalledList = cur;
    } else {
	cur->next = ressList;
	ressList = cur;
    }
    cur->nextHash = rpmRessHashTable[hash];
    rpmRessHashTable[hash] = cur;

found:
    if (rpm->extra->stamp > cur->stamp)
        cur->stamp = rpm->extra->stamp;
    return(cur);
}

/*
 * print an RPM data block
 */
void rpmDataPrint(rpmDataPtr rpm) {
    int i;

    printf("%s\n", rpm->filename);
    printf("%s-%s-%s\n", rpm->name, rpm->version, rpm->release);
    if (rpm->summary)
	printf("   %s\n", rpm->summary);
    if (rpm->extra) {
	for (i = 0; i < rpm->extra->nb_resources ;i++)
	    printf("-> %s\n", rpm->extra->resources[i]->name);
    }
}

/*
 * Add two RPM lists
 */

rpmDataPtr rpmAddList(rpmDataPtr ret, rpmDataPtr cur) {
    rpmDataPtr tmp;

    if (ret == NULL) return (cur);
    if (cur == NULL) return (ret);
    if (cur->next == NULL) {
        cur->next = ret;
	return(cur);
    }
    if (ret->next == NULL) {
        ret->next = cur;
	return(ret);
    }
    tmp = ret;
    while (tmp->next != NULL) tmp = tmp->next;
    tmp->next = cur;
    return(ret);
}

/*
 * A generic sort function for an RPM list.
 * NOTE : since doing a fast sort on a linked list is a pain,
 *        we allocate an array, copy the rpmDataPtr in the array
 *        and use the standard qsort() for decent performances on
 *        large databases.
 */

typedef int (*rpmCmpFunc)(const rpmData *a, const rpmData *b);

typedef int (*qsortCmpFunc)(const void * a, const void * b);

static rpmCmpFunc currentCmpFunc = NULL;

int rpmPtrCmpFunc(const rpmData **a, const rpmData **b) {
    return(currentCmpFunc(*a, *b));
}

void rpmGenericSort(rpmCmpFunc rpmCmp, rpmDataPtr *base, int installed) {
    int i, nb_elems = 0;
    rpmDataPtr list;
    rpmDataPtr *array = NULL;
    int max_elems = 1024;

    if (base == NULL)
	return;
    if (*base == NULL)
	return;
    list = *base;

    array = (rpmDataPtr *) xmlMalloc(max_elems * sizeof(rpmDataPtr));
    if (array == NULL) {
	fprintf(stderr, "rpmGenericSort : array alloc error\n");
	exit(1);
    }

    /*
     * Step 1 : copy the list in the Array
     */
    while (list != NULL) {
        if (nb_elems == max_elems) {
	    max_elems *= 2;
	    array = (rpmDataPtr *)
	             xmlRealloc(array, max_elems * sizeof(rpmDataPtr));
            if (array == NULL) {
	        fprintf(stderr, "rpmGenericSort : array alloc error\n");
		exit(1);
	    }
	}
	array[nb_elems++] = list;
	list = list->next;
    }

    /*
     * Step 2 : qsort() the Array
     */
    currentCmpFunc = rpmCmp;
    qsort(array, nb_elems, sizeof(rpmDataPtr), (qsortCmpFunc) rpmPtrCmpFunc);

    /*
     * Step 3 : rebuild the list.
     */
    for (i = 0;i < nb_elems - 1; i++)
        array[i]->next = array[i + 1];
    array[nb_elems - 1]->next = NULL;

    list = array[0];
    *base = list;
    xmlFree(array);
}

/*
 * A generic sort function for the software list.
 */

void rpmGenericSortSoftware(rpmCmpFunc rpmCmp, int installed) {
    rpmDataPtr list;
    int i, nb_elems = 0;
    rpmDataPtr *array = NULL;
    int max_elems = 1024;

    list = rpmSoftwareList;

    if (list == NULL) return;

    array = (rpmDataPtr *) xmlMalloc(max_elems * sizeof(rpmDataPtr));
    if (array == NULL) {
	fprintf(stderr, "rpmGenericSort : array alloc error\n");
	exit(1);
    }

    /*
     * Step 1 : copy the list in the Array
     */
    while (list != NULL) {
        if (nb_elems == max_elems) {
	    max_elems *= 2;
	    array = (rpmDataPtr *)
	             xmlRealloc(array, max_elems * sizeof(rpmDataPtr));
            if (array == NULL) {
	        fprintf(stderr, "rpmGenericSort : array alloc error\n");
		exit(1);
	    }
	}
	array[nb_elems++] = list;
	list = list->nextSoft;
    }

    /*
     * Step 2 : qsort() the Array
     */
    currentCmpFunc = rpmCmp;
    qsort(array, nb_elems, sizeof(rpmDataPtr), (qsortCmpFunc) rpmPtrCmpFunc);

    /*
     * Step 3 : rebuild the list.
     */
    for (i = 0;i < nb_elems - 1; i++)
        array[i]->nextSoft = array[i + 1];
    array[nb_elems - 1]->nextSoft = NULL;

    rpmSoftwareList = array[0];
    xmlFree(array);
}

/*
 * rpmGroupCmp : comparison of groupe values, if equal, compare
 *               the names.
 */

int rpmGroupCmp(const rpmData *a, const rpmData *b) {
    int res = strcmp(a->group, b->group);

    if (res) return(res);
    res = rpmNameCmp(a, b);
    return(res);
}

/*
 * sort all the RPMs by Group.
 */
void rpmGroupSort(rpmDataPtr *list, int installed) {
    rpmGenericSort(rpmGroupCmp, list, installed);
    rpmGenericSortSoftware(rpmGroupCmp, installed);
}

/*
 * rpmDistribCmp : comparison of distributions values, if equal, compare
 *               the names.
 */

int rpmDistribCmp(const rpmData *a, const rpmData *b) {
    int res = strcmp(a->distribution, b->distribution);

    if (res) return(res);
    res = rpmNameCmp(a, b);
    return(res);
}

/*
 * sort all the RPMs by Distribution.
 */
void rpmDistribSort(rpmDataPtr *list, int installed) {
    rpmGenericSort(rpmDistribCmp, list, installed);
}

/*
 * rpmVendorCmp : comparison of vendor values, if equal, compare
 *               distributions, then compares the names.
 */

int rpmVendorCmp(const rpmData *a, const rpmData *b) {
    int res = strcmp(a->vendor, b->vendor);
    if (res) return(res);
    res = strcmp(a->distribution, b->distribution);
    if (res) return(res);
    res = rpmNameCmp(a, b);
    return(res);
}

/*
 * sort all the RPMs by Vendor.
 */
void rpmVendorSort(rpmDataPtr *list, int installed) {
    rpmGenericSort(rpmVendorCmp, list, installed);
}

/*
 * rpmDateCmp : comparison of date values, if equal, compare
 *              distributions, then compares the names.
 */

int rpmDateCmp(const rpmData *a, const rpmData *b) {
    int res = (b->date - a->date) / (60 * 60 * 12);

    if (res) return(res);
    res = rpmNameCmp(a, b);
    return(res);
}

/*
 * sort all the RPMs by Date.
 */
void rpmDateSort(rpmDataPtr *list, int installed) {
    rpmGenericSort(rpmDateCmp, list, installed);
}

/* Compare two version or release numbers.  If there are leading
   digits convert them to an integer and compare those.  If they
   are equal and a "." follows, skip the "." and repeat.  Compare
   any remaining non-digits using strcasecmp.   Note that the
   numerical sense of this comparison is reversed, so that higher
   version numbers (and thus more recent packages) appear first. */

static int vercmp(const char *a, const char *b)
{
  while (1) {
    /* If we're at the end of both they are equal */
    if (!*a && !*b) return 0;
    /* If we're at the end of a then b is more recent */
    else if (!*a) return -1;
    /* If we're at the end of b then a is more recent */
    else if (!*b) return 1;
    /* If we see non-digits compare lexically */
    else if (!isdigit(*a) || !isdigit(*b))
      return strcasecmp(a, b);
    else {
      /* compare the integers at the beginning.  The larger number is
         more recent. */
      int diff = atoi(a) - atoi(b);
      if (diff != 0) return diff;
      /* If they are equal, skip past them */
      else {
	while (isdigit(*a)) a++;
	while (isdigit(*b)) b++;
      }
    }
    /* If we don't see any version number separators compare lexically */
    if (*a != '.' && *b != '.') return strcasecmp(a, b);
    /* If a's version number ended and b continues, b is more recent. */
    else if (*a != '.') return -1;
    /* Conversely, here a is more recent */
    else if (*b != '.') return 1;
    /* Skip past the periods and comparison the next version element */
    else {a += 1; b += 1;}
  }
}

/*
 * rpmNameCmp : comparison of names.
 */

int rpmNameCmp(const rpmData *a, const rpmData *b) {
    int res;
    const char *ap = a->name;
    const char *bp = b->name;
    /* This algorithm for locating the subpackage name is flawed, as
       it is possible for either the main package name or the sub
       package name to contain a dash. */
#if 1
    /* Here we scan from the right, assuming that the sub-package name
       contains no dashes. */
    const char *asp = &ap[strlen(ap)-1];
    const char *bsp = &bp[strlen(bp)-1];
    while (asp > ap && *asp != '-') asp -= 1;	/* scan to subpackage name */
    while (bsp > bp && *bsp != '-') bsp -= 1;	/* (if any.) */
    if (asp == ap) asp = &ap[strlen(ap)];	/* No subpackage, go to nul */
    if (bsp == bp) bsp = &bp[strlen(bp)];	/*   at end of string */
#else    
    /* Here we scan from the left, assuming that the main package name
       contains no dashes. */
    const char *asp = ap;
    const char *bsp = bp;
    while (*asp && *asp != '-') asp += 1;	/* scan to subpackage name */
    while (*bsp && *bsp != '-') bsp += 1;	/* (if any.) */
#endif

    /* Primary sort key is the package name, then the version number,
       then the release number, and only then the subpackage name. */
#define min(a,b) (((a)<(b))?(a):(b))
    res = strncasecmp(ap, bp, min(asp - ap, bsp - bp));
    /* A shorter name is lexically less. */
    if (res == 0) res = (asp - ap) - (bsp - bp);
    /* Later version numbers should appear first. */
    if (res == 0) res = vercmp(b->version, a->version);
    /* Same for release numbers. */
    if (res == 0) res = vercmp(b->release, a->release);
    /* Sort on sub-package name */
    if (res == 0) res = strcasecmp(asp, bsp);
    return(res);
}

/*
 * sort all the RPMs by Name.
 */
void rpmNameSort(rpmDataPtr *list, int installed) {
    rpmGenericSort(rpmNameCmp, list, installed);
    rpmGenericSortSoftware(rpmNameCmp, installed);
}

/*
 * rpmVersionCmp : comparison of version
 */

int rpmVersionCmp(const rpmDataPtr *a, const rpmDataPtr *b) {
    int res = strcmp((*a)->name, (*b)->name);
    if (res) return(res);
    res = strcmp((*b)->version, (*a)->version);
    if (res) return(res);
    res = strcmp((*b)->release, (*a)->release);
    return(res);
}

/*
 * sort all RPM providing a resource by version number
 */
void rpmRessSort(rpmRessPtr ress) {
    if (ress == NULL) return;
    if (ress->provider == NULL) return;
    if (ress->nb_provider <= 1) return;
    qsort(ress->provider, ress->nb_provider, sizeof(rpmDataPtr),
          (qsortCmpFunc) rpmVersionCmp);
}

/*
 * Create a subdirectory of a given directory
 */

rpmSubdirPtr rpmNewSubdir(rpmSubdirPtr dir, const char *name,
	  const char *htmlpath, const char *rpmpath, const char *color,
	  int html) {
    rpmSubdirPtr res;

    res  = (rpmSubdirPtr) xmlMalloc(sizeof(rpmSubdir));
    if (res == NULL) {
        fprintf(stderr, "rpmNewSubdir : running out of memory !\n");
	exit(1);
    }
    res->max_subdirs = 8;
    res->subdirs = (rpmSubdirPtr *) 
		    xmlMalloc(res->max_subdirs * sizeof(rpmSubdirPtr));
    if (res->subdirs == NULL) {
        fprintf(stderr, "rpmNewSubdir : running out of memory !\n");
	exit(1);
    }
    res->name = rpmdataXmlStrdup(name);
    res->nb_subdirs = 0;
    if (color)
        res->color = rpmdataXmlStrdup(color);
    else
        res->color = NULL;
    res->html = html;

    memset(res->subdirs, 0, res->max_subdirs * sizeof(*res->subdirs));
    if (dir == NULL) {
        res->htmlpath = rpmdataXmlStrdup(name);
        res->rpmpath = rpmdataXmlStrdup(name);
	res->parent = NULL;
    } else {
        res->parent = dir;
	if (dir->parent != NULL) /* top level dir has alway html = 1 */
	    res->html = dir->html;
	if (htmlpath == NULL) {
	    if (dir->htmlpath[0] != '\0') {
		char buf[500];
		snprintf(buf, sizeof(buf), "%s/%s", dir->htmlpath, name);
		res->htmlpath = rpmdataXmlStrdup(buf);
	    } else
		res->htmlpath = rpmdataXmlStrdup(name);
	} else
	    res->htmlpath = rpmdataXmlStrdup(htmlpath);
	if (rpmpath == NULL) {
	    if (dir->rpmpath[0] != '\0') {
		char buf[500];
		snprintf(buf, sizeof(buf), "%s/%s", dir->rpmpath, name);
		res->rpmpath = rpmdataXmlStrdup(buf);
	    } else
		res->rpmpath = rpmdataXmlStrdup(name);
	} else
	    res->rpmpath = rpmdataXmlStrdup(rpmpath);
	if (dir->nb_subdirs >= dir->max_subdirs) {
	    dir->max_subdirs *= 2;
	    dir->subdirs = (rpmSubdirPtr *) xmlRealloc(dir->subdirs,
		             dir->max_subdirs * sizeof(rpmSubdirPtr));
	    if (dir->subdirs == NULL) {
		fprintf(stderr, "rpmNewSubdir : running out of memory !\n");
		exit(1);
	    }
	}
	dir->subdirs[dir->nb_subdirs++] = res;
    }
    return(res);
}

/*
 * Remove one subdirectory from it's parent.
 */

void rpmRemoveSubdir(rpmSubdirPtr tree) {
    int i;
    rpmSubdirPtr cur = tree->parent;

    if (cur == NULL) return;

    /*
     * search the child in the parent list.
     */
    for (i = 0;i < cur->nb_subdirs;i++)
        if (cur->subdirs[i] == tree) break;
    if (i >= cur->nb_subdirs) {
        fprintf(stderr, "rpmRemoveSubdir : child not referenced by parent\n");
	return;
    }

    /*
     * Decrement the parent count, and shift the end of the array.
     */
    cur->nb_subdirs--;
    for (;i < cur->nb_subdirs;i++)
        cur->subdirs[i] = cur->subdirs[i + 1];
    cur->subdirs[i] = NULL;
}

/*
 * Free a full directory tree.
 */

void rpmFreeSubdir(rpmSubdirPtr tree) {
    int i;

    for (i = 0;i < tree->nb_subdirs; i++)
         rpmFreeSubdir(tree->subdirs[i]);
    if (tree->name != NULL) xmlFree(tree->name);
    if (tree->htmlpath != NULL) xmlFree(tree->htmlpath);
    if (tree->rpmpath != NULL) xmlFree(tree->rpmpath);
    if (tree->color != NULL) xmlFree(tree->color);
    tree->parent = NULL;
    xmlFree(tree->subdirs);
    xmlFree(tree);
}

/*
 * print all RPM data blocks
 */
void rpmDataPrintAll(void) {
    rpmDataPtr cur = rpmSoftwareList;

    while (cur != NULL) {
        rpmDataPrint(cur);
	cur = cur->next;
    }
}

/*
 * Try to extract an E-mail address from a string.
 * The string is truncated to the beginning of the E-Mail.
 * e.g. "Joe Average <joe@average.org>" should be truncated
 *      to "Joe Average"
 */

char *extractEMail(char *string) {
    static char buf[200];
    char *start;
    char *end;

    start = strchr(string, '@');
    if (start == NULL) return(NULL);
    end = start;
    do {
	start--;
	if ((!isprint(*start)) ||
	    (*start == ' ') || (*start == '\t') ||
	    (*start == '\n') || (*start == '\r') ||
	    (*start == '@') || (*start == '<') ||
	    (*start == '>') || (*start == '(') || (*start == ')') ||
	    (*start == '[') || (*start == ']') ||
	    (*start == ';') || (*start == ',') || (*start == ',')) {
	    start++;
	    break;
	}
    } while (start != string);
    do {
	end++;
	if ((!isprint(*end)) ||
	    (*start == ' ') || (*start == '\t') ||
	    (*start == '\n') || (*start == '\r') ||
	    (*end == '@') || (*end == '<') ||
	    (*end == '>') || (*end == '(') || (*end == ')') ||
	    (*end == '[') || (*end == ']') ||
	    (*end == ';') || (*end == ',') || (*end == ',')) {
	    break;
	}
    } while (*end != '\0');
    strncpy(buf, start, end - start);
    buf[end - start] = '\0';
    return(buf);
}

/*
 * Try to extract an URL address from a string.
 * The string is truncated to the beginning of the URL
 * e.g. "Nifty Project http://www.nifty.org/project/"
 *      to "Nifty Project"
 */

char *extractURL(char *string) {
    static char buf[500];
    char *start;
    char *end;

    start = strstr(string, "http://");
    if (start == NULL) start = strstr(string, "ftp://");
    if (start == NULL) return(NULL);
    end = start;
    do {
	start--;
	if ((!isprint(*start)) ||
	    (*start == ' ') || (*start == '\t') ||
	    (*start == '\n') || (*start == '\r') ||
	    (*start == '@') || (*start == '<') ||
	    (*start == '>') || (*start == '(') || (*start == ')') ||
	    (*start == '[') || (*start == ']') ||
	    (*start == ';') || (*start == ',') || (*start == ',')) {
	    start++;
	    break;
	}
    } while (start != string);
    do {
	end++;
	if ((!isprint(*end)) ||
	    (*start == ' ') || (*start == '\t') ||
	    (*start == '\n') || (*start == '\r') ||
	    (*end == '@') || (*end == '<') ||
	    (*end == '>') || (*end == '(') || (*end == ')') ||
	    (*end == '[') || (*end == ']') ||
	    (*end == ';') || (*end == ',') || (*end == ',')) {
	    break;
	}
    } while (*end != '\0');
    strncpy(buf, start, end - start);
    buf[end - start] = '\0';
    return(buf);
}

/*
 * Try to protect an e-mail address from a spam bot.
 * The "<someone@domain.tld>" is replaced
 *      with "<someone(at)domain.tld>".
 */

static unsigned char *buffer = NULL;
static int buffer_size = 2000;
static char *at = "(at)";
char *protectEmail(char *string) {
    if (rpm2html_protect_emails == 0 || string == NULL)
        return string;

    unsigned char *cur, *end;
    unsigned char c;

    if (buffer == NULL) {
        buffer = (char *) xmlMalloc(buffer_size * sizeof(char));
	if (buffer == NULL) {
	    perror("xmlMalloc failed");
	    exit(1);
	}
    }
    cur = &buffer[0];
    end = &buffer[buffer_size - 20];

    while (*string != '\0') {
        if (cur >= end) {
	    int delta = cur - buffer;

	    buffer_size *= 2;
	    buffer = (char *) xmlRealloc(buffer, buffer_size * sizeof(char));
	    if (buffer == NULL) {
		perror("xmlMalloc failed");
		exit(1);
	    }
	    end = &buffer[buffer_size - 20];
	    cur = &buffer[delta];
	}
        c = (unsigned char) *(string++);
	if (c == '@') {
	    strcpy(cur, at);
	    cur += strlen(at);
	} else {
	    *(cur++) = c;
	}
    }
    *cur = '\0';
    return(buffer);
}

/*
 * Create a root directory for the installation space, i.e. the
 * filesystem that would be created if the packages were installed
 * on a real system
 * Basic array allocation is 16 items.
 */

rpmRealDirPtr rpmCreateRealRoot(void) {
    rpmRealDirPtr res;

    res = (rpmRealDirPtr) xmlMalloc(sizeof(rpmRealDir));
    if (res == NULL) {
        fprintf(stderr, "rpmCreateRealRoot : out of memory !\n");
	exit(1);
    }
    res->parent = NULL;
    res->name = rpmdataXmlStrdup("/");
    res->nb_elem = 0;
    res->elems = (void *) xmlMalloc(16 * sizeof(void *));
    res->names = (void *) xmlMalloc(16 * sizeof(char *));
    res->flags = (char *) xmlMalloc(16 * sizeof(char));
    if ((res->elems == NULL) || (res->flags == NULL) || (res->names == NULL)) {
        fprintf(stderr, "rpmCreateRealRoot : out of memory !\n");
	exit(1);
    }
    res->max_elem = 16;
    return(res);
}

/*
 * Add a given file to a directory.
 */

int rpmGetRealFile(rpmRealDirPtr parent, const char *file, void *elem) {
    int i, min, max;
    int res;

    /*
     * Search the index for the new file.
     */
    min = i = 0;
    if (parent->nb_elem == 0) goto add_first_elem;
    max = parent->nb_elem - 1;

    while (min < max) {
        i = min + max;
	i /= 2;

        res = strcmp(file, parent->names[i]);
	if (res < 0) max = i - 1;
	else if (res > 0) min = i + 1;
	else {
	    return(i);
	}
    }
    i = min;
    res = strcmp(file, parent->names[i]);
    if (res == 0) {
	    return(i);
    } else if (res > 0) i++;

    /*
     * If more space is needed grow the arrays.
     */
    if (parent->nb_elem >= parent->max_elem) {
        parent->max_elem *= 2;
        parent->elems = (void *)
	        xmlRealloc(parent->elems, parent->max_elem * sizeof(void *));
        parent->names = (void *)
	        xmlRealloc(parent->names, parent->max_elem * sizeof(char *));
        parent->flags = (void *)
	        xmlRealloc(parent->flags, parent->max_elem * sizeof(char));
	if ((parent->elems == NULL) || (parent->flags == NULL) ||
	    (parent->names == NULL)) {
	    fprintf(stderr, "rpmAddRealFile : out of memory !\n");
	    exit(1);
	}
    }

    /*
     * Shift the end of the arrays.
     */

    if (i < parent->nb_elem) {
        memmove(&parent->elems[i + 1], &parent->elems[i], 
	        (parent->nb_elem - i) * sizeof(void *));
        memmove(&parent->names[i + 1], &parent->names[i], 
	        (parent->nb_elem - i) * sizeof(char *));
        memmove(&parent->flags[i + 1], &parent->flags[i], 
	        (parent->nb_elem - i) * sizeof(char));
    }
add_first_elem:
    parent->nb_elem++;

    /*
     * Create the new entry.
     */
    parent->elems[i] = elem;
    parent->names[i] = rpmdataXmlStrdup(file);
    parent->flags[i] = 0;
    return(i);
}

/*
 * Get a given subdirectory of a directory. If it doesn't exist,
 * the subdir is added. The filename may have been added as a file
 * in that case turn it into a directory.
 */

rpmRealDirPtr rpmGetRealSubdir(rpmRealDirPtr parent, const char *subdir) {
    rpmRealDirPtr res;
    int i = rpmGetRealFile(parent, subdir, NULL);

    /*
     * Check if such a directory already exists.
     */
    if (parent->flags[i] & RPM_ELEM_DIRECTORY) {
        return((rpmRealDirPtr) parent->elems[i]);
    }

    /*
     * Allocate a new rpmRealDir block and link it in.
     */

    res = (rpmRealDirPtr) xmlMalloc(sizeof(rpmRealDir));
    if (res == NULL) {
        fprintf(stderr, "rpmCreateRealRoot : out of memory !\n");
	exit(1);
    }
    res->parent = parent;
    res->name = rpmdataXmlStrdup(subdir);
    res->nb_elem = 0;
    res->elems = (void *) xmlMalloc(16 * sizeof(void *));
    res->names = (void *) xmlMalloc(16 * sizeof(char *));
    res->flags = (char *) xmlMalloc(16 * sizeof(char));
    if ((res->elems == NULL) || (res->flags == NULL) || (res->names == NULL)) {
        fprintf(stderr, "rpmCreateRealRoot : out of memory !\n");
	exit(1);
    }
    res->max_elem = 16;

    parent->elems[i] = res;
    parent->flags[i] = RPM_ELEM_DIRECTORY;
    return(res);
}

/*
 * Add a file to the real filesystem tree, and all memorize which
 * RPM provided it
 */

void rpmAddRealFile(rpmRealDirPtr root, const char *file, rpmDataPtr rpm) {
    static char path[2000];
    char *filename;
    char *cur, save;
    size_t file_len;
    rpmRealDirPtr curDir = root;

    if (root == NULL) return;
    if (file == NULL) return;
    if (rpm == NULL) return;
    file_len = strlen(file);
    if (file_len >= sizeof(path)) {
        if (rpm2htmlVerbose > 1)
            fprintf(stderr, "rpmAddRealFile : %s path too long\n", file);
        return;
    }
    memcpy(path, file, file_len + 1);
    filename = &path[0];
    if (*filename != '/') {
        if (rpm2htmlVerbose > 1) 
	    fprintf(stderr, "rpmAddRealFile : %s invalid\n", file);
	return;
    }
    cur = ++filename;
    while (*cur != '\0') {
        if (*cur == '/') {
	    *cur = '\0';
	    curDir = rpmGetRealSubdir(curDir, filename);
	    *cur = '/';
	    cur++;
	    while (*cur == '/') cur++;
	    filename = cur;
	} else if ((*cur == ' ') || (*cur == '\t') ||
	           (*cur == '\n') || (*cur == '\r')) {
	    break;
	} else
	    cur++;
    }
    if (*filename == '\0') return;
    save = *cur;
    *cur = '\0';
    rpmGetRealFile(curDir, filename, rpm);
    *cur = save;
}

/*
 * Merge two real tree, this is somewhat messy since
 * we keep references to only one source package even if
 * two different distributions carries the same file (which
 * have a high probability).
 *
 * NOTE : the old trees are destoyed !!!
 */

rpmRealDirPtr rpmMergeRealRoots(rpmRealDirPtr root1, rpmRealDirPtr root2) {
    rpmRealDirPtr base, add;
    int j;

    if (root1 == NULL) return(root2);
    if (root2 == NULL) return(root1);

    /*
     * We keep the bigger one as-is and add-in the smaller one.
     */
    if (root1->nb_elem > root2->nb_elem) {
        base = root1;
	add = root2;
    } else {
        base = root2;
	add = root1;
    }

    /*
     * For every element in add, search for the same entry in base.
     * If both exists, keep only the directories, if both are directories,
     * recurse !
     */
    for (j = 0;j < add->nb_elem;j++) {
	int i, min, max;
	int res;

	/*
	 * Search the index for the new file.
	 */
	min = i = 0;
	max = base->nb_elem - 1;

	while (min < max) {
	    i = min + max;
	    i /= 2;

	    res = strcmp(add->names[j], base->names[i]);
	    if (res < 0)
	        max = i - 1;
	    else if (res > 0)
	        min = i + 1;
	    else 
	        goto entry_found;
	}
	i = min;
	res = strcmp(add->names[j], base->names[i]);
	if (res == 0) {
		goto entry_found;
	} else if (res > 0) i++;

	/*
	 * The name was not found in base, add it !
	 * If more space is needed grow the arrays.
	 */
	if (base->nb_elem >= base->max_elem) {
	    base->max_elem *= 2;
	    base->elems = (void *)
		    xmlRealloc(base->elems, base->max_elem * sizeof(void *));
	    base->names = (void *)
		    xmlRealloc(base->names, base->max_elem * sizeof(char *));
	    base->flags = (void *)
		    xmlRealloc(base->flags, base->max_elem * sizeof(char));
	    if ((base->elems == NULL) || (base->flags == NULL) ||
		(base->names == NULL)) {
		fprintf(stderr, "rpmMergeRealRoots : out of memory !\n");
		exit(1);
	    }
	}

	/*
	 * Shift the end of the arrays.
	 */

	if (i < base->nb_elem) {
	    memmove(&base->elems[i + 1], &base->elems[i], 
		    (base->nb_elem - i) * sizeof(void *));
	    memmove(&base->names[i + 1], &base->names[i], 
		    (base->nb_elem - i) * sizeof(char *));
	    memmove(&base->flags[i + 1], &base->flags[i], 
		    (base->nb_elem - i) * sizeof(char));
	}
	base->nb_elem++;

	/*
	 * Create the new entry.
	 */
	base->elems[i] = add->elems[j];
	base->names[i] = add->names[j];
	base->flags[i] = add->flags[j];
	continue;

entry_found:
        /*
	 * the name is present in both directories.
	 */
	if ((base->flags[i] & RPM_ELEM_DIRECTORY) &&
	    (add->flags[j] & RPM_ELEM_DIRECTORY)) {
	    /* Two subdirectories, merge them */
	    base->elems[i] = rpmMergeRealRoots(base->elems[i], add->elems[j]);
	/* --------------
	} else if (base->flags[i] & RPM_ELEM_DIRECTORY) {
	     Keep base directory 
	   -------------- */
	} else if (add->flags[j] & RPM_ELEM_DIRECTORY) {
	    void *ptr;
	    int val;

	    /* Keep add directory */
	    ptr = base->names[i] ;
	    base->names[i] = add->names[j];
	    add->names[j] = ptr;
	    ptr = base->elems[i];
	    base->elems[i] = add->elems[j];
	    add->elems[j] = ptr; 
	    val = base->flags[i];
	    base->flags[i] = add->flags[j];
	    add->flags[j] = val;
	/* --------------
	} else {
	     Two files, arbitrarily keep base one, this is simpler ! 
	   -------------- */
	}
    }
    rpmDestroyRealRoot(add);
    return(base);
}

/*
 * Destroy a real tree root.
 */

void rpmDestroyRealRoot(rpmRealDirPtr dir) {
    int i;

    for (i = 0;i < dir->nb_elem;i++) {
        if (dir->flags[i] & RPM_ELEM_DIRECTORY) {
	    rpmDestroyRealRoot(dir->elems[i]);
	}
	xmlFree(dir->names[i]);
    }
    if (dir->elems) xmlFree(dir->elems);
    if (dir->names) xmlFree(dir->names);
    if (dir->flags) xmlFree(dir->flags);
    if (dir->name) xmlFree(dir->name);
    xmlFree(dir);
}

/*
 * Cleanup of the packages lists
 */
void rpmlistCleanup(void) {
    int i;

fprintf(stderr, "rpmlistCleanup\n");

    rpmDataListFree(&rpmSoftwareList);
    rpmRessListFree(&ressList);
    rpmRessListFree(&ressInstalledList);
    for (i = 0; i < HASH_SIZE;i++) {
        rpmHashTable[i] = NULL;
        rpmRessHashTable[i] = NULL;
    }
    rpmHashInitialize();
}

/*
 * Cleanup of this module global varibales.
 */
void rpmdataCleanup(void) {
    int i;

    rpmDirListFree(&dirList);
    rpmDataListFree(&rpmSoftwareList);
    rpmRessListFree(&ressList);
    rpmRessListFree(&ressInstalledList);
    for (i = 0; i < HASH_SIZE;i++) {
        rpmHashTable[i] = NULL;
        rpmRessHashTable[i] = NULL;
    }
    if (dirTree != NULL) {
        rpmFreeSubdir(dirTree);
        dirTree = NULL;
    }
    dirList = NULL;
    treeRoot = NULL;
    rpmHashInitialize();
}

