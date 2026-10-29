/*
 * rpm2html.h : general resources about rpm2html
 *
 * See Copyright for the status of this software.
 *
 * $Id: rpm2html.h,v 1.83 2010/11/09 22:25:55 hany Exp $
 */

#ifndef __RPM2HTML_H__
#define __RPM2HTML_H__

#include "memory.h"
#include "stringbuf.h"

/*
 * General setup default values overriden by config file.
 */
#define RPM2HTML_NAME	"rpm2html"
#define RPM2HTML_VER	"2.0"
#define RPM2HTML_URL    "https://github.com/odeneriksson/rpm2html"
#define RPM2HTML_MAINT	"Oden Eriksson"
#define RPM2HTML_MAIL 	"5888236+odeneriksson@users.noreply.github.com"

/*
 * Constants.
 */

#define MAX_COLOR_PER_LINE	4
#define MAX_TABLE_LENGHT	20
#define MAX_PAGE_LENGHT		250
#define PACKAGE_FIELD_WIDTH	250
#define DESCRIPTION_FIELD_WIDTH	450
#define SYSTEM_FIELD_WIDTH	95
#define MAX_EXTRA_HEADERS	10
#define MAX_NAME_LIST_LENGHT	75
#define DEFAULT_RDF_COUNT_LIMIT 20

typedef enum rpm_dep_flag {
    RPM2HTML_REQ_NONE = 0,
    RPM2HTML_REQ_LT = 1,
    RPM2HTML_REQ_LEQ,
    RPM2HTML_REQ_GT,
    RPM2HTML_REQ_GEQ,
    RPM2HTML_REQ_EQU
} rpm_dep_flag;

/*
 * Unmodifiable variables.
 */
extern char *rpm2html_rpm2html_name;	/* OK */
extern char *rpm2html_rpm2html_ver;	/* OK */
extern char *rpm2html_rpm2html_url;	/* OK */
extern char *rpm2html_rpm2html_thishost;

/*
 * global variables.
 */
extern int   rpm2htmlVerbose;
extern char *rpm2html_maint;	/* OK */
extern char *rpm2html_mail;	/* OK */
extern char *rpm2html_help;
extern char *rpm2html_dir;	/* OK */
extern char *rpm2html_name;	/* OK */
extern char *rpm2html_url;	/* OK */
#ifdef WITH_SQL
extern char *rpm2html_search;
#endif
extern char *rpm2html_ftp;	/* OK */
extern char *rpm2html_ftpsrc;	/* OK */
extern char *rpm2html_host;	/* OK */
extern int   rpm2html_dump_rdf;
extern int   rpm2html_dump_rdf_resources;
extern int   rpm2html_dump_html;
extern int   rpm2html_dump_html_only_if_rpm_newer;
#ifdef HAVE_LIBTEMPLATE
extern char *rpm2html_html_template;
#endif
extern char *rpm2html_rdf_dir;	/* OK */
extern char *rpm2html_rdf_resources_dir;	/* OK */
extern int   rpm2html_rdf_count_limit;
extern int   rpm2html_build_tree;
extern int   rpm2html_cve_linking;
extern int   rpm2html_no_sql;
extern int   rpm2html_protect_emails;

extern int   rpm2html_files;
extern int   rpm2html_size;
extern int   rpm2html_install_files;
extern int   rpm2html_install_size;

extern char *rpm2html_headers_name[MAX_EXTRA_HEADERS];	/* OK */
extern char *rpm2html_headers_url[MAX_EXTRA_HEADERS];	/* OK */
extern int   rpm2html_nb_extra_headers; 

extern int   nb_metadata_mirrors;
extern int   max_metadata_mirrors;
extern char **metadata_mirrors;

extern int readConfigFile(char *filename);
extern void addConfigEntry(char *rpmdir, char *name, char *value);
extern void reinitialize(void);
extern char *xmlStrdupHTML(const char *str);
extern void rpmDirCleanupAll(void);
extern void cleanupCleanup(void);
extern void htmlCleanup(void);
extern void rpmopenCleanup(void);

#endif /* __RPM2HTML_H__ */
