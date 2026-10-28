/*
 * rdf.c : implementation for the RDF encoding/decoding of RPM information.
 *
 * See Copyright for the status of this software.
 *
 * $Id: rdf.c,v 1.50 2010/10/05 14:36:54 hany Exp $
 */

#include "config.h"
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
#include <time.h>
#include <errno.h>
#include <ctype.h>
#include <zlib.h>

#include "rpm2html.h"
#include "rpmdata.h"
#include "html.h"
#include "language.h"

#include "rdf_api.h"
#include "rdf.h"
#ifdef WITH_SQL
#include "sql.h"
#endif

#ifndef HAVE_SNPRINTF
#error Sorry you really need snprintf for basic security reasons
#endif

/*
 * Open and process an RDF file in the incoming tree of an rpm2html mirror.
 */
rpmDataPtr rpmOpenRdf(char *nameRdf, rpmDirPtr dir, rpmSubdirPtr tree) {
    char file[1000];
    rpmDataPtr rpm;
#ifdef WITH_SQL
    int id;
    int i;
#endif

    /*
     * create path
     */
    if (tree->htmlpath[0] != '\0')
	snprintf(file, sizeof(file), "%s/%s/%s", dir->rpmdir, tree->rpmpath, nameRdf);
    else
	snprintf(file, sizeof(file), "%s/%s", dir->rpmdir, nameRdf);

    rpm = rpmOpenRdfFile(file);

    if (rpm != NULL) {
	/* setup tree and subdir information */
	rpm->dir = dir;
	if (tree) {
	    if ((tree->rpmpath != NULL) && (tree->rpmpath[0] != '\0'))
		rpm->subdir = stringAdd(tree->rpmpath);
	    else
		rpm->subdir = NULL;
	} else
	    rpm->subdir = NULL;

#ifdef WITH_SQL
	id = sql_add_package(file, rpm->name, rpm->version, rpm->release,
		    rpm->arch, dir->no, rpm->url, rpm->extra->srcrpm,
		    dir->vendor, rpm->extra->packager,
		    rpm->group, rpm->summary, rpm->extra->description,
		    rpm->extra->copyright, rpm->date, rpm->size, rpm->os,
		    rpm->distribution, rpm->vendor);
	if (id > 0) {
	    for (i = 0;i < rpm->extra->nb_resources;i++)
		sql_add_provides(id, rpm->extra->resources[i]->name);
	    for (i = 0;i < rpm->extra->nb_requires;i++)
		sql_add_requires(id, rpm->extra->requires[i]->name,
			         rpm->extra->requires[i]->flag,
			         rpm->extra->requires[i]->version);
	}
#endif

	/* Add the package files to the real filesystem tree if asked for */
#ifdef WITH_SQL
	if ((rpm->extra->filelist != NULL) &&
	    ((dir->build_tree != 0) || (id > 0))) {
#else
	if ((dir->build_tree != 0) && (rpm->extra->filelist != NULL)) {
#endif
	    char *cur, *filename;
	    
	    cur = rpm->extra->filelist;
	    while ((*cur != '\0') && (*cur != '/')) cur++;
	    filename = cur;
	    while (*cur != '\0') {
		if ((*cur == '\n') || (*cur == '\r')) {
		    if (cur != filename) {
#ifdef WITH_SQL
			if (dir->build_tree != 0)
			    rpmAddRealFile(dir->root, filename, rpm);
			if (id > 0) {
			    *cur = 0;
			    sql_add_file(filename,id);
			    *cur = '\n';
			}
#else
			rpmAddRealFile(dir->root, filename, rpm);
#endif
		    }
		    while ((*cur != '\0') && (*cur != '/')) cur++;
		    filename = cur;
		} else
		    cur++;
	    }
	    if (cur != filename)
		rpmAddRealFile(dir->root, filename, rpm);
	}

	/* Register this package */
	rpmAddSoftware(rpm);

	/* dump the HTML related to this package */
	if (rpm2html_dump_html)
	    dumpRpmHtml(rpm, tree);
	if (rpm2html_dump_rdf)
	    dumpRpmRdf(rpm, tree);

	/* free large amount of data not used later */
        rpmFreeExtraData(rpm);

	/* increment the counters */
	rpm2html_files++;
	rpm2html_size += rpm->size / 1024;
    }
    return(rpm);
}

/*
 * Open an RDF file call the parser to create a XML tree
 * Then walk the tree and build an rpmData structure for
 * the corresponding package.
 */
rpmDataPtr rpmOpenRdfFile(char *file) {
    char nameBuffer[200];
    rdfSchema rdf;
    rdfNamespace rpmNs;
    rdfNamespace rdfNs;
    rdfDescription desc;
    rdfBag bag;
    rdfElement elem;
    char *value;
    rpmDataPtr rpm;
    struct stat buf;
    int i;

    rdf = rdfRead(file);
    if (rdf == NULL) return(NULL);

    /*
     * Start the analyze, check that's an RDf for RPM packages.
     */
    rdfNs = rdfGetNamespace(rdf, "http://www.w3.org/TR/WD-rdf-syntax#");
    if (rdfNs == NULL) {
        fprintf(stderr, "%s is not an RDF schema\n", file);
	rdfDestroySchema(rdf);
	return(NULL);
    }
    rpmNs = rdfGetNamespace(rdf, "http://www.rpm.org/");
    if (rdfNs == NULL) {
        fprintf(stderr, "%s is not an RPM specific RDF schema\n", file);
	rdfDestroySchema(rdf);
	return(NULL);
    }
    desc = rdfFirstDescription(rdf);
    if (rdfNs == NULL) {
        fprintf(stderr, "%s RDF schema seems empty\n", file);
	rdfDestroySchema(rdf);
	return(NULL);
    }

    /*
     * We are pretty sure that it will be a valid schema,
     * allocate a new rpmData block, and an rpmExtraData block fill them
     */
    rpm = (rpmDataPtr) xmlMalloc(sizeof(rpmData));
    if (rpm == NULL) {
        fprintf(stderr, "cannot allocate %d bytes: %s\n", sizeof(rpmData),
	        strerror(errno));
        return(NULL);
    }
    memset(rpm, 0, sizeof(rpmData));
    rpm->extra = (rpmExtraDataPtr) xmlMalloc(sizeof(rpmExtraData));
    if (rpm == NULL) {
        fprintf(stderr, "cannot allocate %d bytes: %s\n", sizeof(rpmExtraData),
	        strerror(errno));
        return(NULL);
    }
    memset(rpm->extra, 0, sizeof(rpmExtraData));

    stat(file, &buf);
    rpm->extra->stamp = buf.st_mtime;

    /*
     * Now extract all the metadata information from the RDF tree
     */
    rdfGetValue(desc, "Name", rpmNs, &value, NULL);
    if (value != NULL) {
        rpm->name = stringAdd(value);
	xmlFree(value);
    } else {
        fprintf(stderr, "%s RDF schema invalid : no Name\n", file);
	rdfDestroySchema(rdf);
	return(NULL);
    }
    rdfGetValue(desc, "Version", rpmNs, &value, NULL);
    if (value != NULL) {
        rpm->version = stringAdd(value);
	xmlFree(value);
    } else {
        fprintf(stderr, "%s RDF schema invalid : no Version\n", file);
	rdfDestroySchema(rdf);
	return(NULL);
    }
    rdfGetValue(desc, "Release", rpmNs, &value, NULL);
    if (value != NULL) {
        rpm->release = stringAdd(value);
	xmlFree(value);
    } else {
        fprintf(stderr, "%s RDF schema invalid : no Release\n", file);
	rdfDestroySchema(rdf);
	return(NULL);
    }
    rdfGetValue(desc, "URL", rpmNs, &value, NULL);
    if (value != NULL) rpm->url = value;
    else rpm->url = NULL;

    rdfGetValue(desc, "Arch", rpmNs, &value, NULL);
    if (value != NULL) {
        rpm->arch = stringAdd(value);
	xmlFree(value);
    } else rpm->arch = xmlStrdup("noarch");

    rdfGetValue(desc, "Os", rpmNs, &value, NULL);
    if (value != NULL) {
        rpm->os = stringAdd(value);
	xmlFree(value);
    } else rpm->os = xmlStrdup("linux");

    rdfGetValue(desc, "Distribution", rpmNs, &value, NULL);
    if (value != NULL) {
        rpm->distribution = stringAdd(value);
	xmlFree(value);
    } else rpm->distribution = stringAdd(localizedStrings[LANG_UNKNOWN]);

    rdfGetValue(desc, "Vendor", rpmNs, &value, NULL);
    if (value != NULL) {
        rpm->vendor = stringAdd(value);
	xmlFree(value);
    } else rpm->vendor = xmlStrdup(localizedStrings[LANG_UNKNOWN]);

    rdfGetValue(desc, "Packager", rpmNs, &value, NULL);
    if (value != NULL) rpm->extra->packager = value;
    else rpm->extra->packager = NULL;

    rdfGetValue(desc, "Group", rpmNs, &value, NULL);
    if (value != NULL) {
        rpm->group = stringAdd(value);
	xmlFree(value);
    } else rpm->group = xmlStrdup(localizedStrings[LANG_NO_GROUP]);
    
    rdfGetValue(desc, "Summary", rpmNs, &value, NULL);
    if (value != NULL) rpm->summary = value;
    else rpm->summary = xmlStrdup(localizedStrings[LANG_NO_SUMMARY]);

    rdfGetValue(desc, "Description", rpmNs, &value, NULL);
    if (value != NULL) rpm->extra->description = value;
    else rpm->extra->description = xmlStrdup(localizedStrings[LANG_NO_DESCRIPTION]);

    rdfGetValue(desc, "Copyright", rpmNs, &value, NULL);
    if (value != NULL) rpm->extra->copyright = value;
    else rpm->extra->copyright = NULL;

    rdfGetValue(desc, "Changelog", rpmNs, &value, NULL);
    if (value != NULL) rpm->extra->changelog = value;
    else rpm->extra->changelog = NULL;

    rdfGetValue(desc, "Sources", rpmNs, &value, NULL);
    if (value != NULL) {
        rpm->extra->srcrpm = value;
    } else rpm->extra->srcrpm = xmlStrdup("");

    rdfGetValue(desc, "Size", rpmNs, &value, NULL);
    if (value != NULL) {
        if (sscanf(value, "%d", &(rpm->size)) != 1)
	    rpm->size = 0;
	xmlFree(value);
    } else rpm->size = 0;

    rdfGetValue(desc, "Date", rpmNs, &value, NULL);
    if (value != NULL) {
        if (sscanf(value, "%d", &i) != 1)
	    rpm->date = 0;
	else
	    rpm->date = i;
	xmlFree(value);
    } else rpm->date = 0;

    rdfGetValue(desc, "BuildHost", rpmNs, &value, NULL);
    if (value != NULL) rpm->extra->host = value;
    else rpm->extra->host = xmlStrdup(localizedStrings[LANG_NO_HOST]);

    rdfGetValue(desc, "Files", rpmNs, &value, NULL);
    if (value != NULL) rpm->extra->filelist = value;
    else rpm->extra->filelist = NULL;

    /*
     * Fetching packages provided is a bit more tricky, one have to
     * find the RDF Bag, and scan it's values.
     */
    rpm->extra->nb_resources = 0;
    rdfGetValue(desc, "Provides", rpmNs, NULL, &bag);
    if (bag != NULL) {
        elem = rdfFirstChild(bag);
	rpm->extra->max_resources = 5;
	rpm->extra->resources = (rpmRessPtr *) xmlMalloc(rpm->extra->max_resources *
	                                       sizeof(rpmRessPtr));
	if (rpm->extra->resources == NULL) {
	    fprintf(stderr, "rpmOpenRdf : ran out of memory\n");
	    exit(1);
	}
	i = 0;
	while (elem != NULL) {
	    char *name; 
	    if (i >= rpm->extra->max_resources) {
	        rpm->extra->max_resources *= 2;
		rpm->extra->resources = (rpmRessPtr *) xmlRealloc(rpm->extra->resources,
		                   rpm->extra->max_resources * sizeof(rpmRessPtr));
		if (rpm->extra->resources == NULL) {
		    fprintf(stderr, "rpmOpenRdf : ran out of memory\n");
		    exit(1);
		}
	    }
	    /*
	     * Check that we are scanning an RPM Resource.
	     */
	    name = rdfElemGetPropertyName(elem);
	    if ((name != NULL) &&
	        (!strcmp(name, "Resource")) &&
	        (rdfElemGetNamespace(elem) == rpmNs)) {
		value = rdfElemGetValue(elem);
		if (value != NULL) {
		    rpm->extra->resources[i] = rpmRessAdd(value, rpm, 0);
		    i++;
		    rpm->extra->nb_resources++;
		    xmlFree(value);
		} else if (rpm2htmlVerbose > 1) {
		    fprintf(stderr, "%s : malformed Resource element !\n", file);
		}
	    } else if (rpm2htmlVerbose > 1) {
	        fprintf(stderr, "%s : malformed Provides bag !\n", file);
	    }
	    if (name != NULL) xmlFree(name);
	    elem = rdfNextElem(elem);
	}
    } else if (rpm2htmlVerbose > 1) {
        fprintf(stderr, "%s doesn't export any resource\n", file);
    }

    /*
     * idem for the dependencies.
     */
    rpm->extra->nb_requires = 0;
    rdfGetValue(desc, "Requires", rpmNs, NULL, &bag);
    if (bag != NULL) {
        elem = rdfFirstChild(bag);
	rpm->extra->max_requires = 5;
	rpm->extra->requires = (rpmRessPtr *) xmlMalloc(rpm->extra->max_requires *
	                                       sizeof(rpmRessPtr));
	if (rpm->extra->requires == NULL) {
	    fprintf(stderr, "rpmOpenRdf : ran out of memory\n");
	    exit(1);
	}
	i = 0;
	while (elem != NULL) {
	    char *name; 
	    if (i >= rpm->extra->max_requires) {
	        rpm->extra->max_requires *= 2;
		rpm->extra->requires = (rpmRessPtr *) xmlRealloc(rpm->extra->requires,
		                   rpm->extra->max_requires * sizeof(rpmRessPtr));
		if (rpm->extra->requires == NULL) {
		    fprintf(stderr, "rpmOpenRdf : ran out of memory\n");
		    exit(1);
		}
	    }
	    /*
	     * Check that we are scanning an RPM Resource.
	     */
	    name = rdfElemGetPropertyName(elem);
	    if ((name != NULL) &&
	        (!strcmp(name, "Resource")) &&
	        (rdfElemGetNamespace(elem) == rpmNs)) {
		value = rdfElemGetValue(elem);
		if (value != NULL) {
		    rpm_dep_flag dep = RPM2HTML_REQ_NONE;
		    xmlChar *reqval = NULL;

		    if ((reqval = xmlGetProp(elem, "gt")) != NULL) {
			dep = RPM2HTML_REQ_GT;
		    } else if ((reqval = xmlGetProp(elem, "geq")) != NULL) {
			dep = RPM2HTML_REQ_GEQ;
		    } else if ((reqval = xmlGetProp(elem, "lt")) != NULL) {
			dep = RPM2HTML_REQ_LT;
		    } else if ((reqval = xmlGetProp(elem, "leq")) != NULL) {
			dep = RPM2HTML_REQ_LEQ;
		    } else if ((reqval = xmlGetProp(elem, "equ")) != NULL) {
			dep = RPM2HTML_REQ_EQU;
		    }

		    /*
		     * TODO: fix to import RDF dept to a specific version
		     */
		    rpm->extra->requires[i] = rpmRequAdd(value,
			                                 reqval, dep, rpm, 0); 
		    i++;
		    rpm->extra->nb_requires++;
		    xmlFree(value);
		    if (reqval != NULL)
			xmlFree(reqval);
		} else if (rpm2htmlVerbose > 1) {
		    fprintf(stderr, "%s : malformed Resource element !\n", file);
		}
	    } else if (rpm2htmlVerbose > 1) {
	        fprintf(stderr, "%s : malformed Provides bag !\n", file);
	    }
	    if (name != NULL) xmlFree(name);
	    elem = rdfNextElem(elem);
	}
    }

    /*
     * Finish filling the rpmData structure.
     */
    value = rdfGetDescriptionAbout(rdf, desc);
    if (value != NULL) {
        char *filename = &value[strlen(value)];

	while ((filename > value) && (*filename != '/'))
	    filename --;
	rpm->filename = xmlStrdup(filename);
	xmlFree(value);
    } else {
	snprintf(nameBuffer, 200, "%s-%s-%s.%s.rpm",
		 rpm->name, rpm->version, rpm->release, rpm->arch);
	rpm->filename = xmlStrdup(nameBuffer);
    }


    /*
     * Cleanup.
     */
    rdfDestroySchema(rdf);
    return(rpm);
}

/*
 * Create and RDF file containing the description for the given RPM data.
 */
void dumpRpmRdf(rpmDataPtr rpm, rpmSubdirPtr tree) {
    FILE *output;
    struct stat stat_buf;
    struct tm * tstruct;
    rpmDirPtr dir = rpm->dir;
    char *base = dir->dir;
    int i;
    char buf[10000];
    char file[1000];
    char dotdot[50];
    char *buffer;
    int size;
    int stat_res = 0;
    rdfSchema rdf;
    rdfNamespace rpmNs;
    rdfDescription desc;
    rdfElement elem;
    rdfBag bag;
    int deep;
    const char *ptr;

    if (!rpm2html_dump_rdf) return;

    if (rpm2html_rdf_dir != NULL) base = rpm2html_rdf_dir;

    deep = 0;
    if (rpm->subdir != NULL)
	for (ptr = rpm->subdir, deep++;*ptr != 0;ptr++)
	    if (*ptr == '/') deep++;
    if (dir->subdir != NULL)
	for (ptr = dir->subdir, deep++;*ptr != 0;ptr++)
	    if (*ptr == '/') deep++;
    
    if ((dir->subdir != NULL) && (dir->subdir[0] != '\0')) {
	if ((rpm->subdir != NULL) && (rpm->subdir[0] != '\0'))
	    snprintf(file, sizeof(file), "%s/%s/%s/%s.rdf", base, dir->subdir,
	            rpm->subdir, rpmName(rpm));
	else
	    snprintf(file, sizeof(file), "%s/%s/%s.rdf", base, dir->subdir, rpmName(rpm));
    } else {
	if ((rpm->subdir != NULL) && (rpm->subdir[0] != '\0'))
	    snprintf(file, sizeof(file), "%s/%s/%s.rdf", base, rpm->subdir, rpmName(rpm));
	else
	    snprintf(file, sizeof(file), "%s/%s.rdf", base, rpmName(rpm));
    }
    dotdot[0] = '\0';
    for (;deep > 0;deep --)
	strcat(dotdot, "../");

    /*
     * Check wether the RPM timestamp is older than the filestamp.
    if ((stat_res = stat(file, &stat_buf)) == 0) {
        if (stat_buf.st_mtime > rpm->stamp) return;
    }
     */
    stat_res = stat(file, &stat_buf);

    /*
     * Create the directory if needed
     */
    if ((dir->subdir != NULL) && (dir->subdir[0] != '\0')) {
	if ((rpm->subdir != NULL) && (rpm->subdir[0] != '\0'))
	    snprintf(buf, sizeof(buf), "%s/%s/%s", base, dir->subdir, rpm->subdir);
	else
	    snprintf(buf, sizeof(buf), "%s/%s", base, dir->subdir);
    } else {
	if ((rpm->subdir != NULL) && (rpm->subdir[0] != '\0'))
	    snprintf(buf, sizeof(buf), "%s/%s", base, rpm->subdir);
	else
	    snprintf(buf, sizeof(buf), "%s", base);
    }
    createDirectory(buf);

    /*
     * build the RDF document tree
     * Note : the order is not important but following the rpmData
     *        structure order is easier to check.
     */
    rdf = rdfNewSchema();
    rpmNs = rdfNewNamespace(rdf, "http://www.rpm.org/", "RPM");

    if ((rpm->subdir != NULL) && (rpm->subdir[0] != '\0')) {
	if (dir->mirrors[0] != NULL)
	    snprintf(buf, sizeof(buf), "%s/%s/%s",
		dir->mirrors[0], rpm->subdir, rpm->filename);
	else
	    snprintf(buf, sizeof(buf), "%s/%s/%s",
		dir->ftp, rpm->subdir, rpm->filename);
    } else {
	if (dir->mirrors[0] != NULL)
	    snprintf(buf, sizeof(buf), "%s/%s",
		dir->mirrors[0], rpm->filename);
	else
	    snprintf(buf, sizeof(buf), "%s/%s",
		dir->ftp, rpm->filename);
    }
    desc = rdfAddDescription(rdf, NULL, buf);

    rdfSetValue(desc, "Name", rpmNs, rpm->name);
    rdfSetValue(desc, "Version", rpmNs, rpm->version);
    rdfSetValue(desc, "Release", rpmNs, rpm->release);
    if (rpm->url)
        rdfSetValue(desc, "URL", rpmNs, rpm->url);
    if (rpm->arch)
        rdfSetValue(desc, "Arch", rpmNs, rpm->arch);
    if (rpm->os)
        rdfSetValue(desc, "Os", rpmNs, rpm->os);
    if (rpm->distribution)
        rdfSetValue(desc, "Distribution", rpmNs, rpm->distribution);
    if (rpm->vendor)
        rdfSetValue(desc, "Vendor", rpmNs, rpm->vendor);
    if (rpm->extra->packager)
        rdfSetValue(desc, "Packager", rpmNs, rpm->extra->packager);
    if (rpm->group)
        rdfSetValue(desc, "Group", rpmNs, rpm->group);
    if (rpm->summary)
	rdfSetValue(desc, "Summary", rpmNs, rpm->summary);
    if (rpm->extra->description)
	rdfSetValue(desc, "Description", rpmNs, rpm->extra->description);
    if (rpm->extra->copyright)
	rdfSetValue(desc, "Copyright", rpmNs, rpm->extra->copyright);
    if (rpm->extra->changelog)
        rdfSetValue(desc, "Changelog", rpmNs, rpm->extra->changelog);
    if (rpm->extra->srcrpm) {
	rdfSetValue(desc, "Sources", rpmNs, rpm->extra->srcrpm);
	if (dir->ftpsrc) {
	    rdfSetValue(desc, "SourcesFtp", rpmNs, dir->ftpsrc);
	}
    }
    tstruct = localtime(&(rpm->date));
#ifdef HAVE_STRFTIME
    strftime(buf, sizeof(buf) - 1, "%c", tstruct);
#else
#error "no strftime, please check !"
#endif
    rdfSetValue(desc, "BuildDate", rpmNs, buf);
    snprintf(buf, sizeof(buf), "%d", (int) rpm->date);
    rdfSetValue(desc, "Date", rpmNs, buf);
    snprintf(buf, sizeof(buf), "%d", rpm->size);
    rdfSetValue(desc, "Size", rpmNs, buf);
    if (rpm->extra->host)
        rdfSetValue(desc, "BuildHost", rpmNs, rpm->extra->host);
    if (rpm->extra->nb_resources > 0) {
	bag = rdfBagCreate(rdf, desc, "Provides", rpmNs);
        for (i = 0;i < rpm->extra->nb_resources;i++) {
	    elem = rdfBagAddValue(bag, "Resource", rpmNs,
	                   rpm->extra->resources[i]->name, NULL);
	    if (rpm->extra->resources[i]->name[0] != '/') {
		snprintf(buf, sizeof(buf) - 1, "%sresources/%s.rdf",
		         dotdot, rpm->extra->resources[i]->name);
		xmlSetProp(elem, "href", buf);
	    }
        }
    }
    if (rpm->extra->nb_requires > 0) {
	bag = rdfBagCreate(rdf, desc, "Requires", rpmNs);
        for (i = 0;i < rpm->extra->nb_requires;i++) {
	    elem = rdfBagAddValue(bag, "Resource", rpmNs,
	                   rpm->extra->requires[i]->name, NULL);
	    if (rpm->extra->requires[i]->name[0] != '/') {
		snprintf(buf, sizeof(buf) - 1, "%sresources/%s.rdf",
		         dotdot, rpm->extra->requires[i]->name);
		xmlSetProp(elem, "href", buf);
	    }
	    if (rpm->extra->requires[i]->version != NULL) {
		switch (rpm->extra->requires[i]->flag) {
		    case RPM2HTML_REQ_LT:
			xmlSetProp(elem, "lt",
				   rpm->extra->requires[i]->version);
			break;
		    case RPM2HTML_REQ_LEQ:
			xmlSetProp(elem, "leq",
				   rpm->extra->requires[i]->version);
			break;
		    case RPM2HTML_REQ_GT:
			xmlSetProp(elem, "gt",
				   rpm->extra->requires[i]->version);
			break;
		    case RPM2HTML_REQ_GEQ:
			xmlSetProp(elem, "geq",
				   rpm->extra->requires[i]->version);
			break;
		    case RPM2HTML_REQ_EQU:
			xmlSetProp(elem, "equ",
				   rpm->extra->requires[i]->version);
			break;
		    case RPM2HTML_REQ_NONE:
			break;
		}
	    }
        }
    }

    if (rpm->extra->filelist)
        rdfSetValue(desc, "Files", rpmNs, rpm->extra->filelist);

    /*
     * Save the RDF to a buffer, remove the tree.
     */
    rdfWriteMemory(rdf, &buffer, &size);
    rdfDestroySchema(rdf);
    if (buffer == NULL) return;

    /*
     * if the file already exists avoid to overwrite it if the content
     * didn't change.
     */
    if ((stat_res == 0) && (stat_buf.st_size == size)) {
        char *buffer2 = xmlMalloc(size * sizeof(char));

        if (buffer2 == NULL) {
	    fprintf(stderr, " : running out of memory\n");
	    xmlFree(buffer);
	    return;
	}
	output = fopen(file, "r");
	if (output == NULL) {
	    fprintf(stderr, "couldn't open %s for reading !\n", file);
	    xmlFree(buffer2);
	    xmlFree(buffer);
	    return;
	}
	if (fread(buffer2, size, 1, output) != 1) {
	    fprintf(stderr, "Problem reading from %s !\n", file);
	} else if (!memcmp(buffer, buffer2, size)) {
	    /* Only compare bytes after a successful read. */
	    fclose(output);
	    xmlFree(buffer2);
	    xmlFree(buffer);
	    if (rpm2htmlVerbose > 1)
		fprintf(stderr, "File %s : Content identical !\n", file);
	    return;
	}
	fclose(output);
	xmlFree(buffer2);
    }

    /*
     * Dump the file.
     */
    if (rpm2htmlVerbose > 1) {
        printf("Dumping %s\n", file);
    }
    output = fopen(file, "w");
    if (output == NULL) {
        fprintf(stderr, "couldn't open %s for writing !\n", file);
	xmlFree(buffer);
	return;
    }
    if (fwrite(buffer, size, 1, output) != 1) {
        fprintf(stderr, "Problem writing to %s !\n", file);
    }
    if (fclose(output) != 0) {
        fprintf(stderr, "Problem closing %s after writing !\n", file);
    }
    xmlFree(buffer);
}

/*
 * Create and RDF file containing information about a resource.
 */

void dumpResRdf(rpmRessPtr res) {
    FILE *output;
    struct stat stat_buf;
    char *base;
    int i;
    char buf[10000];
    char *file;
    size_t base_len, name_len, file_size, j;
    char *buffer;
    int size;
    int stat_res;
    rpmDataPtr rpm;
    rpmDirPtr dir;
    rdfSchema rdf;
    rdfNamespace rpmNs;
    rdfDescription desc;
    static int initialized = 0;

    if (!rpm2html_dump_rdf_resources) return;

    if (rpm2html_rdf_resources_dir != NULL) base = rpm2html_rdf_resources_dir;
    else return;

    if (res->nb_provider == 0) return;

    /* Preserve the complete resource name and the ".rdf" suffix. */
    base_len = strlen(base);
    name_len = strlen(res->name);
    if (base_len > SIZE_MAX - 6 || name_len > SIZE_MAX - base_len - 6) {
        fprintf(stderr, "RDF resource path is too long\n");
        return;
    }
    file_size = base_len + name_len + 6; /* slash, ".rdf", NUL */
    file = xmlMalloc(file_size);
    if (file == NULL) {
        fprintf(stderr, " : running out of memory\n");
        return;
    }
    memcpy(file, base, base_len);
    file[base_len] = '/';
    memcpy(file + base_len + 1, res->name, name_len);
    memcpy(file + base_len + 1 + name_len, ".rdf", sizeof(".rdf"));

    if (res->name[0] == '/') {
        /* Preserve the original conversion of leading-slash resources. */
        for (j = 0; j < name_len; j++) {
            if (file[base_len + 1 + j] == '/')
                file[base_len + 1 + j] = '_';
        }
        if (rpm2htmlVerbose > 1) {
            fputs("warning: '/' in resource name converted into '_': ", stdout);
            fwrite(file + base_len + 1, 1, name_len, stdout);
            fputc('\n', stdout);
        }
    }

    /*
     * Create the directory if needed
     */
    if (!initialized) {
	snprintf(buf, sizeof(buf), "%s", base);
	createDirectory(buf);
        initialized = 1;
    }

    /*
     * build the RDF document tree
     * Note : the order is not important but following the rpmData
     *        structure order is easier to check.
     */
    rdf = rdfNewSchema();
    rpmNs = rdfNewNamespace(rdf, "http://www.rpm.org/", "RPM");
    for (i = 0;i < res->nb_provider;i++) {
        rpm = res->provider[i];
	dir = rpm->dir;
	if ((rpm->subdir != NULL) && (rpm->subdir[0] != '\0')) {
	    if (dir->mirrors[0] != NULL)
		snprintf(buf, sizeof(buf), "%s/%s/%s",
		    dir->mirrors[0], rpm->subdir, rpm->filename);
	    else
		snprintf(buf, sizeof(buf), "%s/%s/%s",
		    dir->ftp, rpm->subdir, rpm->filename);
	} else {
	    if (dir->mirrors[0] != NULL)
		snprintf(buf, sizeof(buf), "%s/%s",
		    dir->mirrors[0], rpm->filename);
	    else
		snprintf(buf, sizeof(buf), "%s/%s",
		    dir->ftp, rpm->filename);
	}
	desc = rdfAddDescription(rdf, NULL, buf);
	if ((dir->subdir != NULL) && (dir->subdir[0] != '\0')) {
	    if ((rpm->subdir != NULL) && (rpm->subdir[0] != '\0'))
		snprintf(buf, sizeof(buf), "../%s/%s/%s.rdf", dir->subdir,
			rpm->subdir, rpmName(rpm));
	    else
		snprintf(buf, sizeof(buf), "../%s/%s.rdf", dir->subdir, rpmName(rpm));
	} else {
	    if ((rpm->subdir != NULL) && (rpm->subdir[0] != '\0'))
		snprintf(buf, sizeof(buf), "../%s/%s.rdf", rpm->subdir, rpmName(rpm));
	    else
		snprintf(buf, sizeof(buf), "../%s.rdf", rpmName(rpm));
	}
	xmlSetProp(desc, "href", buf);
	rdfSetValue(desc, "Name", rpmNs, rpm->name);
	rdfSetValue(desc, "Version", rpmNs, rpm->version);
	rdfSetValue(desc, "Release", rpmNs, rpm->release);
	if (rpm->arch)
	    rdfSetValue(desc, "Arch", rpmNs, rpm->arch);
	if (rpm->os)
	    rdfSetValue(desc, "Os", rpmNs, rpm->os);
	if (rpm->distribution)
	    rdfSetValue(desc, "Distribution", rpmNs, rpm->distribution);
	if (rpm->vendor)
	    rdfSetValue(desc, "Vendor", rpmNs, rpm->vendor);
	snprintf(buf, sizeof(buf), "%d", (int) rpm->date);
	rdfSetValue(desc, "Date", rpmNs, buf);
	snprintf(buf, sizeof(buf), "%d", (int) rpm->size);
	rdfSetValue(desc, "Size", rpmNs, buf);
	if ((rpm->subdir != NULL) && (rpm->subdir[0] != '\0')) {
	    if ((dir->subdir != NULL) && (dir->subdir[0] != '\0'))
		snprintf(buf, sizeof(buf), "%s/%s",
		    dir->subdir, rpm->subdir);
	    else
		snprintf(buf, sizeof(buf), "%s",
		    rpm->subdir);
	} else {
	    if ((dir->subdir != NULL) && (dir->subdir[0] != '\0'))
		snprintf(buf, sizeof(buf), "%s",
		    dir->subdir);
	    else
		buf[0] = '\0';
	}
	if (buf[0] != '\0') 
	    rdfSetValue(desc, "Subdir", rpmNs, buf);
    }

    /*
     * Save the RDF to a buffer, remove the tree.
     */
    rdfWriteMemory(rdf, &buffer, &size);
    rdfDestroySchema(rdf);
    if (buffer == NULL) {
        xmlFree(file);
        return;
    }

    /*
     * if the file already exists avoid to overwrite it if the content
     * didn't change.
     */
    stat_res = stat(file, &stat_buf);

    if ((stat_res == 0) && (stat_buf.st_size == size)) {
        char *buffer2 = xmlMalloc(size * sizeof(char));

        if (buffer2 == NULL) {
	    fprintf(stderr, " : running out of memory\n");
	    xmlFree(buffer);
	    xmlFree(file);
	    return;
	}
	output = fopen(file, "r");
	if (output == NULL) {
	    fprintf(stderr, "couldn't open %s for reading !\n", file);
	    xmlFree(buffer);
	    xmlFree(buffer2);
	    xmlFree(file);
	    return;
	}
	if (fread(buffer2, size, 1, output) != 1) {
	    fprintf(stderr, "Problem reading from %s !\n", file);
	} else if (!memcmp(buffer, buffer2, size)) {
	    /* Compare only bytes that were successfully read. */
	    fclose(output);
	    xmlFree(buffer2);
	    xmlFree(buffer);
	    if (rpm2htmlVerbose > 1)
		fprintf(stderr, "File %s : Content identical !\n", file);
	    xmlFree(file);
	    return;
	}
	fclose(output);
	xmlFree(buffer2);
    }

    /*
     * Dump the file.
     */
    if (rpm2htmlVerbose > 1) {
        printf("Dumping %s\n", file);
    }
    output = fopen(file, "w");
    if (output == NULL) {
        fprintf(stderr, "couldn't open %s for writing !\n", file);
	xmlFree(buffer);
	xmlFree(file);
	return;
    }
    if (fwrite(buffer, size, 1, output) != 1) {
        fprintf(stderr, "Problem writing to %s !\n", file);
    }
    if (fclose(output) != 0) {
        fprintf(stderr, "Problem closing %s after writing !\n", file);
    }
    xmlFree(buffer);
    xmlFree(file);
}

/*
 * Dump all the RDf files about known resources.
 */

void dumpAllResRdf(void) {
    rpmRessPtr cur;

    if (!rpm2html_dump_rdf_resources) return;

    if (rpm2htmlVerbose)
      printf("Dumping resource RDF\n");

    cur = ressList;

    while (cur != NULL) {
        dumpResRdf(cur);
	cur = cur->next;
    }
}

/*
 * Dump an apropos RDF file for all packages.
 * Since the file is really too big to be kept as-if, it is compressed
 */

void dumpAproposRdf(void) {
    rpmDataPtr rpm;
    char file[1000];
    char buf[1000];
    rdfSchema rdf;
    rdfNamespace rpmNs;
    rdfDescription desc;
    rpmDirPtr dir;
    char *base;
    char *buffer;
    int size;
    gzFile output;
    int res;

    if (!rpm2html_dump_rdf_resources) return;

    if (rpm2html_rdf_resources_dir != NULL) base = rpm2html_rdf_resources_dir;
    else return;

    if (rpm2htmlVerbose)
      printf("Dumping apropos RDF\n");

    snprintf(file, sizeof(file), "%s/fullIndex.rdf.gz", base);
    if (rpm2htmlVerbose > 1) {
        printf("Dumping %s\n", file);
    }

    /*
     * build the RDF document tree, one only dump the minimum needed
     * for searching.
     */
    rdf = rdfNewSchema();
    rpmNs = rdfNewNamespace(rdf, "http://www.rpm.org/", "RPM");

    rpm = rpmSoftwareList;

    while (rpm != NULL) {
	dir = rpm->dir;
	if ((rpm->subdir != NULL) && (rpm->subdir[0] != '\0')) {
	    if (dir->mirrors[0] != NULL)
		snprintf(buf, sizeof(buf), "%s/%s/%s",
		    dir->mirrors[0], rpm->subdir, rpm->filename);
	    else
		snprintf(buf, sizeof(buf), "%s/%s/%s",
		    dir->ftp, rpm->subdir, rpm->filename);
	} else {
	    if (dir->mirrors[0] != NULL)
		snprintf(buf, sizeof(buf), "%s/%s",
		    dir->mirrors[0], rpm->filename);
	    else
		snprintf(buf, sizeof(buf), "%s/%s",
		    dir->ftp, rpm->filename);
	}
	desc = rdfAddDescription(rdf, NULL, buf);
	rdfSetValue(desc, "Name", rpmNs, rpm->name);
	rdfSetValue(desc, "Summary", rpmNs, rpm->summary);
	rpm = rpm->nextSoft;
    }

    /*
     * Dump the RDF tree, and cleanup.
     */
    rdfWriteMemory(rdf, &buffer, &size);
    rdfDestroySchema(rdf);
    if (buffer == NULL) return;

    /*
     * Write the compressed version of the RDF base.
     */
    output = gzopen(file, "w9");
    if (output == NULL) {
        fprintf(stderr, "Could not write %s : gzopen failed\n", file);
	xmlFree(buffer);
	return;
    }
    res = gzwrite(output, buffer, size);
    if (res <= 0) {
        fprintf(stderr, "Could not write %s : gzwrite failed\n", file);
	xmlFree(buffer);
	return;
    }
    gzclose(output);
    xmlFree(buffer);
}

/*
 * Dump the distribs/xxx.rdf file giving information about a
 * specific distribution
 */

void dumpDistRdf(rpmDirPtr dir) {
    char buf[101] = "";
    char file[1000];
    rdfSchema rdf;
    rdfNamespace rpmNs;
    rdfDescription desc;
    rdfBag mirrors;
    rdfElement mirror;
    struct stat stat_buf;
    int stat_res;
    char *base, *ptr;
    char *buffer;
    int size;
    FILE *output;
    int i;

    if (!rpm2html_dump_rdf_resources) return;

    if (rpm2html_rdf_resources_dir != NULL) base = rpm2html_rdf_resources_dir;
    else return;

    if (dir->subdir)
	snprintf(buf, sizeof(buf), "%s", dir->subdir);
    for (ptr = &buf[0]; *ptr; ptr++) if (*ptr == '/') *ptr = '_';
    snprintf(file, sizeof(file), "%s/distribs", base);
    createDirectory(file);
    snprintf(file, sizeof(file), "%s/distribs/%s.rdf", base, buf);

    /*
     * build the RDF document tree, one only dump the minimum needed
     * for searching distributions information.
     */
    rdf = rdfNewSchema();
    rpmNs = rdfNewNamespace(rdf, "http://www.rpm.org/", "RPM");

    desc = rdfAddDescription(rdf, NULL, dir->ftp);
    rdfSetValue(desc, "ID", rpmNs, dir->subdir);
    rdfSetValue(desc, "Name", rpmNs, dir->name);
    rdfSetValue(desc, "Origin", rpmNs, dir->ftp);
    rdfSetValue(desc, "Sources", rpmNs, dir->ftpsrc);
    snprintf(buf, sizeof(buf), "%d", dir->files);
    mirrors = rdfBagCreate(rdf, desc, "Mirrors", rpmNs);
    for (i = 0;i < dir->nb_mirrors;i++) {
	mirror = rdfBagAddValue(mirrors, "Mirror", rpmNs, NULL, NULL);
	rdfSetElementResource(rdf, mirror, dir->mirrors[i]);
    }

    /*
     * Dump the RDF tree, and cleanup.
     */
    rdfWriteMemory(rdf, &buffer, &size);
    rdfDestroySchema(rdf);
    if (buffer == NULL) return;

    /*
     * if the file already exists avoid to overwrite it if the content
     * didn't change.
     */
    stat_res = stat(file, &stat_buf);

    if ((stat_res == 0) && (stat_buf.st_size == size)) {
        char *buffer2 = xmlMalloc(size * sizeof(char));

        if (buffer2 == NULL) {
	    fprintf(stderr, " : running out of memory\n");
	    xmlFree(buffer);
	    return;
	}
	output = fopen(file, "r");
	if (output == NULL) {
	    fprintf(stderr, "couldn't open %s for reading !\n", file);
	    xmlFree(buffer2);
	    xmlFree(buffer);
	    return;
	}
	if (fread(buffer2, size, 1, output) != 1) {
	    fprintf(stderr, "Problem reading from %s !\n", file);
	}
	fclose(output);

	/*
	 * Now compare the content !
	 */
	if (!memcmp(buffer, buffer2, size)) {
	    xmlFree(buffer2);
	    xmlFree(buffer);
	    if (rpm2htmlVerbose > 1)
		fprintf(stderr, "File %s : Content identical !\n", file);
	    return;
	}
	xmlFree(buffer2);
    }

    /*
     * Dump the file.
     */
    if (rpm2htmlVerbose > 1) {
        printf("Dumping %s\n", file);
    }
    output = fopen(file, "w");
    if (output == NULL) {
        fprintf(stderr, "couldn't open %s for writing !\n", file);
	xmlFree(buffer);
	return;
    }
    if (fwrite(buffer, size, 1, output) != 1) {
        fprintf(stderr, "Problem writing to %s !\n", file);
	xmlFree(buffer);
    }
    fclose(output);
    xmlFree(buffer);
}

/*
 * Dump the distribs/metadata.rdf file listing all the recognized
 * metadata mirrors sites.
 */

void dumpMetadataListRdf(void) {
    char file[1000];
    rdfSchema rdf;
    rdfNamespace rpmNs;
    rdfDescription desc;
    struct stat stat_buf;
    int stat_res;
    char *base;
    char *buffer;
    int size;
    FILE *output;
    int i;

    if (!rpm2html_dump_rdf_resources) return;

    if (rpm2html_rdf_resources_dir != NULL) base = rpm2html_rdf_resources_dir;
    else return;

    snprintf(file, sizeof(file), "%s/distribs", base);
    createDirectory(file);
    snprintf(file, sizeof(file), "%s/distribs/metadata.rdf", base);

    /*
     * build the RDF document tree, one only dump the minimum needed
     * for searching distributions information.
     */
    rdf = rdfNewSchema();
    rpmNs = rdfNewNamespace(rdf, "http://www.rpm.org/", "RPM");

    for (i = 0;i < nb_metadata_mirrors;i++){
        desc = rdfAddDescription(rdf, NULL, metadata_mirrors[i]);
	rdfSetValue(desc, "URI", rpmNs, metadata_mirrors[i]);
    }

    /*
     * Dump the RDF tree, and cleanup.
     */
    rdfWriteMemory(rdf, &buffer, &size);
    rdfDestroySchema(rdf);
    if (buffer == NULL) return;

    /*
     * if the file already exists avoid to overwrite it if the content
     * didn't change.
     */
    stat_res = stat(file, &stat_buf);

    if ((stat_res == 0) && (stat_buf.st_size == size)) {
        char *buffer2 = xmlMalloc(size * sizeof(char));

        if (buffer2 == NULL) {
	    fprintf(stderr, " : running out of memory\n");
	    xmlFree(buffer);
	    return;
	}
	output = fopen(file, "r");
	if (output == NULL) {
	    fprintf(stderr, "couldn't open %s for reading !\n", file);
	    xmlFree(buffer);
	    xmlFree(buffer2);
	    return;
	}
	if (fread(buffer2, size, 1, output) != 1) {
	    fprintf(stderr, "Problem reading from %s !\n", file);
	}
	fclose(output);

	/*
	 * Now compare the content !
	 */
	if (!memcmp(buffer, buffer2, size)) {
	    xmlFree(buffer2);
	    xmlFree(buffer);
	    if (rpm2htmlVerbose > 1)
		fprintf(stderr, "File %s : Content identical !\n", file);
	    return;
	}
	xmlFree(buffer2);
    }

    /*
     * Dump the file.
     */
    if (rpm2htmlVerbose > 1) {
        printf("Dumping %s\n", file);
    }
    output = fopen(file, "w");
    if (output == NULL) {
        fprintf(stderr, "couldn't open %s for writing !\n", file);
	return;
    }
    if (fwrite(buffer, size, 1, output) != 1) {
        fprintf(stderr, "Problem writing to %s !\n", file);
    }
    fclose(output);
    xmlFree(buffer);
}

/*
 * Dump the distribs/list.rdf file listing all the recognized distributions
 * as well as the metadata mirrors list.
 */

void dumpDistListRdf(void) {
    rpmDirPtr dir;
    char file[1000];
    rdfSchema rdf;
    rdfNamespace rpmNs;
    rdfDescription desc;
    struct stat stat_buf;
    int stat_res;
    char *base;
    char *buffer;
    int size;
    FILE *output;

    if (!rpm2html_dump_rdf_resources) return;

    if (rpm2html_rdf_resources_dir != NULL) base = rpm2html_rdf_resources_dir;
    else return;

    if (rpm2htmlVerbose)
      printf("Dumping distribution RDF\n");

    dumpMetadataListRdf();

    snprintf(file, sizeof(file), "%s/distribs", base);
    createDirectory(file);
    snprintf(file, sizeof(file), "%s/distribs/list.rdf", base);

    /*
     * build the RDF document tree, one only dump the minimum needed
     * for searching distributions information.
     */
    rdf = rdfNewSchema();
    rpmNs = rdfNewNamespace(rdf, "http://www.rpm.org/", "RPM");

    dir = dirList;
    while (dir != NULL) {
        desc = rdfAddDescription(rdf, NULL, dir->ftp);
	rdfSetValue(desc, "ID", rpmNs, dir->subdir);
	dumpDistRdf(dir);
        dir = dir->next;
    }
    /*
     * Dump the RDF tree, and cleanup.
     */
    rdfWriteMemory(rdf, &buffer, &size);
    rdfDestroySchema(rdf);
    if (buffer == NULL) return;

    /*
     * if the file already exists avoid to overwrite it if the content
     * didn't change.
     */
    stat_res = stat(file, &stat_buf);

    if ((stat_res == 0) && (stat_buf.st_size == size)) {
        char *buffer2 = xmlMalloc(size * sizeof(char));

        if (buffer2 == NULL) {
	    fprintf(stderr, " : running out of memory\n");
	    xmlFree(buffer);
	    return;
	}
	output = fopen(file, "r");
	if (output == NULL) {
	    fprintf(stderr, "couldn't open %s for reading !\n", file);
	    xmlFree(buffer);
	    xmlFree(buffer2);
	    return;
	}
	if (fread(buffer2, size, 1, output) != 1) {
	    fprintf(stderr, "Problem reading from %s !\n", file);
	}
	fclose(output);

	/*
	 * Now compare the content !
	 */
	if (!memcmp(buffer, buffer2, size)) {
	    xmlFree(buffer2);
	    xmlFree(buffer);
	    if (rpm2htmlVerbose > 1)
		fprintf(stderr, "File %s : Content identical !\n", file);
	    return;
	}
	xmlFree(buffer2);
    }

    /*
     * Dump the file.
     */
    if (rpm2htmlVerbose > 1) {
        printf("Dumping %s\n", file);
    }
    output = fopen(file, "w");
    if (output == NULL) {
        fprintf(stderr, "couldn't open %s for writing !\n", file);
	return;
    }
    if (fwrite(buffer, size, 1, output) != 1) {
        fprintf(stderr, "Problem writing to %s !\n", file);
    }
    fclose(output);
    xmlFree(buffer);
}

