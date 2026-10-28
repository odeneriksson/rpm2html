/*
 * rpm2html.c : Application to generate an HTML view of an ensemble
 *              of RPM packages.
 *
 * See Copyright for the status of this software.
 *
 * $Id: rpm2html.c,v 1.49 2007/04/01 13:37:34 hany Exp $
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
#include <time.h>

#include <ctype.h>
#include <errno.h>
#include <rpm/rpmlib.h>

#include "rpm2html.h"
#include "rpmdata.h"
#include "html.h"
#include "rdf.h"
#include <libxml/tree.h>
#include "language.h"
#ifdef WITH_SQL
#include "sql.h"
#endif

void usage(const char *argv0) {
    fprintf(stderr, "%s %s : Web page generator for RPM packages\n",
	    RPM2HTML_NAME, RPM2HTML_VER);
    fprintf(stderr, 
 "usage : %s [-q/-v] [--nosql] [-force] [-lang msgfile] configfile1 [configfile2 ...]\n",
            argv0);
    fprintf(stderr, "   or   %s [-dumplang msgfile]\n", argv0);
    exit(1);
}

int main(int argc, char *argv[]) {
    rpmDataPtr list = NULL;
    int i;
    time_t start_time;
    int nb_config_files = 0;
    char host[200];
    int mask;
    const char *dist = NULL;
    const char *dir = NULL;

    start_time = time(NULL);
    gethostname(host, sizeof(host));
    currentTime = time(NULL);
    rpm2html_rpm2html_thishost = &host[0];

    xmlKeepBlanksDefault(0);

    rpmReadConfigFiles(NULL, NULL);

    /*
     * Ensure that all files and directories created a world readable !
     */
    mask = umask(022);

    for (i = 1; i < argc ; i++) {
        if (argv[i][0] == '-') {
	    char *ptr = argv[i];
	    while (*ptr == '-') ptr++;
	    if (!strcmp(ptr, "lang")) {
		i++;
		if (i >= argc) break;
		readLanguageFile(argv[i]);
	    } else if (!strcmp(ptr, "dumplang")) {
		i++;
		if (i >= argc) break;
		writeLanguageFile(ptr);
		return(0);
	    } else if (!strcmp(ptr, "nosql")) {
		rpm2html_no_sql++;
	    } else if (!strcmp(ptr, "force")) {
		force++;
	    } else if (!strcmp(ptr, "cleanup")) {
#ifdef WITH_SQL
		if (init_sql(NULL, "rpmfind", NULL, NULL) < 0) {
		    exit(1);
		}
		sql_check_packages();
#endif
		exit(0);
	    } else if (!strcmp(ptr, "dir")) {
		i++;
		if (i >= argc) break;
		dir = argv[i];
	    } else if (!strcmp(ptr, "dist")) {
		i++;
		if (i >= argc) break;
		dist = argv[i];
	    } else if (!strcmp(ptr, "v")) {
		rpm2htmlVerbose++;
	    } else if (!strcmp(ptr, "q")) {
		rpm2htmlVerbose = 0;
	    } else {
	        fprintf(stderr, "Unknown option: %s\n", argv[i]);
		usage(argv[0]);
	    }
	} else {
#ifdef WITH_SQL
	    if (init_sql(NULL, "rpmfind", NULL, NULL) < 0) {
		exit(1);
	    }
#else
	    rpm2html_no_sql = 1;
#endif
	    if (readConfigFile(argv[i]) >= 0) {
	        if (rpm2html_dump_html != 0) {
	            dumpDirIcon();
#ifdef HAVE_LIBTEMPLATE
	            initTemplateEngine(rpm2html_html_template);
#endif
                }

		/*
		 * A config file has been found and parsed.
		 * Scan the directories, dump the results
		 * and reinitialize.
		 */
	        nb_config_files++;
		if ((dist == NULL) && (dir == NULL)) {
		    /*
		     * Full reindex and regeneration of the cross-index
		     * pages.
		     */
		    list = rpmDirScanAll();
		    if (rpm2html_dump_html != 0) {
			if (rpm2html_files != 0) {
			    dumpAllRessHtml(0);
			    rpmNameSort(&list, 0);
			    dumpRpmByName(rpmSoftwareList, 0);
			    rpmDistribSort(&list, 0);
			    dumpRpmByDistribs(list, 0);
			    rpmGroupSort(&list, 0);
			    dumpRpmByGroups(rpmSoftwareList, 0);
			    rpmVendorSort(&list, 0);
			    dumpRpmByVendors(list, 0);
			    rpmDateSort(&list, 0);
			    dumpRpmByDate(list, 0);
			    dumpIndex(start_time, 0);
			}
			if (rpm2html_install_files != 0) {
			    dumpAllRessHtml(1);
			    rpmNameSort(&list, 1);
			    dumpRpmByName(rpmSoftwareList, 1);
			    rpmDistribSort(&list, 1);
			    dumpRpmByDistribs(list, 1);
			    rpmGroupSort(&list, 1);
			    dumpRpmByGroups(rpmSoftwareList, 1);
			    rpmVendorSort(&list, 1);
			    dumpRpmByVendors(list, 1);
			    rpmDateSort(&list, 1);
			    dumpRpmByDate(list, 1);
			    dumpIndex(start_time, 1);
			}
		    }
		    if (rpm2html_no_sql) {
			if (rpm2html_dump_rdf_resources) {
			    dumpAllResRdf();
			    dumpAproposRdf();
			    dumpDistListRdf();
			    rpmDirCleanupAll();
			}
		    }
#ifdef WITH_SQL
		    else
			sql_check_packages();
#endif
		} else if (dir != NULL) {
		    printf("rpm2html: indexing %s\n", dir);
		    rpmDirScanOneDir(dir);
		} else {
		    rpmDirScanOneDist(dist);
		}
		if (rpm2htmlVerbose) {
		   printf(
   "To see the result of the indexing point your Web browser to:\n");	      
		   if (rpm2html_rpm2html_thishost == NULL)
		       printf("\thttp://localhost%s/\n", rpm2html_url);
		   else    
		       printf("\thttp://%s%s/\n", rpm2html_rpm2html_thishost,
		              rpm2html_url);
		}
                reinitialize();
            }
	}
    }

    /*
     * reset the umask
     */
    umask(mask);

#ifdef WITH_SQL
    if ((!rpm2html_no_sql) && (nb_config_files == 0)) {
	if (readConfigSql() >= 0) {
	    if (rpm2html_dump_html != 0) {
	        dumpDirIcon();
#ifdef HAVE_LIBTEMPLATE
	        initTemplateEngine(rpm2html_html_template);
#endif
            }

	    /*
	     * A config file has been found and parsed.
	     * Scan the directories, dump the results
	     * and reinitialize.
	     */
	    nb_config_files++;
	    if (dist == NULL) {
		list = rpmDirScanAll();
		if (rpm2html_dump_html != 0) {
		    if (rpm2html_files != 0) {
			dumpAllRessHtml(0);
			rpmNameSort(&list, 0);
			dumpRpmByName(rpmSoftwareList, 0);   /* Modif. by A. Gibert */
			rpmDistribSort(&list, 0);
			dumpRpmByDistribs(list, 0);
			rpmGroupSort(&list, 0);
			dumpRpmByGroups(rpmSoftwareList, 0);   /* Modif. by A. Gibert */
			rpmVendorSort(&list, 0);
			dumpRpmByVendors(list, 0);
			rpmDateSort(&list, 0);
			dumpRpmByDate(list, 0);
			dumpIndex(start_time, 0);
		    }
		    if (rpm2html_install_files != 0) {
			dumpAllRessHtml(1);
			rpmNameSort(&list, 1);
			dumpRpmByName(rpmSoftwareList, 1);   /* Modif. by A. Gibert */
			rpmDistribSort(&list, 1);
			dumpRpmByDistribs(list, 1);
			rpmGroupSort(&list, 1);
			dumpRpmByGroups(rpmSoftwareList, 1);   /* Modif. by A. Gibert */
			rpmVendorSort(&list, 1);
			dumpRpmByVendors(list, 1);
			rpmDateSort(&list, 1);
			dumpRpmByDate(list, 1);
			dumpIndex(start_time, 1);
		    }
		}
		sql_check_packages();
	    } else {
		rpmDirScanOneDist(dist);
	    }
	    if (rpm2htmlVerbose) {
	       printf(
"To see the result of the indexing point your Web browser to:\n");	      
	       if (rpm2html_rpm2html_thishost == NULL)
		   printf("\thttp://localhost%s/\n", rpm2html_url);
	       else    
		   printf("\thttp://%s%s/\n", rpm2html_rpm2html_thishost,
			  rpm2html_url);
	    }
	    reinitialize();
	}
    }
#endif
    if ((rpm2html_no_sql) && (nb_config_files == 0))
	usage(argv[0]);
    xmlMemoryDump();

#ifdef WITH_SQL
    if (!rpm2html_no_sql) {
	close_sql();
	sqlListsCleanup();
    }
#endif
    return(0);
}

