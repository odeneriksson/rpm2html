/*
 * rpmdata.h : describes the data gathered for each RPM analyzed.
 *
 * See Copyright for the status of this software.
 *
 * $Id: rpmdata.h,v 1.58 2010/10/05 14:36:54 hany Exp $
 */

#ifndef __RPMDATA_H__
#define __RPMDATA_H__

#include <sys/types.h>
#include <stdint.h>
#include <rpm/rpmio.h>
#include <rpm/header.h>

/*
 * Structure associated with a local directory containing RPM files.
 */

#define MAX_ARCHS	(sizeof(int) * 8)

typedef struct rpm_dir {
    struct rpm_dir *next;	/* next in list */
    int no;			/* ID in SQL base */
    int vendor;			/* vendor ID in SQL base */
    char *color;		/* The bgcolor for the pages :-) */
    char *dir;		        /* where to store the HTML files */
    char *subdir;		/* subdirectory for multiple archive */
    char *ftp;		        /* The host URL it's mirrored from */
    char *ftpsrc;		/* The host URL where src.rpm can be found */
    char *host;                 /* Hostname for the server */
    char *mail;			/* mail contact for this directory */
    char *maint;		/* Maintainer name for this directory */
    char *name;			/* Name for this repository */
    char *rpmdir;               /* The local repository */
    char *trust;		/* Confidences in the files */
    char *url;		        /* The output URL */
    int   files;		/* number of files */
    int   installbase;		/* Is this from an installed base */
    int   size;			/* aggregated size of files */
    int   nb_mirrors;		/* numbers of mirrors */
    int   max_mirrors;		/* numbers of slot allocated */
    char **mirrors;             /* Urls for download */
    int   html;		        /* Should we build the HTML pages */
    int   build_tree;		/* Should we build the filesystem tree */
    int   follow_symlinks;	/* Should we follow all symlinks */
    int   rpm_symlinks;		/* Should we open links to RPM files */
    struct rpm_realdir *root;	/* The corresponding filesystem tree */
    char *dbpath;               /* The rpm db path */   /* Added by A. Gibert */  
} rpmDir, *rpmDirPtr;

/*
 * Structure associated with a local directory containing HTML files.
 * We build a tree providing an actual map of the generated site.
 */

typedef struct rpm_subdir {
    struct rpm_subdir *parent;	/* Parent directory ususally .. */
    char  *name;		/* subdirectory name */
    char  *color;		/* Color for the HTML output */
    char  *htmlpath;		/* full path w.r.t. rpm2html_url */
    char  *rpmpath;		/* full path w.r.t. rpm2html_dir */
    int    html;		/* Should we build the HTML pages */
    int    nb_subdirs;		/* number of subdirectories */
    int    max_subdirs;		/* number of subdirectories allocated */
    struct rpm_subdir **subdirs;/* list of the subdirectories */ 
} rpmSubdir, *rpmSubdirPtr;

/*
 * Structure associated with a real directory containing files owned
 * by RPM packages. It reflect the content of user's filesystem if
 * it installed the packages.
 */

#define RPM_ELEM_DIRECTORY	0x1   /* File or directory ? */

typedef struct rpm_realdir {
    struct rpm_realdir *parent;	/* Parent directory */
    char  *name;		/* directory name */
    int    nb_elem;		/* number of files (and subdirs)  */
    int    max_elem;		/* size of allocated arrays       */
                                /* pointers to an array of either */
    void **elems;               /* filename or subdir rpm_realdir */
    char **names;               /* pointer to the array of names  */
    char  *flags;               /* pointer to array of flags      */
} rpmRealDir, *rpmRealDirPtr;

/*
 * structure associated with an architecture.
 */

typedef struct rpm_arch {
    struct rpm_arch *next;	/* next in the list */
    char *os;			/* the Operating System name */
    char *arch;			/* the CPU architecture */
} rpmArch, *rpmArchPtr;

/*
 * structure associated with an rpm
 */

typedef struct rpm_data {
    struct rpm_data *next;	/* next in the full list of RPMs */
    struct rpm_data *nextSoft;	/* next software */
    struct rpm_data *nextArch;	/* RPM for same software on other archs */
    struct rpm_data *nextHash;	/* next in the hash list */
    /*
     * These information need to be kept until the end of the processing
     * to allow the dump of resources directories.
     */
    rpmDirPtr dir;		/* directory infos */
    char *url;                  /* URL for the software */
    char *filename;		/* name of the file */
    const char *name;		/* name of the software */
    char *summary;		/* 1 line summary */
    const char *group;		/* type of software */
    const char *version;	/* version of the software */
    const char *release;	/* software release */
    const char *arch;           /* the architecture system */
    const char *os;             /* the target system */
    const char *distribution;	/* general OS distribution */
    const char *vendor;		/* general OS vendor */
    time_t date;		/* date of packaging */
    int32_t size;		/* size of the software */
    const char *subdir;		/* subdirectory holding the rpm */

    struct rpm_extra_data *extra;/* see below */
} rpmData, *rpmDataPtr;

typedef struct rpm_extra_data {
    /*
     * These information are loaded and discarded as soon as the
     * HTML and RDF pages are built.
     */
    char *packager;		/* the packager */
    char *description;          /* short description */
    char *copyright;            /* software copyright */
    char *changelog;            /* changelog */
    char *srcrpm;		/* source RPM */
    time_t stamp;		/* modification file of the archive */
    char *host;			/* build host */
    int   nb_resources;	        /* #of resources provided */
    int   max_resources;	/* #of resources slot allocated */
    struct rpm_resource **resources;/* list of them */
    int   nb_requires;	        /* #of resources required */
    int   max_requires;	        /* #of resources slot allocated */
    struct rpm_resource **requires;/* list of them */
    char *filelist;             /* the filelist */
    int   nb_sigs;	        /* #of signatures required */
    int   max_sigs;	        /* #of signatures slot allocated */
    struct rpm_sig **sigs;	/* list of them */
} rpmExtraData, *rpmExtraDataPtr;

/*
 * structure associated with a resource
 */
typedef struct rpm_resource {
    struct rpm_resource *next;	/* next in the list */
    struct rpm_resource *nextHash;/* next in the hash table list */
    char *name;			/* name of the resource */
    char *version;              /* version of the resource */
    rpm_dep_flag flag;          /* flag of the resource */
    int   nb_provider;		/* #of resources provided */
    int   max_provider;		/* #of resources slot allocated */
    time_t stamp;		/* max stamp of the provider */
    struct rpm_data **provider; /* list of them */
} rpmRess, *rpmRessPtr;

/*
 * structure associated with signature
 */
typedef struct rpm_sig {
    int size;			/* size of signature */
    int tag;			/* signature tag */
    int type;			/* signature type */
    int *sig;			/* signature */
    #if defined(WITH_GPG)
    char *resolve;		/* resolved signature */
    #endif
} rpmSig, *rpmSigPtr;

/*
 * Variables.
 */
extern int   rpm2htmlVerbose;
extern int   force;
extern time_t currentTime;

extern const char *archNames[MAX_ARCHS];
extern int nbArchs;

extern rpmDataPtr rpmSoftwareList; /* avoid different archs clash */
extern rpmRessPtr ressList;
extern rpmArchPtr archList;
extern rpmRessPtr ressInstalledList;
extern rpmDirPtr dirList;
extern rpmSubdirPtr dirTree;
extern rpmRealDirPtr treeRoot;

/*
 * Functions
 */
extern rpmSubdirPtr rpmNewSubdir(rpmSubdirPtr dir, const char *name,
     const char *htmlpath, const char *rpmpath, const char *color, int html);
extern void rpmRemoveSubdir(rpmSubdirPtr tree);
extern void rpmFreeSubdir(rpmSubdirPtr tree);
extern void rpmDirFree(rpmDirPtr dir);

extern void rpmAddSoftware(rpmDataPtr rpm);
extern void rpmDataPrint(rpmDataPtr rpm);
extern rpmDataPtr rpmSearchSoftware(const char *name, const char *version,
				    const char *release, const char *arch);
/* extern void rpmDataPrintAll(void); */
extern rpmRessPtr rpmRessAdd(char *ress, rpmDataPtr rpm, int installed);
extern rpmRessPtr rpmRequAdd(char *requ, char *requv, rpm_dep_flag requf, rpmDataPtr rpm, int installed);
extern rpmArchPtr rpmArchAdd(char *os, char *arch);
extern rpmDataPtr rpmOpen(char *nameRpm, rpmDirPtr dir, rpmSubdirPtr tree);
extern rpmDataPtr rpmDirScanAll(void);
extern void rpmDirScanOneDist(const char *dist);
extern void rpmDirScanOneDir(const char *directory);
extern char *extractEMail(char *string);
extern char *protectEmail(char *string);

extern rpmDataPtr rpmAddList(rpmDataPtr old, rpmDataPtr newlist);

extern void rpmGroupSort(rpmDataPtr *list, int installed);
extern void rpmDistribSort(rpmDataPtr *list, int installed);
extern void rpmVendorSort(rpmDataPtr *list, int installed);
extern void rpmDateSort(rpmDataPtr *list, int installed);
extern void rpmNameSort(rpmDataPtr *list, int installed);
extern void rpmRessSort(rpmRessPtr ress);

extern rpmRealDirPtr rpmCreateRealRoot(void);
extern void rpmDestroyRealRoot(rpmRealDirPtr dir);
extern void rpmAddRealFile(rpmRealDirPtr root, const char *file,
                           rpmDataPtr rpm);
extern rpmRealDirPtr rpmMergeRealRoots(rpmRealDirPtr root1,
                                       rpmRealDirPtr root2);
extern void rpmFreeExtraData(rpmDataPtr rpm);
extern void rpmdataCleanup(void);
extern void rpmlistCleanup(void);
extern void rpmDataListFree(rpmDataPtr *base);
extern void rpmRessListFree(rpmRessPtr *base);
extern void rpmDirListFree(rpmDirPtr *base);
#endif /* __RPMDATA_H__ */
