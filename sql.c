/*
 * sql.c: front-end for MySQL database
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
/********
#include <rpm/rpmlib.h>
 ********/
#include <zlib.h>

#include <mysql/mysql.h>
#include <mysql/errmsg.h>
#include <zlib.h>
#include "rpm2html.h"
#include "rpmdata.h"
/********
#include "rpmdata.h"
 ********/
#include "sql.h"
#include "html.h"

/* #define SQL_DEBUG_TIMING */
#define SQL_DEBUG_MEM 

#define SQL_MAX_DISTRIBS 500

/************************************************************************
 * 									*
 * 		Generic inititialisation/close of the DB		*
 * 									*
 ************************************************************************/

static MYSQL *sql = NULL;
static const char *myhost, *mybase, *myuser, *mypasswd;

int init_sql(const char *host, const char *base, const char *user, const char *passwd) {
    int attempts = 0;

    /*
     * Close the previous connection if any
     */
    if (sql != NULL) {
	close_sql();
	if (myhost != NULL) free((char *) myhost);
	myhost = NULL;
	if (mybase != NULL) free((char *) mybase);
	mybase = NULL;
	if (myuser != NULL) free((char *) myuser);
	myuser = NULL;
	if (mypasswd != NULL) free((char *) mypasswd);
	mypasswd = NULL;
    }

    if (host == NULL)
	host = getenv("MySQL_HOST");
    if (host == NULL)
	host = "localhost";
    if (base == NULL)
	base = getenv("MySQL_BASE");
    if (base == NULL)
	base = "rpmfind";
    if (passwd == NULL)
	passwd = getenv("MySQL_PASS");
    if (user == NULL)
	user = getenv("MySQL_USER");
    if (user == NULL)
	user = getenv("USER");
    if (user == NULL)
	user = getenv("USERNAME");
    if (user == NULL)
	user = getenv("LOGNAME");
    sql = mysql_init(NULL);
    if (mysql_errno(sql)) {
	fprintf(stderr, "mysql_init failed: %s\n", mysql_error(sql));
	return(-1);
    }
    for (attempts = 0; ;attempts ++) {
	mysql_real_connect(sql, host, user, passwd, base, 0, NULL, 0);
	if (mysql_errno(sql)) {
	    if ((attempts == 0) && (passwd == NULL)) {
		fprintf(stderr, "No passwd defined, use MySQL_PASS env\n");
	    }
	    if (attempts >= 20) {
		fprintf(stderr,
			"mysql: connect as %s to %s on host %s  failed: %s\n",
		        user, base, host, mysql_error(sql));
		return(-1);
	    } else {
		fprintf(stderr,
			"mysql: retrying connect as %s to %s on host %s  failed: %s\n",
		        user, base, host, mysql_error(sql));
	    }
	} else {
	    break;
	}
	sleep(15);
    }
    myhost = strdup(host);
    mybase = strdup(base);
    myuser = strdup(user);
    mypasswd = strdup(passwd);
    sql_check_tables();
    return(0);
}

int close_sql(void) {
    mysql_close(sql);
    if (mysql_errno(sql)) {
	fprintf(stderr, "mysql_close failed: %s\n", mysql_error(sql));
	return(-1);
    }
    sql = NULL;
    return(0);
}

int restart_sql(void) {
    int attempts = 0;

    /*
     * Close the previous connection if any
     */
    if (sql != NULL) {
	close_sql();
	sql = NULL;
    }

    sql = mysql_init(NULL);
    if (mysql_errno(sql)) {
	fprintf(stderr, "mysql_init failed: %s\n", mysql_error(sql));
	return(-1);
    }
    for (attempts = 0; ;attempts ++) {
	mysql_real_connect(sql, myhost, myuser, mypasswd, mybase, 0, NULL, 0);
	if (mysql_errno(sql)) {
	    if (attempts >= 20) {
		fprintf(stderr,
			"mysql: reconnect as %s to %s on host %s  failed: %s\n",
		        myuser, mybase, myhost, mysql_error(sql));
	    return(-1);
	    }
	} else {
	    break;
	}
	sleep(15);
    }
    sql_check_tables();
    return(0);
}

/*
 * Handle disconnects by doing a reconnect and a retry.
 */
int do_sql_query(const char *query, int len) {
    int res;

    if (sql == NULL) {
	res = restart_sql();
	if (res != 0)
	    return(res);
    }
    res = mysql_real_query(sql, query, len);
    if ((res == CR_SERVER_GONE_ERROR) || (res == CR_SERVER_LOST)) {
	res = restart_sql();
	if (res != 0)
	    return(res);
	res = mysql_real_query(sql, query, len);
    }
    return(res);
}

/************************************************************************
 * 									*
 * 		Generic functions to access the tables			*
 * 									*
 ************************************************************************/

#define MAX_QUERY 8000
#define SMALL_QUERY 500

int sql_update_id(const char *table, int id,
	          const char *field, const char *value) {
    MYSQL_RES *result;
    char query[MAX_QUERY];
    int nb_fields = 0;
    int left = MAX_QUERY - 1;
    int len;
    char *end;

    if ((table == NULL) ||
	(field == NULL) || (value == NULL))
	return(-1);

    len = snprintf(query, left, "UPDATE %s SET %s='", table, field);
    if (len < 0) {
	fprintf(stderr, "sql_update_id : %s(%d).%s overflow\n",
		table, id, field);
	return(-1);
    }
    end = &query[len];
    left -= len;
    len = strlen(value);
    if (len * 2 >= left) {
	fprintf(stderr, "sql_update_id : %s(%d).%s overflow\n",
		table, id, field);
	return(-1);
    }
    len = mysql_escape_string(end, value, len);
    left -= len;
    if (left <= 0) {
	fprintf(stderr, "sql_update_id : %s(%d).%s overflow\n",
		table, id, field);
	return(-1);
    }
    end += len;
    len = snprintf(end, left, "' WHERE ID=%d", id);
    if (len < 0) {
	fprintf(stderr, "sql_update_id : %s(%d).%s overflow\n",
		table, id, field);
	return(-1);
    }
    end += len;
    query[MAX_QUERY - 1] = 0;

    if (do_sql_query(query, (unsigned int) (end - query))) {
	printf("sql_update_id: UPDATE %s %d failed: %s\n",
	       table, id, mysql_error(sql));
	return(-1);
    }
    result = mysql_store_result(sql);
    if (result != NULL) {
	nb_fields = mysql_num_fields(result);
	mysql_free_result(result);
    } else {
	nb_fields = 1;
    }
    if(mysql_errno(sql)) {
	fprintf(stderr, "sql_update_id UPDATE error: %s\n",
		 mysql_error(sql));
	return(-1);
    }
    return(nb_fields);
}

int sql_blind_insert(const char *table, const char *key,
                     const char *value, int id) {
    MYSQL_RES *result;
    char query[MAX_QUERY];
    int left = MAX_QUERY - 1;
    int len;
    char *end;
    int insert = -1;

    if ((table == NULL) ||
	(key == NULL) || (value == NULL))
	return(-1);

    /*
     * Search first for the ID if it already exists
     */
    if (id > 0) {
	len = snprintf(query, left, "INSERT INTO %s (ID, %s) VALUES (%d, '",
		       table, key, id);
	if (len < 0) {
	    fprintf(stderr, "sql_blind_insert : %s(%d).%s overflow\n",
		    table, id, key);
	    return(-1);
	}
	end = &query[len];
	left -= len;
	len = strlen(value);
	if (len * 2 >= left) {
	    fprintf(stderr, "sql_blind_insert : %s(%d).%s overflow\n",
		    table, id, key);
	    return(-1);
	}
	len = mysql_escape_string(end, value, len);
	left -= len;
	if (left <= 0) {
	    fprintf(stderr, "sql_blind_insert : %s(%d).%s overflow\n",
		    table, id, key);
	    return(-1);
	}
	end += len;
	len = snprintf(end, left, "')");
	if (len < 0) {
	    fprintf(stderr, "sql_blind_insert : %s(%d).%s overflow\n",
		    table, id, key);
	    return(-1);
	}
	end += len;
	query[MAX_QUERY - 1] = 0;

    } else {
	len = snprintf(query, left, "INSERT INTO %s (%s) VALUES ('",
		       table, key);
	if (len < 0) {
	    fprintf(stderr, "sql_blind_insert : %s.%s overflow\n",
		    table, key);
	    return(-1);
	}
	end = &query[len];
	left -= len;
	len = strlen(value);
	if (len * 2 >= left) {
	    fprintf(stderr, "sql_blind_insert : %s.%s overflow\n",
		    table, key);
	    return(-1);
	}
	len = mysql_escape_string(end, value, len);
	left -= len;
	if (left <= 0) {
	    fprintf(stderr, "sql_blind_insert : %s.%s overflow\n",
		    table, key);
	    return(-1);
	}
	end += len;
	len = snprintf(end, left, "')");
	if (len < 0) {
	    fprintf(stderr, "sql_blind_insert : %s.%s overflow\n",
		    table, key);
	    return(-1);
	}
	end += len;
	query[MAX_QUERY - 1] = 0;

    }
    query[MAX_QUERY - 1] = 0;
    if (do_sql_query(query, (unsigned int) (end - query))) {
#ifdef SQL_DEBUG
	fprintf(stderr, "sql_blind_insert Error: %s\n", mysql_error(sql));
#endif
	return(-1);
    }
    result = mysql_store_result(sql);
    insert = mysql_insert_id(sql);
    if (result) {
	mysql_free_result(result);
	return(insert);
    }
    if(mysql_errno(sql)) {
	fprintf(stderr, "sql_blind_insert Error: %s\n", mysql_error(sql));
	return(-1);
    }
    return(insert);
}

int sql_update(const char *table, const char *name,
	       const char *field, const char *value) {
    MYSQL_RES *result;
    MYSQL_ROW row;
    char query[MAX_QUERY];
    int id;
    int nb_fields = 0;
    int left = MAX_QUERY - 1;
    int len;
    char *end;

    if ((name == NULL) || (table == NULL) ||
	(field == NULL) || (value == NULL))
	return(-1);

    /*
     * Search first for the ID if it already exists
     */
    snprintf(query, MAX_QUERY - 1, "SELECT ID FROM %s WHERE Name='%s'", table, name);
    query[MAX_QUERY - 1] = 0;
    if (mysql_query(sql,query)) {
	printf("sql_update: SELECT failed\n");
	return(-1);
    }

    result = mysql_use_result(sql);
    if (result) {
	while((row = mysql_fetch_row(result)))
	{
	    if (row[0] == NULL) {
		printf("sql_update: select ID for %s returns NULL !\n", name);
		return(-1);
	    }
	    if (sscanf(row[0], "%d", &id) != 1) {
		printf("sql_update: ID non numeric %s\n", row[0]);
		return(-1);
	    }

	    left = MAX_QUERY - 1;
	    len = snprintf(query, left, "UPDATE %s SET %s='", table, field);
	    if (len < 0) {
		fprintf(stderr, "sql_update : %s(%d).%s overflow\n",
			table, id, field);
		return(-1);
	    }
	    end = &query[len];
	    left -= len;
	    len = strlen(value);
	    if (len * 2 >= left) {
		fprintf(stderr, "sql_update : %s(%d).%s overflow\n",
			table, id, field);
		return(-1);
	    }
	    len = mysql_escape_string(end, value, len);
	    left -= len;
	    if (left <= 0) {
		fprintf(stderr, "sql_update : %s(%d).%s overflow\n",
			table, id, field);
		return(-1);
	    }
	    end += len;
	    len = snprintf(end, left, "' WHERE ID=%d", id);
	    if (len < 0) {
		fprintf(stderr, "sql_update : %s(%d).%s overflow\n",
			table, id, field);
		return(-1);
	    }
	    end += len;
	    query[MAX_QUERY - 1] = 0;

	    mysql_free_result(result);
	    if (do_sql_query(query, (unsigned int) (end - query))) {
		printf("sql_update: UPDATE failed: %s\n", mysql_error(sql));
		return(-1);
	    }
	    result = mysql_store_result(sql);
	    if (result != NULL) {
		nb_fields = mysql_num_fields(result);
		mysql_free_result(result);
	    } else {
		return(1);
	    }
	    /* Do not loop ... only the first */
	    return(nb_fields);
	}
	mysql_free_result(result);
	if (nb_fields == 0) {
	    /*
	     * Propagate an insert
	     */
	    snprintf(query, MAX_QUERY - 1,
		    "INSERT INTO %s (Name,%s) VALUES ('%s','%s')",
		     table, field, name, value);
	    query[MAX_QUERY - 1] = 0;
	    if (mysql_query(sql, query)) {
		printf("sql_update: INSERT failed: %s\n", mysql_error(sql));
		return(-1);
	    }
	    result = mysql_store_result(sql);
	    if (result != NULL) {
		nb_fields = mysql_num_fields(result);
		mysql_free_result(result);
	    } else {
		return(1);
	    }
	}
    }
    if(mysql_errno(sql)) {
	fprintf(stderr, "sql_update Error: %s\n", mysql_error(sql));
	return(-1);
    }
    return(nb_fields);
}

int sql_get_key(const char *table, const char *name, const char *value) {
    int id;
    MYSQL_RES *result;
    MYSQL_ROW row;
    char query[MAX_QUERY];
    int left = MAX_QUERY - 1;
    int len;
    char *end;

    if ((table == NULL) || (name == NULL))
	return(-1);


    /*
     * Search first for the ID if it already exists
     */
    len = snprintf(query, left, "SELECT ID FROM %s WHERE %s='",
	           table, name);
    if (len < 0) {
	fprintf(stderr, "sql_get_key : %s(%s).%s overflow\n",
		table, name, value);
	return(-1);
    }
    end = &query[len];
    left -= len;
    len = strlen(value);
    if (len * 2 >= left) {
	fprintf(stderr, "sql_get_key : %s(%s).%s overflow\n",
		table, name, value);
	return(-1);
    }
    len = mysql_escape_string(end, value, len);
    left -= len;
    if (left <= 0) {
	fprintf(stderr, "sql_get_key : %s(%s).%s overflow\n",
		table, name, value);
	return(-1);
    }
    end += len;
    len = snprintf(end, left, "'");
    if (len < 0) {
	fprintf(stderr, "sql_get_key : %s(%s).%s overflow\n",
		table, name, value);
	return(-1);
    }
    end += len;
    query[MAX_QUERY - 1] = 0;

    if (mysql_query(sql,query)) {
	printf("sql_get_key: SELECT %s failed %s\n", name, mysql_error(sql));
	return(-1);
    }

    result = mysql_use_result(sql);
    if (result) {
	while((row = mysql_fetch_row(result)))
	{
	    /*
	     * Lookup the first ID and return it
	     */
	    if (row[0] == NULL) {
		mysql_free_result(result);
		printf("sql_get_key: select returns NULL !\n");
		return(-1);
	    }
	    if (sscanf(row[0], "%d", &id) != 1) {
		mysql_free_result(result);
		printf("sql_get_key: ID non numeric %s\n", row[0]);
		return(-1);
	    }
	    mysql_free_result(result);
	    return(id);
	}
	mysql_free_result(result);
    }
    if(mysql_errno(sql)) {
	fprintf(stderr, "Error: %s\n", mysql_error(sql));
	return(-1);
    }

    /*
     * Do a creation
     */

    query[SMALL_QUERY - 1] = 0;
    len = snprintf(query, left, "INSERT INTO %s (%s) VALUES ('",
	           table, name);
    if (len < 0) {
	fprintf(stderr, "sql_get_key : %s(%s).%s overflow\n",
		table, name, value);
	return(-1);
    }
    end = &query[len];
    left -= len;
    len = strlen(value);
    if (len * 2 >= left) {
	fprintf(stderr, "sql_get_key : %s(%s).%s overflow\n",
		table, name, value);
	return(-1);
    }
    len = mysql_escape_string(end, value, len);
    left -= len;
    if (left <= 0) {
	fprintf(stderr, "sql_get_key : %s(%s).%s overflow\n",
		table, name, value);
	return(-1);
    }
    end += len;
    len = snprintf(end, left, "')");
    if (len < 0) {
	fprintf(stderr, "sql_get_key : %s(%s).%s overflow\n",
		table, name, value);
	return(-1);
    }
    end += len;
    query[MAX_QUERY - 1] = 0;

    if (mysql_query(sql,query)) {
	printf("sql_get_key: INSERT %s failed %s\n", name, mysql_error(sql));
	return(-1);
    }
    id = mysql_insert_id(sql);
    result = mysql_store_result(sql);
    if (result != NULL)
	mysql_free_result(result);
    return(id);
}

int sql_read_key(const char *table, const char *name) {
    int id;
    MYSQL_RES *result;
    MYSQL_ROW row;
    char query[SMALL_QUERY];

    if ((table == NULL) || (name == NULL))
	return(-1);

    /*
     * Search for the ID it has to exist
     */
    snprintf(query, SMALL_QUERY - 1, "SELECT ID FROM %s WHERE Name='%s'", table, name);
    query[SMALL_QUERY - 1] = 0;
    if (mysql_query(sql,query)) {
	printf("sql_read_key: SELECT %s failed %s\n", name, mysql_error(sql));
	return(-1);
    }

    result = mysql_use_result(sql);
    if (result) {
	while((row = mysql_fetch_row(result)))
	{
	    /*
	     * Lookup the first ID and return it
	     */
	    if (row[0] == NULL) {
		mysql_free_result(result);
		printf("sql_read_key: select returns NULL !\n");
		return(-1);
	    }
	    if (sscanf(row[0], "%d", &id) != 1) {
		mysql_free_result(result);
		printf("sql_read_key: ID non numeric %s\n", row[0]);
		return(-1);
	    }
	    mysql_free_result(result);
	    return(id);
	}
	mysql_free_result(result);
    }
    if(mysql_errno(sql)) {
	fprintf(stderr, "Error: %s\n", mysql_error(sql));
	return(-1);
    }
    return(-1);
}

int sql_read_info_key(const char *table, const char *name, const char *value) {
    int id;
    MYSQL_RES *result;
    MYSQL_ROW row;
    char query[SMALL_QUERY];

    if ((table == NULL) || (name == NULL) || (value == NULL))
	return(-1);

    /*
     * Search for the ID it has to exist
     */
    snprintf(query, SMALL_QUERY - 1, "SELECT ID FROM %s WHERE %s='%s'", table, name, value);
    query[SMALL_QUERY - 1] = 0;
    if (mysql_query(sql,query)) {
	printf("sql_read_info_key: SELECT %s failed %s\n", name, mysql_error(sql));
	return(-1);
    }

    result = mysql_use_result(sql);
    if (result) {
	while((row = mysql_fetch_row(result)))
	{
	    /*
	     * Lookup the first ID and return it
	     */
	    if (row[0] == NULL) {
		mysql_free_result(result);
		printf("sql_read_info_key: select returns NULL !\n");
		return(-1);
	    }
	    if (sscanf(row[0], "%d", &id) != 1) {
		mysql_free_result(result);
		printf("sql_read_info_key: ID non numeric %s\n", row[0]);
		return(-1);
	    }
	    mysql_free_result(result);
	    return(id);
	}
	mysql_free_result(result);
    }
    if(mysql_errno(sql)) {
	fprintf(stderr, "Error: %s\n", mysql_error(sql));
	return(-1);
    }
    return(-1);
}

/************************************************************************
 *									*
 *				Tables handling				*
 *									*
 ************************************************************************/

int sql_rebuild_config(void) {
    const char *query =
"CREATE TABLE Config ( \n\
    ID int(11) NOT NULL auto_increment, \n\
    Name varchar(50) NOT NULL, \n\
    Value varchar(255), \n\
    PRIMARY KEY (ID), \n\
    KEY Name (Name(10)) \n\
)";

    if (mysql_query(sql,query)) {
	printf("sql_rebuild_config: CREATE TABLE Config failed %s\n",
	       mysql_error(sql));
	return(-1);
    }
    return(0);
}

int sql_rebuild_vendors(void) {
    const char *query =
"CREATE TABLE Vendors ( \n\
    ID int(11) NOT NULL auto_increment, \n\
    Name varchar(255) NOT NULL, \n\
    URL varchar(255), \n\
    Key1 text, \n\
    Key2 text, \n\
    Key3 text, \n\
    Description text, \n\
    PRIMARY KEY (ID), \n\
    KEY Name (Name(10)) \n\
)";

    if (mysql_query(sql,query)) {
	printf("sql_rebuild_vendors: CREATE TABLE Vendors failed %s\n",
	       mysql_error(sql));
	return(-1);
    }
    return(0);
}

int sql_rebuild_distributions(void) {
    const char *query =
"CREATE TABLE Distributions ( \n\
    ID int(11) NOT NULL auto_increment, \n\
    Name varchar(255) NOT NULL, \n\
    URL varchar(255), \n\
    Key1 text, \n\
    Key2 text, \n\
    Key3 text, \n\
    Description text, \n\
    PRIMARY KEY (ID), \n\
    KEY Name (Name(10)) \n\
)";

    if (mysql_query(sql,query)) {
	printf("sql_rebuild_ditributions: CREATE TABLE Distributions failed %s\n",
	       mysql_error(sql));
	return(-1);
    }
    return(0);
}

int sql_rebuild_searches(void) {
    const char *query =
"CREATE TABLE Searches ( \n\
    URL varchar(255) NOT NULL, \n\
    name varchar(255) NOT NULL, \n\
    active int, \n\
    UNIQUE(URL), \n\
    UNIQUE(name) \n\
)";

    if (mysql_query(sql,query)) {
	printf("sql_rebuild_searches: CREATE TABLE Searches failed %s\n",
	       mysql_error(sql));
	return(-1);
    }
    return(0);
}

int sql_rebuild_mirrors(void) {
    const char *query =
"CREATE TABLE Mirrors ( \n\
    ID int(11), \n\
    URL varchar(255) NOT NULL, \n\
    Country int(11), \n\
    UNIQUE(URL) \n\
)";

    if (mysql_query(sql,query)) {
	printf("sql_rebuild_mirrors: CREATE TABLE Mirrors failed %s\n",
	       mysql_error(sql));
	return(-1);
    }
    return(0);
}

int sql_rebuild_metadata(void) {
    const char *query =
"CREATE TABLE Metadata ( \n\
    URL varchar(255) NOT NULL, \n\
    Maintainer int(11), \n\
    Country int(11), \n\
    UNIQUE(URL) \n\
)";

    if (mysql_query(sql,query)) {
	printf("sql_rebuild_metadata: CREATE TABLE Metadata failed %s\n",
	       mysql_error(sql));
	return(-1);
    }
    return(0);
}

int sql_rebuild_distribs(void) {
    const char *query =
"CREATE TABLE Distribs ( \n\
    ID int(11) NOT NULL auto_increment, \n\
    Name varchar(255) NOT NULL, \n\
    Vendor int(11), \n\
    Directory varchar(255), \n\
    Path varchar(100) NOT NULL, \n\
    URL varchar(255), \n\
    URLSrc varchar(255), \n\
    Html varchar(8), \n\
    Color varchar(10), \n\
    Key1 text, \n\
    Key2 text, \n\
    Description text, \n\
    PRIMARY KEY (ID), \n\
    KEY Name (Name(10)) \n\
)";

    if (mysql_query(sql,query)) {
	printf("sql_rebuild_distribs: CREATE TABLE Distribs failed %s\n",
	       mysql_error(sql));
	return(-1);
    }
    return(0);
}

int sql_rebuild_packages(void) {
    const char *query =
"CREATE TABLE Packages ( \n\
ID int(11) NOT NULL auto_increment, \n\
filename varchar(255) NOT NULL, \n\
Name varchar(50) NOT NULL, \n\
Version varchar(50) NOT NULL, \n\
Release varchar(50) NOT NULL, \n\
Arch varchar(15) NOT NULL, \n\
Dist int(11), \n\
URL varchar(255), \n\
URLSrc varchar(255), \n\
Vendor int(11), \n\
Packager int(11), \n\
Category varchar(255), \n\
Summary varchar(255), \n\
Description text, \n\
Copyright varchar(255), \n\
Date int(11), \n\
Size int(11), \n\
Os varchar(12), \n\
PRIMARY KEY (ID), \n\
KEY filename (filename(80)), \n\
KEY Name (Name(15)) \n\
)";

    if (mysql_query(sql,query)) {
	printf("sql_rebuild_packages: CREATE TABLE Packages failed %s\n",
	       mysql_error(sql));
	return(-1);
    }
    return(0);
}

int sql_rebuild_files(void) {
    const char *query =
"CREATE TABLE Files ( \n\
    ID int(11) NOT NULL, \n\
    Path varchar(35) NOT NULL, \n\
    UNIQUE KEY id (ID,Path(35)), \n\
    INDEX (ID), \n\
    INDEX (Path) \n\
)";

    if (mysql_query(sql,query)) {
	printf("sql_rebuild_files: CREATE TABLE Files failed %s\n",
	       mysql_error(sql));
	return(-1);
    }
    return(0);
}

int sql_rebuild_provides(void) {
    const char *query =
"CREATE TABLE Provides ( \n\
    ID int(11) NOT NULL, \n\
    Resource varchar(35) NOT NULL, \n\
    UNIQUE KEY id (ID,Resource(35)), \n\
    INDEX (ID), \n\
    INDEX (Resource) \n\
)";

    if (mysql_query(sql,query)) {
	printf("sql_rebuild_provides: CREATE TABLE Provides failed %s\n",
	       mysql_error(sql));
	return(-1);
    }
    return(0);
}

int sql_rebuild_requires(void) {
    const char *query =
"CREATE TABLE Requires ( \n\
    ID int(11) NOT NULL, \n\
    Resource varchar(35) NOT NULL, \n\
    Rel char(2), \n\
    Value varchar(20), \n\
    UNIQUE KEY id (ID,Resource(35)), \n\
    INDEX (ID), \n\
    INDEX (Resource) \n\
)";

    if (mysql_query(sql,query)) {
	printf("sql_rebuild_requires: CREATE TABLE Requires failed %s\n",
	       mysql_error(sql));
	return(-1);
    }
    return(0);
}

int sql_rebuild_queries(void) {
    const char *query =
"CREATE TABLE Queries ( \n\
    ID int(11) NOT NULL auto_increment,\n\
    Value varchar(50) NOT NULL, \n\
    Count int(11) NOT NULL, \n\
    Results int(11) NOT NULL, \n\
    UNIQUE KEY id (ID,Value(35)), \n\
    INDEX (ID), \n\
    INDEX (Value) \n\
)";

    if (mysql_query(sql,query)) {
	printf("sql_rebuild_queries: CREATE TABLE Queries failed %s\n",
	       mysql_error(sql));
	return(-1);
    }
    return(0);
}


int sql_check_tables(void) {
    const char *query = "SHOW TABLES";
    MYSQL_RES *result;
    MYSQL_ROW row;
    int config = 0;
    int distribs = 0;
    int requires = 0;
    int provides = 0;
    int vendors = 0;
    int mirrors = 0;
    int metadata = 0;
    int packages = 0;
    int searches = 0;
    int files = 0;
    int queries = 0;
    int distributions = 0;

    int rebuilt = 0;

    if (mysql_query(sql,query)) {
	printf("sql_check_tables: SHOW TABLES failed %s\n",
	       mysql_error(sql));
	return(-1);
    }

    result = mysql_use_result(sql);
    if (result) {
	while((row = mysql_fetch_row(result)))
	{
	    if (row[0] == NULL) {
		mysql_free_result(result);
		printf("sql_check_tables: SHOW TABLES returns NULL !\n");
		return(-1);
	    }
	    if (!strcmp(row[0], "Config"))
		config = 1;
	    if (!strcmp(row[0], "Distribs"))
		distribs = 1;
	    if (!strcmp(row[0], "Vendors"))
		vendors = 1;
	    if (!strcmp(row[0], "Mirrors"))
		mirrors = 1;
	    if (!strcmp(row[0], "Metadata"))
		metadata = 1;
	    if (!strcmp(row[0], "Packages"))
		packages = 1;
	    if (!strcmp(row[0], "Files"))
		files = 1;
	    if (!strcmp(row[0], "Requires"))
		requires = 1;
	    if (!strcmp(row[0], "Provides"))
		provides = 1;
	    if (!strcmp(row[0], "Queries"))
		queries = 1;
	    if (!strcmp(row[0], "Distributions"))
		distributions = 1;
	    if (!strcmp(row[0], "Searches"))
		searches = 1;
	}
	mysql_free_result(result);
    }
    if(mysql_errno(sql)) {
	fprintf(stderr, "Error: %s\n", mysql_error(sql));
	return(-1);
    }

    if (!config) {
	fprintf(stderr, "Table Config disapeared: rebuilding it\n");
	if (!sql_rebuild_config())
	    rebuilt++;
    }
    if (!vendors) {
	fprintf(stderr, "Table Vendors disapeared: rebuilding it\n");
	if (!sql_rebuild_vendors())
	    rebuilt++;
    }
    if (!distribs) {
	fprintf(stderr, "Table Distribs disapeared: rebuilding it\n");
	if (!sql_rebuild_distribs())
	    rebuilt++;
    }
    if (!mirrors) {
	fprintf(stderr, "Table Mirrors disapeared: rebuilding it\n");
	if (!sql_rebuild_mirrors())
	    rebuilt++;
    }
    if (!metadata) {
	fprintf(stderr, "Table Metadata disapeared: rebuilding it\n");
	if (!sql_rebuild_metadata())
	    rebuilt++;
    }
    if (!packages) {
	fprintf(stderr, "Table Packages disapeared: rebuilding it\n");
	if (!sql_rebuild_packages())
	    rebuilt++;
    }
    if (!files) {
	fprintf(stderr, "Table Files disapeared: rebuilding it\n");
	if (!sql_rebuild_files())
	    rebuilt++;
    }
    if (!requires) {
	fprintf(stderr, "Table Requires disapeared: rebuilding it\n");
	if (!sql_rebuild_requires())
	    rebuilt++;
    }
    if (!provides) {
	fprintf(stderr, "Table Provides disapeared: rebuilding it\n");
	if (!sql_rebuild_provides())
	    rebuilt++;
    }
    if (!queries) {
	fprintf(stderr, "Table Queries disapeared: rebuilding it\n");
	if (!sql_rebuild_queries())
	    rebuilt++;
    }
    if (!distributions) {
	fprintf(stderr, "Table Distributions disapeared: rebuilding it\n");
	if (!sql_rebuild_distributions())
	    rebuilt++;
    }
    if (!searches) {
	fprintf(stderr, "Table Searches disapeared: rebuilding it\n");
	if (!sql_rebuild_searches())
	    rebuilt++;
    }
    return(rebuilt);
}

/************************************************************************
 *									*
 *			Specific rpm2html functions			*
 *									*
 ************************************************************************/

int sql_add_dist_mirror(int distrib, const char *URL, int country) {
    if (URL == NULL)
	return(-1);
    if (distrib < 0)
	return(-1);
    return(sql_blind_insert("Mirrors", "URL", URL, distrib));
}

int sql_add_mirror(const char *Name, const char *URL, int country) {
    int distrib;

    if ((Name == NULL) || (URL == NULL))
	return(-1);
    distrib = sql_read_key("Distribs", Name);
    if (distrib < 0)
	return(distrib);
    return(sql_blind_insert("Mirrors", "URL", URL, distrib));
}

int sql_add_file(const char *filename, int package) {
    if ((filename == NULL) || (package <= 0))
	return(-1);
    if (strlen(filename) > 35)
	return(0);

    return(sql_blind_insert("Files", "Path", filename, package));
}

int sql_add_provides(int package, const char *resource) {
    if ((resource == NULL) || (package <= 0))
	return(-1);
    if (strlen(resource) > 35)
	return(0);

    return(sql_blind_insert("Provides", "Resource", resource, package));
}

int sql_add_requires(int package, const char *resource, rpm_dep_flag rel,
	             const char *value) {
    int record;
    if ((resource == NULL) || (package <= 0))
	return(-1);
    if (strlen(resource) > 35)
	return(0);

    record = sql_blind_insert("Requires", "Resource", resource, package);
    if ((rel != RPM2HTML_REQ_NONE) && (value != NULL) &&
	(strlen(value) <= 50)) {
	char query[SMALL_QUERY];

	switch (rel) {
	    case RPM2HTML_REQ_LT:
		snprintf(query, SMALL_QUERY - 1,
			 "UPDATE Requires SET Rel='<',Value='%s' WHERE ID=%d",
			 value, record);
	        break;
	    case RPM2HTML_REQ_LEQ:
		snprintf(query, SMALL_QUERY - 1,
			 "UPDATE Requires SET Rel='<=',Value='%s' WHERE ID=%d",
			 value, record);
	        break;
	    case RPM2HTML_REQ_GT:
		snprintf(query, SMALL_QUERY - 1,
			 "UPDATE Requires SET Rel='>',Value='%s' WHERE ID=%d",
			 value, record);
	        break;
	    case RPM2HTML_REQ_GEQ:
		snprintf(query, SMALL_QUERY - 1,
			 "UPDATE Requires SET Rel='>=',Value='%s' WHERE ID=%d",
			 value, record);
	        break;
	    case RPM2HTML_REQ_EQU:
		snprintf(query, SMALL_QUERY - 1,
			 "UPDATE Requires SET Rel='=',Value='%s' WHERE ID=%d",
			 value, record);
	        break;
	    case RPM2HTML_REQ_NONE:
		query[0] = 0;
	}
	query[SMALL_QUERY - 1] = 0;
	if (mysql_query(sql,query)) {
	    printf("sql_add_requires: UPDATE Requires %d failed %s\n",
		   record, mysql_error(sql));
	    return(record);
	}
    }
    return(record);
}

int sql_add_vendor(const char *Name, const char *URL, const char *Description) {
    int id;

    if (Name == NULL)
	return(-1);

    id = sql_get_key("Vendors", "Name", Name);
    if (id <= 0) {
	id = sql_blind_insert("Vendors", "Name", Name, 0);
	if (id <= 0)
	    return(id);
    }
    if (URL != NULL)
	sql_update_id("Vendors", id, "URL", URL);
    if (Description != NULL)
	sql_update_id("Vendors", id,
				   "Description", Description);
    
    return(id);
}

int sql_add_distribution(const char *Name, const char *URL, const char *Description) {
    int id;

    if (Name == NULL)
	return(-1);

    id = sql_get_key("Distributions", "Name", Name);
    if (id <= 0) {
	id = sql_blind_insert("Distributions", "Name", Name, 0);
	if (id <= 0)
	    return(id);
    }
    if (URL != NULL)
	sql_update_id("Distributions", id, "URL", URL);
    if (Description != NULL)
	sql_update_id("Distributions", id,
				   "Description", Description);
    
    return(id);
}

int sql_get_package_id(const char *filename) {
    int id;
    MYSQL_RES *result;
    MYSQL_ROW row;
    char query[MAX_QUERY];
    int left = MAX_QUERY - 1;
    int len;
    char *end;
    static int queries = 0;

    if (filename == NULL)
	return(0);

    queries++;

    /*
     * Search first for the ID if it already exists
     */
    len = snprintf(query, left, "SELECT ID FROM Packages WHERE filename='%s'",
	           filename);
    if (len < 0) {
	fprintf(stderr, "sql_get_package_id : %s overflow\n",
		filename);
	return(0);
    }
    query[len] = 0;

    if (mysql_query(sql,query)) {
	printf("sql_get_package_id: SELECT %s failed %s\n", filename,
	       mysql_error(sql));
	return(0);
    }

    result = mysql_use_result(sql);
    if (result) {
	if ((row = mysql_fetch_row(result)))
	{
	    /*
	     * Lookup the first ID and return it
	     */
	    if (row[0] == NULL) {
		mysql_free_result(result);
		printf("sql_get_package_id: select returns NULL !\n");
		return(0);
	    }
	    if (sscanf(row[0], "%d", &id) != 1) {
		mysql_free_result(result);
		printf("sql_get_package_id: ID non numeric %s\n", row[0]);
		return(0);
	    }
	    mysql_free_result(result);
	    return(id);
	}
	mysql_free_result(result);
    }
    if (mysql_errno(sql)) {
	fprintf(stderr, "Error: %s\n", mysql_error(sql));
	return(-1);
    }

    printf("%d New package %s\n", queries, filename);
    return(0);
}

int sql_get_top_queries(int count) {
    int id;
    MYSQL_RES *result;
    MYSQL_ROW row;
    char query[MAX_QUERY];
    int left = MAX_QUERY - 1;
    int len;
    char *end;
    static int queries = 0;

    if (count <= 0)
	count = 100;

    queries++;

    /*
     * Search first for the ID if it already exists
     */
    len = snprintf(query, left,
	    "select Value, Count from Queries order by count desc limit %d",
	           count);
    if (len < 0) {
	fprintf(stderr, "sql_get_top_queries : overflow\n");
	return(0);
    }
    query[len] = 0;

    if (mysql_query(sql,query)) {
	printf("sql_get_top_queries : SELECT failed %s\n", mysql_error(sql));
	return(0);
    }

    result = mysql_use_result(sql);
    if (result) {
	printf("<queries>\n");
	while ((row = mysql_fetch_row(result)))
	{
	    /*
	     * Lookup the first ID and return it
	     */
	    if ((row[0] == NULL) || (row[1] == NULL)) {
		mysql_free_result(result);
		fprintf(stderr, "sql_get_top_queries: select returns NULL !\n");
		break;
	    }
	    printf("<query occur='%s'>%s</query>\n", row[1], row[0]);
	}
	printf("</queries>\n");
	mysql_free_result(result);
    }
    if (mysql_errno(sql)) {
	fprintf(stderr, "Error: %s\n", mysql_error(sql));
	return(-1);
    }
    return(0);
}
int sql_add_package(const char *filename,
	const char *Name, const char *Version, const char *Release,
	const char *Arch,
	int dist, const char *URL, const char *URLSrc, int vendor,
	const char *Packager, const char *Category, const char *Summary,
	const char *Description, const char *Copyright, int Date, int Size,
	const char *Os, const char *Distribution, const char *Vendor) {
    int id;
    int nb_fields = 0;
    char installed_filename[500];

    if (filename == NULL) {
	sprintf(installed_filename, "localbase/%s-%s-%s.%s.rpm",
		Name,Version,Release,Arch);
	filename = installed_filename;
    }

    if (dist < 0)
	return(-1);
    if ((Name == NULL) || (Version == NULL) || (Release == NULL) ||
	(Arch == NULL))
	return(-1);

    id = sql_get_key("Packages", "filename", filename);
    if (id <= 0)
	return(-1);
    nb_fields = 1;

    if (rpm2htmlVerbose > 1)
	printf("Adding %s ID %d\n", filename, id);

    if (Name != NULL)
	nb_fields += sql_update_id("Packages", id, "Name", Name);
    if (Version != NULL)
	nb_fields += sql_update_id("Packages", id, "Version", Version);
    if (Release != NULL)
	nb_fields += sql_update_id("Packages", id, "Release", Release);
    if (Release != NULL)
	nb_fields += sql_update_id("Packages", id, "Arch", Arch);
    if (Category != NULL)
	nb_fields += sql_update_id("Packages", id, "Category", Category);
    if (URL != NULL)
	nb_fields += sql_update_id("Packages", id, "URL", URL);
    if (URLSrc != NULL)
	nb_fields += sql_update_id("Packages", id, "URLSrc", URLSrc);
    if (dist >= 0) {
	char str[30];
	snprintf(str, 30, "%d", dist);
	str[29] = 0;
	nb_fields += sql_update_id("Packages", id, "Dist", str);
    }
    if (Summary != NULL) {
	nb_fields += sql_update_id("Packages", id, "Summary", Summary);
    }
    if (Description != NULL) {
	nb_fields += sql_update_id("Packages", id,
				   "Description", Description);
    }
    if (Copyright != NULL)
	nb_fields += sql_update_id("Packages", id, "Copyright", Copyright);
    if (Date != 0) {
	char str[30];
	snprintf(str, 30, "%d", Date);
	str[29] = 0;
	nb_fields += sql_update_id("Packages", id, "Date", str);
    }
    if (Size != 0) {
	char str[30];
	snprintf(str, 30, "%d", Size);
	str[29] = 0;
	nb_fields += sql_update_id("Packages", id, "Size", str);
    }
    if (Os != NULL) {
	nb_fields += sql_update_id("Packages", id, "Os", Os);
    }
    if (Packager != NULL) {
	char str[30];
	int packager = sql_add_vendor(Packager, NULL, NULL);

	if (packager > 0) {
	    snprintf(str, 30, "%d", packager);
	    str[29] = 0;
	    nb_fields += sql_update_id("Packages", id, "Packager", str);
	}
    }
    if (Distribution != NULL) {
	char str[30];
	int packager = sql_add_distribution(Distribution, NULL, NULL);

	if (packager > 0) {
	    snprintf(str, 30, "%d", packager);
	    str[29] = 0;
	    nb_fields += sql_update_id("Packages", id, "Vendor", str);
	}
    }
    /***************
    if (vendor > 0) {
	char str[30];
	snprintf(str, 30, "%d", vendor);
	str[29] = 0;
	nb_fields += sql_update_id("Packages", id, "Vendor", str);
    }
     ***************/
    
    return(id);
}

int sql_get_distrib_by_name(const char *Name) {
    int ret;

    if (Name == NULL)
	return(-1);
    ret = sql_read_key("Distribs", Name);

#ifdef SQL_DEBUG
fprintf(stderr, "sql_get_distrib_by_name(%s) => %d\n", Name, ret);
#endif

    return(ret);
}

int sql_get_distrib_by_directory(const char *Directory) {
    int ret;

    if (Directory == NULL)
	return(-1);

    ret = sql_get_key("Distribs", "Directory", Directory);

#ifdef SQL_DEBUG
fprintf(stderr, "sql_get_distrib_by_directory(%s) => %d\n", Directory, ret);
#endif

    return(ret);
}

int sql_add_distrib(const char *Name, const char *Vendor,
	const char *Directory, const char *Path, const char *URL,
	const char *URLSrc, const char *Description,
	const char *Html, const char *Color) {
    int id, vendor;
    int nb_fields = 0;
    char VendorStr[15];

    if (Name == NULL)
	return(-1);

    id = sql_get_key("Distribs", "Name", Name);
    nb_fields = 1;
    if (Vendor != NULL) {
	vendor = sql_get_key("Vendors", "Name", Vendor);
	sprintf(VendorStr, "%d", vendor);
	nb_fields += sql_update_id("Distribs", id, "Vendor", VendorStr);
    }
    if (Directory != NULL)
	nb_fields += sql_update_id("Distribs", id, "Directory", Directory);
    if (Path != NULL)
	nb_fields += sql_update_id("Distribs", id, "Path", Path);
    if (URL != NULL)
	nb_fields += sql_update_id("Distribs", id, "URL", URL);
    if (URLSrc != NULL)
	nb_fields += sql_update_id("Distribs", id, "URLSrc", URLSrc);
    if (Html != NULL)
	nb_fields += sql_update_id("Distribs", id, "Html", Html);
    if (Color != NULL)
	nb_fields += sql_update_id("Distribs", id, "Color", Color);
    if (Description != NULL)
	nb_fields += sql_update_id("Distribs", id,
				   "Description", Description);
    
    return(nb_fields);
}

void sql_add_config_info(const char *name, const char *value) {
    sql_update("Config", name, "Value", value);
}

void sql_add_metadata_base(const char *URL) {
    sql_blind_insert("Metadata", "URL", URL, -1);
}

/************************************************************************
 * 									*
 * 			Cleanup functions				*
 * 									*
 ************************************************************************/

int sql_remove_package(int id) {
    char query[SMALL_QUERY];

    if (id <= 0)
	return(-1);

    /*
     * remove the ID from the package list
     */
    snprintf(query, SMALL_QUERY - 1, "DELETE FROM Packages WHERE ID=%d", id);
    query[SMALL_QUERY - 1] = 0;
    if (mysql_query(sql,query)) {
	printf("sql_remove_package: DELETE package %d failed %s\n", id, mysql_error(sql));
	return(-1);
    }
    if(mysql_errno(sql)) {
	fprintf(stderr, "Error: %s\n", mysql_error(sql));
	return(-1);
    }

    /*
     * remove the associated files from the Files list
     */
    snprintf(query, SMALL_QUERY - 1, "DELETE FROM Files WHERE ID=%d", id);
    query[SMALL_QUERY - 1] = 0;
    if (mysql_query(sql,query)) {
	printf("sql_remove_package: DELETE Files %d failed %s\n", id, mysql_error(sql));
	return(-1);
    }
    if(mysql_errno(sql)) {
	fprintf(stderr, "Error: %s\n", mysql_error(sql));
	return(-1);
    }

    /*
     * remove the associated Provides entries
     */
    snprintf(query, SMALL_QUERY - 1, "DELETE FROM Provides WHERE ID=%d", id);
    query[SMALL_QUERY - 1] = 0;
    if (mysql_query(sql,query)) {
	printf("sql_remove_package: DELETE Provides %d failed %s\n", id, mysql_error(sql));
	return(-1);
    }
    if(mysql_errno(sql)) {
	fprintf(stderr, "Error: %s\n", mysql_error(sql));
	return(-1);
    }

    /*
     * remove the associated Requires entries
     */
    snprintf(query, SMALL_QUERY - 1, "DELETE FROM Requires WHERE ID=%d", id);
    query[SMALL_QUERY - 1] = 0;
    if (mysql_query(sql,query)) {
	printf("sql_remove_package: DELETE Requires %d failed %s\n", id, mysql_error(sql));
	return(-1);
    }
    if(mysql_errno(sql)) {
	fprintf(stderr, "Error: %s\n", mysql_error(sql));
	return(-1);
    }

    /*
     * Remove the RDF and HTML files.
     * TODO
     */
    return(-1);
}

int sql_check_packages(void) {
    MYSQL_RES *result;
    MYSQL_ROW row;
    char *query = "SELECT filename,ID FROM Packages";
    struct stat buf;
    int id;
    int ids[2500]; /* Do not remove more than 2500 package per run */
    int index;
    int total = 0;

    if (rpm2htmlVerbose)
	printf("Database cleanup\n");
    sqlInitSqlDistributionList();
    index = 0;
    /*
     * Search first for the ID if it already exists
     */
    if (mysql_query(sql,query)) {
	printf("sql_check_packages: SELECT from Packages failed: %s\n",
	       mysql_error(sql));
	return(-1);
    }

    result = mysql_use_result(sql);
    if (result) {
	while((row = mysql_fetch_row(result)))
	{
	    if ((row[0] == NULL) || (row[1] == NULL)) {
		printf("sql_check_packages: Path or ID is NULL !\n");
		continue;
	    }
	    if (!strncmp(row[0], "localbase", 9))
		continue;

	    if ((stat(row[0], &buf) < 0) || (buf.st_size < 50)) {
		/*
		 * Need to remove the package from the database.
		 */
		if (sscanf(row[1], "%d", &id) != 1) {
		    printf("sql_check_packages: ID non numeric %s\n", row[1]);
		    continue;
		}
		ids[index++] = id;
		if (rpm2htmlVerbose)
		    printf("Removing %s ID %d\n", row[0], id);
		if (index >= 2500)
		    break;
	    }
	}
	mysql_free_result(result);
    }
    if(mysql_errno(sql)) {
	fprintf(stderr, "sql_update Error: %s\n", mysql_error(sql));
	return(-1);
    }

    /*
     * Do the cleanup.
     */
    if (rpm2htmlVerbose)
	printf("Database cleanup : removing %d entries...\n", index);
    for (id = 0;id < index;id++) {
	sql_remove_package(ids[id]);
    }

    printf("Database cleanup : removed %d entries\n", index);
    return(total);
}

/************************************************************************
 *									*
 *			Package lists extraction			*
 *									*
 ************************************************************************/
int sqlDirListInitialized = 0;
rpmDirPtr sqlDirList[SQL_MAX_DISTRIBS];
int sqlVendorListInitialized = 0;
int sqlVendorListLen;
char **sqlVendorList;
int sqlDistributionListInitialized = 0;
int sqlDistributionListLen;
char **sqlDistributionList;
int sqlResourceListInitialized = 0;
int sqlResourceListLen = 0;
int sqlResourceListMax = 0;
char **sqlResourceList;

char *
sqlRpmAnalyzeResourceRow(MYSQL_ROW row) {
    const char *name;
    char **tmp, *ret;

    if (row == NULL)
	return(NULL);
    name = row[0];
    if (name == NULL)
	return(NULL);

    if (sqlResourceListLen >= sqlResourceListMax) {
	sqlResourceListMax *= 2;
	tmp = (char **) xmlRealloc(sqlResourceList,
		                     (sqlResourceListMax) * sizeof(char *));
	if (sqlResourceList == NULL) {
	    fprintf(stderr, "sqlInitSqlResourceList Error: out of memory\n");
	    return(NULL);
	}
	sqlResourceList = tmp;
    }

    ret = sqlResourceList[sqlResourceListLen++] = xmlStrdup(name);
    return(ret);
}

void
sqlInitSqlResourceList(void) {
    MYSQL_RES *result;
    MYSQL_ROW row;
    char *query;
    int i;

    if (sqlResourceListInitialized != 0)
	return;

#ifdef SQL_DEBUG_MEM
    printf("sqlInitSqlResourceList\n");
#endif
    if (rpm2htmlVerbose)
	printf("sqlInitSqlResourceList query\n");

    /*
     * Allocate the array
     */
    sqlResourceListLen = 0;
    sqlResourceListMax = 1000;
    sqlResourceList = (char **) xmlMalloc((sqlResourceListMax) *
	                                    sizeof(char *));
    if (sqlResourceList == NULL) {
	fprintf(stderr, "sqlInitSqlResourceList Error: out of memory\n");
	return;
    }
    
    /*
     * query the database for the values
     */
    query = "select Resource from Provides group by Resource";

    if (mysql_query(sql,query)) {
	printf("sqlInitSqlResourceList: SELECT from Provides failed: %s\n",
	       mysql_error(sql));
	return;
    }

    i = 0;
    result = mysql_use_result(sql);
    if (result) {
	while((row = mysql_fetch_row(result)))
	{
	    if (sqlRpmAnalyzeResourceRow(row) != NULL)
		i++;
	}
	mysql_free_result(result);
    }
    if(mysql_errno(sql)) {
	fprintf(stderr, "sqlInitSqlResourceList Error: %s\n", mysql_error(sql));
	return;
    }


    if (rpm2htmlVerbose)
	printf("sqlInitSqlResourceList done: %d entries\n", i);
    sqlResourceListInitialized++;
    return;
}

char *
sqlRpmAnalyzeDistributionRow(MYSQL_ROW row) {
    const char *id;
    const char *name;
    char **tmp;
    int distribution;

    if (row == NULL)
	return(NULL);
    id = row[0];
    name = row[1];
    if ((id == NULL) || (name == NULL))
	return(NULL);

    if (sscanf(id, "%d", &distribution) != 1)
	return(NULL);
    if ((distribution <= 0) || (distribution > 100000)) {
	fprintf(stderr, "Dist number out of range %d\n", distribution);
	return(NULL);
    }
    if (distribution >= sqlDistributionListLen) {
	int i = sqlDistributionListLen;
	sqlDistributionListLen = distribution + 100;
	tmp = (char **) xmlRealloc(sqlDistributionList,
		                     (sqlDistributionListLen) * sizeof(char *));
	if (sqlDistributionList == NULL) {
	    fprintf(stderr, "sqlInitSqlDistributionList Error: out of memory\n");
	    return(NULL);
	}
	sqlDistributionList = tmp;
	memset(&sqlDistributionList[i], 0, sizeof(char *) * (sqlDistributionListLen - i));
    }

    sqlDistributionList[distribution] = xmlStrdup(name);
    return(sqlDistributionList[distribution]);
}

void
sqlInitSqlDistributionList(void) {
    MYSQL_RES *result;
    MYSQL_ROW row;
    char *query;
    int i;

    if (sqlDistributionListInitialized != 0)
	return;

#ifdef SQL_DEBUG_MEM
    printf("sqlInitSqlDistributionList\n");
#endif
    if (rpm2htmlVerbose)
	printf("sqlInitSqlDistributionList query\n");

    /*
     * Allocate the array
     */
    sqlDistributionListLen = 100;
    sqlDistributionList = (char **) xmlMalloc((sqlDistributionListLen) * sizeof(char *));
    if (sqlDistributionList == NULL) {
	fprintf(stderr, "sqlInitSqlDistributionList Error: out of memory\n");
	return;
    }
    memset(sqlDistributionList, 0, (sqlDistributionListLen) * sizeof(char *));

    /*
     * query the database for the values
     */
    query = "select ID,Name from Distributions";

    if (mysql_query(sql,query)) {
	printf("sqlInitSqlDistributionList: SELECT from Distributions failed: %s\n",
	       mysql_error(sql));
	return;
    }

    i = 0;
    result = mysql_use_result(sql);
    if (result) {
	while((row = mysql_fetch_row(result)))
	{
	    if (sqlRpmAnalyzeDistributionRow(row) != NULL)
		i++;
	}
	mysql_free_result(result);
    }
    if(mysql_errno(sql)) {
	fprintf(stderr, "sqlInitSqlDistributionList Error: %s\n", mysql_error(sql));
	return;
    }


    if (rpm2htmlVerbose)
	printf("sqlInitSqlDistributionList done: %d entries\n", i);
    sqlDistributionListInitialized++;
    return;
}


char *
sqlRpmAnalyzeVendorRow(MYSQL_ROW row) {
    const char *id;
    const char *name;
    char **tmp;
    int vendor;

    if (row == NULL)
	return(NULL);
    id = row[0];
    name = row[1];
    if ((id == NULL) || (name == NULL))
	return(NULL);

    if (sscanf(id, "%d", &vendor) != 1)
	return(NULL);
    if ((vendor <= 0) || (vendor > 100000)) {
	fprintf(stderr, "Dist number out of range %d\n", vendor);
	return(NULL);
    }
    if (vendor >= sqlVendorListLen) {
	int i = sqlVendorListLen;
	sqlVendorListLen = vendor + 100;
	tmp = (char **) xmlRealloc(sqlVendorList,
		                     (sqlVendorListLen) * sizeof(char *));
	if (sqlVendorList == NULL) {
	    fprintf(stderr, "sqlInitSqlVendorList Error: out of memory\n");
	    return(NULL);
	}
	sqlVendorList = tmp;
	memset(&sqlVendorList[i], 0, sizeof(char *) * (sqlVendorListLen - i));
    }

    sqlVendorList[vendor] = xmlStrdup(name);
    return(sqlVendorList[vendor]);
}

void
sqlInitSqlVendorList(void) {
    MYSQL_RES *result;
    MYSQL_ROW row;
    char *query;
    int i;

    if (sqlVendorListInitialized != 0)
	return;

#ifdef SQL_DEBUG_MEM
    printf("sqlInitSqlVendorList\n");
#endif
    if (rpm2htmlVerbose)
	printf("sqlInitSqlVendorList query\n");

    /*
     * Allocate the array
     */
    sqlVendorListLen = 100;
    sqlVendorList = (char **) xmlMalloc((sqlVendorListLen) * sizeof(char *));
    if (sqlVendorList == NULL) {
	fprintf(stderr, "sqlInitSqlVendorList Error: out of memory\n");
	return;
    }
    memset(&sqlVendorList[0], 0, sizeof(char *) * sqlVendorListLen);
    
    /*
     * query the database for the values
     */
    query = "select ID,Name from Vendors";

    if (mysql_query(sql,query)) {
	printf("sqlInitSqlVendorList: SELECT from Vendors failed: %s\n",
	       mysql_error(sql));
	return;
    }

    i = 0;
    result = mysql_use_result(sql);
    if (result) {
	while((row = mysql_fetch_row(result)))
	{
	    if (sqlRpmAnalyzeVendorRow(row) != NULL)
		i++;
	}
	mysql_free_result(result);
    }
    if(mysql_errno(sql)) {
	fprintf(stderr, "sqlInitSqlVendorList Error: %s\n", mysql_error(sql));
	return;
    }


    if (rpm2htmlVerbose)
	printf("sqlInitSqlVendorList done: %d entries\n", i);
    sqlVendorListInitialized++;
    return;
}

rpmDirPtr
sqlRpmAnalyzeDirRow(MYSQL_ROW row) {
    rpmDirPtr ret;
    const char *id;
    const char *name;
    const char *vendor;
    const char *directory;
    const char *path;
    const char *url;
    const char *urlSrc;
    const char *html;
    const char *color;
    int dist;

    if (row == NULL)
	return(NULL);
    id = row[0];
    name = row[1];
    vendor = row[2];
    directory = row[3];
    path = row[4];
    url = row[5];
    urlSrc = row[6];
    html = row[7];
    color = row[8];
    if ((id == NULL) || (name == NULL) ||
        (directory == NULL) || (path == NULL) || (url == NULL))
	return(NULL);

    if (sscanf(id, "%d", &dist) != 1)
	return(NULL);
    if ((dist <= 0) || (dist > SQL_MAX_DISTRIBS)) {
	fprintf(stderr, "Dist number out of range %d\n", dist);
	return(NULL);
    }


    ret = (rpmDirPtr) xmlMalloc(sizeof(rpmDir));
    if (ret == NULL)
	return(NULL);
    memset(ret, 0, sizeof(rpmDir));
    /* Generic stuff */
    if (rpm2html_dir != NULL)
    ret->dir = xmlStrdup(rpm2html_dir);
    if (rpm2html_host != NULL)
    ret->host = xmlStrdup(rpm2html_host);
    if (rpm2html_maint != NULL)
    ret->maint = xmlStrdup(rpm2html_maint);
    if (rpm2html_mail != NULL)
    ret->mail = xmlStrdup(rpm2html_mail);
    if (rpm2html_url != NULL)
    ret->url = xmlStrdup(rpm2html_url);
    /* specific stuff */
    ret->no = dist;
    ret->rpmdir = xmlStrdup(directory);
    ret->subdir = xmlStrdup(path);
    ret->name = xmlStrdup(name);
    ret->ftp = xmlStrdup(url);
    if (color != NULL)
	ret->color = xmlStrdup(color);
    else
	ret->color = xmlStrdup("#ffffff");
    if (urlSrc != NULL)
	ret->ftpsrc = xmlStrdup(urlSrc);
    return(ret);
}

void
sqlInitSqlDirList(void) {
    char host[200];
    rpmDirPtr cur;
    MYSQL_RES *result;
    MYSQL_ROW row;
    char *query;
    int i;

    if (sqlDirListInitialized != 0)
	return;

#ifdef SQL_DEBUG_MEM
    printf("sqlInitSqlDirList\n");
#endif
    gethostname(host, sizeof(host));
    currentTime = time(NULL);
    rpm2html_rpm2html_thishost = &host[0];
    readConfigSql();
    if (rpm2html_host == NULL) {
	rpm2html_host = strdup(rpm2html_rpm2html_thishost);
    }
    for (i = 0; i < SQL_MAX_DISTRIBS;i++)
	 sqlDirList[i] = NULL;

    if (rpm2htmlVerbose)
	printf("sqlInitSqlDirList query\n");

    query = "select ID,Name,Vendor,Directory,Path,URL,URLSrc,Html,Color from Distribs";

    if (mysql_query(sql,query)) {
	printf("sqlInitSqlDirList: SELECT from Packages failed: %s\n",
	       mysql_error(sql));
	return;
    }

    i = 0;
    result = mysql_use_result(sql);
    if (result) {
	while((row = mysql_fetch_row(result)))
	{
	    cur = sqlRpmAnalyzeDirRow(row);
	    if (cur != NULL) {
		sqlDirList[cur->no] = cur;
		i++;
	    }
	}
	mysql_free_result(result);
    }
    if(mysql_errno(sql)) {
	fprintf(stderr, "sqlInitSqlDirList Error: %s\n", mysql_error(sql));
	return;
    }

    if (rpm2htmlVerbose)
	printf("sqlInitSqlDirList done: %d entries\n", i);
    sqlDirListInitialized++;
    return;
}

rpmDataPtr
sqlRpmAnalyzeRow(MYSQL_ROW row) {
    rpmDataPtr ret;
    const char *id;
    const char *name;
    const char *version;
    const char *release;
    const char *arch;
    const char *date;
    const char *summary;
    const char *filename;
    const char *distrib;
    const char *group;
    const char *os;
    const char *packager;
    const char *vendor;
    int dist;

    if (row == NULL)
	return(NULL);
    id = row[0];
    name = row[1];
    version = row[2];
    release = row[3];
    arch = row[4];
    date = row[5];
    summary = row[6];
    filename = row[7];
    distrib = row[8];
    group = row[9];
    os = row[10];
    packager = row[11];
    vendor = row[12];
    if ((id == NULL) || (name == NULL) || (version == NULL) ||
        (release == NULL) || (arch == NULL) || (date == NULL) ||
	(summary == NULL) || (filename == NULL) || (distrib == NULL))
	return(NULL);

    ret = (rpmDataPtr) xmlMalloc(sizeof(rpmData));
    if (ret == NULL)
	return(NULL);
    memset(ret, 0, sizeof(rpmData));
    ret->name = xmlStrdup(name);
    ret->version = xmlStrdup(version);
    ret->release = xmlStrdup(release);
    ret->arch = xmlStrdup(arch);
    ret->summary = xmlStrdup(summary);
    ret->name = xmlStrdup(name);
    ret->filename = xmlStrdup(filename);
    if (os == NULL)
	ret->os = xmlStrdup("Linux");
    else
	ret->os = xmlStrdup(os);
    if (group != NULL)
	ret->group = xmlStrdup(group);
    sscanf(date, "%d", (int *) &(ret->date));
    if (packager != NULL) {
	int tmp = 0;
	sscanf(packager, "%d", (int *) &tmp);
	if ((tmp > 0) && (tmp < sqlVendorListLen)) {
	    if (sqlVendorList[tmp] != NULL)
		ret->vendor = xmlStrdup(sqlVendorList[tmp]);
	}
    }
    if (vendor != NULL) {
	int tmp = 0;
	sscanf(vendor, "%d", (int *) &tmp);
	if ((tmp > 0) && (tmp < sqlDistributionListLen)) {
	    if (sqlVendorList[tmp] != NULL)
		ret->distribution = xmlStrdup(sqlDistributionList[tmp]);
	}
    }
    if (ret->vendor == NULL)
	ret->vendor =  xmlStrdup("Unknown");
    if (ret->distribution == NULL)
	ret->distribution =  xmlStrdup("Unknown");
    sscanf(distrib, "%d", &dist);
    ret->dir = sqlDirList[dist];
    if ((ret->dir != NULL) && (ret->dir->rpmdir != NULL)) {
	int len = strlen(ret->dir->rpmdir);
	if (!strncmp(ret->filename, ret->dir->rpmdir, len)) {
	    char *start, *end;
	    start = &(ret->filename[len]);
	    while (*start == '/') start++;
	    end = &start[strlen(start) - 1];
	    while ((end >= start) && (*end != '/')) end--;
            if (end > start) {
		char *tmp;
		tmp = xmlMalloc((end - start) + 1);
		if (tmp != NULL) {
		    strncpy(tmp, start, end - start);
		    tmp[end - start] = 0;
		    ret->subdir = tmp;
		}
	    }
	}
    }
    return(ret);
}

rpmDataPtr
sqlRpmByDate(void) {
    rpmDataPtr list = NULL;
    rpmDataPtr cur, last = NULL;
    MYSQL_RES *result;
    MYSQL_ROW row;
    char *query;

    sqlInitSqlDirList();
    sqlInitSqlVendorList();
    sqlInitSqlDistributionList();
    if (rpm2htmlVerbose)
	printf("sqlRpmByDate query\n");

    query = "select ID,Name,Version,Release,Arch,Date,Summary,filename,Dist,Category,Os,Packager,Vendor from Packages where Date IS NOT NULL  and Date < UNIX_TIMESTAMP() + 300000 order by Date desc limit 1000";

    /*
     * Search first for the ID if it already exists
     */
    if (mysql_query(sql,query)) {
	printf("sqlRpmByDate: SELECT from Packages failed: %s\n",
	       mysql_error(sql));
	return(NULL);
    }

    result = mysql_use_result(sql);
    if (result) {
	while((row = mysql_fetch_row(result)))
	{
	    cur = sqlRpmAnalyzeRow(row);
	    if (cur != NULL) {
		if (last == NULL)
		    list = cur;
		else
		    last->next = cur;
		last = cur;
	    }
	}
	mysql_free_result(result);
    }
    if(mysql_errno(sql)) {
	fprintf(stderr, "sqlRpmByDate Error: %s\n", mysql_error(sql));
	return(NULL);
    }

    if (rpm2htmlVerbose)
	printf("sqlRpmByDate done\n");
    return(list);
}

void 
sql_top_index(void) {
    sqlInitSqlDirList();
    dumpTopIndex(sqlDirList);
}

rpmDataPtr
sqlRpmAll(void) {
    rpmDataPtr list = NULL;
    rpmDataPtr cur, last = NULL;
    MYSQL_RES *result;
    MYSQL_ROW row;
    char *query;

    sqlInitSqlDirList();
    dumpTopIndex(sqlDirList);
    sqlInitSqlVendorList();
    sqlInitSqlDistributionList();
    if (rpm2htmlVerbose)
	printf("sqlRpmAll query\n");

    query = "select ID,Name,Version,Release,Arch,Date,Summary,filename,Dist,Category,Os,Packager,Vendor from Packages";

    /*
     * Search first for the ID if it already exists
     */
    if (mysql_query(sql,query)) {
	printf("sqlRpmByDate: SELECT from Packages failed: %s\n",
	       mysql_error(sql));
	return(NULL);
    }

    result = mysql_use_result(sql);
    if (result) {
	while((row = mysql_fetch_row(result)))
	{
	    cur = sqlRpmAnalyzeRow(row);
	    if (cur != NULL) {
		if (last == NULL)
		    list = cur;
		else
		    last->next = cur;
		last = cur;

		rpmAddSoftware(cur);
	    }
	}
	mysql_free_result(result);
    }
    if(mysql_errno(sql)) {
	fprintf(stderr, "sqlRpmByDate Error: %s\n", mysql_error(sql));
	return(NULL);
    }

    if (rpm2htmlVerbose)
	printf("sqlRpmAll done\n");
    return(list);
}

void sqlListsCleanup(void) {
    int i;

#ifdef SQL_DEBUG_MEM
    printf("sqlListsCleanup\n");
#endif
    if (sqlDirListInitialized) {
#ifdef SQL_DEBUG_MEM
	printf("sqlListsCleanup: freeing sqlDirList\n");
#endif
	for (i = 0 ; i < SQL_MAX_DISTRIBS; i++) {
	    if (sqlDirList[i] != NULL)
		rpmDirFree(sqlDirList[i]);
	    sqlDirList[i] = NULL;
	}
	sqlDirListInitialized = 0;
    }
    if (sqlVendorListInitialized) {
#ifdef SQL_DEBUG_MEM
	printf("sqlListsCleanup: freeing sqlVendorList\n");
#endif
	for (i = 1 ; i < sqlVendorListLen;i++) {
	    if (sqlVendorList[i] != NULL)
		xmlFree(sqlVendorList[i]);
	    sqlVendorList[i] = NULL;
	}
	xmlFree(sqlVendorList);
	sqlVendorListInitialized = 0;
    }
    if (sqlDistributionListInitialized) {
#ifdef SQL_DEBUG_MEM
	printf("sqlListsCleanup: freeing sqlDistributionList\n");
#endif
	for (i = 0 ; i < sqlDistributionListLen;i++) {
	    if (sqlDistributionList[i] != NULL)
		xmlFree(sqlDistributionList[i]);
	    sqlDistributionList[i] = NULL;
	}
	xmlFree(sqlDistributionList);
	sqlDistributionListInitialized = 0;
    }
    if (sqlResourceListInitialized) {
#ifdef SQL_DEBUG_MEM
	printf("sqlListsCleanup: freeing sqlResourceList\n");
#endif
	for (i = 0 ; i < sqlResourceListLen;i++) {
	    if (sqlResourceList[i] != NULL)
		xmlFree(sqlResourceList[i]);
	    sqlResourceList[i] = NULL;
	}
	xmlFree(sqlResourceList);
	sqlResourceListInitialized = 0;
    }
}

/************************************************************************
 * 									*
 * 			Export functions				*
 * 									*
 ************************************************************************/

void sql_dump_rdf_full_index(void) {
    MYSQL_RES *result;
    MYSQL_ROW row;
    char query[SMALL_QUERY];
    gzFile rdf;
    char *filename;
    char *dist;
    char *name;
    char *version;
    char *release;
    char *arch;
    char *summary;
    char *subdir;
    char *psubdir;
    char *rpmdir;
    char *start, *end;
    int distrib;
    int len;
    char path[500];

    if (!rpm2html_dump_rdf_resources)
	return;
    if (!rpm2html_rdf_resources_dir)
	return;
    if ((rpm2html_host == NULL) || (rpm2html_url == NULL))
	return;

    /*
     * Open the RDF resource descritpion.
     */
    mkdir(rpm2html_rdf_resources_dir, 0755);
    snprintf(path, 500, "%s/fullIndex.rdf.gz",
	     rpm2html_rdf_resources_dir);
    path[499] = 0;
    rdf = gzopen(path, "w9");
    if (rdf == NULL) {
	fprintf(stderr, "unable to write to %s\n", path);
	return;
    }
    gzprintf(rdf, "<?xml version=\"1.0\"?>\n");
    gzprintf(rdf, "<rdf:RDF xmlns:RDF=\"http://www.w3.org/TR/WD-rdf-syntax#\" xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\" xmlns:RPM=\"http://www.rpm.org/\">\n");

    /*
     * Query the database for the information providing the resource.
     */
    snprintf(query, SMALL_QUERY,
       "select filename,Dist,Name,Version,Release,Arch,Summary from Packages");
    query[SMALL_QUERY - 1] = 0;
    if (mysql_query(sql, query)) {
	printf("sql_dump_rdf_full_index: SELECT from Packages failed: %s\n",
	       mysql_error(sql));
        gzprintf(rdf, "</rdf:RDF>\n");
	fclose(rdf);
	return;
    }
    result = mysql_use_result(sql);
    if (result == NULL) {
	printf("sql_dump_rdf_full_index: SELECT from Packages failed: %s\n",
	       mysql_error(sql));
        gzprintf(rdf, "</rdf:RDF>\n");
	fclose(rdf);
	return;
    }
    while((row = mysql_fetch_row(result))) {
	filename = row[0];
	dist = row[1];
	name = row[2];
	version = row[3];
	release = row[4];
	arch = row[5];
	summary = row[6];
	if ((filename == NULL) || (dist == NULL) || (name == NULL) ||
	    (summary == NULL)) {
	    continue;
	}
	if (!strncmp(filename, "localbase", 9))
	    continue;
	if (sscanf(dist, "%d", &distrib) != 1)
	    continue;
	if ((distrib < 1) || (distrib >= SQL_MAX_DISTRIBS) ||
	    (sqlDirList[distrib] == NULL))
	    continue;

	subdir = sqlDirList[distrib]->subdir;
	rpmdir = sqlDirList[distrib]->rpmdir;
	len = strlen(rpmdir);
	if (strncmp(filename, rpmdir, len)) {
	    fprintf(stderr, "%s out of subdir %s\n", filename, subdir);
	    continue;
	}
	start = &(filename[len]);
	while (*start == '/') start++;
	end = &start[strlen(start) - 1];
	while ((end >= start) && (*end != '/')) end--;
	if (end <= start) {
	    psubdir = xmlStrdup("");
	} else {
	    psubdir = xmlMalloc((end - start) + 1);
	    if (psubdir == NULL) {
		fprintf(stderr, "Out of memory\n");
		continue;
	    }
	    strncpy(psubdir, start, end - start);
	    psubdir[end - start] = 0;
	}
        gzprintf(rdf, "  <rdf:Description about=\"%s/%s/%s-%s-%s.%s.rpm\">\n",
		sqlDirList[distrib]->ftp,psubdir,name,version,release,arch);
	gzprintf(rdf, "    <RPM:Name>%s</RPM:Name>\n", name);
	gzprintf(rdf, "    <RPM:Summary>%s</RPM:Summary>\n", 
		 convertXML(summary));
	gzprintf(rdf, "  </rdf:Description>\n");
	xmlFree(psubdir);
    }
    gzprintf(rdf, "</rdf:RDF>\n");
    gzclose(rdf);
    mysql_free_result(result);
    if(mysql_errno(sql)) {
	fprintf(stderr, "Error: %s\n", mysql_error(sql));
    }
}


void sql_dump_rdf(const char *resource) {
    MYSQL_RES *result;
    MYSQL_ROW row;
    char query[MAX_QUERY];
    FILE *rdf;
    char *filename;
    char *dist;
    char *name;
    char *version;
    char *release;
    char *arch;
    char *os;
    char *packager;
    char *date;
    char *size;
    char *vendorstr;
    char *subdir;
    char *psubdir;
    char *rpmdir;
    char *start, *end;
    int distrib;
    int vendor;
    int person;
    int len;
    char path[500];

    if (resource == NULL)
	return;
    if (resource[0] == '/')
	return;
    if (!rpm2html_dump_rdf_resources)
	return;
    if (!rpm2html_rdf_resources_dir)
	return;
    if ((rpm2html_host == NULL) || (rpm2html_url == NULL))
	return;

    /*
     * Open the RDF resource descritpion.
     */
    mkdir(rpm2html_rdf_resources_dir, 0755);
    snprintf(path, 500, "%s/%s.rdf",
	     rpm2html_rdf_resources_dir, resource);
    path[499] = 0;
    rdf = fopen(path, "w");
    if (rdf == NULL) {
	fprintf(stderr, "unable to write to %s\n", path);
	return;
    }
    fprintf(rdf, "<?xml version=\"1.0\"?>\n");
    fprintf(rdf, "<rdf:RDF xmlns:RDF=\"http://www.w3.org/TR/WD-rdf-syntax#\" xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\" xmlns:RPM=\"http://www.rpm.org/\">\n");

    /*
     * Query the database for the information providing the resource.
     */
    snprintf(query, MAX_QUERY,
"select Packages.filename,Packages.Dist,Packages.Name,Packages.Version,Packages.Release,Packages.Arch,Packages.Os,Packages.Packager,Packages.Date,Packages.Size,Packages.Vendor from Packages,Provides where Provides.Resource=\"%s\" and Provides.ID = Packages.ID",
             resource);
    query[MAX_QUERY - 1] = 0;
    if (mysql_query(sql, query)) {
	printf("sql_dump_rdf: SELECT from Packages failed: %s\n",
	       mysql_error(sql));
        fprintf(rdf, "</rdf:RDF>\n");
	fclose(rdf);
	return;
    }
    result = mysql_use_result(sql);
    if (result == NULL) {
	printf("sql_dump_rdf: SELECT from Packages failed: %s\n",
	       mysql_error(sql));
        fprintf(rdf, "</rdf:RDF>\n");
	fclose(rdf);
	return;
    }
    while((row = mysql_fetch_row(result))) {
	filename = row[0];
	dist = row[1];
	name = row[2];
	version = row[3];
	release = row[4];
	arch = row[5];
	os = row[6];
	packager = row[7];
	date = row[8];
	size = row[9];
	vendorstr = row[10];
	if ((filename == NULL) || (dist == NULL) || (name == NULL) ||
	    (version == NULL) || (release == NULL) || (arch == NULL) ||
	    (os == NULL) || (date == NULL) || (vendorstr == NULL) ||
	    (size == NULL)) {
	    continue;
	}
	if (!strncmp(filename, "localbase", 9))
	    continue;
	if (sscanf(dist, "%d", &distrib) != 1)
	    continue;
	if ((distrib < 1) || (distrib >= SQL_MAX_DISTRIBS) ||
	    (sqlDirList[distrib] == NULL))
	    continue;
	if (vendorstr != NULL) {
	    if (sscanf(vendorstr, "%d", &vendor) != 1)
		continue;
	}
	if ((vendor < 1) || (vendor >= sqlDistributionListLen) ||
	    (sqlDistributionList[vendor] == NULL))
	    continue;
	if (packager != NULL) {
	    if (sscanf(packager, "%d", &person) != 1)
		continue;
	}
	if ((person < 1) || (person >= sqlVendorListLen) ||
	    (sqlVendorList[person] == NULL))
	    continue;

	subdir = sqlDirList[distrib]->subdir;
	rpmdir = sqlDirList[distrib]->rpmdir;
	len = strlen(rpmdir);
	if (strncmp(filename, rpmdir, len)) {
	    fprintf(stderr, "%s out of subdir %s\n", filename, subdir);
	    continue;
	}
	start = &(filename[len]);
	while (*start == '/') start++;
	end = &start[strlen(start) - 1];
	while ((end >= start) && (*end != '/')) end--;
	if (end <= start) {
	    psubdir = xmlStrdup("");
	} else {
	    psubdir = xmlMalloc((end - start) + 1);
	    if (psubdir == NULL) {
		fprintf(stderr, "Out of memory\n");
		continue;
	    }
	    strncpy(psubdir, start, end - start);
	    psubdir[end - start] = 0;
	}
        fprintf(rdf, "  <rdf:Description about=\"%s/%s/%s-%s-%s.%s.rpm\" ",
		sqlDirList[distrib]->ftp,psubdir,name,version,release,arch);
	fprintf(rdf, "href=\"../%s/%s/%s-%s-%s.%s.rdf\">\n",
	        subdir,psubdir,name,version,release,arch);
	fprintf(rdf, "    <RPM:Name>%s</RPM:Name>\n", name);
	fprintf(rdf, "    <RPM:Version>%s</RPM:Version>\n", version);
	fprintf(rdf, "    <RPM:Release>%s</RPM:Release>\n", release);
	fprintf(rdf, "    <RPM:Arch>%s</RPM:Arch>\n", arch);
	fprintf(rdf, "    <RPM:Os>%s</RPM:Os>\n", os);
	if (packager == NULL)
	    fprintf(rdf, "    <RPM:Distribution>Unknown</RPM:Distribution>\n");
	else
	    fprintf(rdf, "    <RPM:Distribution>%s</RPM:Distribution>\n",
		    convertXML(sqlDistributionList[vendor]));
	fprintf(rdf, "    <RPM:Vendor>%s</RPM:Vendor>\n",
	    convertXML(sqlVendorList[person]));
	fprintf(rdf, "    <RPM:Date>%s</RPM:Date>\n", date);
	fprintf(rdf, "    <RPM:Size>%s</RPM:Size>\n", size);
	fprintf(rdf, "    <RPM:Subdir>%s</RPM:Subdir>\n", subdir);
	fprintf(rdf, "  </rdf:Description>\n");
	xmlFree(psubdir);
    }
    fprintf(rdf, "</rdf:RDF>\n");
    fclose(rdf);
    mysql_free_result(result);
    if(mysql_errno(sql)) {
	fprintf(stderr, "Error: %s\n", mysql_error(sql));
    }
}

void sql_show_config(void) {
    MYSQL_RES *result;
    MYSQL_ROW row;
    int id, i;
    int index = 0;
    int ids[500];
    char query[SMALL_QUERY];


    printf(";\n; Configuration file for rpm2html\n");
    printf("; http://rpmfind.net/linux/rpm2html/\n;\n\n");
    mysql_query(sql,"SELECT Name,Value FROM Config");
    result = mysql_use_result(sql);

    while((row = mysql_fetch_row(result)))
    {
	if (row[0] == NULL) {
	    printf("\n");
	} else {
	    if (!strcmp(row[0], "maint"))
		printf("; maintainer of the local rpm mirror\n");
	    else if (!strcmp(row[0], "mail"))
		printf("; mail for the maintainer\n");
	    else if (!strcmp(row[0], "dir"))
                printf("; Directory to store the HTML pages produced\n");
	    else if (!strcmp(row[0], "url"))
                printf("; The relative URL for front pages\n");
	    else if (!strcmp(row[0], "header"))
                printf("; Extra link in the navigation bar\n");
	    else if (!strcmp(row[0], "html"))
                printf("; Export the local packages in HTML format\n");
	    else if (!strcmp(row[0], "tree"))
                printf("; Build the tree for the distributions\n");
	    else if (!strcmp(row[0], "rdf"))
                printf("; Export the local packages in RDF format\n");
	    else if (!strcmp(row[0], "rdf_dir"))
                printf("; Directory to store the RDf tree\n");
	    else if (!strcmp(row[0], "rdf_resources"))
                printf("; Compile a list of resources in RDF format\n");
	    else if (!strcmp(row[0], "rdf_resources_dir"))
                printf("; Directory to store the RDf resources tree\n");

	    if (row[1] == NULL)
		printf("%s\n\n", row[0]);
	    else
		printf("%s=%s\n\n", row[0], row[1]);
	}
    }
    mysql_free_result(result);
    if(mysql_errno(sql)) {
	fprintf(stderr, "Error: %s\n", mysql_error(sql));
    }

    /*
     * Dump the Metadata
     */
    printf(";\n; The metadata mirrors list\n;\n\n[metadata]\n");
    mysql_query(sql,"SELECT URL FROM Metadata");
    result = mysql_use_result(sql);

    while((row = mysql_fetch_row(result)))
    {
	if (row[0] != NULL) {
	    printf("mirror=%s\n", row[0]);
	}
    }
    mysql_free_result(result);
    if(mysql_errno(sql)) {
	fprintf(stderr, "Error: %s\n", mysql_error(sql));
    }

    /*
     * Dump the distributions information
     * 1/ collect the list of IDs for the distribs
     */
    printf("\n\n;\n; The distribution list\n;\n\n");
    mysql_query(sql,"SELECT ID FROM Distribs");
    result = mysql_use_result(sql);

    while((row = mysql_fetch_row(result)))
    {
	if (row[0] != NULL) {
	    if (sscanf(row[0], "%d", &id) == 1) {
		ids[index++] = id;
	    }
	}
    }
    mysql_free_result(result);

    /*
     * Dump each distribution separately.
     */
    for (i = 0;i < index;i++) {

	snprintf(query, SMALL_QUERY - 1,
 "SELECT Directory,Name,Vendor,Path,URL,URLSrc,Description,Html,Color \
		  FROM Distribs WHERE ID=%d", ids[i]);
	query[SMALL_QUERY - 1] = 0;
	if (mysql_query(sql,query)) {
	    printf("sql_show_config: SELECT Distrib %d failed: %s\n",
		   ids[i], mysql_error(sql));
	    continue;
	}

	result = mysql_use_result(sql);
	if (result) {
	    while((row = mysql_fetch_row(result)))
	    {
		if (row[0] == NULL)
		    break;
		printf("[%s]\n", row[0]);
		if (row[1] != NULL)
		    printf("name=%s\n", row[1]);
		if (row[3] != NULL)
		    printf("subdir=%s\n", row[3]);
		if (row[4] != NULL)
		    printf("ftp=%s\n", row[4]);
		if (row[5] != NULL)
		    printf("ftpsrc=%s\n", row[5]);
		if (row[7] != NULL)
		    printf("html=%s\n", row[7]);
		if (row[8] != NULL)
		    printf("color=%s\n", row[8]);
	    }
	}
	mysql_free_result(result);

	/*
	 * Extract the mirrors for this distribution.
	 */
	snprintf(query, SMALL_QUERY - 1,
		 "SELECT URL FROM Mirrors WHERE ID=%d", ids[i]);
	query[SMALL_QUERY - 1] = 0;
	if (mysql_query(sql,query)) {
	    printf("sql_show_config: SELECT Mirrors %d failed: %s\n",
		   ids[i], mysql_error(sql));
	    printf("\n\n");
	    continue;
	}
	result = mysql_use_result(sql);
	if (result) {
	    while((row = mysql_fetch_row(result)))
	    {
		if (row[0] == NULL)
		    continue;
		printf("mirror=%s\n", row[0]);
	    }
	}
	mysql_free_result(result);

	printf("\n\n");
    }

    printf(";\n; End of the configuration file for rpm2html\n;\n");
}

void sql_show_metadata(void) {
    MYSQL_RES *result;
    MYSQL_ROW row;


    mysql_query(sql,"SELECT URL FROM Metadata");
    result = mysql_use_result(sql);

    while((row = mysql_fetch_row(result)))
    {
	if (row[0] == NULL)
	    printf("NULL !\n");
	else {
	    printf("%s\n", row[0]);
	}
    }
    if(mysql_errno(sql)) {
	fprintf(stderr, "Error: %s\n", mysql_error(sql));
    }

}

void sql_show_mirrors(void) {
    MYSQL_RES *result;
    MYSQL_ROW row;


    mysql_query(sql,"SELECT URL FROM Mirrors");
    result = mysql_use_result(sql);

    while((row = mysql_fetch_row(result)))
    {
	if (row[0] == NULL)
	    printf("NULL !\n");
	else {
	    printf("%s\n", row[0]);
	}
    }
    if(mysql_errno(sql)) {
	fprintf(stderr, "Error: %s\n", mysql_error(sql));
    }
}

void sql_show_vendors(void) {
    MYSQL_RES *result;
    MYSQL_ROW row;


    mysql_query(sql,"SELECT Name, URL FROM Vendors");
    result = mysql_use_result(sql);

    while((row = mysql_fetch_row(result)))
    {
	if (row[0] == NULL)
	    printf("NULL !\n");
	else {
	    if (row[1] == NULL)
		printf("%s : no url\n", row[0]);
	    else
		printf("%s : %s\n", row[0], row[1]);
	}
    }
    if(mysql_errno(sql)) {
	fprintf(stderr, "Error: %s\n", mysql_error(sql));
    }

}

void sql_show_distribs(void) {
    MYSQL_RES *result;
    MYSQL_ROW row;


    mysql_query(sql,"SELECT Name, Path, URL FROM Distribs");
    result = mysql_use_result(sql);

    while((row = mysql_fetch_row(result)))
    {
	if (row[0] == NULL)
	    printf("NULL !\n");
	else {
	    if (row[1] == NULL)
		printf("%s : no Path\n", row[0]);
	    else {
		if (row[2] == NULL)
		    printf("%s : %s : no url\n", row[0], row[1]);
		else
		    printf("%s : %s : %s\n", row[0], row[1], row[2]);
	    }
	}
    }
    if(mysql_errno(sql)) {
	fprintf(stderr, "Error: %s\n", mysql_error(sql));
    }

}

int sql_show_table_stats(const char *table, const char *key) {
    char query[MAX_QUERY];
    MYSQL_RES *result;
    MYSQL_ROW row;
    int res;

    /*
     * Search first for the ID if it already exists
     */
    snprintf(query, MAX_QUERY - 1, "SELECT COUNT(%s) FROM %s", key, table);
    query[MAX_QUERY - 1] = 0;
    if (mysql_query(sql,query)) {
	printf("sql_show_table_stats: SELECT COUNT failed: %s\n",
	       mysql_error(sql));
	return(-1);
    }

    result = mysql_use_result(sql);
    if (result) {
	while((row = mysql_fetch_row(result)))
	{
	    /*
	     * Lookup the value and return it
	     */
	    if (row[0] == NULL) {
		mysql_free_result(result);
		printf("sql_show_table_stats: select count returns NULL !\n");
		return(-1);
	    }
	    if (sscanf(row[0], "%d", &res) != 1) {
		mysql_free_result(result);
		printf("sql_show_table_stats: value non numeric %s\n", row[0]);
		return(-1);
	    }
	    mysql_free_result(result);
	    if (res <= 0)
		printf("   %s is empty\n", table);
	    else
		printf("   %s contains %d records\n", table, res);
	    return(res);
	}
	mysql_free_result(result);
    }
    if(mysql_errno(sql)) {
	fprintf(stderr, "sql_show_stats error: %s\n", mysql_error(sql));
    }
    return(-1);
}

int sql_show_stats(void) {
    const char *query = "SHOW TABLES";
    MYSQL_RES *result;
    MYSQL_ROW row;

    int tables = 0;
    int records = 0;

    if (mysql_query(sql,query)) {
	printf("sql_check_tables: SHOW TABLES failed %s\n",
	       mysql_error(sql));
	return(-1);
    }

    result = mysql_use_result(sql);
    if (result) {
	while((row = mysql_fetch_row(result)))
	{
	    if (row[0] == NULL) {
		mysql_free_result(result);
		printf("sql_check_tables: SHOW TABLES returns NULL !\n");
		return(-1);
	    }
	    tables++;
	}
	mysql_free_result(result);
    }
    if(mysql_errno(sql)) {
	fprintf(stderr, "sql_show_stats error: %s\n", mysql_error(sql));
    }
    printf("%d tables in use\n", tables);
    records += sql_show_table_stats("Config", "Name");
    records += sql_show_table_stats("Distribs", "Name");
    records += sql_show_table_stats("Vendors", "Name");
    records += sql_show_table_stats("Mirrors", "URL");
    records += sql_show_table_stats("Metadata", "URL");
    records += sql_show_table_stats("Packages", "Name");
    records += sql_show_table_stats("Requires", "Resource");
    records += sql_show_table_stats("Provides", "Resource");
    records += sql_show_table_stats("Files", "Path");
    printf("Total: %d records\n", records);
    return(records);
}

void sql_show_latests(void) {
    rpmDataPtr list;

    list = sqlRpmByDate();
    if (list == NULL) {
	fprintf(stderr, "no package in database\n");
	return;
    }
    dumpRpmByDate(list, 0);
}


void sql_show_resources(void) {
    int i;

    sqlInitSqlDirList();
    sqlInitSqlResourceList();
    sqlInitSqlDistributionList();
    sqlInitSqlVendorList();
    for (i = 0;i < sqlResourceListLen;i++) {
	if (sqlResourceList[i] == NULL)
	    break;
	dumpRessRedirHtml(sqlResourceList[i]);
	if (rpm2html_dump_rdf_resources)
	    sql_dump_rdf(sqlResourceList[i]);
    }
    if (rpm2htmlVerbose)
	fprintf(stderr, "Dumped %d redirrect resource pages\n", i);
    if (rpm2html_dump_rdf_resources)
	sql_dump_rdf_full_index();
    sqlListsCleanup();
}

void sql_reindex(void) {
    int i;
    int pid, status;

    sqlInitSqlDirList();

#ifdef SQL_DEBUG
    printf("sql_reindex\n");
#endif
    if (sqlDirListInitialized) {
	for (i = 0 ; i < SQL_MAX_DISTRIBS;i++) {
	    if (sqlDirList[i] == NULL)
	        continue;
	    if (sqlDirList[i]->rpmdir != NULL) {
		printf("sqltools indexing: %s\n", sqlDirList[i]->name);
		pid = fork();
		if (pid < 0) {
		    fprintf(stderr, "fork failed \n");
		    continue;
		}
		if (pid == 0) {
		    printf("%s %s %s %s\n",
			   "./rpm2html", "-dir", sqlDirList[i]->rpmdir,
			   "./rpm2html-local.config");
		    execlp("./rpm2html",
			  "./rpm2html", "-dir", sqlDirList[i]->rpmdir,
			  "./rpm2html-local.config", NULL);
		    fprintf(stderr, "Unable to launch rpm2html\n");
		    exit(127);
		}
		do {
		    if (waitpid(pid, &status, 0) == -1) {
			if (errno != EINTR)
			    goto done;
		    } else {
			if (status != 0)
			    printf("indexing: %s exit %d\n",
				   sqlDirList[i]->name, status);
			break;
		    }
		} while(1);

	    }
	}
    }
done:
    sqlListsCleanup();
}

void sql_show_index(void) {
    sqlInitSqlDirList();
    sqlInitSqlDistributionList();
    if (rpm2html_dump_rdf_resources)
	sql_dump_rdf_full_index();
    sqlListsCleanup();
}

void sql_show_all(void) {
    rpmDataPtr list;
#ifdef SQL_DEBUG_TIMING
    time_t cur, last;

    last = time(NULL);
#endif
    list = sqlRpmAll();
    if (list == NULL) {
	fprintf(stderr, "no package in database\n");
	return;
    }
#ifdef SQL_DEBUG_TIMING
    cur = time(NULL);
    fprintf(stderr, "sqlRpmAll took %d seconds\n", (int)(cur - last));
    last = cur;
#endif

    rpmNameSort(&list, 0);
    dumpRpmByName(rpmSoftwareList, 0);

#ifdef SQL_DEBUG_TIMING
    cur = time(NULL);
    fprintf(stderr, "dumpRpmByName took %d seconds\n", (int)(cur - last));
    last = cur;
#endif

    rpmGroupSort(&list, 0);
    dumpRpmByGroups(rpmSoftwareList, 0);

#ifdef SQL_DEBUG_TIMING
    cur = time(NULL);
    fprintf(stderr, "dumpRpmByGroups took %d seconds\n", (int)(cur - last));
    last = cur;
#endif

    rpmVendorSort(&list, 0);
    dumpRpmByVendors(list, 0);

#ifdef SQL_DEBUG_TIMING
    cur = time(NULL);
    fprintf(stderr, "dumpRpmByVendors took %d seconds\n", (int)(cur - last));
    last = cur;
#endif

    rpmDistribSort(&list, 0);
    dumpRpmByDistribs(list, 0);

#ifdef SQL_DEBUG_TIMING
    cur = time(NULL);
    fprintf(stderr, "dumpRpmByDistribs took %d seconds\n", (int)(cur - last));
    last = cur;
#endif

    rpmDateSort(&list, 0);
    dumpRpmByDate(list, 0);

#ifdef SQL_DEBUG_TIMING
    cur = time(NULL);
    fprintf(stderr, "dumpRpmByDate took %d seconds\n", (int)(cur - last));
    last = cur;
#endif

    rpmdataCleanup();
    sleep(30);
}

/************************************************************************
 *									*
 *			rpm2html configuration functions		*
 *									*
 ************************************************************************/

int readConfigSql(void) {
    char query[MAX_QUERY];
    MYSQL_RES *result;
    MYSQL_ROW row;
    int dir = 0;

    /*
     * General configuration information
     */
    snprintf(query, MAX_QUERY - 1, "SELECT Name,Value FROM Config");
    query[MAX_QUERY - 1] = 0;
    if (mysql_query(sql,query)) {
	printf("sql_check_tables: SELECT Config failed %s\n",
	       mysql_error(sql));
	return(-1);
    }

    result = mysql_use_result(sql);
    if (result) {
	while((row = mysql_fetch_row(result)))
	{
	    if ((row[0] == NULL) || (row[1] == NULL)) {
		fprintf(stderr, "readConfigSql : found NULL value\n");
		continue;
	    }
	    if (!strcmp(row[0], "dir"))
		dir = 1;
	    addConfigEntry(RPM2HTML_NAME, row[0], row[1]);
	}
	mysql_free_result(result);
    }
    if(mysql_errno(sql)) {
	fprintf(stderr, "sql_show_stats error: %s\n", mysql_error(sql));
    }
    if (dir == 0) {
	fprintf(stderr, "readConfigSql : no directory\n");
	return(-1);
    }

    /*
     * The metadata mirror list.
     */
    snprintf(query, MAX_QUERY - 1, "SELECT URL FROM Metadata");
    query[MAX_QUERY - 1] = 0;
    if (mysql_query(sql,query)) {
	printf("sql_check_tables: SELECT Metadata failed %s\n",
	       mysql_error(sql));
	return(-1);
    }

    result = mysql_use_result(sql);
    if (result) {
	while((row = mysql_fetch_row(result)))
	{
	    if (row[0] == NULL) {
		fprintf(stderr, "readConfigSql : found NULL metadata\n");
		continue;
	    }
	    addConfigEntry("metadata", "mirror", row[0]);
	}
	mysql_free_result(result);
    }
    if(mysql_errno(sql)) {
	fprintf(stderr, "sql_show_stats error: %s\n", mysql_error(sql));
    }

    /*
     * The distribution lists
     */
    snprintf(query, MAX_QUERY - 1, "SELECT Directory,Name,URL,URLSrc,Path FROM Distribs");
    query[MAX_QUERY - 1] = 0;
    if (mysql_query(sql,query)) {
	printf("sql_check_tables: SELECT Distribs failed %s\n",
	       mysql_error(sql));
	return(-1);
    }

    result = mysql_use_result(sql);
    if (result) {
	while((row = mysql_fetch_row(result)))
	{
	    if (row[0] == NULL) {
		fprintf(stderr, "readConfigSql : found NULL distro\n");
		continue;
	    }

	    if (row[1] != NULL)
		addConfigEntry(row[0], "name", row[1]);
	    if (row[2] != NULL)
		addConfigEntry(row[0], "ftp", row[2]);
	    if (row[3] != NULL)
		addConfigEntry(row[0], "ftpsrc", row[3]);
	    if (row[4] != NULL)
		addConfigEntry(row[0], "subdir", row[4]);
	}
	mysql_free_result(result);
    }
    if(mysql_errno(sql)) {
	fprintf(stderr, "sql_show_stats error: %s\n", mysql_error(sql));
    }

    /*
     * The Mirrors
     */
    snprintf(query, MAX_QUERY - 1, "SELECT Distribs.Directory,Mirrors.URL FROM Distribs,Mirrors WHERE Distribs.ID = Mirrors.ID");
    query[MAX_QUERY - 1] = 0;
    if (mysql_query(sql,query)) {
	printf("sql_check_tables: SELECT Distribs failed %s\n",
	       mysql_error(sql));
	return(-1);
    }

    result = mysql_use_result(sql);
    if (result) {
	while((row = mysql_fetch_row(result)))
	{
	    if ((row[0] == NULL) || (row[1] == NULL)) {
		fprintf(stderr, "readConfigSql : found NULL mirror\n");
		continue;
	    }

	    addConfigEntry(row[0], "mirror", row[1]);
	}
	mysql_free_result(result);
    }
    if(mysql_errno(sql)) {
	fprintf(stderr, "sql_show_stats error: %s\n", mysql_error(sql));
    }

    /*
     * TODO: add the language(s) stuff
     */
    return(0);
}

void sqlConfigEntry(const char *rpmdir, const char *name, const char *value) {
    int distrib;

    if (rpm2htmlVerbose > 1)
	printf("sqlConfigEntry(\"%s\", \"%s\", \"%s\")\n", rpmdir, name, value);

    /*
     * case of global option for rpm2html.
     */
    if (!strcasecmp(rpmdir, RPM2HTML_NAME)) {
	sql_add_config_info(name, value);
	return;
    }

    /*
     * Options for the metadata mirrors.
     */
    if (!strcasecmp(rpmdir, "metadata")) {
	if (!strcasecmp(name, "mirror")) {
	    sql_add_metadata_base(value);
	} else {
	    printf("Config file : %s entry for [metadata] ignored\n", name);
	}
	return;
    }

    /*
     * option for a directory.
     */
    if (!strcasecmp(name, "name")) {
	sql_add_distrib(value, NULL, rpmdir, NULL, NULL, NULL, NULL,
		        NULL, NULL);
    } else if (!strcasecmp(name, "subdir")) {
	distrib = sql_read_info_key("Distribs", "Directory", rpmdir);
	if (distrib > 0)
            sql_update_id("Distribs", distrib, "Path", value);
	else
	    fprintf(stderr, "database has no distrib: %s",
		    rpmdir);
    } else if (!strcasecmp(name, "url")) {
	distrib = sql_read_info_key("Distribs", "Directory", rpmdir);
	if (distrib > 0)
            sql_update_id("Distribs", distrib, "URL", value);
	else
	    fprintf(stderr, "database has no distrib: %s",
		    rpmdir);
    } else if (!strcasecmp(name, "ftp")) {
	distrib = sql_read_info_key("Distribs", "Directory", rpmdir);
	if (distrib > 0)
            sql_update_id("Distribs", distrib, "URL", value);
	else
	    fprintf(stderr, "database has no distrib: %s",
		    rpmdir);
    } else if (!strcasecmp(name, "ftpsrc")) {
	distrib = sql_read_info_key("Distribs", "Directory", rpmdir);
	if (distrib > 0)
            sql_update_id("Distribs", distrib, "URLSrc", value);
	else
	    fprintf(stderr, "database has no distrib: %s",
		    rpmdir);
    } else if (!strcasecmp(name, "html")) {
	distrib = sql_read_info_key("Distribs", "Directory", rpmdir);
	if (distrib > 0)
            sql_update_id("Distribs", distrib, "Html", value);
	else
	    fprintf(stderr, "database has no distrib: %s",
		    rpmdir);
    } else if (!strcasecmp(name, "color")) {
	distrib = sql_read_info_key("Distribs", "Directory", rpmdir);
	if (distrib > 0)
            sql_update_id("Distribs", distrib, "Color", value);
	else
	    fprintf(stderr, "database has no distrib: %s",
		    rpmdir);
    } else if (!strcasecmp(name, "mirror")) {
	distrib = sql_read_info_key("Distribs", "Directory", rpmdir);
	if (distrib > 0)
	    sql_add_dist_mirror(distrib, value, 0);
	else
	    fprintf(stderr, "database has no distrib: %s",
		    rpmdir);
    } else {
	printf("Config file : %s entry for [%s] ignored\n", name, rpmdir);
    }
}

