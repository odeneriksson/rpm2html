/*
 * sqltools.c: front-end program for MySQL database
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
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
/********
#include <rpm/rpmlib.h>
 ********/

#include "rpm2html.h"
#include "rpmdata.h"
#include "sql.h"

void usage(const char *name) {
    printf("%s: usage\n", name);
    printf("	config: dump a config file from the database\n");
    printf("	stats: show database usage statistics\n");
    printf("	reindex: reindex all distro one by one\n");
    printf("	check: remove superfluous entries from the database\n");
    printf("	vendors: list the registered vendors\n");
    printf("	distribs: list the registered distribs\n");
    printf("	metadata: list the registered metadata servers\n");
    printf("	mirrors: list the registered mirrors\n");
    printf("	latests: list the latest packages\n");
    printf("	index: regenerates the full index RDF\n");
    printf("	add distrib name [vendor [directory [path [url [ urlsrc [description]]]]]]\n");
    printf("	add vendor name [url [description]]\n");
    printf("	add mirror distrib url\n");
    printf("	queries : dump the most requested queries\n");
    exit(1);
}

int main(int argc, char **argv) {
    int res;

    if (getenv("DEBUG_SQLTOOLS") == NULL)
	rpm2htmlVerbose = 0;
    if (argc < 2)
        usage(argv[0]);

    if (init_sql(NULL, NULL, NULL, NULL) < 0)
	exit(1);

    res = sql_check_tables();
    if (res > 0) {
	printf("rebuilt %d tables\n", res);
    }

    if (!strcmp(argv[1], "vendors"))
	sql_show_vendors();
    else if (!strcmp(argv[1], "metadata"))
	sql_show_metadata();
    else if (!strcmp(argv[1], "mirrors"))
	sql_show_mirrors();
    else if (!strcmp(argv[1], "distribs"))
	sql_show_distribs();
    else if (!strcmp(argv[1], "config"))
	sql_show_config();
    else if (!strcmp(argv[1], "stats"))
	sql_show_stats();
    else if (!strcmp(argv[1], "latests"))
	sql_show_latests();
    else if (!strcmp(argv[1], "all"))
	sql_show_all();
    else if (!strcmp(argv[1], "resources"))
	sql_show_resources();
    else if (!strcmp(argv[1], "check"))
	sql_check_packages();
    else if (!strcmp(argv[1], "topindex"))
	sql_top_index();
    else if (!strcmp(argv[1], "index"))
	sql_show_index();
    else if (!strcmp(argv[1], "reindex"))
	sql_reindex();
    else if (!strcmp(argv[1], "queries"))
	sql_get_top_queries(300);
    else if (!strcmp(argv[1], "add")) {
	if (argc < 5)
	    usage(argv[0]);
	if (!strcmp(argv[2], "distrib")) {
	    char *Name = NULL;
	    char *Path = NULL;
	    char *Directory = NULL;
	    char *Vendor = NULL;
	    char *URL = NULL;
	    char *URLSrc = NULL;
            char *Description = NULL;
	    char *Color = NULL;
	    char *Html = NULL;
	    if ((argc > 3) && (argv[3][0] != 0))
		Name = argv[3];
	    if ((argc > 4) && (argv[4][0] != 0))
		Vendor = argv[4];
	    if ((argc > 5) && (argv[5][0] != 0))
		Directory = argv[5];
	    if ((argc > 6) && (argv[6][0] != 0))
		Path = argv[6];
	    if ((argc > 7) && (argv[7][0] != 0))
		URL = argv[7];
	    if ((argc > 8) && (argv[8][0] != 0))
		URLSrc = argv[8];
	    if ((argc > 9) && (argv[9][0] != 0))
		Description = argv[9];
	    res = sql_add_distrib(Name, Vendor, Directory, Path,
		                  URL, URLSrc, Description, Color, Html);
	    printf("updated %d fields\n", res);
	} else if (!strcmp(argv[2], "vendor")) {
	    char *Name = NULL;
	    char *URL = NULL;
            char *Description = NULL;
	    if ((argc > 3) && (argv[3][0] != 0))
		Name = argv[3];
	    if ((argc > 4) && (argv[4][0] != 0))
		URL = argv[4];
	    if ((argc > 5) && (argv[5][0] != 0))
		Description = argv[5];
	    res = sql_add_vendor(Name, URL, Description);
	    printf("updated %d fields\n", res);
	} else if (!strcmp(argv[2], "mirror")) {
	    char *Name = NULL;
	    char *URL = NULL;
	    if ((argc > 3) && (argv[3][0] != 0))
		Name = argv[3];
	    if ((argc > 4) && (argv[4][0] != 0))
		URL = argv[4];
	    res = sql_add_mirror(Name, URL, 0);
	    printf("updated %d fields\n", res);
	}
    }

    if (close_sql() < 0)
	return(1);
    exit(0);
}

