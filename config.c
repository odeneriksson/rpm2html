/*
 * config.c : handle the configuration file.
 *
 * See Copyright for the status of this software.
 *
 * $Id: config.c,v 1.52 2010/11/09 22:16:29 hany Exp $
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
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "rpm2html.h"
#include "rpmdata.h"
#ifdef WITH_SQL
#include "sql.h"
#endif

/*
 * configuration variables for rpm2html
 */

char *rpm2html_rpm2html_name	= RPM2HTML_NAME;
char *rpm2html_rpm2html_ver	= RPM2HTML_VER;
char *rpm2html_rpm2html_url	= RPM2HTML_URL;
char *rpm2html_rpm2html_thishost= NULL;
char *rpm2html_maint		= RPM2HTML_MAINT;
char *rpm2html_mail		= RPM2HTML_MAIL;
char *rpm2html_help		= NULL;
char *rpm2html_dir		= NULL;
char *rpm2html_name		= NULL;
char *rpm2html_url		= NULL;
#ifdef WITH_SQL
char *rpm2html_search		= "/linux/rpm2html/search.php";
#endif
char *rpm2html_ftp		= NULL;
char *rpm2html_ftpsrc		= NULL;
char *rpm2html_host		= NULL;
int   rpm2html_build_tree	= 0;
int   rpm2html_cve_linking      = 1;
#ifdef HAVE_LIBTEMPLATE
char *rpm2html_html_template	= "rpm2html.tpl";
#endif
int   rpm2html_dump_html	= 1;
int   rpm2html_dump_html_only_if_rpm_newer = 0;
int   rpm2html_dump_rdf		= 0;
int   rpm2html_dump_rdf_resources= 0;
char *rpm2html_rdf_dir		= NULL;
char *rpm2html_rdf_resources_dir= NULL;
int   rpm2html_rdf_count_limit  = DEFAULT_RDF_COUNT_LIMIT;

int  rpm2html_files = 0;
int  rpm2html_size = 0;

int  rpm2html_no_sql = 0;
int  rpm2html_install_files = 0;
int  rpm2html_install_size = 0;
int  rpm2html_protect_emails = 0;

char *rpm2html_headers_name[MAX_EXTRA_HEADERS];
char *rpm2html_headers_url[MAX_EXTRA_HEADERS];
int   rpm2html_nb_extra_headers = 0; 

int   nb_metadata_mirrors = 0;
int   max_metadata_mirrors = 0;
char **metadata_mirrors = NULL;

void addHeader(char *value);

#ifdef HAVE_STRNDUP
extern char *strndup (const char *source, size_t len);
#else /* ! HAVE_STRNDUP */
/*
 * 
 */
char *strndup (const char *source, size_t len) {
    char* tmp = NULL;

    if ((source == NULL) || (len < 0)) return(NULL);
    if (len <= strlen(source)) return xmlStrdup(source);

    tmp = xmlMalloc(len+1);
    strncpy(tmp, source, len);
    tmp[len] = '\0';

    return(tmp);
}
#endif /* HAVE_STRNDUP */

/*
 * free a directory structure.
 */

void rpmDirFree(rpmDirPtr dir) {
    int i;

    if (dir->color) xmlFree(dir->color);
    if (dir->dir != NULL) xmlFree(dir->dir);
    if (dir->ftp != NULL) xmlFree(dir->ftp);
    if (dir->ftpsrc != NULL) xmlFree(dir->ftpsrc);
    if (dir->host != NULL) xmlFree(dir->host);
    if (dir->mail != NULL) xmlFree(dir->mail);
    if (dir->maint != NULL) xmlFree(dir->maint);
    if (dir->name != NULL) xmlFree(dir->name);
    if (dir->url != NULL) xmlFree(dir->url);
    if (dir->subdir != NULL) xmlFree(dir->subdir);
    for (i = 0;i < dir->nb_mirrors ; i++)
        if (dir->mirrors[i] != NULL) xmlFree(dir->mirrors[i]);
    if (dir->mirrors != NULL) xmlFree(dir->mirrors);
    if (dir->trust != NULL) xmlFree(dir->trust);
    if (dir->rpmdir != NULL) xmlFree(dir->rpmdir);
    if (dir->dbpath != NULL) xmlFree(dir->dbpath);   /* Added by A. Gibert */
    dir->next = NULL;
    memset(dir, 0, sizeof(rpmDir));
    xmlFree(dir);
}

/*
 * free a directory list.
 */

void rpmDirListFree(rpmDirPtr *base) {
    rpmDirPtr dir = *base, next;

    while (dir != NULL) {
        next = dir->next;
	rpmDirFree(dir);
	dir = next;
    }
    *base = NULL;
}

/*
 * free an RPM structure.
 */

void rpmDataFree(rpmDataPtr rpm) {
    if (rpm->filename) xmlFree(rpm->filename);
    if (rpm->name != NULL) stringFree(rpm->name);
    if (rpm->version != NULL) stringFree(rpm->version);
    if (rpm->release != NULL) stringFree(rpm->release);
    if (rpm->url != NULL) xmlFree(rpm->url);
    if (rpm->arch != NULL) stringFree(rpm->arch);
    if (rpm->os != NULL) stringFree(rpm->os);
    if (rpm->distribution != NULL) stringFree(rpm->distribution);
    if (rpm->vendor != NULL) stringFree(rpm->vendor);
    if (rpm->group != NULL) stringFree(rpm->group);
    if (rpm->summary != NULL) xmlFree(rpm->summary);
    if (rpm->subdir != NULL) stringFree(rpm->subdir);
    rpmDataListFree(&(rpm->nextArch));
    memset(rpm, 0, sizeof(rpmData));
    xmlFree(rpm);
}

/*
 * free an RPM list.
 */

void rpmDataListFree(rpmDataPtr *base) {
    rpmDataPtr rpm = *base, next;

    while (rpm != NULL) {
        next = rpm->nextSoft;
	rpmDataFree(rpm);
	rpm = next;
    }
    *base = NULL;
}

/*
 * free a resource structure.
 */

void rpmRessFree(rpmRessPtr res) {
    if (res->name != NULL) xmlFree(res->name);
    if (res->version != NULL) xmlFree(res->version);
    if (res->provider != NULL) xmlFree(res->provider);
    memset(res, 0, sizeof(rpmRess));
    xmlFree(res);
}

/*
 * free a resource list.
 */

void rpmRessListFree(rpmRessPtr *base) {
    rpmRessPtr res = *base, next;

    while (res != NULL) {
        next = res->next;
	rpmRessFree(res);
	res = next;
    }
    *base = NULL;
}

/*
 * Search a directory in the list. If not found, create a new one.
 */

rpmDirPtr rpmDirSearch(char *dirname) {
    rpmDirPtr cur = dirList;

    while (cur != NULL) {
        if (!strcmp(dirname, cur->rpmdir)) return(cur);
	cur = cur->next;
    }
    cur = (rpmDirPtr) xmlMalloc(sizeof(rpmDir));
    if (cur == NULL) {
        fprintf(stderr, "rpmDirSearch : ran out of memory!\n");
	exit(1);
    }
    memset(cur, 0, sizeof(rpmDir));
    cur->max_mirrors = 5;
    cur->mirrors = (char **) xmlMalloc(cur->max_mirrors * sizeof(char *));
    cur->color = xmlStrdup("#ffffff");
    cur->dir = NULL;
    cur->files = 0;
    cur->html = 1;
    cur->ftp = NULL;
    cur->ftpsrc = NULL;
    cur->host = NULL;
    cur->mail = NULL;
    cur->maint = NULL;
    cur->name = NULL;
    cur->nb_mirrors = 0;
    cur->mirrors[0] = NULL;
    cur->rpmdir = xmlStrdup(dirname);
    cur->size = 0;
    cur->trust = "1.0";
    cur->trust = NULL;
    cur->url = NULL;
    cur->build_tree = rpm2html_build_tree;
    cur->follow_symlinks = 0;
    cur->rpm_symlinks = 0;

    if (strncmp(dirname, "localbase", 9))   /* Added by A. Gibert */
        cur->installbase = 0;
    else
        cur->installbase = 1;

    cur->next = dirList;
    dirList = cur;
    return(cur);
}

/*
 * addConfigEntry : an entry in the config file has just been read.
 */
void addConfigEntry(char *rpmdir, char *name, char *value) {
    rpmDirPtr cur;
    char *temp;

    if (rpm2htmlVerbose > 1)
	printf("addConfigEntry(\"%s\", \"%s\", \"%s\")\n", rpmdir, name, value);

    /*
     * case of global option for rpm2html.
     */
    if (!strcasecmp(rpmdir, RPM2HTML_NAME)) {
        if (!strcasecmp(name, "url")) {
	    rpm2html_url = xmlStrdup(value);
	} else if (!strcasecmp(name, "maint")) {
	    rpm2html_maint = xmlStrdup(value);
	} else if (!strcasecmp(name, "help")) {
	    rpm2html_help = xmlStrdup(value);
	} else if (!strcasecmp(name, "mail")) {
	    rpm2html_mail = xmlStrdup(value);
	} else if (!strcasecmp(name, "dir")) {
	    rpm2html_dir = xmlStrdup(value);
	} else if (!strcasecmp(name, "ftp")) {
	    rpm2html_ftp = xmlStrdup(value);
	} else if (!strcasecmp(name, "ftpsrc")) {
	    rpm2html_ftpsrc = xmlStrdup(value);
	} else if (!strcasecmp(name, "name")) {
	    rpm2html_name = xmlStrdup(value);
	} else if (!strcasecmp(name, "host")) {
	    rpm2html_host = xmlStrdup(value);
	} else if (!strcasecmp(name, "tree")) {
	    if ((!strcasecmp(value, "true")) ||
	        (!strcasecmp(value, "yes"))) {
	        rpm2html_build_tree = 1;
	    } else if ((!strcasecmp(value, "false")) ||
	        (!strcasecmp(value, "no"))) {
	        rpm2html_build_tree = 0;
	    } else {
		printf("Config file : %s global entry ignored,\n", name);
		printf("\tuse \"tree=true\" or \"tree=false\"\n");
	    }
	} else if (!strcasecmp(name, "rdf")) {
	    if ((!strcasecmp(value, "true")) ||
	        (!strcasecmp(value, "yes"))) {
	        rpm2html_dump_rdf = 1;
	    } else if ((!strcasecmp(value, "false")) ||
	        (!strcasecmp(value, "no"))) {
	        rpm2html_dump_rdf = 0;
	    } else {
		printf("Config file : %s global entry ignored,\n", name);
		printf("\tuse \"rdf=true\" or \"rdf=false\"\n");
	    }
	} else if (!strcasecmp(name, "rdf_dir")) {
	    rpm2html_rdf_dir = xmlStrdup(value);
	} else if (!strcasecmp(name, "rdf_resources")) {
	    if ((!strcasecmp(value, "true")) ||
	        (!strcasecmp(value, "yes"))) {
	        rpm2html_dump_rdf_resources = 1;
	    } else if ((!strcasecmp(value, "false")) ||
	        (!strcasecmp(value, "no"))) {
	        rpm2html_dump_rdf_resources = 0;
	    } else {
		printf("Config file : %s global entry ignored,\n", name);
		printf("\tuse \"rdf_resources=true\" or \"rdf_resources=false\"\n");
	    }
	} else if (!strcasecmp(name, "rdf_resources_dir")) {
	    rpm2html_rdf_resources_dir = xmlStrdup(value);
	} else if (!strcasecmp(name, "rdf_count_limit")) {
	    rpm2html_rdf_count_limit = strtol(value, &temp, 10);
	    if ((*value == '\0') || (*temp != '\0')) {
		printf("Config file : %s global entry ignored,\n", name);
		printf("\tuse \"rdf_count_limit=<number>\"\n");
		rpm2html_rdf_count_limit = DEFAULT_RDF_COUNT_LIMIT;
	    }
	} else if (!strcasecmp(name, "html")) {
	    if ((!strcasecmp(value, "true")) ||
	        (!strcasecmp(value, "yes"))) {
	        rpm2html_dump_html = 1;
	    } else if ((!strcasecmp(value, "false")) ||
	        (!strcasecmp(value, "no"))) {
	        rpm2html_dump_html = 0;
	    } else {
		printf("Config file : %s global entry ignored,\n", name);
		printf("\tuse \"html=true\" or \"html=false\"\n");
	    }
        #ifdef HAVE_LIBTEMPLATE
	} else if (!strcasecmp(name, "html_template")) {
	    rpm2html_html_template = xmlStrdup(value);
        #endif
	} else if (!strcasecmp(name, "protectemails")) {
	    if ((!strcasecmp(value, "true")) ||
	        (!strcasecmp(value, "yes"))) {
	        rpm2html_protect_emails = 1;
	    } else if ((!strcasecmp(value, "false")) ||
	        (!strcasecmp(value, "no"))) {
	        rpm2html_protect_emails = 0;
	    } else {
		printf("Config file : %s global entry ignored,\n", name);
		printf("\tuse \"protectemails=true\" or \"protectemails=false\"\n");
	    }
	} else if (!strcasecmp(name, "cve_linking")) {
	    if ((!strcasecmp(value, "true")) ||
	        (!strcasecmp(value, "yes"))) {
	        rpm2html_cve_linking = 1;
	    } else if ((!strcasecmp(value, "false")) ||
	        (!strcasecmp(value, "no"))) {
	        rpm2html_cve_linking = 0;
	    } else {
		printf("Config file : %s global entry ignored,\n", name);
		printf("\tuse \"cve_linking=true\" or \"cve_linking=false\"\n");
	    }
	} else if (!strcasecmp(name, "header")) {
	    addHeader(value);
	} else if (!strcasecmp(name, "dump_html_only_if_rpm_newer")) {
	    if ((!strcasecmp(value, "true")) ||
	        (!strcasecmp(value, "yes"))) {
	        rpm2html_dump_html_only_if_rpm_newer = 1;
	    } else if ((!strcasecmp(value, "false")) ||
	        (!strcasecmp(value, "no"))) {
	        rpm2html_dump_html_only_if_rpm_newer = 0;
	    } else {
		printf("Config file : %s global entry ignored,\n", name);
		printf("\tuse \"dump_html_only_if_rpm_newer=true\" or \"dump_html_only_if_rpm_newer=false\"\n");
	    }
        } else {
	    printf("Config file : %s global entry ignored\n", name);
	}
	return;
    }

    /*
     * Options for the metadata mirrors.
     */
    if (!strcasecmp(rpmdir, "metadata")) {
	if (!strcasecmp(name, "mirror")) {
	    /*
	     * all "mirrors" values are collected in the metadata_mirrors array.
	     */
	    if (metadata_mirrors == NULL) {
	        max_metadata_mirrors = 10;
		nb_metadata_mirrors = 0;
		metadata_mirrors = (char **)
		          xmlMalloc(max_metadata_mirrors * sizeof(char *));
		if (metadata_mirrors == NULL) {
		    fprintf(stderr, "addConfigEntry : ran out of memory!\n");
		    exit(1);
		}
	    }
	    if (nb_metadata_mirrors >= max_metadata_mirrors) {
		max_metadata_mirrors *= 2;
		metadata_mirrors = (char **) xmlRealloc(metadata_mirrors,
				       max_metadata_mirrors * sizeof(char *));
		if (metadata_mirrors == NULL) {
		    fprintf(stderr, "addConfigEntry : ran out of memory!\n");
		    exit(1);
		}
	    }
	    metadata_mirrors[nb_metadata_mirrors++] = xmlStrdup(value);
	} else {
	    printf("Config file : %s entry for [metadata] ignored\n", name);
	}
	return;
    }

    /*
     * option for a directory.
     */
    cur = rpmDirSearch(rpmdir);
    if (!strcasecmp(name, "name")) {
	cur->name = xmlStrdup(value);
    } else if (!strcasecmp(name, "dir")) {
	cur->dir = xmlStrdup(value);
    } else if (!strcasecmp(name, "subdir")) {
	cur->subdir = xmlStrdup(value);
    } else if (!strcasecmp(name, "dbpath")) {   /* Added by A. Gibert */
       cur->dbpath = xmlStrdup(value);   /* Added by A. Gibert */
    } else if (!strcasecmp(name, "url")) {
	cur->url = xmlStrdup(value);
    } else if (!strcasecmp(name, "ftp")) {
	cur->ftp = xmlStrdup(value);
    } else if (!strcasecmp(name, "ftpsrc")) {
	cur->ftpsrc = xmlStrdup(value);
    } else if (!strcasecmp(name, "color")) {
	if (cur->color != NULL) xmlFree(cur->color);
	cur->color = xmlStrdup(value);
    } else if (!strcasecmp(name, "trust")) {
	if (cur->trust != NULL) xmlFree(cur->color);
	cur->trust = xmlStrdup(value);
    } else if (!strcasecmp(name, "host")) {
	rpm2html_host = xmlStrdup(value);
    } else if (!strcasecmp(name, "rdf_dir")) {
	rpm2html_rdf_dir = xmlStrdup(value);
    } else if (!strcasecmp(name, "html")) {
	if ((!strcasecmp(value, "true")) ||
	    (!strcasecmp(value, "yes"))) {
	    cur->html = 1;
	} else if ((!strcasecmp(value, "false")) ||
	    (!strcasecmp(value, "no"))) {
	    cur->html = 0;
	} else {
	    printf("Config file : %s directory entry ignored,\n", name);
	    printf("\tuse \"html=true\" or \"html=false\"\n");
	}
    } else if (!strcasecmp(name, "tree")) {
	if ((!strcasecmp(value, "true")) ||
	    (!strcasecmp(value, "yes"))) {
	    cur->build_tree = 1;
	} else if ((!strcasecmp(value, "false")) ||
	    (!strcasecmp(value, "no"))) {
	    cur->build_tree = 0;
	} else {
	    printf("Config file : %s directory entry ignored,\n", name);
	    printf("\tuse \"tree=true\" or \"tree=false\"\n");
	}
    } else if (!strcasecmp(name, "followsymlinks")) {
	if ((!strcasecmp(value, "true")) ||
	    (!strcasecmp(value, "yes"))) {
	    cur->follow_symlinks = 1;
	} else if ((!strcasecmp(value, "false")) ||
	    (!strcasecmp(value, "no"))) {
	    cur->follow_symlinks = 0;
	} else {
	    printf("Config file : %s directory entry ignored,\n", name);
	    printf("\tuse \"tree=true\" or \"tree=false\"\n");
	}
    } else if (!strcasecmp(name, "rpmsymlinks")) {
	if ((!strcasecmp(value, "true")) ||
	    (!strcasecmp(value, "yes"))) {
	    cur->rpm_symlinks = 1;
	} else if ((!strcasecmp(value, "false")) ||
	    (!strcasecmp(value, "no"))) {
	    cur->rpm_symlinks = 0;
	} else {
	    printf("Config file : %s directory entry ignored,\n", name);
	    printf("\tuse \"tree=true\" or \"tree=false\"\n");
	}
    } else if (!strcasecmp(name, "mirror")) {
        /*
	 * all "mirrors" values are collected in the mirrors array.
	 */
	if (cur->nb_mirrors >= cur->max_mirrors) {
	    cur->max_mirrors *= 2;
	    cur->mirrors = (char **) xmlRealloc(cur->mirrors,
	                               cur->max_mirrors * sizeof(char *));
	    if (cur->mirrors == NULL) {
	        fprintf(stderr, "addConfigEntry : ran out of memory!\n");
		exit(1);
	    }
	}
	cur->mirrors[cur->nb_mirrors++] = xmlStrdup(value);
    } else {
	printf("Config file : %s entry for [%s] ignored\n", name, rpmdir);
    }
}

/****************************************************************
 *								*
 *		The configuration file parser			*
 *								*
 ****************************************************************/

/*
 * A few macro needed to help building the parser
 */

#define IS_BLANK(ptr) \
     (((*(ptr)) == ' ') || ((*(ptr)) == '\b') || \
      ((*(ptr)) == '\n') || ((*(ptr)) == '\r'))
#define SKIP_BLANK(ptr) \
     { while (((*(ptr)) == ' ') || ((*(ptr)) == '\b') || \
              ((*(ptr)) == '\n') || ((*(ptr)) == '\r')) ptr++; }
#define GOTO_EQL(ptr) \
     { while (((*(ptr)) != '\0') && ((*(ptr)) != '=') && \
              ((*(ptr)) != '\n') && ((*(ptr)) != '\r')) ptr++; }
#define GOTO_EOL(ptr) \
     { while (((*(ptr)) != '\0') && \
              ((*(ptr)) != '\n') && ((*(ptr)) != '\r')) ptr++; }

/*
 * addHeader : parse an Header entry, we expect the first
 *             part to be an URL and the end of the line is the name.
 *
 * e.g: "help.html Get Help"
 */
void addHeader(char *value) {
    char *url;
    char *name;
    char save;

    /*
     * Check for more room in the header arrays.
     * Yes I'm lazy ...
     */
    if (rpm2html_nb_extra_headers >= MAX_EXTRA_HEADERS) {
        fprintf(stderr, "Too many headers, increase MAX_EXTRA_HEADERS\n");
	return;
    }

    /*
     * Check the url, parse until finding a blank.
     */
    name = value;
    while (!IS_BLANK(name)) name++;
    save = *name;
    *name = 0;
    url = xmlStrdup(value);
    *name = save;
    if (url == NULL) {
        fprintf(stderr, "strndup \"%s\" failed\n", value);
	exit(1);
    }
    SKIP_BLANK(name);
    if (*name == '\0') {
        fprintf(stderr, "Config file : expecting \"header URL description\"\n");
        fprintf(stderr, "\tfound \"header: %s\", ignored\n", value);
	xmlFree(url);
	return;
    }

    /*
     * Store the values in global variables.
     */
    rpm2html_headers_name[rpm2html_nb_extra_headers] = xmlStrdup(name);
    rpm2html_headers_url[rpm2html_nb_extra_headers] = url;
    rpm2html_nb_extra_headers++;
}


/*
 * read config file: parse a configuration file.
 */
int readConfigFile(char *filename)
{
   FILE *input;
   char *str, *base;
   char string[1000];
   char rpmdir[1000] = "rpm2html";
   char *name;
   char *value;
   int errors = 0;

   rpm2html_host = rpm2html_rpm2html_thishost;
   input = fopen (filename, "r");
   if (input == NULL)
     {
	fprintf (stderr, "Cannot read config from %s :\n", filename);
	perror ("fopen failed");
	return -1;
     }

   while (1)
     {
	/*
	 * read one line in string buffer.
	 */
	if (fgets (&string[0], sizeof (string) - 1, input) == NULL)
	   break;

	str = &string[0];
	SKIP_BLANK (str)
	string[sizeof (string) - 1] = '\0';

	/*
	 * Comment starts with a semicolumn.
	 */
	if (*str == ';')
	   continue;
	if (*str == '\0')
	   continue;

	/*
	 * sections are indicated between brackets, e.g. [amaya]
	 */
	if (*str == '[')
	  {
	     str++;
	     SKIP_BLANK (str)
	     base = str;
	     while ((*str != '\0') && (*str != ']'))
		str++;
	     if (*str == '\0')
	       {
		  fprintf (stderr, "config file %s corrupted :\n\t\"%s\"\n",
			   filename, string);
		  break;
	       }
	     *str = '\0';
	     strcpy (&rpmdir[0], base);
	     if (strcasecmp(rpmdir, "metadata"))
		 rpmDirSearch(rpmdir);

	     if (rpm2htmlVerbose > 1)
		 fprintf (stderr, "readConfigFile section [%s]\n", rpmdir);

	     continue;
	  }

	/*
	 * entries have the following form :
	 *    name=value
	 */
	name = str;
	GOTO_EQL (str)
	if (*str != '=') {
	   errors++;
	   if (errors >= 30) {
	       fprintf (stderr, "config file %s seems invalid\n", filename);
	       exit(1);
	   }
	   continue;
	}
	*str++ = '\0';
	SKIP_BLANK (str)
	value = str;
	GOTO_EOL (str)
	*str = '\0';
	addConfigEntry(rpmdir, name, value);
#ifdef WITH_SQL
	/*
	 * Duplicate the info in the SQL base
	 */
	sqlConfigEntry(rpmdir, name, value);
#endif
     }

   fclose (input);
   return(0);
}

/*
 * reinitialize the base setup.
 */
void reinitialize(void) {
    if (rpm2html_dir) {
        xmlFree(rpm2html_dir);
	rpm2html_dir = NULL;
    }
    if (rpm2html_name) {
        xmlFree(rpm2html_name);
	rpm2html_name = NULL;
    }
    if (rpm2html_url) {
        xmlFree(rpm2html_url);
	rpm2html_url = NULL;
    }
    if (rpm2html_ftp) {
        xmlFree(rpm2html_ftp);
	rpm2html_ftp = NULL;
    }
    if (rpm2html_ftpsrc) {
        xmlFree(rpm2html_ftpsrc);
	rpm2html_ftpsrc = NULL;
    }
    if ((rpm2html_rpm2html_name != NULL) &&
        (strcmp(rpm2html_rpm2html_name, RPM2HTML_NAME))) {
        xmlFree(rpm2html_rpm2html_name);
    }
    rpm2html_rpm2html_name = RPM2HTML_NAME;
    if ((rpm2html_rpm2html_ver != NULL) &&
        (strcmp(rpm2html_rpm2html_ver, RPM2HTML_VER))) {
        xmlFree(rpm2html_rpm2html_ver);
    }
    rpm2html_rpm2html_ver = RPM2HTML_VER;
    if ((rpm2html_rpm2html_url != NULL) &&
        (strcmp(rpm2html_rpm2html_url, RPM2HTML_URL))) {
        xmlFree(rpm2html_rpm2html_url);
    }
    rpm2html_rpm2html_url = RPM2HTML_URL;
    if ((rpm2html_maint != NULL) && (strcmp(rpm2html_maint, RPM2HTML_MAINT))) {
        xmlFree(rpm2html_maint);
    }
    rpm2html_maint = RPM2HTML_MAINT;
    if ((rpm2html_mail != NULL) && (strcmp(rpm2html_mail, RPM2HTML_MAIL))) {
        xmlFree(rpm2html_mail);
    }
    rpm2html_mail = RPM2HTML_MAIL;
    rpm2html_dir = NULL;
    rpm2html_name = NULL;
    rpm2html_url = NULL;
    rpm2html_ftp = NULL;
    rpm2html_ftpsrc = NULL;
    if (rpm2html_host != NULL && rpm2html_host != rpm2html_rpm2html_thishost) xmlFree(rpm2html_host);
    rpm2html_host = rpm2html_rpm2html_thishost;
    rpm2html_build_tree = 0;
    rpm2html_dump_rdf = 0;
    rpm2html_dump_rdf_resources = 0;
    rpm2html_dump_html = 1;
    #ifdef HAVE_LIBTEMPLATE
    if (rpm2html_html_template != NULL) xmlFree(rpm2html_html_template);
    rpm2html_html_template = "rpm2html.tpl";
    #endif
    if (rpm2html_rdf_dir != NULL) xmlFree(rpm2html_rdf_dir);
    rpm2html_rdf_dir = NULL;
    if (rpm2html_rdf_resources_dir != NULL) xmlFree(rpm2html_rdf_resources_dir);
    rpm2html_rdf_resources_dir = NULL;

    for (;rpm2html_nb_extra_headers > 0;) {
	rpm2html_nb_extra_headers--;
        xmlFree(rpm2html_headers_name[rpm2html_nb_extra_headers]);
	rpm2html_headers_name[rpm2html_nb_extra_headers] = NULL;
	xmlFree(rpm2html_headers_url[rpm2html_nb_extra_headers]);
	rpm2html_headers_url[rpm2html_nb_extra_headers] = NULL;
    }
    rpm2html_nb_extra_headers = 0;

    /*
     * Cleanup of other modules.
     */
    rpmdataCleanup();
    htmlCleanup();
    rpmopenCleanup();
    #ifdef HAVE_LIBTEMPLATE
    destroyTemplateEngine();
    #endif
}

