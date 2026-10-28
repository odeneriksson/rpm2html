/*
 * html.h : resources concerning the dump as an HTML base.
 *
 * See Copyright for the status of this software.
 *
 * $Id: html.h,v 1.20 2000/12/10 18:19:52 veillard Exp $
 */

#ifndef __RPM_HTML_H__
#define __RPM_HTML_H__

extern void dumpDirIcon(void);
extern void createDirectory(const char *dirname);
extern void dumpAllRessHtml(int installed);
extern void dumpRpmHtml(rpmDataPtr rpm, rpmSubdirPtr tree);
extern void dumpRpmByGroups(rpmDataPtr list, int installed);
extern void dumpRpmByDistribs(rpmDataPtr list, int installed);
extern void dumpRpmByVendors(rpmDataPtr list, int installed);
extern void dumpRpmByDate(rpmDataPtr list, int installed);
extern void dumpRpmByName(rpmDataPtr list, int installed);
extern rpmDataPtr dumpDirIndex(rpmDirPtr dir, rpmSubdirPtr tree,
                               rpmDataPtr list);
extern void dumpIndex(time_t start_time, int installed);
extern void rpmDumpHtmlRealRoot(rpmDirPtr dir);
extern const char *rpmName(rpmDataPtr cur);
extern const char *resourceName(const char *resource);
extern int checkDate(const char *filename, time_t stamp);
extern void dumpRessRedirHtml(const char *name);
char *convertHTML(const char *str);
char *convertXML(const char *str);

#endif /* __RPM_HTML_H__ */
