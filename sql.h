/*
 * sql.h : header for the database fron-end of rpm2html
 */

#ifndef __RPM2HTML_SQL_H__
#define __RPM2HTML_SQL_H__
/*
 * Connection handling
 */
int init_sql(const char *host, const char *base, const char *user, const char *passwd);
int close_sql(void);

/*
 * Init stuff
 */
void sqlInitSqlDistributionList(void);
void sqlListsCleanup(void);

/*
 * Configuration I/O
 */
int readConfigSql(void);
void sqlConfigEntry(const char *rpmdir, const char *name, const char *value);

/*
 * Table handling
 */
int sql_check_tables(void);
int sql_rebuild_vendors(void);
int sql_rebuild_distribs(void);
int sql_rebuild_mirrors(void);
int sql_rebuild_files(void);
int sql_rebuild_provides(void);
int sql_rebuild_requires(void);

/*
 * display/dump
 */
void sql_show_vendors(void);
void sql_show_mirrors(void);
void sql_show_metadata(void);
void sql_show_config(void);
void sql_show_distribs(void);
int sql_show_stats(void);
void sql_show_resources(void);
void sql_show_index(void);
void sql_show_all(void);
void sql_top_index(void);

/*
 * Reindex
 */
void sql_reindex(void);

/*
 * Update of pages
 */
void sql_show_latests(void);
int sql_get_top_queries(int count);

/*
 * Insertion/Update of records
 */
int sql_get_package_id(const char *filename);
int sql_add_distrib(const char *Name, const char *Vendor, const char *Path,
	const char *Directory, const char *URL, const char *URLSrc,
	const char *Description, const char *Html, const char *Color);
int sql_get_distrib_by_name(const char *Name);
int sql_get_distrib_by_directory(const char *Directory);
int sql_add_vendor(const char *Name, const char *URL, const char *Description);
int sql_add_dist_mirror(int distrib, const char *URL, int country);
int sql_add_mirror(const char *Name, const char *URL, int country);
int sql_add_package(const char *filename,
	const char *Name, const char *Version, const char *Release,
	const char *Arch,
	int dist, const char *URL, const char *URLSrc, int vendor,
	const char *Packager, const char *Category, const char *Summary,
	const char *Description, const char *Copyright, int Date, int Size,
	const char *Os, const char *Distribution, const char *Vendor);
int sql_add_file(const char *filename, int package);
int sql_add_provides(int package, const char *resource);
int sql_add_requires(int package, const char *resource, rpm_dep_flag rel,
	             const char *value);

/*
 * Checking/Removal of records
 */
int sql_remove_package(int id);
int sql_check_packages(void);

#endif /* __RPM2HTML_SQL_H__ */
