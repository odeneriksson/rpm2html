/*
 * cleanup.c : Cleanup the RDF and HTML trees to remove files related to
 *             older packages.
 *
 *  The RDF output tree looks like this:
 *
 *  root ----- distrib1 ---- package1.rdf
 *                         - package2.rdf
 *			   - ...
 *           - distrib2 ---- packagen.rdf
 *	                   - ...
 *           - distribm ...
 *	     - resources --- resource1.rdf
 *	                   - resource2.rdf
 *			   - ...
 *			   - distribs ------ distrib1.rdf
 *			                   - distrib2.rdf
 *			                   - distribn.rdf
 *
 * See Copyright for the status of this software.
 *
 * $Id: cleanup.c,v 1.10 2010/10/05 14:36:54 hany Exp $
 */

#include <config.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <dirent.h>
#include <errno.h>
#include <time.h>

#include <rpm/rpmlib.h>

#include "rpm2html.h"
#include "rpmdata.h"
#include "html.h"
#include "rdf.h"
#include "rdf_api.h"
#include "language.h"

#ifndef HAVE_SNPRINTF
#error You really need snprintf ...
#endif
static int nb_removed_files = 0;

/* Build a complete path; never use a truncated name for file cleanup. */
static char *rpmCleanupPath(const char *dir, const char *filename) {
    size_t dirlen = strlen(dir);
    size_t filelen = strlen(filename);
    char *path;

    if (dirlen > SIZE_MAX - 2 || filelen > SIZE_MAX - dirlen - 2) {
        errno = ENAMETOOLONG;
        return NULL;
    }
    path = malloc(dirlen + filelen + 2);
    if (path == NULL)
        return NULL;

    memcpy(path, dir, dirlen);
    path[dirlen] = '/';
    memcpy(path + dirlen + 1, filename, filelen + 1);
    return path;
}

/*
 * Cleanup the global varibales of this module
 */
void cleanupCleanup(void) {
    nb_removed_files = 0;
}

/*
 * Check an RPM RDF file for obsolescence.
 */

static void rpmOneFileCleanup(char *file) {
    rdfSchema rdf;
    rdfNamespace rpmNs;
    rdfNamespace rdfNs;
    rdfDescription desc;
    char *name;
    char *version;
    char *release;
    char *arch;
    char *URL;
    int   aboutRPM = 0;
    int   isOk = 0;
    rpmDataPtr found;

    rdf = rdfRead(file);
    if (rdf == NULL) {
	unlink(file);
	nb_removed_files++;
        return;
    }

    /*
     * Start the analyze, check that's an RDF for RPM packages.
     */
    rdfNs = rdfGetNamespace(rdf, "http://www.w3.org/TR/WD-rdf-syntax#");
    if (rdfNs == NULL) {
	if (rpm2htmlVerbose)
	    printf("%s is not an RDF schema\n", file);
	rdfDestroySchema(rdf);
	unlink(file);
	nb_removed_files++;
	return;
    }
    rpmNs = rdfGetNamespace(rdf, "http://www.rpm.org/");
    if (rdfNs == NULL) {
	if (rpm2htmlVerbose)
	    printf("%s is not an RPM specific RDF schema\n", file);
	rdfDestroySchema(rdf);
	unlink(file);
	nb_removed_files++;
	return;
    }
    desc = rdfFirstDescription(rdf);
    if (rdfNs == NULL) {
	if (rpm2htmlVerbose)
	    printf("%s RDF schema seems empty\n", file);
	rdfDestroySchema(rdf);
	unlink(file);
	nb_removed_files++;
	return;
    }

    /*
     * We are pretty sure that it will be a valid schema,
     * Walk the tree and collect the packages descriptions.
     */
    if (desc != NULL) {
        /*
	 * Get the resource URL ...
	 */
	URL = rdfGetDescriptionHref(rdf, desc);
	if (URL == NULL) {
	    if (rpm2htmlVerbose)
		printf("%s RDF schema invalid : Desc without href\n",
		        file);
	    rdfDestroySchema(rdf);
	    unlink(file);
	    nb_removed_files++;
	    return;
	}

	/*
	 * Now extract all the metadata information from the RDF tree
	 */
	rdfGetValue(desc, "Name", rpmNs, &name, NULL);
	if (name == NULL) {
	    if (rpm2htmlVerbose)
		printf("%s RDF schema invalid : no Name\n", file);
	    rdfDestroySchema(rdf);
	    unlink(file);
	    nb_removed_files++;
	    return;
	}
	rdfGetValue(desc, "Version", rpmNs, &version, NULL);
	if (version == NULL) {
	    if (rpm2htmlVerbose)
		printf("%s RDF schema invalid : no Version\n", file);
	    rdfDestroySchema(rdf);
	    unlink(file);
	    nb_removed_files++;
	    return;
	}
	rdfGetValue(desc, "Release", rpmNs, &release, NULL);
	if (release == NULL) {
	    if (rpm2htmlVerbose)
		printf("%s RDF schema invalid : no Release\n", file);
	    rdfDestroySchema(rdf);
	    unlink(file);
	    nb_removed_files++;
	    return;
	}
	rdfGetValue(desc, "Arch", rpmNs, &arch, NULL);
	if (arch == NULL) {
	    if (rpm2htmlVerbose)
		printf("%s RDF schema invalid : no Arch\n", file);
	    rdfDestroySchema(rdf);
	    unlink(file);
	    nb_removed_files++;
	    return;
	}

        /*********************
	rdfGetValue(desc, "Os", rpmNs, &os, NULL);
	if (os == NULL) {
	    if (rpm2htmlVerbose)
		printf("%s RDF schema invalid : no Os\n", file);
	    rdfDestroySchema(rdf);
	    unlink(file);
	    nb_removed_files++;
	    return;
	}
	 **********************/
	aboutRPM = 1;

        /*
	 * Now that we have gathered name,version,release,arch and os
	 * Check that this is in the package list.
	 */
        found = rpmSearchSoftware(name, version, release, arch);
	if (found) {
	    isOk = 1;
	}
	if (URL != NULL) free(URL);

        /*
	 * Skip to next Description in the RDf file.
	 */
	desc = rdfNextDescription(desc); 
    }

    if ((aboutRPM) && (!isOk)) {
	/* if (rpm2htmlVerbose) */
	    printf("Removing outdated %s RDF description\n", file);
        unlink(file);
	nb_removed_files++;
    }

    /*
     * Cleanup.
     */
    rdfDestroySchema(rdf);
}

/*
 * Scan a distribution directory for RDF files
 */
static void rpmOneDirCleanup(char *dir) {
    char *filename;
    char *path;
    struct stat buf;
    int len;
    DIR *d;
    struct dirent *file;

    d = opendir(dir);
    if (d == NULL) {
        fprintf(stderr, "rpmOneDirCleanup: Listing of %s failed: %s\n", dir,
	        strerror(errno));
	return;
    } else {
        while ((file = readdir(d)) != NULL) {
	    filename = file->d_name;
	    len = strlen(filename);

	    /*
	     * Compute the full path
	     */
	    path = rpmCleanupPath(dir, filename);
	    if (path == NULL) {
	        fprintf(stderr, "Couldn't build cleanup path %s/%s: %s\n",
	                dir, filename, strerror(errno));
	        continue;
	    }

	    /*
	     * Stat() the file to detect directory and symlimks
	     */
	    if (lstat(path, &buf) != 0) {
	        fprintf(stderr, "rpmOneDirCleanup: Couldn't stat(%s)\n", path);
		free(path);
		continue;
	    }

	    /*
	     * Don't follow of analyze symlinks, remove them
	     */
            if (S_ISLNK(buf.st_mode)) {
		if (rpm2htmlVerbose)
		    fprintf(stderr, "Cleaning: removing symlink %s\n", path);
		unlink(path);
		nb_removed_files++;
	        free(path);
	        continue;
	    }

	    /*
	     * Check for RDF files by looking at the suffix
	     */
	    else if ((len >= 5) && (!strcasecmp(&filename[len - 4], ".rdf"))) {
	        rpmOneFileCleanup(path);
	    }

	    /*
	     * Else if this is a directory, recurse !
	     */
	    else if (S_ISDIR(buf.st_mode)) {
		if (filename[0] != '.') {
		    rpmOneDirCleanup(path);
		}
	    }
	    free(path);
	}
    }
    closedir(d);
}

/*
 * Check a resource RDF file for obsolescence.
 */

static void rpmOneResourceCleanup(char *file) {
    rdfSchema rdf;
    rdfNamespace rpmNs;
    rdfNamespace rdfNs;
    rdfDescription desc;
    char *name;
    char *version;
    char *release;
    char *arch;
    char *URL;
    int   aboutRPM = 0;
    int   isOk = 0;
    rpmDataPtr found;

    rdf = rdfRead(file);
    if (rdf == NULL) {
	unlink(file);
	nb_removed_files++;
        return;
    }

    /*
     * Start the analyze, check that's an RDF for RPM resource.
     */
    rdfNs = rdfGetNamespace(rdf, "http://www.w3.org/TR/WD-rdf-syntax#");
    if (rdfNs == NULL) {
	if (rpm2htmlVerbose)
	    printf("%s is not an RDF schema\n", file);
	rdfDestroySchema(rdf);
	unlink(file);
	nb_removed_files++;
	return;
    }
    rpmNs = rdfGetNamespace(rdf, "http://www.rpm.org/");
    if (rdfNs == NULL) {
	if (rpm2htmlVerbose)
	    printf("%s is not an RPM specific RDF schema\n", file);
	rdfDestroySchema(rdf);
	unlink(file);
	nb_removed_files++;
	return;
    }
    desc = rdfFirstDescription(rdf);
    if (rdfNs == NULL) {
	if (rpm2htmlVerbose)
	    printf("%s RDF schema seems empty\n", file);
	rdfDestroySchema(rdf);
	unlink(file);
	nb_removed_files++;
	return;
    }

    /*
     * We are pretty sure that it will be a valid schema,
     * Walk the tree and collect the packages descriptions.
     */
    if (desc != NULL) {
        /*
	 * Get the resource URL ...
	 */
	URL = rdfGetDescriptionHref(rdf, desc);
	if (URL == NULL) {
	    if (rpm2htmlVerbose)
		printf("%s RDF schema invalid : Desc without href\n",
		        file);
	    rdfDestroySchema(rdf);
	    unlink(file);
	    nb_removed_files++;
	    return;
	}

	/*
	 * Now extract all the metadata information from the RDF tree
	 */
	rdfGetValue(desc, "Name", rpmNs, &name, NULL);
	if (name == NULL) {
	    if (rpm2htmlVerbose)
		printf("%s RDF schema invalid : no Name\n", file);
	    rdfDestroySchema(rdf);
	    return;
	}
	rdfGetValue(desc, "Version", rpmNs, &version, NULL);
	if (version == NULL) {
	    if (rpm2htmlVerbose)
		printf("%s RDF schema invalid : no Version\n", file);
	    rdfDestroySchema(rdf);
	    return;
	}
	rdfGetValue(desc, "Release", rpmNs, &release, NULL);
	if (release == NULL) {
	    if (rpm2htmlVerbose)
		printf("%s RDF schema invalid : no Release\n", file);
	    rdfDestroySchema(rdf);
	    return;
	}
	rdfGetValue(desc, "Arch", rpmNs, &arch, NULL);
	if (arch == NULL) {
	    if (rpm2htmlVerbose)
		printf("%s RDF schema invalid : no Arch\n", file);
	    rdfDestroySchema(rdf);
	    return;
	}

        /***********
	rdfGetValue(desc, "Os", rpmNs, &os, NULL);
	if (os == NULL) {
	    if (rpm2htmlVerbose)
		printf("%s RDF schema invalid : no Os\n", file);
	    rdfDestroySchema(rdf);
	    return;
	}
	 *************/

	aboutRPM = 1;

        /*
	 * Now that we have gathered name,version,release,arch and os
	 * Check that this is in the package list.
	 */
        found = rpmSearchSoftware(name, version, release, arch);
	if (found) {
	    isOk = 1;
	}

        /*
	 * Skip to next Description in the RDf file.
	 */
	desc = rdfNextDescription(desc); 
    }

    if ((aboutRPM) && (!isOk)) {
	/* if (rpm2htmlVerbose) */
	    printf("Removing outdated %s RDF resource\n", file);
        unlink(file);
	nb_removed_files++;
    }
    /*
     * Cleanup.
     */
    rdfDestroySchema(rdf);
}

/*
 * Check the resources directory.
 */

static void rpmResourcesCleanup(char *dir) {
    char *filename;
    char *path;
    struct stat buf;
    int len;
    DIR *d;
    struct dirent *file;

    d = opendir(dir);
    if (d == NULL) {
        fprintf(stderr, "rpmResourcesCleanup: Listing of %s failed: %s\n", dir,
	        strerror(errno));
	return;
    } else {
        while ((file = readdir(d)) != NULL) {
	    filename = file->d_name;
	    len = strlen(filename);

	    /*
	     * Compute the full path
	     */
	    path = rpmCleanupPath(dir, filename);
	    if (path == NULL) {
	        fprintf(stderr, "Couldn't build cleanup path %s/%s: %s\n",
	                dir, filename, strerror(errno));
	        continue;
	    }

	    /*
	     * Stat() the file to detect directory and symlimks
	     */
	    if (lstat(path, &buf) != 0) {
	        fprintf(stderr, "Couldn't stat(%s)\n", path);
		unlink(path);
		nb_removed_files++;
		free(path);
		continue;
	    }

	    /*
	     * Don't follow of analyze symlinks, remove them
	     */
            if (S_ISLNK(buf.st_mode)) {
		if (rpm2htmlVerbose)
		    fprintf(stderr, "Cleaning: removing symlink %s\n", path);
		unlink(path);
		nb_removed_files++;
	        free(path);
	        continue;
	    }

	    /*
	     * Check for RDF files by looking at the suffix
	     */
	    else if ((len >= 5) && (!strcasecmp(&filename[len - 4], ".rdf"))) {
	        rpmOneResourceCleanup(path);
	    }

	    /*
	     * Else if this is a directory, recurse !
	     */
	    else if (S_ISDIR(buf.st_mode)) {
		if (filename[0] != '.') {
		    if (!strcmp(filename, "distribs")) {
		        /* TOTO : explore the distrib too ! */
		    } else {
			if (rpm2htmlVerbose)
	fprintf(stderr, "Cleaning: directory resources/%s should not exist\n",
		            filename);
		    }
		}
	    }
	    free(path);
	}
    }
    closedir(d);
}

/*
 * Cleanup the whole RDF repository
 */

void rpmDirCleanupAll(void) {
    char *filename;
    char *path;
    struct stat buf;
    int len;
    DIR *d;
    struct dirent *file;
    int start_time;
    int end_time;

    if (!rpm2html_dump_rdf) return;
    if (!rpm2html_rdf_dir) return;

    nb_removed_files = 0;
    start_time = time(NULL);

    d = opendir(rpm2html_rdf_dir);
    if (d == NULL) {
        fprintf(stderr, "Listing of %s failed: %s\n", rpm2html_rdf_dir,
	        strerror(errno));
	return;
    } else {
        while ((file = readdir(d)) != NULL) {
	    filename = file->d_name;
	    len = strlen(filename);

	    /*
	     * Compute the full path
	     */
	    path = rpmCleanupPath(rpm2html_rdf_dir, filename);
	    if (path == NULL) {
	        fprintf(stderr, "Couldn't build cleanup path %s/%s: %s\n",
	                rpm2html_rdf_dir, filename, strerror(errno));
	        continue;
	    }

	    /*
	     * Stat() the file to detect directory and symlimks
	     */
	    if (lstat(path, &buf) != 0) {
	        fprintf(stderr, "rpmDirCleanupAll: Couldn't stat(%s)\n", path);
		free(path);
		continue;
	    }

	    /*
	     * Don't follow of analyze symlinks,
	     */
            if (S_ISLNK(buf.st_mode)) {
		if (rpm2htmlVerbose)
		    fprintf(stderr, "Cleaning: removing symlink %s\n", path);
		unlink(path);
		nb_removed_files++;
	        free(path);
	        continue;
	    }

	    /*
	     * We shouldn't get any non-rdf file here
	     */
	    else if ((len >= 5) && (!strcasecmp(&filename[len - 4], ".rdf"))) {
	        rpmOneFileCleanup(filename);
	    }

	    /*
	     * Else if this is a directory, handle it
	     */
	    else if (S_ISDIR(buf.st_mode)) {
		if (!strcmp(filename, "resources")) {
		    rpmResourcesCleanup(path);
		} else if (filename[0] != '.') {
		    rpmOneDirCleanup(path);
		}
	    }
	    
	    /*
	     * Else, remove that file it shouldn't be here !
	     */
            else {
		if (rpm2htmlVerbose)
		    fprintf(stderr, "Cleaning: removing file %s\n", path);
		unlink(path);
		nb_removed_files++;
	        free(path);
	        continue;
	    }

	    free(path);
	}
    }
    closedir(d);
    end_time = time(NULL);
    /* if (rpm2htmlVerbose) */
        printf("Cleanup took %d second, %d files removed\n",
	       end_time - start_time, nb_removed_files);
}

