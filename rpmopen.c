/*
 * rpmopen.c : open an extract information from RPM files.
 *
 * See Copyright for the status of this software.
 *
 * $Id: rpmopen.c,v 1.124 2010/11/09 21:57:42 hany Exp $
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
#include <dirent.h>
#include <errno.h>
#include <time.h>

#include <rpm/rpmlib.h>
#include <rpm/rpmmacro.h>
#include <rpm/rpmdb.h>
#include <rpm/rpmts.h>
#include <rpm/rpmtd.h>
#include <stdint.h>
#ifdef HAVE_RPM42
#include <rpm/rpmfi.h>
#endif

#if defined(WITH_GPG)
#include <sys/wait.h>
#endif

#include "rpm2html.h"
#include "rpmdata.h"
#include "html.h"
#include "rdf.h"
#include "language.h"
#ifdef WITH_SQL
#include "sql.h"
#endif

#ifndef HAVE_SNPRINTF
#error You really need snprintf ...
#endif

/*
 * The pre-rpmtd headerGetEntry() API returned a borrowed scalar pointer,
 * but a free()-able char ** for string arrays. Preserve that contract here.
 * Values remain valid until Header is released. Only pointer arrays are
 * allocated; individual string elements belong to librpm.
 */
static int
rpm2htmlHeaderGetEntry(Header h, rpmTagVal tag, int *type,
                       void **data, int *count)
{
    struct rpmtd_s td = {0};
    int i;

    *data = NULL;
    *type = RPM_NULL_TYPE;
    *count = 0;
    if (tag == RPMTAG_SUMMARY || tag == RPMTAG_DESCRIPTION ||
        tag == RPMTAG_GROUP) {
        const char *value = headerGetString(h, tag);
        if (value == NULL)
            return 0;
        *type = RPM_STRING_TYPE;
        *data = (void *)value;
        *count = 1;
        return 1;
    }
    if (!headerGet(h, tag, &td, HEADERGET_MINMEM | HEADERGET_RAW))
        return 0;
    *type = td.type;
    *count = td.count;
    if (td.type == RPM_STRING_ARRAY_TYPE || td.type == RPM_I18NSTRING_TYPE) {
        char **values = calloc(td.count + 1, sizeof(*values));
        if (values == NULL) {
            rpmtdFreeData(&td);
            return 0;
        }
        for (i = 0; i < td.count; i++) {
            rpmtdSetIndex(&td, i);
            values[i] = (char *)rpmtdGetString(&td);
        }
        *data = values;
    } else {
        /* HEADERGET_MINMEM borrows header storage, no scalar allocation. */
        *data = td.data;
    }
    rpmtdFreeData(&td);
    return 1;
}

#define headerGetEntry rpm2htmlHeaderGetEntry

/* Preserve libxml2 allocation while exposing C string fields as char *. */
static char *
rpmopenXmlStrdup(const char *value)
{
    return (char *) xmlStrdup((const xmlChar *) value);
}

/*
 * Get the internal number associated with an RPM tag.
 */
// removal of this possibly breaks compatibility with some ancient RPM but while
// some tags are alredy used as macros directly, without 'getTagNumber()', removal
// of this methond hopefully wont make in worser BUT it will help us compile this
// against rpm.org's rpm-4.6
// XXX TODO: clean-up
//static int
//getTagNumber(char *tag)
//{
//    int i;
//    headerTagTableEntryPtr t;
//
//    t = (headerTagTableEntryPtr) rpmTagTable;
//    for (i = 0; i < rpmTagTableSize; i++, t++) {
//        if (!strcasecmp(tag, t->name))
//            return (t->val);
//    }
//    fprintf(stderr, "getTagNumber(%s) : unknown tag !\n", tag);
//    return (-1);
//}

/*
 * Free up the extra data not needed for the last stage processing.
 */

void
rpmFreeExtraData(rpmDataPtr rpm)
{
    rpmExtraDataPtr extra;
    int n1;

    if ((rpm == NULL) || (rpm->extra == NULL))
        return;
    extra = rpm->extra;
    if (extra->packager != NULL)
        xmlFree(extra->packager);
    if (extra->description != NULL)
        xmlFree(extra->description);
    if (extra->copyright != NULL)
        xmlFree(extra->copyright);
    if (extra->changelog != NULL)
        xmlFree(extra->changelog);
    if (extra->srcrpm != NULL)
        xmlFree(extra->srcrpm);
    if (extra->host != NULL)
        xmlFree(extra->host);
    if (extra->resources != NULL)
        xmlFree(extra->resources);
    if (extra->requires != NULL)
        xmlFree(extra->requires);
    if (extra->filelist != NULL)
        xmlFree(extra->filelist);
    if (extra->sigs != NULL) {
        for (n1 = 0; n1 < extra->nb_sigs; n1++)
            if (extra->sigs[n1] != NULL) {
                if (extra->sigs[n1]->sig != NULL)
                    xmlFree(extra->sigs[n1]->sig);
#if defined(WITH_GPG)
                if (extra->sigs[n1]->resolve != NULL)
                    xmlFree(extra->sigs[n1]->resolve);
#endif
                xmlFree(extra->sigs[n1]);
            }
        xmlFree(extra->sigs);
    }
    xmlFree(extra);
}

/*
 * check a dependancy name to get rid of bad deps due to bad scripts
 * or spec files.
 */
int
checkResourceName(const char *res)
{
    int isname = 0;

    if (*res == 0)
        return (0);

    /*
     * we only accept absolute pathnames.
     */
    if (*res == '/')
        isname = 1;
    while (*res != 0) {
        if (((*res >= 'A') && (*res <= 'Z')) ||
            ((*res >= 'a') && (*res <= 'z')) ||
            ((*res >= '0') && (*res <= '9')) ||
            (*res == '-') || (*res == '.') ||
            (*res == '@') || (*res == '_') ||
            (*res == '(') || (*res == ')') ||
            (*res == '+') || ((*res == '/') && (isname == 1)))
            res++;
        else
            return (0);
        //if ((res - base) > 100)
        //    return (0);
    }
    return (1);
}

/*
 * check a release name
 */
int
checkReleaseName(const char *res)
{
    const char *base = res;

    if (*res == 0)
        return (0);

    while (*res != 0) {
        if (((*res >= 'A') && (*res <= 'Z')) ||
            ((*res >= 'a') && (*res <= 'z')) ||
            ((*res >= '0') && (*res <= '9')) ||
            (*res == '-') || (*res == '.') ||
            (*res == '@') || (*res == '_') || (*res == '+'))
            res++;
        else
            return (0);
        if ((res - base) > 65)
            return (0);
    }
    return (1);
}

/*
 * check a group name
 */
int
checkGroupName(const char *res)
{

    while (*res != 0) {
        if (((*res >= 'A') && (*res <= 'Z')) ||
            ((*res >= 'a') && (*res <= 'z')) ||
            ((*res >= '0') && (*res <= '9')) ||
            (*res == ' ') || (*res == '.') ||
            (*res == '(') || (*res == ')') ||
            (*res == '+') || (*res == '/') || (*res == '-'))
            res++;
        else
            return (0);
        //if ((res - base) > 60)
        //    return (0);
    }
    return (1);
}

/*
 * check a version name
 */
int
checkVersionName(const char *res)
{

    if (*res == 0)
        return (0);

    while (*res != 0) {
        if (((*res >= 'A') && (*res <= 'Z')) ||
            ((*res >= 'a') && (*res <= 'z')) ||
            ((*res >= '0') && (*res <= '9')) ||
            (*res == '-') || (*res == '.') ||
            (*res == '@') || (*res == '_') || (*res == '+'))
            res++;
        else
            return (0);
        //if ((res - base) > 20)
        //    return (0);
    }
    return (1);
}

#if defined(WITH_GPG)
/*
 * rpmResolveGPGPSignatures : resolve GPG and PGP signatures found in package
 * code based on rpm-4.0-0.45
 */
void
rpmResolveGPGPSignature(const char *nameRpm, const char *sigtarget,
                        const char *sigfile, rpmDataPtr rpm, int sign)
{
    int pid, status, outpipe[2];
    FILE *file;
    int rsize = 128, crsize;
    unsigned char buf[8192];
    char *temp;

    /* Now run GPG */
    outpipe[0] = outpipe[1] = 0;
    pipe(outpipe);

    if (!(pid = fork())) {
        close(outpipe[0]);
        /* gpg version 0.9 sends its output to stderr. */
        dup2(outpipe[1], STDERR_FILENO);

        execlp(GPG_PROG, GPG_PROG,
               "--batch", "--no-verbose",
               "--verify", sigfile, sigtarget, NULL);
        fprintf(stderr, "%s exec failed!\n", GPG_PROG);
        exit(1);
    }

    close(outpipe[1]);
    file = fdopen(outpipe[0], "r");
    rpm->extra->sigs[sign]->resolve = xmlMalloc(sizeof(char) * rsize);
    rpm->extra->sigs[sign]->resolve[0] = '\0';
    while (fgets(buf, 1024, file)) {
        crsize = strlen(buf) + strlen(rpm->extra->sigs[sign]->resolve);
        if (crsize >= rsize) {
            rsize = crsize * 2;
            rpm->extra->sigs[sign]->resolve =
                xmlRealloc(rpm->extra->sigs[sign]->resolve,
                           sizeof(char) * rsize);
            if (rpm->extra->sigs[sign]->resolve == NULL) {
                fprintf(stderr, "cannot re-allocate %d bytes: %s\n",
                        rsize, strerror(errno));
                exit(1);
            }
        }

        strcat(rpm->extra->sigs[sign]->resolve, buf);
    }
    fclose(file);
    
    temp = xmlStrdup(protectEmail(rpm->extra->sigs[sign]->resolve));
    xmlFree(rpm->extra->sigs[sign]->resolve);
    rpm->extra->sigs[sign]->resolve = temp;

    (void) waitpid(pid, &status, 0);
}

/*
 * rpmResolveMD5Signatures : resolve MD5 signatures found in package
 * code based on rpm-4.0-0.45
 * for now, we are not checking whether MD5 sums are OK
 */
/*void rpmResolveMD5Signature(const char *nameRpm, const char *sigtarget,
                            const char *sigfile, rpmDataPtr rpm, int sign) {
}*/

/*
 * rpmResolveSignatures : resolve signatures found in package
 * code based on rpm-4.0-0.45
 */
void
rpmResolveSignatures(const char *path, const char *nameRpm,
                     const char *sigtarget, rpmDataPtr rpm)
{
    int sign = 0;

    if (rpm == NULL || rpm->extra->nb_sigs == 0 || sigtarget == NULL)
        return;

    for (sign = 0; sign < rpm->extra->nb_sigs; sign++) {
        FD_t sfd;
        char *sigfile = NULL;

        if (rpm->extra->sigs[sign]->tag != RPMSIGTAG_PGP
            && rpm->extra->sigs[sign]->tag != RPMSIGTAG_GPG)
            continue;

        /* Write out the signature */
        sigfile = xmlMalloc(30);
        strcpy(sigfile, "/tmp/rpm2html.sig.XXXXXX");
        sigfile = mktemp(sigfile);
        sfd = Fopen(sigfile, "w.fdio");
        (void) Fwrite(rpm->extra->sigs[sign]->sig, sizeof(char),
                      rpm->extra->sigs[sign]->size, sfd);
        Fclose(sfd);

        switch (rpm->extra->sigs[sign]->tag) {
            case RPMSIGTAG_PGP:
            case RPMSIGTAG_GPG:
                rpmResolveGPGPSignature(nameRpm, sigtarget, sigfile, rpm,
                                        sign);
                break;
                /*case RPMSIGTAG_MD5:
                 * rpmResolveMD5Signature(nameRpm, sigtarget, sigfile, rpm, sign);
                 * break; */
            default:
                break;
        }

        if (sigfile != NULL) {
            unlink(sigfile);
            xmlFree(sigfile);
        }
    }
}
#endif

/*
 * rpmAnalyze : analyze an RPM record, read and parse the header and 
 *              fill the information in the database.
 */
#define ENTRY_CLEANUP(p)						\
    if ((type == RPM_STRING_ARRAY_TYPE || type == RPM_I18NSTRING_TYPE)&&\
        (p != NULL)) { free((p)); p = NULL; }

static char *buffer = NULL;
static int buffer_size = 50 * 1024 * sizeof(char);

rpmDataPtr
rpmAnalyze(char *path, char *nameRpm, const char *sigtarget,
           Header h, Header s, rpmDirPtr dir, rpmSubdirPtr tree,
           time_t stamp, int isSource)
{
    int installed = dir->installbase;
    char *name = NULL, *version = NULL, *release = NULL;
    int count, type;
    void *p = NULL;
    int val, i, j;
    rpmDataPtr rpm = NULL;
    static char nameBuffer[500];

#ifdef WITH_SQL
    int id;
#endif
    (void)s;

    if (buffer == NULL) {
        buffer = (char *) xmlMalloc(buffer_size);
        if (buffer == NULL) {
            fprintf(stderr, "cannot allocate %d bytes: %s\n", buffer_size,
                    strerror(errno));
            exit(1);
        }
    }

    /* extract information from the header */
    headerGetEntry(h, RPMTAG_NAME, &type, (void **) &name, &count);
    if (name == NULL) {
        fprintf(stderr, "Invalid package %s : no name\n", nameRpm);
        return (NULL);
    }
    if (!(((name[0] >= 'a') && (name[0] <= 'z')) ||
          ((name[0] >= 'A') && (name[0] <= 'Z')) ||
          ((name[0] >= '0') && (name[0] <= '9')))) {
        fprintf(stderr, "Invalid package %s : garbled name\n", nameRpm);
        ENTRY_CLEANUP(name)
            return (NULL);
    }
    if (!checkResourceName(name)) {
        fprintf(stderr, "Invalid package %s : garbled name\n", nameRpm);
        ENTRY_CLEANUP(name)
            return (NULL);
    }
    headerGetEntry(h, RPMTAG_VERSION, &type, (void **) &version, &count);
    if ((version == NULL) || (version[0] == 0)) {
        fprintf(stderr, "Invalid package %s : version invalid\n", nameRpm);
        ENTRY_CLEANUP(name)
            ENTRY_CLEANUP(version)
            return (NULL);
    }
    if (!checkVersionName(version)) {
        fprintf(stderr, "Invalid package %s : garbled version\n", nameRpm);
        ENTRY_CLEANUP(name)
            ENTRY_CLEANUP(version)
            return (NULL);
    }
    headerGetEntry(h, RPMTAG_RELEASE, &type, (void **) &release, &count);
    if ((release == NULL) || (release[0] == 0)) {
        fprintf(stderr, "Invalid package %s : release invalid\n", nameRpm);
        ENTRY_CLEANUP(name)
            ENTRY_CLEANUP(version)
            ENTRY_CLEANUP(release)
            return (NULL);
    }
    if (!checkReleaseName(release)) {
        fprintf(stderr, "Invalid package %s : garbled release\n", nameRpm);
        ENTRY_CLEANUP(name)
            ENTRY_CLEANUP(version)
            ENTRY_CLEANUP(release)
            return (NULL);
    }

    /* allocate a new rpmData block, and an rpmExtraData block fill them */
    rpm = (rpmDataPtr) xmlMalloc(sizeof(rpmData));
    if (rpm == NULL) {
        fprintf(stderr, "cannot allocate %zu bytes: %s\n", sizeof(rpmData),
                strerror(errno));
        return (NULL);
    }
    memset(rpm, 0, sizeof(rpmData));
    rpm->extra = (rpmExtraDataPtr) xmlMalloc(sizeof(rpmExtraData));
    if (rpm->extra == NULL) {
        fprintf(stderr, "cannot allocate %zu bytes: %s\n",
                sizeof(rpmExtraData), strerror(errno));
        xmlFree(rpm);
        return (NULL);
    }
    memset(rpm->extra, 0, sizeof(rpmExtraData));
    rpm->dir = dir;
    if (tree) {
        if ((tree->rpmpath != NULL) && (tree->rpmpath[0] != '\0'))
            rpm->subdir = stringAdd(tree->rpmpath);
        else
            rpm->subdir = NULL;
    } else
        rpm->subdir = NULL;
    rpm->extra->stamp = stamp;
    rpm->name = stringAdd(name);
    ENTRY_CLEANUP(name);
        rpm->version = stringAdd(version);
    ENTRY_CLEANUP(version);
        rpm->release = stringAdd(release);
    ENTRY_CLEANUP(release);

        /* get all the resources provided by this RPM */
    if (!headerGetEntry(h, RPMTAG_SUMMARY, &type, &p, &count) || !p) {
        rpm->summary = rpmopenXmlStrdup(localizedStrings[LANG_NO_SUMMARY]);
    } else {
        rpm->summary = rpmopenXmlStrdup((char *) p);
    }
    ENTRY_CLEANUP(p);
    if (!headerGetEntry(h, RPMTAG_DESCRIPTION, &type, &p, &count) || !p ||
        (type != RPM_STRING_TYPE)) {
        rpm->extra->description =
            rpmopenXmlStrdup(localizedStrings[LANG_NO_DESCRIPTION]);
    } else {
        rpm->extra->description = rpmopenXmlStrdup((char *) p);
    }
    ENTRY_CLEANUP(p);
    if (!headerGetEntry(h, RPMTAG_DISTRIBUTION, &type, &p, &count) || !p ||
        (type != RPM_STRING_TYPE)) {
        rpm->distribution = stringAdd(localizedStrings[LANG_UNKNOWN]);
    } else {
        rpm->distribution = stringAdd((char *) p);
    }
    ENTRY_CLEANUP(p);
    if (isSource) {
        rpm->arch = stringAdd("src");
        if (nameRpm == NULL) {
            snprintf(nameBuffer, sizeof(nameBuffer), "%s-%s-%s.src.rpm",
                     name, version, release);
            nameRpm = nameBuffer;
        }
    } else {
        if (!headerGetEntry(h, RPMTAG_ARCH, &type, &p, &count) || !p ||
            (type != RPM_STRING_TYPE)) {
            if (type == RPM_INT8_TYPE) {
                /*
                 * Old packages.
                 */
                switch (*((char *) p)) {
                    case 1:
                        rpm->arch = stringAdd("i386");
                        break;
                    default:
                        rpm->arch = stringAdd("src");
                        break;
                }
            } else
                rpm->arch = stringAdd(localizedStrings[LANG_NONE]);
            if (nameRpm == NULL) {
                snprintf(nameBuffer, sizeof(nameBuffer), "%s-%s-%s.rpm",
                         name, version, release);
                nameRpm = nameBuffer;
            }
        } else {
            rpm->arch = stringAdd((char *) p);
            if (nameRpm == NULL) {
                snprintf(nameBuffer, sizeof(nameBuffer), "%s-%s-%s.%s.rpm",
                         name, version, release, (char *) p);
                nameRpm = nameBuffer;
            }
        }
        ENTRY_CLEANUP(p);
    }
    if (!headerGetEntry(h, RPMTAG_OS, &type, &p, &count) || !p ||
        (type != RPM_STRING_TYPE)) {
        if (type == RPM_INT8_TYPE) {
            /*
             * Old packages.
             */
            switch (*((char *) p)) {
                case 1:
                    rpm->os = stringAdd("linux");
                    break;
                default:
                    rpm->os = stringAdd("linux");
                    break;
            }
        } else
            rpm->os = stringAdd("");
    } else {
        rpm->os = stringAdd((char *) p);
    }
    ENTRY_CLEANUP(p);
    if (!headerGetEntry(h, RPMTAG_VENDOR, &type, &p, &count) || !p ||
        (type != RPM_STRING_TYPE)) {
        rpm->vendor = NULL;
    } else {
        rpm->vendor = stringAdd(protectEmail((char *) p));
    }
    ENTRY_CLEANUP(p);
    if (!headerGetEntry(h, RPMTAG_GROUP, &type, &p, &count) || !p ||
        (type != RPM_STRING_TYPE)) {
        rpm->group = stringAdd(localizedStrings[LANG_NO_GROUP]);
    } else {
        rpm->group = stringAdd((char *) p);
    }
    ENTRY_CLEANUP(p);
    if (!checkGroupName(rpm->group)) {
        fprintf(stderr, "Invalid package %s : garbled group\n", nameRpm);
        if (rpm->name)
            stringFree(rpm->name);
        if (rpm->version)
            stringFree(rpm->version);
        if (rpm->release)
            stringFree(rpm->release);
        if (rpm->summary)
            xmlFree(rpm->summary);
        if (rpm->distribution)
            stringFree(rpm->distribution);
        if (rpm->arch)
            stringFree(rpm->arch);
        if (rpm->os)
            stringFree(rpm->os);
        if (rpm->vendor != NULL)
            stringFree(rpm->vendor);
        if (rpm->group != NULL)
            stringFree(rpm->group);
        xmlFree(rpm);
        return (NULL);
    }
    if (!headerGetEntry(h, RPMTAG_BUILDHOST, &type, &p, &count) || !p ||
        (type != RPM_STRING_TYPE)) {
        rpm->extra->host = rpmopenXmlStrdup(localizedStrings[LANG_NO_HOST]);
    } else {
        rpm->extra->host = rpmopenXmlStrdup((char *) p);
    }
    ENTRY_CLEANUP(p);
    if (!headerGetEntry(h, RPMTAG_SIZE, &type, &p, &count) || !p) {
        rpm->size = 0;
    } else {
        rpm->size = *((int *) p);
    }
    ENTRY_CLEANUP(p);
    if (installed) {
        if (!headerGetEntry(h, RPMTAG_INSTALLTIME, &type, &p, &count)
            || !p) {
            rpm->date = 0;
        } else {
            rpm->date = *((int32_t *) p);
        }
        ENTRY_CLEANUP(p);
    } else {
        if (!headerGetEntry(h, RPMTAG_BUILDTIME, &type, &p, &count) || !p) {
            rpm->date = 0;
        } else {
            rpm->date = *((int32_t *) p);
        }
        ENTRY_CLEANUP(p);
    }
    if (!headerGetEntry(h, RPMTAG_SOURCERPM, &type, &p, &count) || !p ||
        (type != RPM_STRING_TYPE)) {
        rpm->extra->srcrpm = rpmopenXmlStrdup("");
    } else {
        rpm->extra->srcrpm = rpmopenXmlStrdup((char *) p);
    }
    ENTRY_CLEANUP(p);
    if (!headerGetEntry(h, RPMTAG_URL, &type, &p, &count) || !p ||
        (type != RPM_STRING_TYPE)) {
        rpm->url = NULL;
    } else {
        rpm->url = rpmopenXmlStrdup((char *) p);
    }
    ENTRY_CLEANUP(p);
    if (!headerGetEntry(h, RPMTAG_PACKAGER, &type, &p, &count) || !p ||
        (type != RPM_STRING_TYPE)) {
        rpm->extra->packager = NULL;
    } else {
        rpm->extra->packager = rpmopenXmlStrdup(protectEmail((char *) p));
    }
    ENTRY_CLEANUP(p);
    if (!headerGetEntry(h, RPMTAG_LICENSE, &type, &p, &count) || !p ||
        (type != RPM_STRING_TYPE)) {
        rpm->extra->copyright = NULL;
    } else {
        rpm->extra->copyright = rpmopenXmlStrdup((char *) p);
    }
    ENTRY_CLEANUP(p);
    /* Pick up changelog entries */
    if (!headerGetEntry(h, RPMTAG_CHANGELOGTEXT, &type, &p, &count) || !p) {
        rpm->extra->changelog = NULL;
    } else {
        int32_t *dates;	/* necessary when sizeof(time_t) != sizeof(int32_t) */
        time_t date;
        char **names;
        char **holdp = (char **) p;
        char *cp;
        struct tm *tm_buf;
        int i,
          len,
          pos;
        char date_string[50];
        char *res = buffer;

        *res = '\0';
        headerGetEntry(h, RPMTAG_CHANGELOGTIME, &type, (void *) &dates,
                       &count);
        headerGetEntry(h, RPMTAG_CHANGELOGNAME, &type, (void *) &names,
                       &count);
        for (i = 0; i < count; i++) {
            if ((res - buffer) > (buffer_size - 1024)) {
                int delta = res - buffer;

                buffer_size *= 2;
                buffer = (char *) xmlRealloc(buffer, buffer_size);
                if (buffer == NULL) {
                    fprintf(stderr, "cannot re-allocate %d bytes: %s\n",
                            buffer_size, strerror(errno));
                    exit(1);
                }
                res = &buffer[delta];
            }
            date = dates[i];
            tm_buf = localtime(&date);
            strftime(date_string, sizeof(date_string) - 1, "%a %b %d %Y",
                     tm_buf);
            len =
                snprintf(res, buffer_size, "* %s %s\n", date_string,
                         names[i]);
            res += len;
            cp = holdp[i];
            pos = 0;

            if ((res - buffer + strlen(holdp[i])) > buffer_size) {
                int delta = res - buffer;

                buffer_size *= 2;
                buffer = (char *) xmlRealloc(buffer, buffer_size);
                if (buffer == NULL) {
                    fprintf(stderr, "cannot re-allocate %d bytes: %s\n",
                            buffer_size, strerror(errno));
                    exit(1);
                }
                res = &buffer[delta];
            }

            while (*cp) {
                if (pos++ == 0) {
                    *res++ = ' ';
                    *res++ = ' ';
                }
                *res++ = *cp;
                if (*cp++ == '\n')
                    pos = 0;
            }
            *res++ = '\n';
        }
        *res = '\0';
        rpm->extra->changelog = rpmopenXmlStrdup(protectEmail(buffer));
        ENTRY_CLEANUP(names);
    }
    ENTRY_CLEANUP(p);
    if (rpm->vendor == NULL) {
        if (rpm->extra->packager != NULL)
            rpm->vendor = rpmopenXmlStrdup(rpm->extra->packager);
        else
            rpm->vendor = rpmopenXmlStrdup(localizedStrings[LANG_UNKNOWN]);
    }

    /* Signature metadata is not exposed by the old API on RPM 6. */
    rpm->extra->sigs = NULL;

    rpm->filename = rpmopenXmlStrdup(nameRpm);

    /* package-xxx.rpm provides at least the resource "package" */
    val = RPMTAG_PROVIDES;
    if (!headerGetEntry(h, val, &type, &p, &count) || !p) {
        rpm->extra->nb_resources = 1;
        rpm->extra->max_resources = 1;
        rpm->extra->resources =
            (rpmRessPtr *) xmlMalloc(sizeof(rpmRessPtr) *
                                     rpm->extra->max_resources);
        if (rpm->extra->resources == NULL) {
            fprintf(stderr, ": ran out of memory\n");
            exit(1);
        }
        rpm->extra->resources[0] = rpmRessAdd(name, rpm, installed);
    } else {
        rpm->extra->max_resources = count + 1;
        rpm->extra->resources =
            (rpmRessPtr *) xmlMalloc(sizeof(rpmRessPtr) *
                                     rpm->extra->max_resources);
        if (rpm->extra->resources == NULL) {
            fprintf(stderr, ": ran out of memory\n");
            exit(1);
        }
        rpm->extra->resources[0] = rpmRessAdd(name, rpm, installed);
        rpm->extra->nb_resources = 1;

        for (i = 0, j = rpm->extra->nb_resources; i < count; i++) {
            if (!checkResourceName(((char **) p)[i]))
                continue;
            if (!strcmp(((char **) p)[i], name))
                continue;
            rpm->extra->resources[j++] =
                rpmRessAdd(((char **) p)[i], rpm, installed);
        }
        rpm->extra->nb_resources = j;
    }
    ENTRY_CLEANUP(p);

    val = RPMTAG_REQUIRENAME;
    if (!headerGetEntry(h, val, &type, &p, &count) || !p) {
        rpm->extra->nb_requires = 0;
        rpm->extra->requires = NULL;
    } else {
        int count2,
          count3,
          type2,
          type3;
        void *q = NULL;
        void *r = NULL;
        int valv,
          valf;

        rpm->extra->max_requires = count;
        rpm->extra->requires =
            (rpmRessPtr *) xmlMalloc(sizeof(rpmRessPtr) *
                                     rpm->extra->max_requires);
        if (rpm->extra->requires == NULL) {
            fprintf(stderr, ": ran out of memory\n");
            exit(1);
        }
        valv = RPMTAG_REQUIREVERSION;
        valf = RPMTAG_REQUIREFLAGS;
        headerGetEntry(h, valv, &type2, &q, &count2);
        headerGetEntry(h, valf, &type3, &r, &count3);

        rpm->extra->nb_requires = 0;

        for (i = 0, j = 0; (i < count2) && (i < count3); i++) {
            uint32_t anint = ((uint32_t *) r)[i];
            rpm_dep_flag flag = RPM2HTML_REQ_NONE;

            if ((anint & RPMSENSE_LESS) && (anint & RPMSENSE_EQUAL))
                flag = RPM2HTML_REQ_LEQ;
            else if (anint & RPMSENSE_LESS)
                flag = RPM2HTML_REQ_LT;
            if ((anint & RPMSENSE_GREATER) && (anint & RPMSENSE_EQUAL))
                flag = RPM2HTML_REQ_GEQ;
            else if (anint & RPMSENSE_GREATER)
                flag = RPM2HTML_REQ_GT;
            else if (anint & RPMSENSE_EQUAL)
                flag = RPM2HTML_REQ_EQU;
            if (!checkResourceName(((char **) p)[i]))
                continue;
            rpm->extra->requires[j++] =
                rpmRequAdd(((char **) p)[i], ((char **) q)[i],
                           flag, rpm, installed);
        }
        rpm->extra->nb_requires = j;
        if ((type2 == RPM_STRING_ARRAY_TYPE
             || type2 == RPM_I18NSTRING_TYPE) && (q != NULL)) {
            free((q));
            q = NULL;
        }
        if ((type3 == RPM_STRING_ARRAY_TYPE
             || type3 == RPM_I18NSTRING_TYPE) && (r != NULL)) {
            free((r));
            r = NULL;
        }
        for (; i < count; i++)
            rpmRequAdd(((char **) p)[i], NULL, RPM2HTML_REQ_NONE,
                       rpm, installed);
    }
    ENTRY_CLEANUP(p);

#ifdef HAVE_RPM42
    const char *fname;
    rpmfi fi;
    fi = rpmfiNew(NULL, h, RPMTAG_BASENAMES, 0);

    if (rpmfiFC(fi) <= 0)
	rpm->extra->filelist = NULL;	/* No filelist in the package ! */
    else {
    	fi = rpmfiInit(fi, 0);
    	char *ptr = buffer;

        while (rpmfiNext(fi) >= 0) {
            fname = rpmfiFN(fi);
#else
  #if defined(RPMTAG_FILENAMES)
    val = RPMTAG_FILENAMES;
    headerGetEntry(h, val, &type, &p, &count);
  #else
    rpmBuildFileList(h, (const char ***) &p, &count);
  #endif

    if (count == 0) {
        rpm->extra->filelist = NULL;    /* No filelist in the package ! */
    } else {
        char *ptr = buffer;

        for (i = 0; i < count; i++) {
#endif
            if ((ptr - buffer) > (buffer_size - 1024)) {
                int delta = ptr - buffer;

                buffer_size *= 2;
                buffer = (char *) xmlRealloc(buffer, buffer_size);
                if (buffer == NULL) {
                    fprintf(stderr, "cannot re-allocate %d bytes: %s\n",
                            buffer_size, strerror(errno));
                    exit(1);
                }
                ptr = &buffer[delta];
            }
#ifdef HAVE_RPM42
            ptr += snprintf(ptr, buffer_size, "%s\n", fname);
#else
            ptr += snprintf(ptr, buffer_size, "%s\n", ((char **) p)[i]);
#endif
        }
#ifdef HAVE_RPM42
        fi = rpmfiFree(fi);
#endif
        rpm->extra->filelist = rpmopenXmlStrdup(buffer);
    }
#ifndef HAVE_RPM42
    ENTRY_CLEANUP(p);
#endif

#ifdef WITH_GPG
    rpmResolveSignatures(path, nameRpm, sigtarget, rpm);
#endif

#ifdef WITH_SQL
    if ((path == NULL) || (!sql_get_package_id(path))) {
        id = sql_add_package(path, rpm->name, rpm->version, rpm->release,
                             rpm->arch, dir->no, rpm->url,
                             rpm->extra->srcrpm, dir->vendor,
                             rpm->extra->packager, rpm->group,
                             rpm->summary, rpm->extra->description,
                             rpm->extra->copyright, rpm->date, rpm->size,
                             rpm->os, rpm->distribution, rpm->vendor);
        if (id > 0) {
            for (i = 0; i < rpm->extra->nb_resources; i++)
                sql_add_provides(id, rpm->extra->resources[i]->name);
            for (i = 0; i < rpm->extra->nb_requires; i++)
                sql_add_requires(id, rpm->extra->requires[i]->name,
                                 rpm->extra->requires[i]->flag,
                                 rpm->extra->requires[i]->version);
        }
    } else
        id = 0;
#endif

    /* Add the package files to the real filesystem tree if asked for */
#ifdef WITH_SQL
    if ((rpm->extra->filelist != NULL) &&
        ((dir->build_tree != 0) || (id > 0))) {
#else
    if ((dir->build_tree != 0) && (rpm->extra->filelist != NULL)) {
#endif
        char *cur,
         *filename;

        cur = rpm->extra->filelist;
        while ((*cur != '\0') && (*cur != '/'))
            cur++;
        filename = cur;
        while (*cur != '\0') {
            if ((*cur == '\n') || (*cur == '\r')) {
                if (cur != filename) {
#ifdef WITH_SQL
                    if (dir->build_tree != 0)
                        rpmAddRealFile(dir->root, filename, rpm);
                    if (id > 0) {
                        *cur = 0;
                        sql_add_file(filename, id);
                        *cur = '\n';
                    }
#else
                    rpmAddRealFile(dir->root, filename, rpm);
#endif
                }
                while ((*cur != '\0') && (*cur != '/'))
                    cur++;
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
    if ((rpm2html_dump_html) && (dir->html))
        dumpRpmHtml(rpm, tree);
    if (rpm2html_dump_rdf)
        // TODO compare also last-modification-time of package on RDF (if existing) and dump RDF only if package is newer or RDF does not exists
        dumpRpmRdf(rpm, tree);

    /* free the extra data */
    rpmFreeExtraData(rpm);

    /* increment the counters */
    if (installed) {
        rpm2html_install_files++;
        rpm2html_install_size += rpm->size / 1024;
    } else {
        rpm2html_files++;
        rpm2html_size += rpm->size / 1024;
    }

    return (rpm);
}

/*
 * rpmOpen : open an RPM file, read and parse the header and 
 *           fill the information in the database.
 */
rpmDataPtr
rpmOpen(char *nameRpm, rpmDirPtr dir, rpmSubdirPtr tree)
{
    rpmDataPtr cur;
    FD_t fd;
    int rc;
    Header h = NULL;
    Header s = NULL;
    int isSource;
    char buffer[500];
    struct stat buf;

#if defined(WITH_GPG)
    FD_t ofd = NULL;
    char *sigtarget = NULL;
    ssize_t count;
    unsigned char stbuffer[8192];
#endif

    /* open the file for reading */
    if (tree->htmlpath[0] != '\0')
        snprintf(buffer, sizeof(buffer), "%s/%s/%s", dir->rpmdir,
                 tree->rpmpath, nameRpm);
    else
        snprintf(buffer, sizeof(buffer), "%s/%s", dir->rpmdir, nameRpm);

    if (access(buffer, R_OK) < 0) {
        fprintf(stderr, "open of %s failed: %s\n", buffer,
                strerror(errno));
        return (NULL);
    }
    fd = Fopen(buffer, "r.fdio");
    if (fd == NULL || Ferror(fd)) {
        fprintf(stderr, "Fopen of %s failed: %s\n", buffer, Fstrerror(fd));
        return (NULL);
    }

    stat(buffer, &buf);

    /* rpm 6 verifies digest data. This is read-only indexing, not trust enforcement:
     * skip signatures so third-party RPM repositories can be indexed. */
    {
        rpmts ts = rpmtsCreate();
        if (ts == NULL) {
            Fclose(fd);
            return NULL;
        }
        rpmtsSetVSFlags(ts, RPMVSF_NOOPENPGP | RPMVSF_NORSAHEADER |
                            RPMVSF_NODSAHEADER | RPMVSF_NORSA | RPMVSF_NODSA |
                            RPMVSF_NOPAYLOAD);
        rc = rpmReadPackageFile(ts, fd, buffer, &h);
        rpmtsFree(ts);
    }
    if (rc != RPMRC_OK || h == NULL) {
        fprintf(stderr, "Cannot read RPM header from %s (rpmRC=%d)\n",
                buffer, rc);
        Fclose(fd);
        if (h != NULL)
            headerFree(h);
        return NULL;
    }
    isSource = headerIsSource(h);

    /* close the descriptor */
    Fclose(fd);

#if defined(WITH_GPG)
    cur =
        rpmAnalyze(buffer, nameRpm, sigtarget, h, s, dir, tree,
                   buf.st_mtime, isSource);
#else
    cur =
        rpmAnalyze(buffer, nameRpm, NULL, h, s, dir, tree, buf.st_mtime,
                   isSource);
#endif

    /* free the header */
    headerFree(h);
    headerFree(s);

#if defined(WITH_GPG)
    if (sigtarget) {
        unlink(sigtarget);
        xmlFree(sigtarget);
    }
#endif

    return (cur);
}

/*
 * Scan one directory for RPM files this is where the recursive handling
 * is done.
 */
static rpmDataPtr
rpmOneDirScan(rpmDirPtr dir, rpmSubdirPtr tree)
{
    rpmSubdirPtr subtree;
    rpmDataPtr ret = NULL,
      cur;
    char *filename;
    static char path[2000];
    struct stat buf;
    int len;
    DIR *d;
    struct dirent *file;


    /*
     * Create the directory for the HTML pages
     */
    if ((rpm2html_dump_html) && (dir->html)) {
        if (tree->htmlpath[0] != '\0')
            snprintf(path, sizeof(path), "%s/%s", dir->dir,
                     tree->htmlpath);
        else
            snprintf(path, sizeof(path), "%s", dir->dir);
        createDirectory(path);
    }

    /*
     * Scan the repository.
     */
    if (tree->rpmpath[0] != '\0')
        snprintf(path, sizeof(path), "%s/%s", dir->rpmdir, tree->rpmpath);
    else
        snprintf(path, sizeof(path), "%s", dir->rpmdir);
    if (rpm2htmlVerbose > 1)
        printf("Scanning directory %s\n", path);

    d = opendir(path);
    if (d == NULL) {
        fprintf(stderr, "Listing of %s failed: %s\n", path,
                strerror(errno));
        rpmRemoveSubdir(tree);
        rpmFreeSubdir(tree);
        tree = NULL;
        return (NULL);
    } else {
        while ((file = readdir(d)) != NULL) {
            cur = NULL;
            filename = file->d_name;
            len = strlen(filename);

            /*
             * Compute the full path
             */
            if (tree->rpmpath[0] != '\0')
                snprintf(path, sizeof(path), "%s/%s/%s", dir->rpmdir,
                         tree->rpmpath, filename);
            else
                snprintf(path, sizeof(path), "%s/%s", dir->rpmdir,
                         filename);

            /*
             * Stat() the file to detect directory and symlimks
             */
            if (lstat(path, &buf) != 0) {
                fprintf(stderr, "Couldn't stat(%s)\n", path);
                continue;
            }

            /*
             * Check for RPM files by looking at the suffix
             * Note that SuSE source RPMs have a ".spm" suffix
             * We may or may not follow symlinks to RPM files
             */
            if (((len >= 5) &&
                 (!strcasecmp(&filename[len - 4], ".rpm"))) ||
                ((len >= 5) &&
                 (!strcasecmp(&filename[len - 4], ".spm")))) {

                if ((S_ISLNK(buf.st_mode)) && (dir->rpm_symlinks == 0)) {
                    if (rpm2htmlVerbose > 1)
                        fprintf(stderr, "Dropping symlink %s\n", path);
                    continue;
                }
                cur = rpmOpen(filename, dir, tree);
            }

            /*
             * Don't follow of analyze symlinks,
             */
            else if ((S_ISLNK(buf.st_mode)) && (dir->follow_symlinks == 0)) {
                if (rpm2htmlVerbose > 1)
                    fprintf(stderr, "Dropping symlink %s\n", path);
                continue;
            }

            /*
             * Check for RDF files by looking at the suffix
             */
            else if ((len >= 5)
                     && (!strcasecmp(&filename[len - 4], ".rdf"))) {
                cur = rpmOpenRdf(filename, dir, tree);
            }

            /*
             * Else if this is a directory, recurse !
             */
            else if (S_ISDIR(buf.st_mode)) {
                if (filename[0] != '.') {
                    subtree =
                        rpmNewSubdir(tree, filename, NULL, NULL, NULL, 0);
                    cur = rpmOneDirScan(dir, subtree);
                }
            }
            if (cur != NULL)
                ret = rpmAddList(ret, cur);
        }
    }
    closedir(d);

    /*
     * Dump the pages related to this directory.
     */
    if (ret != NULL)
        ret = dumpDirIndex(dir, tree, ret);
    else {
        rpmRemoveSubdir(tree);
        rpmFreeSubdir(tree);
        tree = NULL;
    }

    return (ret);
}

/*
 * Scan a directory for RPM files.
 */
static rpmDataPtr
rpmDirScan(rpmDirPtr dir, rpmSubdirPtr tree)
{
    rpmSubdirPtr cur;
    rpmDataPtr ret;

    if ((rpm2html_dump_html) && (dir->html) && (dir->build_tree != 0)) {
        dir->root = rpmCreateRealRoot();
    }
    cur = rpmNewSubdir(tree, dir->name,
                       dir->subdir == NULL ? "" : dir->subdir, "",
                       dir->color, dir->html);
    ret = rpmOneDirScan(dir, cur);
    rpmDumpHtmlRealRoot(dir);
    return (ret);
}

/*
 * Scan the local RPM database for RPM files.
 */
static rpmDataPtr
rpmBaseScan(rpmDirPtr dir)
{
    static char path[2000];
    rpmDataPtr ret = NULL,
      cur;
    rpmts ts;
    Header h = NULL;
    rpmdbMatchIterator mi;

#ifdef WITH_SQL
    dir->no = sql_get_distrib_by_directory(dir->rpmdir);
#endif

    /*
     * Create the directory for the HTML pages
     */
    if ((rpm2html_dump_html) && (dir->html)) {
        if (dir->subdir)
            snprintf(path, sizeof(path), "%s/%s", dir->dir, dir->subdir);
        else
            snprintf(path, sizeof(path), "%s", dir->dir);
        createDirectory(path);
    }

    /*
     * Create the directory for the RDF pages
     */
    if (rpm2html_rdf_dir != NULL) {
        if (dir->subdir)
            snprintf(path, sizeof(path), "%s/%s", rpm2html_rdf_dir,
                     dir->subdir);
        else
            snprintf(path, sizeof(path), "%s", rpm2html_rdf_dir);
        createDirectory(path);
    }

    if (rpm2htmlVerbose)
        printf("Scanning the database of installed RPMs\n");

    if (dir->dbpath != NULL)
        rpmPushMacro(NULL, "_dbpath", NULL, dir->dbpath, -7);

    ts = rpmtsCreate();
    if (ts == NULL)
        return NULL;
    if (rpmtsOpenDB(ts, O_RDONLY) != 0) {
        rpmtsFree(ts);
        return NULL;
    }

    mi = rpmtsInitIterator(ts, RPMDBI_PACKAGES, NULL, 0);
    while ((h = rpmdbNextIterator(mi)) != NULL) {
        cur = rpmAnalyze(NULL, NULL, NULL, h, NULL, dir, NULL, 0, 0);
        if (cur != NULL)
            ret = rpmAddList(ret, cur);
    }
    rpmdbFreeIterator(mi);

    rpmtsCloseDB(ts);
    rpmtsFree(ts);

    return (ret);
}

/*
 * Scan one directory only
 */

void
rpmDirScanOneDir(const char *directory)
{
    rpmDirPtr dir;

    /*
     * first try to find the distrib
     */
    dir = dirList;
    while (dir != NULL) {
        if (!strcasecmp(dir->rpmdir, directory))
            break;
        dir = dir->next;
    }
    if (dir == NULL) {
        fprintf(stderr, "rpmDirScanOneDir(%s): distribution not found\n",
                directory);
        return;
    }
    if (rpm2htmlVerbose)
        printf("indexing %s\n", dir->name);

    /*
     * Allocate a directory tree.
     */
    dirTree = rpmNewSubdir(NULL, "", "", "", NULL, 1);

    /*
     * Override default setting.
     */
    if ((dir->maint == NULL) && (rpm2html_maint != NULL))
        dir->maint = rpmopenXmlStrdup(rpm2html_maint);
    if ((dir->mail == NULL) && (rpm2html_mail != NULL))
        dir->mail = rpmopenXmlStrdup(rpm2html_mail);
    if ((dir->ftp == NULL) && (rpm2html_ftp != NULL))
        dir->ftp = rpmopenXmlStrdup(rpm2html_ftp);
    if ((dir->ftpsrc == NULL) && (rpm2html_ftpsrc != NULL))
        dir->ftpsrc = rpmopenXmlStrdup(rpm2html_ftpsrc);
    if ((dir->dir == NULL) && (rpm2html_dir != NULL))
        dir->dir = rpmopenXmlStrdup(rpm2html_dir);
    if ((dir->host == NULL) && (rpm2html_host != NULL))
        dir->host = rpmopenXmlStrdup(rpm2html_host);
    if ((dir->name == NULL) && (rpm2html_name != NULL))
        dir->name = rpmopenXmlStrdup(rpm2html_name);
    if ((dir->url == NULL) && (rpm2html_url != NULL))
        dir->url = rpmopenXmlStrdup(rpm2html_url);

    if (dir->rpmdir == NULL) {
        fprintf(stderr, "?!? rpmDir without directory ?!? disabled !\n");
    } else if (!strncmp(dir->rpmdir, "localbase", 9)) {
        /* Scan the local RPM database instead of a directory */
        rpmBaseScan(dir);
    } else if (dir->ftp == NULL) {
        fprintf(stderr, "Directory %s disabled : no ftp field\n",
                dir->rpmdir);
    } else {
#ifdef WITH_SQL
        dir->no = sql_get_distrib_by_directory(dir->rpmdir);
#endif
        if (rpm2htmlVerbose)
            printf("Scanning directory %s for RPMs\n", dir->rpmdir);
        rpmDirScan(dir, dirTree);
    }

    if (dir->root != NULL) {
        rpmDestroyRealRoot(dir->root);
        dir->root = NULL;
    }
    rpmFreeSubdir(dirTree);
    dirTree = NULL;
}


/*
 * Scan one distribution only
 */

void
rpmDirScanOneDist(const char *dist)
{
    rpmDirPtr dir;

    /*
     * first try to find the distrib
     */
    dir = dirList;
    while (dir != NULL) {
        if (!strcasecmp(dir->name, dist))
            break;
        if (!strcasecmp(dir->subdir, dist))
            break;
        if (!strcasecmp(dir->rpmdir, dist))
            break;
        dir = dir->next;
    }
    if (dir == NULL) {
        dir = dirList;
        while (dir != NULL) {
            if (!strstr(dir->name, dist))
                break;
            if (!strstr(dir->subdir, dist))
                break;
            if (!strstr(dir->rpmdir, dist))
                break;
            dir = dir->next;
        }
    }
    if (dir == NULL) {
        fprintf(stderr, "rpmDirScanOneDist(%s): distribution not found\n",
                dist);
        return;
    }
    if (rpm2htmlVerbose)
        printf("indexing %s\n", dir->name);

    /*
     * Allocate a directory tree.
     */
    dirTree = rpmNewSubdir(NULL, "", "", "", NULL, 1);

    /*
     * Override default setting.
     */
    if ((dir->maint == NULL) && (rpm2html_maint != NULL))
        dir->maint = rpmopenXmlStrdup(rpm2html_maint);
    if ((dir->mail == NULL) && (rpm2html_mail != NULL))
        dir->mail = rpmopenXmlStrdup(rpm2html_mail);
    if ((dir->ftp == NULL) && (rpm2html_ftp != NULL))
        dir->ftp = rpmopenXmlStrdup(rpm2html_ftp);
    if ((dir->ftpsrc == NULL) && (rpm2html_ftpsrc != NULL))
        dir->ftpsrc = rpmopenXmlStrdup(rpm2html_ftpsrc);
    if ((dir->dir == NULL) && (rpm2html_dir != NULL))
        dir->dir = rpmopenXmlStrdup(rpm2html_dir);
    if ((dir->host == NULL) && (rpm2html_host != NULL))
        dir->host = rpmopenXmlStrdup(rpm2html_host);
    if ((dir->name == NULL) && (rpm2html_name != NULL))
        dir->name = rpmopenXmlStrdup(rpm2html_name);
    if ((dir->url == NULL) && (rpm2html_url != NULL))
        dir->url = rpmopenXmlStrdup(rpm2html_url);

    if (dir->rpmdir == NULL) {
        fprintf(stderr, "?!? rpmDir without directory ?!? disabled !\n");
    } else if (!strncmp(dir->rpmdir, "localbase", 9)) {
        /* Scan the local RPM database instead of a directory */
        rpmBaseScan(dir);
    } else if (dir->ftp == NULL) {
        fprintf(stderr, "Directory %s disabled : no ftp field\n",
                dir->rpmdir);
    } else {
#ifdef WITH_SQL
        dir->no = sql_get_distrib_by_directory(dir->rpmdir);
#endif
        if (rpm2htmlVerbose)
            printf("Scanning directory %s for RPMs\n", dir->rpmdir);
        rpmDirScan(dir, dirTree);
    }

    if (dir->root != NULL) {
        rpmDestroyRealRoot(dir->root);
        dir->root = NULL;
    }
    rpmFreeSubdir(dirTree);
    dirTree = NULL;
}

/*
 * Scan all registered directories.
 * One fist check for completeness of the information in
 * the rpmDir structure.
 */

rpmDataPtr
rpmDirScanAll(void)
{
    rpmDirPtr dir,
      next;
    rpmDataPtr ret = NULL,
      cur;

#ifndef WITH_SQL
    int maxLists = 50;
    rpmDataPtr *rpmLists;
    int nbLists = 0;
    int i;
#endif

    /*
     * first reverse the list ....
     */
    dir = dirList;
    dirList = NULL;
    while (dir != NULL) {
        next = dir->next;
        dir->next = dirList;
        dirList = dir;
        dir = next;
    }
    dir = dirList;

    /*
     * Allocate a directory tree.
     */
    dirTree = rpmNewSubdir(NULL, "", "", "", NULL, 1);

#ifndef WITH_SQL
    rpmLists = (rpmDataPtr *) xmlMalloc(maxLists * sizeof(rpmDataPtr));
    if (rpmLists == NULL) {
        fprintf(stderr, "rpmDirScanAll : running out of memory !\n");
        exit(1);
    }
#endif

    while (dir != NULL) {
        cur = NULL;

        /*
         * Complete with default setting.
         */
        if ((dir->maint == NULL) && (rpm2html_maint != NULL))
            dir->maint = rpmopenXmlStrdup(rpm2html_maint);
        if ((dir->mail == NULL) && (rpm2html_mail != NULL))
            dir->mail = rpmopenXmlStrdup(rpm2html_mail);
        if ((dir->ftp == NULL) && (rpm2html_ftp != NULL))
            dir->ftp = rpmopenXmlStrdup(rpm2html_ftp);
        if ((dir->ftpsrc == NULL) && (rpm2html_ftpsrc != NULL))
            dir->ftpsrc = rpmopenXmlStrdup(rpm2html_ftpsrc);
        if ((dir->dir == NULL) && (rpm2html_dir != NULL))
            dir->dir = rpmopenXmlStrdup(rpm2html_dir);
        if ((dir->host == NULL) && (rpm2html_host != NULL))
            dir->host = rpmopenXmlStrdup(rpm2html_host);
        if ((dir->name == NULL) && (rpm2html_name != NULL))
            dir->name = rpmopenXmlStrdup(rpm2html_name);
        if ((dir->url == NULL) && (rpm2html_url != NULL))
            dir->url = rpmopenXmlStrdup(rpm2html_url);

        if (dir->rpmdir == NULL) {
            fprintf(stderr,
                    "?!? rpmDir without directory ?!? disabled !\n");
        } else if (!strncmp(dir->rpmdir, "localbase", 9)) {
            /* Scan the local RPM database instead of a directory */
            cur = rpmBaseScan(dir);
        } else if (dir->ftp == NULL) {
            fprintf(stderr, "Directory %s disabled : no ftp field\n",
                    dir->rpmdir);
        } else {
#ifdef WITH_SQL
            dir->no = sql_get_distrib_by_directory(dir->rpmdir);
#endif
            if (rpm2htmlVerbose)
                printf("Scanning directory %s for RPMs\n", dir->rpmdir);
            cur = rpmDirScan(dir, dirTree);
        }

#ifdef WITH_SQL
        /*
         * The packages list will be reextratced from the
         * database at the end, minimize memory consumption
         */
        rpmlistCleanup();
#else
        if (cur != NULL) {
            if (nbLists >= maxLists) {
                maxLists *= 2;
                rpmLists = (rpmDataPtr *) xmlRealloc(rpmLists,
                                                     maxLists *
                                                     sizeof(rpmDataPtr));
                if (rpmLists == NULL) {
                    fprintf(stderr,
                            "rpmDirScanAll : running out of memory!\n");
                    exit(1);
                }
            }
            rpmLists[nbLists] = cur;
            nbLists++;
        }
#endif
        if (dir->root != NULL) {
            rpmDestroyRealRoot(dir->root);
            dir->root = NULL;
        }
        dir = dir->next;
    }
#ifndef WITH_SQL
    for (i = 0; i < nbLists; i++)
        ret = rpmAddList(ret, rpmLists[i]);
    xmlFree(rpmLists);
#endif
    return (ret);
}

/*
 * Cleanup the global variables from this module
 */
void
rpmopenCleanup(void)
{
    if (buffer != NULL)
        xmlFree(buffer);
    buffer = NULL;
    buffer_size = 50 * 1024 * sizeof(char);
}

