/*
 * html.c: code concerning the dump as an HTML base.
 *
 * See Copyright for the status of this software.
 *
 * $Id: html.c,v 1.127 2010/11/09 22:16:29 hany Exp $
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
#include <errno.h>
#include <ctype.h>

#include <rpm/rpmlib.h>
#include <rpm/rpmtag.h>

#include "rpm2html.h"
#include "rpmdata.h"
#include "html.h"
#include "language.h"

#ifdef HAVE_LIBTEMPLATE
#include "template.h"
#endif

#ifndef HAVE_SNPRINTF
#error You really need snprintf ...
#endif

/*
 * Global variables concerning the dump environment
 */

static char buf[500];

/*
 * The entities list for the extended ASCII charset.
 */
static char *html_flags[] = {
    "",
    "&lt;",
    "&lt;=",
    "&gt;",
    "&gt;=",
    "="
};

/*
 * The entities list for the extended ASCII charset.
 */
static char *entities[256] = {
NULL,		/* 000 */
NULL,		/* 001 */
NULL,		/* 002 */
NULL,		/* 003 */
NULL,		/* 004 */
NULL,		/* 005 */
NULL,		/* 006 */
NULL,		/* 007 */
NULL,		/* 008 */
NULL,		/* 009 */
NULL,		/* 010 */
NULL,		/* 011 */
NULL,		/* 012 */
NULL,		/* 013 */
NULL,		/* 014 */
NULL,		/* 015 */
NULL,		/* 016 */
NULL,		/* 017 */
NULL,		/* 018 */
NULL,		/* 019 */
NULL,		/* 020 */
NULL,		/* 021 */
NULL,		/* 022 */
NULL,		/* 023 */
NULL,		/* 024 */
NULL,		/* 025 */
NULL,		/* 026 */
NULL,		/* 027 */
NULL,		/* 028 */
NULL,		/* 029 */
NULL,		/* 030 */
NULL,		/* 031 */
NULL,		/* 032 */
NULL,		/* 033 */
"&quot;",	/* 034 */
NULL,		/* 035 */
NULL,		/* 036 */
NULL,		/* 037 */
"&amp;",	/* 038 */
NULL,		/* 039 */
NULL,		/* 040 */
NULL,		/* 041 */
NULL,		/* 042 */
NULL,		/* 043 */
NULL,		/* 044 */
NULL,		/* 045 */
NULL,		/* 046 */
NULL,		/* 047 */
NULL,		/* 048 */
NULL,		/* 049 */
NULL,		/* 050 */
NULL,		/* 051 */
NULL,		/* 052 */
NULL,		/* 053 */
NULL,		/* 054 */
NULL,		/* 055 */
NULL,		/* 056 */
NULL,		/* 057 */
NULL,		/* 058 */
NULL,		/* 059 */
"&lt;",		/* 060 */
NULL,		/* 061 */
"&gt;",		/* 062 */
NULL,		/* 063 */
NULL,		/* 064 */
NULL,		/* 065 */
NULL,		/* 066 */
NULL,		/* 067 */
NULL,		/* 068 */
NULL,		/* 069 */
NULL,		/* 070 */
NULL,		/* 071 */
NULL,		/* 072 */
NULL,		/* 073 */
NULL,		/* 074 */
NULL,		/* 075 */
NULL,		/* 076 */
NULL,		/* 077 */
NULL,		/* 078 */
NULL,		/* 079 */
NULL,		/* 080 */
NULL,		/* 081 */
NULL,		/* 082 */
NULL,		/* 083 */
NULL,		/* 084 */
NULL,		/* 085 */
NULL,		/* 086 */
NULL,		/* 087 */
NULL,		/* 088 */
NULL,		/* 089 */
NULL,		/* 090 */
NULL,		/* 091 */
NULL,		/* 092 */
NULL,		/* 093 */
NULL,		/* 094 */
NULL,		/* 095 */
NULL,		/* 096 */
NULL,		/* 097 */
NULL,		/* 098 */
NULL,		/* 099 */
NULL,		/* 100 */
NULL,		/* 101 */
NULL,		/* 102 */
NULL,		/* 103 */
NULL,		/* 104 */
NULL,		/* 105 */
NULL,		/* 106 */
NULL,		/* 107 */
NULL,		/* 108 */
NULL,		/* 109 */
NULL,		/* 110 */
NULL,		/* 111 */
NULL,		/* 112 */
NULL,		/* 113 */
NULL,		/* 114 */
NULL,		/* 115 */
NULL,		/* 116 */
NULL,		/* 117 */
NULL,		/* 118 */
NULL,		/* 119 */
NULL,		/* 120 */
NULL,		/* 121 */
NULL,		/* 122 */
NULL,		/* 123 */
NULL,		/* 124 */
NULL,		/* 125 */
NULL,		/* 126 */
NULL,		/* 127 */
NULL,		/* 128 */
NULL,		/* 129 */
NULL,		/* 130 */
NULL,		/* 131 */
NULL,		/* 132 */
NULL,		/* 133 */
NULL,		/* 134 */
NULL,		/* 135 */
NULL,		/* 136 */
NULL,		/* 137 */
NULL,		/* 138 */
NULL,		/* 139 */
NULL,		/* 140 */
NULL,		/* 141 */
NULL,		/* 142 */
NULL,		/* 143 */
NULL,		/* 144 */
NULL,		/* 145 */
NULL,		/* 146 */
NULL,		/* 147 */
NULL,		/* 148 */
NULL,		/* 149 */
NULL,		/* 150 */
NULL,		/* 151 */
NULL,		/* 152 */
NULL,		/* 153 */
NULL,		/* 154 */
NULL,		/* 155 */
NULL,		/* 156 */
NULL,		/* 157 */
NULL,		/* 158 */
NULL,		/* 159 */
"&nbsp;",	/* 160 */
"&iexcl;",	/* 161 */
"&cent;",	/* 162 */
"&pound;",	/* 163 */
"&curren;",	/* 164 */
"&yen;",	/* 165 */
"&brvbar;",	/* 166 */
"&sect;",	/* 167 */
"&uml;",	/* 168 */
"&copy;",	/* 169 */
"&ordf;",	/* 170 */
"&laquo;",	/* 171 */
"&not;",	/* 172 */
"&shy;",	/* 173 */
"&reg;",	/* 174 */
"&macr;",	/* 175 */
"&deg;",	/* 176 */
"&plusmn;",	/* 177 */
"&sup;",	/* 178 */
"&sup;",	/* 179 */
"&acute;",	/* 180 */
"&micro;",	/* 181 */
"&para;",	/* 182 */
"&middot;",	/* 183 */
"&cedil;",	/* 184 */
"&sup;",	/* 185 */
"&ordm;",	/* 186 */
"&raquo;",	/* 187 */
"&frac;",	/* 188 */
"&frac;",	/* 189 */
"&frac;",	/* 190 */
"&iquest;",	/* 191 */
"&Agrave;",	/* 192 */
"&Aacute;",	/* 193 */
"&Acirc;",	/* 194 */
"&Atilde;",	/* 195 */
"&Auml;",	/* 196 */
"&Aring;",	/* 197 */
"&AElig;",	/* 198 */
"&Ccedil;",	/* 199 */
"&Egrave;",	/* 200 */
"&Eacute;",	/* 201 */
"&Ecirc;",	/* 202 */
"&Euml;",	/* 203 */
"&Igrave;",	/* 204 */
"&Iacute;",	/* 205 */
"&Icirc;",	/* 206 */
"&Iuml;",	/* 207 */
"&ETH;",	/* 208 */
"&Ntilde;",	/* 209 */
"&Ograve;",	/* 210 */
"&Oacute;",	/* 211 */
"&Ocirc;",	/* 212 */
"&Otilde;",	/* 213 */
"&Ouml;",	/* 214 */
"&times;",	/* 215 */
"&Oslash;",	/* 216 */
"&Ugrave;",	/* 217 */
"&Uacute;",	/* 218 */
"&Ucirc;",	/* 219 */
"&Uuml;",	/* 220 */
"&Yacute;",	/* 221 */
"&THORN;",	/* 222 */
"&szlig;",	/* 223 */
"&agrave;",	/* 224 */
"&aacute;",	/* 225 */
"&acirc;",	/* 226 */
"&atilde;",	/* 227 */
"&auml;",	/* 228 */
"&aring;",	/* 229 */
"&aelig;",	/* 230 */
"&ccedil;",	/* 231 */
"&egrave;",	/* 232 */
"&eacute;",	/* 233 */
"&ecirc;",	/* 234 */
"&euml;",	/* 235 */
"&igrave;",	/* 236 */
"&iacute;",	/* 237 */
"&icirc;",	/* 238 */
"&iuml;",	/* 239 */
"&eth;",	/* 240 */
"&ntilde;",	/* 241 */
"&ograve;",	/* 242 */
"&oacute;",	/* 243 */
"&ocirc;",	/* 244 */
"&otilde;",	/* 245 */
"&ouml;",	/* 246 */
"&divide;",	/* 247 */
"&oslash;",	/* 248 */
"&ugrave;",	/* 249 */
"&uacute;",	/* 250 */
"&ucirc;",	/* 251 */
"&uuml;",	/* 252 */
"&yacute;",	/* 253 */
"&thorn;",	/* 254 */
"&yuml;"	/* 255 */
};

#ifdef HAVE_LIBTEMPLATE
/*
 * Template engine used to produce HTML output
 */
struct tpl_engine* engine = NULL;

void templateError(enum tpl_error tplerrno) {
    switch (tplerrno) {
        case tpl_not_found:
            fprintf(stderr, "template not found\n");
            break;
        case tpl_empty:
            fprintf(stderr, "template is empty\n");
            break;
        case tpl_element_not_found:
            fprintf(stderr, "template element not found\n");
            break;
        case tpl_element_empty:
            fprintf(stderr, "template element empty\n");
            break;
        case tpl_error_external:
            fprintf(stderr, "external template error\n");
            break;
        default:
            fprintf(stderr, "unknown template error\n");
            break;
    }
    perror("template engine error");
    exit(1);
}

void initTemplateEngine(const char *templateFile) {
    if (rpm2htmlVerbose)
        printf("Using HTML template: %s\n", templateFile);

    engine = tpl_engine_new();
    tpl_file_load(engine, templateFile);
    if (engine->tplerrno)
        templateError(engine->tplerrno);
}

void destroyTemplateEngine() {
    tpl_engine_delete(engine);
}
#endif

/*
 * Converting extended ASCII charset to valid HTML text.
 * The string returned is a shared location.
 */

static unsigned char *buffer = NULL;
static int buffer_size = 2000;
char *convertHTML(const char *str) {

    unsigned char *cur, *end;
    unsigned char c;

    if (buffer == NULL) {
        buffer = (char *) xmlMalloc(buffer_size * sizeof(char));
	if (buffer == NULL) {
	    perror("xmlMalloc failed");
	    exit(1);
	}
    }
    cur = &buffer[0];
    end = &buffer[buffer_size - 20];

    while (*str != '\0') {
        if (cur >= end) {
	    int delta = cur - buffer;

	    buffer_size *= 2;
	    buffer = (char *) xmlRealloc(buffer, buffer_size * sizeof(char));
	    if (buffer == NULL) {
		perror("xmlRealloc failed");
		exit(1);
	    }
	    end = &buffer[buffer_size - 20];
	    cur = &buffer[delta];
	}
        c = (unsigned char) *(str++);
	if (entities[(unsigned int) c] == NULL) {
	    *(cur++) = c;
	} else {
	    strcpy(cur, entities[(unsigned int) c]);
	    cur += strlen(entities[(unsigned int) c]);
	}
    }
    *cur = '\0';
    return(buffer);
}

/*
 * Converting extended ASCII charset to valid XML text.
 * The string returned is a shared location.
 */

char *convertXML(const char *str) {

    unsigned char *cur, *end;
    unsigned char c;

    if (buffer == NULL) {
        buffer = (char *) xmlMalloc(buffer_size * sizeof(char));
	if (buffer == NULL) {
	    perror("xmlMalloc failed");
	    exit(1);
	}
    }
    cur = &buffer[0];
    end = &buffer[buffer_size - 20];

    while (*str != '\0') {
        if (cur >= end) {
	    int delta = cur - buffer;

	    buffer_size *= 2;
	    buffer = (char *) xmlRealloc(buffer, buffer_size * sizeof(char));
	    if (buffer == NULL) {
		perror("xmlRealloc failed");
		exit(1);
	    }
	    end = &buffer[buffer_size - 20];
	    cur = &buffer[delta];
	}
        c = (unsigned char) *(str++);
	if (((c) == 0x09) || ((c) == 0x0A) || ((c) == 0x0D) ||
	    (((c) >= 0x20) && ((c) <= 0x8F))) {
	    if (entities[(unsigned int) c] == NULL) {
		*(cur++) = c;
	    } else {
		strcpy(cur, entities[(unsigned int) c]);
		cur += strlen(entities[(unsigned int) c]);
	    }
	}
    }
    *cur = '\0';
    return(buffer);
}

/*
 * Convert CAN-yyyy-nnnn and CVE-yyyy-nnnn to hyperlinks
 * The string returned is a shared location.
 */
static unsigned char *buffer2 = NULL;
static int buffer2_size = 2000;
enum STATE { C = 1, CV = 2, CVE = 3, CA = 4, CAN = 5,
             DIGIT1 = 6, DIGIT2 = 7 };
char *convertCVE(const char *str) {
    unsigned char *cur, *end;
    unsigned char c;
    int state = 0;
    int count = 0;
    unsigned char parse[14]; /* "CVE-yyyy-nnnn" */
    unsigned char *p = parse;

    if (rpm2html_cve_linking == 0)
        return str;

    if (buffer2 == NULL) {
        buffer2 = (char *) xmlMalloc(buffer2_size * sizeof(char));
	if (buffer2 == NULL) {
	    perror("xmlMalloc failed");
	    exit(1);
	}
    }
    cur = &buffer2[0];
    end = &buffer2[buffer2_size - 100];

    while (*str != '\0') {
        if (cur >= end) {
	    int delta = cur - buffer2;

	    buffer2_size *= 2;
	    buffer2 = (char *) xmlRealloc(buffer2, buffer2_size * sizeof(char));
	    if (buffer2 == NULL) {
		perror("xmlRealloc failed");
		exit(1);
	    }
	    end = &buffer2[buffer2_size - 100];
	    cur = &buffer2[delta];
        }
 
	c = (unsigned char) *(str++);
	switch (c) {
	case 'C':
	    if (state == 0) {
		*p++ = c;
		state = C;
		continue;
	    }
	    break;
	case 'V':
	    if (state == C) {
		*p++ = c;
		state = CV;
		continue;
	    }
	    break;
	case 'E':
	    if (state == CV) {
		*p++ = c;
		state = CVE;
		continue;
	    }
	    break;
	case 'A':
	    if (state == C) {
		*p++ = c;
		state = CA;
		continue;
	    }
	    break;
	case 'N':
	    if (state == CA) {
		*p++ = c;
		state = CAN;
		continue;
	    }
	    break;
	case '-':
	    if (state == CVE || state == CAN) {
		*p++ = c;
		state = DIGIT1;
		count = 0;
		continue;
	    }
	    else if (state == DIGIT1 && count == 4) {
		*p++ = c;
		state = DIGIT2;
		count = 0;
		continue;
	    }
	    break;
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	    if (state == DIGIT1 || state == DIGIT2) {
		count++;
		if (count == 4) {
		    *p++ = c;
		    if (state == DIGIT2) {
			unsigned char url[90];
			*p = '\0';
			sprintf(url, "<a href=\"http://cve.mitre.org/cgi-bin/cvename.cgi?"
			             "name=%s\">%s</a>", parse, parse);
			strcpy(cur, url);
			cur += strlen(url);
			p = parse;
			state = 0;
		     }
		     continue;
		}
		else if (count < 4) {
		     *p++ = c;
		     continue;
		}
	    }
	    break;
	default:
	    break;
	}
	if (p != parse) {
	    memcpy(cur, parse, p - parse);
	    cur += p - parse;
	    p = parse;
	}
	state = 0;
	*(cur++) = c;
    }
    *cur = '\0';
    return(buffer2);
}

/*
 * Generates signature info (extended ASCII charset)
 * The string returned is a shared location.
 */
#define X(_x)   (unsigned)((_x) & 0xff)

static unsigned char *sigInfoMD5 = "internal MD5: ";
#if defined(WITH_GPG)
static unsigned char *sigInfoGPG = "GPG:\n";
static unsigned char *sigInfoPGP = "PGP:\n";
#else
static unsigned char *sigInfoGPG = "GPG";
static unsigned char *sigInfoPGP = "PGP";
#endif
static unsigned char *sigInfoU = "unknown";

static unsigned char *sbuffer = NULL;
static int sbuffer_size = 2000;
static unsigned char *sbuffer2 = NULL;
static int sbuffer2_size = 3;
char *convertSIG(rpmSigPtr sig) {
    int n1;
    unsigned char *md5sum = (unsigned char *) sig->sig;

    if (sbuffer == NULL) {
        sbuffer = (char *) xmlMalloc(sbuffer_size * sizeof(char));
	if (sbuffer == NULL) {
	    perror("xmlMalloc failed");
	    exit(1);
	}
    }
    if (sbuffer2 == NULL) {
        sbuffer2 = (char *) xmlMalloc(sbuffer2_size * sizeof(char));
	if (sbuffer2 == NULL) {
	    perror("xmlMalloc failed");
	    exit(1);
	}
    }

    switch(sig->tag) {
        case RPMSIGTAG_MD5:
            strncpy(sbuffer, sigInfoMD5, sbuffer_size);
            for(n1 = 0; n1 < sig->size; n1++) {
                snprintf(sbuffer2, sbuffer2_size, "%02x", X(md5sum[n1]));
                strncat(sbuffer, sbuffer2, sbuffer_size);
            }
            return(sbuffer);
        case RPMSIGTAG_GPG:
            #if defined(WITH_GPG)
            strncpy(sbuffer, sigInfoGPG, sbuffer_size);
            if (sig->resolve != NULL)
                strncat(sbuffer, sig->resolve, sbuffer_size);
            return(sbuffer);
            #else
            return(sigInfoGPG);
            #endif
        case RPMSIGTAG_PGP:
            #if defined(WITH_GPG)
            strncpy(sbuffer, sigInfoPGP, sbuffer_size);
            if (sig->resolve != NULL)
                strncat(sbuffer, sig->resolve, sbuffer_size);
            return(sbuffer);
            #else
            return(sigInfoPGP);
            #endif
        default:
            return(sigInfoU);
    }
}

/*
 * Determine whether given HTML output file is older than given timestamp and thus needs updating.
 *
 * Returned 0 means that no update is needed for HTML, non-zero means update is needed.
 *
 * TODO: Make it used also by "resource pages": Resource page gets the timestamp of newest RPM file which contributes
 * to its content and that is used for compare.
 */
int needsDump(char *htmlFileName, time_t rpmFileTimestamp) {
    struct stat buf;

    if (!rpm2html_dump_html_only_if_rpm_newer)
    	// option turned off => update needed always
    	return 1;

	// determine the last modification time of the HTML file
    if (stat(htmlFileName, &buf) != 0)
    	// well, something bad happened so we err in the direction of caution and tell the caller update is needed
    	return 1;

	// compare time stamps
    if (rpmFileTimestamp < buf.st_mtime)
    	return 0;

	return 1;
}

/*
 * Cleanup the global varibales of this module
 */
void htmlCleanup(void) {
    if (buffer != NULL)
        xmlFree(buffer);
    if (buffer2 != NULL)
        xmlFree(buffer2);
    if (sbuffer != NULL)
        xmlFree(sbuffer);
    if (sbuffer2 != NULL)
        xmlFree(sbuffer2);
    buffer = NULL;
    buffer_size = 2000;
    buffer2 = NULL;
    buffer2_size = 2000;
    sbuffer = NULL;
    sbuffer_size = 2000;
    sbuffer2 = NULL;
    sbuffer2_size = 3;
}

/*
 * Converting extended ASCII charset to valid HTML text.
 * A new memory area is allocated for the returned value.
 */

char *xmlStrdupHTML(const char *str) {
    return(xmlStrdup(convertHTML(str)));
}

/*
 * createDirectory : create a directory. It does recurse and create the
 *                   father dir if needed.
 */

void createDirectory(const char *dirname) {
    static char last_dir[2000] = "";

    /*
     * avoid costly syscalls.
     */
    if (!strcmp(last_dir, dirname)) return;

    if (mkdir(dirname, 0777) != 0) {
        switch (errno) {
#ifdef EEXIST
            case EEXIST:
		if (rpm2htmlVerbose > 1)
		    fprintf(stderr, "Directory \"%s\" already exists\n",
		            dirname);
		return;
#endif
	    case ENOENT: {
	        char *father = xmlStrdup(dirname);
		char *cur = &father[strlen(father)];

		while (cur > father) {
		    if (*cur == '/') {
		        *cur = '\0';
			break;
	            }
		    cur--;
		}
		if (cur > father) {
		    createDirectory(father);
		    if (mkdir(dirname, 0777) != 0) {
			fprintf(stderr, "createDirectory \"%s\" failed\n",
			        dirname);
			perror("mkdir failed:");
			return;
		    }
		}
		xmlFree(father);
		
	        break;
	    }
            default:
	        fprintf(stderr, "createDirectory \"%s\" failed\n", dirname);
	        perror("mkdir failed:");
		return;
	}
    } else if (rpm2htmlVerbose)
        fprintf(stderr, "Created directory \"%s\"\n", dirname);
}

/*
 * Stuff needed for the icon creation.
 */

#include <zlib.h>
#include "dir.png.h"
#include "new.png.h"

void dumpDirIcon(void) {
    char path[500];
    char *content;
    int fd;
    /* struct stat *buf; !!!!!! */

    if (!rpm2html_dump_html) return;

    createDirectory(rpm2html_dir);
    snprintf(path, sizeof(path), "%s/dir.png", rpm2html_dir);
    content = read_data_buffer_dir_png();
    if ((fd = creat(path, 0644)) < 0) {
        fprintf(stderr, "creat() failed on %s\n", path);
	return;
    }
    write(fd, content, data_buffer_dir_png_size);
    close(fd);
    snprintf(path, sizeof(path), "%s/new.png", rpm2html_dir);
    content = read_data_buffer_new_png();
    if ((fd = creat(path, 0644)) < 0) {
        fprintf(stderr, "creat() failed on %s\n", path);
	return;
    }
    write(fd, content, data_buffer_new_png_size);
    close(fd);
}

/*
 * checkDate : check whether the last modification time of a file
 *             is anterior to a given time
 */

int checkDate(const char *filename, time_t stamp) {
    struct stat buf;

    if (force) return(0);
    if ((stat(filename, &buf)) != 0) {
        return(0);
    }
    if (buf.st_size < 10) {
        return(0);
    }
    return(buf.st_mtime > stamp);
}

/*
 * checkDirectory : check if this directory exist
 */

int checkDirectory(const char *filename) {
    struct stat buf;

    if (force) return(0);
    if ((stat(filename, &buf)) != 0) {
        return(0);
    }
    if ((S_ISDIR(buf.st_mode)) || (S_ISLNK(buf.st_mode)))
        return(1);
    return(0);
}

/*
 * checkFile : check if this file exist
 */

int checkFile(const char *filename) {
    struct stat buf;

    if (force) return(0);
    if ((stat(filename, &buf)) != 0) {
        return(0);
    }
    return(1);
}

/*
 * Transformation function from rpm to filename.
 */

const char *rpmName(rpmDataPtr cur) {
    static char rbuf[500];

    if (cur->arch != NULL)
	snprintf(rbuf, sizeof(rbuf), "%s-%s-%s.%s", 
	    cur->name, cur->version, cur->release, cur->arch);
    else
	snprintf(rbuf, sizeof(rbuf), "%s-%s-%s", 
	    cur->name, cur->version, cur->release);
    return(rbuf);
}

/*
 * Transformation function from rpm to Software name.
 */

const char *rpmSoftwareName(rpmDataPtr cur) {
    static char rbuf[500];

    snprintf(rbuf, sizeof(rbuf), "%s-%s-%s", 
	cur->name, cur->version, cur->release);
    return(rbuf);
}

/*
 * remove symbols that we do not want in filenames..
 */
const char *cleanName (const char *name) {
    static char cbuf[500];
    char *cur = cbuf;
 
    while ((*name != '\0') && (cur < cbuf + sizeof(cbuf) - 1)) {
        if ((*name == '/')
	 || (*name == ' ')
	 || (*name == '/')
	 || (*name == '"')
	 || (*name == '<')
	 || (*name == '>')
	 || (*name == ':')
	 || (*name == '|')
	 || (*name == '@')
	 || (*name == '\t')
	 || (*name == '\r')
	 || (*name == '\n')) {
	    *cur++ = '_';
	    name++;
	} else *cur++ = *name++;
    }
    *cur = '\0';
    return (cbuf);
}

/*
 * do the URI escaping needed for that name.
 */
const char *escapeName (const char *name) {
    static char cbuf[500];
    char *cur = cbuf, *end = &cbuf[495];
    char tmp;
 
    while ((*name != '\0') && (cur < end)) {
	tmp = *name++;
        if (((tmp >= 'a') && (tmp <= 'z')) ||
            ((tmp >= 'A') && (tmp <= 'Z')) ||
	    ((tmp >= '0') && (tmp <= '9')) ||
	    (tmp == '-') || (tmp == '_') || (tmp == '.') ||
	    (tmp == '!') || (tmp == '~') || (tmp == '*') || (tmp == '\'') ||
	    (tmp == '(') || (tmp == ')')) {
	    *cur++ = tmp;

	} else {
	    int val = (unsigned char)tmp;
	    int hi = val / 0x10, lo = val % 0x10;
	    *cur++ = '%';
	    *cur++ = hi + (hi > 9? 'A'-10 : '0');
	    *cur++ = lo + (lo > 9? 'A'-10 : '0');
	}
    }
    *cur = '\0';
    return (cbuf);
}

/*
 * Generate a bounded HTML filename, preserving the suffix for long names.
 */

static void htmlFileName(char *filename, size_t size, const char *name) {
    const char *suffix = localizedStrings[LANG_HTML_SUFFIX];
    size_t suffix_len = strlen(suffix);

    if (suffix_len >= size) {
        snprintf(filename, size, "%s", suffix);
        return;
    }

    snprintf(filename, size, "%.*s%s",
             (int)(size - suffix_len - 1), cleanName(name), suffix);
}

/*
 * Transformation function from group to filename.
 */

const char *groupName(const char *group) {
    static char gbuf[500];

    htmlFileName(gbuf, sizeof(gbuf), group);
    return(gbuf);
}

/*
 * Transformation function from group to filename.
 */

const char *distribName(const char *distrib) {
    static char dbuf[500];

    htmlFileName(dbuf, sizeof(dbuf), distrib);
    return(dbuf);
}

/*
 * Generate a full URL
 */

void fullURL(char *buf, int len, rpmDirPtr dir, const char *subdir, const char *filename) {
    if (dir == NULL) {
	snprintf(buf, len, "http://%s%s", rpm2html_host, rpm2html_url);
    } else {
	    if (subdir == NULL)
	        snprintf(buf, len, "http://%s%s/%s",
			 rpm2html_host, rpm2html_url,
			 filename);
            else
	        snprintf(buf, len, "http://%s%s/%s/%s",
			 rpm2html_host, rpm2html_url,
			 subdir, filename);
    }
}

/*
 * Generate a full directory path.
 */

void fullPathName(char *buf, int len, rpmDirPtr dir, const char *subdir, const char *filename) {
    if (dir == NULL) {
	snprintf(buf, len, "%s/%s", rpm2html_dir, filename);
    } else {
	    if (subdir == NULL)
	        snprintf(buf, len, "%s/%s", dir->dir, filename);
            else
	        snprintf(buf, len, "%s/%s/%s", dir->dir, subdir, filename);
    }
}

/*
 * Generate a full directory path plus ditinguish with a letter.
 */

void fullPathNameLr(char *buf, int len, rpmDirPtr dir, char *subdir,
                    char *filename, char letter) {
    if (dir == NULL) {
	snprintf(buf, len, "%s/%c%s", rpm2html_dir, letter, filename);
    } else {
	if (subdir == NULL)
	    snprintf(buf, len, "%s/%c%s", dir->dir, letter, filename);
	else
	    snprintf(buf, len, "%s/%s/%c%s", dir->dir, subdir, letter, filename);
    }
}

/*
 * Generate a full directory path plus ditinguish with a number.
 */

void fullPathNameNr(char *buf, int len, rpmDirPtr dir, char *subdir,
                    char *filename, int number) {
    if (dir == NULL) {
	snprintf(buf, len, "%s/%d%s", rpm2html_dir, number, filename);
    } else {
	if (subdir == NULL)
	    snprintf(buf, len, "%s/%d%s", dir->dir, number, filename);
	else
	    snprintf(buf, len, "%s/%s/%d%s", dir->dir, subdir, number, filename);
    }
}

/*
 * Transformation function from vendor to filename.
 */

const char *vendorName(const char *vendor) {
    static char vbuf[500];

    htmlFileName(vbuf, sizeof(vbuf), vendor);
    return(vbuf);
}

/*
 * Transformation function from resource to filename.
 */

const char *resourceName(const char *resource) {
    static char rbuf[500];

    htmlFileName(rbuf, sizeof(rbuf), resource);
    return(rbuf);
}

#ifdef HAVE_LIBTEMPLATE
/*
 * Parse template item and write the result to file
 */
void parseNwriteTemplate(FILE *html, struct tpl_engine *engine, char *template_name) {
    char* result = NULL;

    tpl_parse(engine, template_name, "parse_result", 0);
    result = tpl_element_get(engine, "parse_result");
    if (result)
        fprintf(html, "%s", result);
    else {
        perror("template item parsing failed");
        exit(1);
    }
}

/*
 * Parse meaningfull value into 'p_new_item' element
 */
void fillNewItemElement(struct tpl_engine *engine) {
    tpl_element_set(engine, "url", rpm2html_url);
    tpl_parse(engine, "new_item", "p_new_item", 0);	// XXX maybe not necessary to call this more than once? => optimize to call just once
    tpl_element_set(engine, "new", tpl_element_get(engine, "p_new_item"));
}

/*
 * Clear 'p_new_item' element
 */
void clearNewItemElement(struct tpl_engine *engine) {
    tpl_element_set(engine, "new", "");
}
#endif

/*
 * Generate an HTML header
 */

void generateHtmlHeader(FILE *html, char *title, char *color) {
#ifdef HAVE_LIBTEMPLATE
    tpl_element_set(engine, "generator_name", rpm2html_rpm2html_name);
    tpl_element_set(engine, "generator_version", rpm2html_rpm2html_ver);
    tpl_parse(engine, title, "p_title", 0);
    tpl_element_set(engine, "title", tpl_element_get(engine, "p_title"));

    parseNwriteTemplate(html, engine, "header");

#else	// ifdef HAVE_LIBTEMPLATE

    fprintf(html, "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\">\n");
    fprintf(html, "<html>\n<head>\n<title>%s</title>\n", title);
    fprintf(html, "<meta name=\"GENERATOR\" content=\"%s %s\">\n",
            rpm2html_rpm2html_name, rpm2html_rpm2html_ver);
    if (color == NULL)
        fprintf(html,
	        "</head>\n<body bgcolor=\"#ffffff\" text=\"#000000\">\n");
    else 
        fprintf(html, "</head>\n<body bgcolor=\"%s\" text=\"#000000\">\n",
	        color);
#endif	// ifdef HAVE_LIBTEMPLATE
}

/*
 * Generate an HTML footer
 */

void generateHtmlFooter(FILE *html) {
    struct tm * tstruct;

    tstruct = localtime(&currentTime);

#ifdef HAVE_LIBTEMPLATE
    tpl_element_set(engine, "generator_name", rpm2html_rpm2html_name);
    tpl_element_set(engine, "generator_version", rpm2html_rpm2html_ver);
    tpl_element_set(engine, "generator_url", rpm2html_rpm2html_url);
    tpl_element_set(engine, "maintainer_mail", rpm2html_mail);
    tpl_element_set(engine, "maintainer_name", rpm2html_maint);
    tpl_element_set(engine, "time", asctime(tstruct));

    parseNwriteTemplate(html, engine, "footer");

#else	// ifdef HAVE_LIBTEMPLATE

    fprintf(html, "<hr>\n");
    fprintf(html, "<p>%s <a href=\"%s\">%s %s</a>\n",
            localizedStrings[LANG_GENERATED],
	    rpm2html_rpm2html_url, rpm2html_rpm2html_name,
	    rpm2html_rpm2html_ver);
    if (rpm2html_help != NULL) {
	fprintf(html, "<p><a href=\"%s\">%s</a>, %s\n",
		rpm2html_help, rpm2html_maint, asctime(tstruct));
    } else {
	fprintf(html, "<p><a href=\"mailto:%s\">%s</a>, %s\n",
		rpm2html_mail, rpm2html_maint, asctime(tstruct));
    }
    fprintf(html, "</body>\n</html>\n");
#endif	// ifdef HAVE_LIBTEMPLATE
}

/*
 * Generate a the opening anchor tag for a package
 */
#ifdef HAVE_LIBTEMPLATE
/* note: we diverge here from "ussual templatization" while we do not want to have just
   '<a href="{uri}">' in the template - too small item; instead, this function will return just
   URI and caller function will do the rest */
char *generateHtmlRpmAnchor(rpmDataPtr cur) {
    if (buffer == NULL) {
        buffer = (char *) xmlMalloc(buffer_size * sizeof(char));
	if (buffer == NULL) {
	    perror("xmlMalloc failed");
	    exit(1);
	}
    }
    /* carefull: initial value for buffer_size is 2000 so if URI generated here is longer,
       it'll be truncated thus very likely invalid */

    if (cur->dir == NULL) {
	buffer[0] = '\0';
	return buffer;
    }
    if ((cur->dir->subdir != NULL) && (cur->dir->subdir[0] != '\0')) {
        /*
	 * More than one mirror, there is an HTML subdir
	 */
	if ((cur->subdir != NULL) && (cur->subdir[0] != '\0')) {
	    if (cur->dir->url)
		snprintf(buffer, buffer_size, "%s/%s/%s/%s.html",
			cur->dir->url, cur->dir->subdir,
			cur->subdir, rpmName(cur));
	    else
		snprintf(buffer, buffer_size, "%s/%s/%s.html",
			cur->subdir, cur->dir->subdir, rpmName(cur));
	} else {
	    if (cur->dir->url)
		snprintf(buffer, buffer_size, "%s/%s/%s.html",
		        cur->dir->url, cur->dir->subdir, rpmName(cur));
	    else
		snprintf(buffer, buffer_size, "%s/%s.html",
		        cur->dir->subdir, rpmName(cur));
	}
    } else {
        /*
	 * Only one mirror, no HTML subdir
	 */
	if ((cur->subdir != NULL) && (cur->subdir[0] != '\0')) {
	    if (cur->dir->url)
		snprintf(buffer, buffer_size, "%s/%s/%s.html",
			cur->dir->url, cur->subdir, rpmName(cur));
	    else
		snprintf(buffer, buffer_size, "%s/%s.html",
			cur->subdir, rpmName(cur));
	} else {
	    if (cur->dir->url)
		snprintf(buffer, buffer_size, "%s/%s.html",
		        cur->dir->url, rpmName(cur));
	    else
		snprintf(buffer, buffer_size, "%s.html",
		        rpmName(cur));
	}
    }

    return buffer;
}
#else	// ifdef HAVE_LIBTEMPLATE
void generateHtmlRpmAnchor(FILE *html, rpmDataPtr cur) {
    if (cur->dir == NULL) {
	fprintf(html, "<a href=\"\">");
	return;
    }
    if ((cur->dir->subdir != NULL) && (cur->dir->subdir[0] != '\0')) {
        /*
	 * More than one mirror, there is an HTML subdir
	 */
	if ((cur->subdir != NULL) && (cur->subdir[0] != '\0')) {
	    if (cur->dir->url)
		fprintf(html, "<a href=\"%s/%s/%s/%s.html\">",
			cur->dir->url, cur->dir->subdir,
			cur->subdir, rpmName(cur));
	    else
		fprintf(html, "<a href=\"%s/%s/%s.html\">",
			cur->subdir, cur->dir->subdir, rpmName(cur));
	} else {
	    if (cur->dir->url)
		fprintf(html, "<a href=\"%s/%s/%s.html\">",
		        cur->dir->url, cur->dir->subdir, rpmName(cur));
	    else
		fprintf(html, "<a href=\"%s/%s.html\">",
		        cur->dir->subdir, rpmName(cur));
	}
    } else {
        /*
	 * Only one mirror, no HTML subdir
	 */
	if ((cur->subdir != NULL) && (cur->subdir[0] != '\0')) {
	    if (cur->dir->url)
		fprintf(html, "<a href=\"%s/%s/%s.html\">",
			cur->dir->url, cur->subdir, rpmName(cur));
	    else
		fprintf(html, "<a href=\"%s/%s.html\">",
			cur->subdir, rpmName(cur));
	} else {
	    if (cur->dir->url)
		fprintf(html, "<a href=\"%s/%s.html\">",
		        cur->dir->url, rpmName(cur));
	    else
		fprintf(html, "<a href=\"%s.html\">",
		        rpmName(cur));
	}
    }
}
#endif	// ifdef HAVE_LIBTEMPLATE

/*
 * Generate an RSS link element for a package
 */
void generateRSSLink(FILE *RSS, rpmDataPtr cur) {
    fprintf(RSS, "    <link>http://%s", rpm2html_host);
    if ((cur->dir->subdir != NULL) && (cur->dir->subdir[0] != '\0')) {
        /*
	 * More than one mirror, there is an HTML subdir
	 */
	if ((cur->subdir != NULL) && (cur->subdir[0] != '\0')) {
	    if (cur->dir->url)
		fprintf(RSS, "%s/%s/%s/%s.html",
			cur->dir->url, cur->dir->subdir,
			cur->subdir, rpmName(cur));
	    else
		fprintf(RSS, "%s/%s/%s.html",
			cur->subdir, cur->dir->subdir, rpmName(cur));
	} else {
	    if (cur->dir->url)
		fprintf(RSS, "%s/%s/%s.html",
		        cur->dir->url, cur->dir->subdir, rpmName(cur));
	    else
		fprintf(RSS, "%s/%s.html",
		        cur->dir->subdir, rpmName(cur));
	}
    } else {
        /*
	 * Only one mirror, no HTML subdir
	 */
	if ((cur->subdir != NULL) && (cur->subdir[0] != '\0')) {
	    if (cur->dir->url)
		fprintf(RSS, "%s/%s/%s.html",
			cur->dir->url, cur->subdir, rpmName(cur));
	    else
		fprintf(RSS, "%s/%s.html",
			cur->subdir, rpmName(cur));
	} else {
	    if (cur->dir->url)
		fprintf(RSS, "%s/%s.html",
		        cur->dir->url, rpmName(cur));
	    else
		fprintf(RSS, "%s.html",
		        rpmName(cur));
	}
    }
    fprintf(RSS, "</link>\n");
}

/*
 * Generate a line in a table for a RPM.
 * Don't list the source RPMs
 */

void generateHtmlRpmRow(FILE *html, rpmDataPtr cur, int shownew) {
#ifdef SHOW_DATE
    static char buf[500];
    struct tm * tstruct;
#endif

    if (!strcmp(cur->arch, "src")) return;

#ifdef SHOW_DATE
    tstruct = localtime(&(cur->date));
#ifdef HAVE_STRFTIME
    strftime(buf, sizeof(buf) - 1, "%c", tstruct);
#else
#error "no strftime, please check !"
#endif
#endif

#ifdef HAVE_LIBTEMPLATE
    if (cur->dir != NULL)
        tpl_element_set(engine, "bgcolor", cur->dir->color);
    tpl_element_set(engine, "uri", generateHtmlRpmAnchor(cur));
    tpl_element_set(engine, "name", rpmName(cur));
    if ((shownew) && (cur->date > 0) &&
        ((currentTime - cur->date) < (24 * 60 * 60 * 15)))
        fillNewItemElement(engine);
    else
        clearNewItemElement(engine);
    tpl_element_set(engine, "summary", convertHTML(cur->summary));
#ifdef SHOW_DATE
    tpl_element_set(engine, "system", buf);
#else
    if ((cur->dir != NULL) && (cur->dir->name != NULL))
        tpl_element_set(engine, "system", cur->dir->name);
    else
        tpl_element_set(engine, "system", "");
#endif

    parseNwriteTemplate(html, engine, "rpm_row");

#else	// ifdef HAVE_LIBTEMPLATE

    if (cur->dir != NULL) {
	fprintf(html, "<tr bgcolor=\"%s\"><td width=\"%d\">",
		cur->dir->color, PACKAGE_FIELD_WIDTH);
    } else {
	fprintf(html, "<tr><td width=\"%d\">",
		PACKAGE_FIELD_WIDTH);
    }
    generateHtmlRpmAnchor(html, cur);
    fprintf(html, "%s</a>", rpmName(cur));
    if ((shownew) && (cur->date > 0) &&
        ((currentTime - cur->date) < (24 * 60 * 60 * 15)))
	fprintf(html, "<img src=\"%s/new.png\" alt=\"New\">", rpm2html_url);
    fprintf(html, "</td>\n<td width=\"%d\">%s</td>\n",
            DESCRIPTION_FIELD_WIDTH,
	    convertHTML(cur->summary));
#ifdef SHOW_DATE
    fprintf(html, "<td>%s</td></tr>\n", buf);
#else
    if ((cur->dir != NULL) && (cur->dir->name != NULL))
	fprintf(html, "<td>%s</td></tr>\n", cur->dir->name);
#endif
#endif	// ifdef HAVE_LIBTEMPLATE
}

/*
 * Generate an entry in an RSS channel for an RPM.
 */
void generateRSSItem(FILE *RSS, rpmDataPtr cur) {
    fprintf(RSS, "  <item>\n");
    fprintf(RSS, "    <title>%s</title>\n", rpmName(cur));
    if (cur->summary != NULL)
	fprintf(RSS, "    <description>%s</description>\n",
		convertHTML(cur->summary));
    generateRSSLink(RSS, cur);
    fprintf(RSS, "  </item>\n");
}

/*
 * Generate a line in a table for an RPM software and all it's architectures.
 */

void generateHtmlRpmArchRow(FILE *html, rpmDataPtr cur) {

    rpmDataPtr tmp;

#ifdef HAVE_LIBTEMPLATE
    tpl_element_set(engine, "name", rpmSoftwareName(cur));
    tpl_element_set(engine, "summary", convertHTML(cur->summary));
    parseNwriteTemplate(html, engine, "arch_row");

    /* dump the archs list */
    tmp = cur;
    while (tmp != NULL) {
	if (strcmp(tmp->arch, "src")) {
	    tpl_element_set(engine, "uri", generateHtmlRpmAnchor(tmp));
	    tpl_element_set(engine, "os", tmp->os);
	    tpl_element_set(engine, "arch", tmp->arch);
	    if ((currentTime - tmp->date) < (24 * 60 * 60 * 15))
	        fillNewItemElement(engine);
	    else
	        clearNewItemElement(engine);
	    if (tmp->dir != NULL) {
	        tpl_element_set(engine, "bgcolor", tmp->dir->color);
	        parseNwriteTemplate(html, engine, "arch_row_item_dir");
	    }
	    else
	        parseNwriteTemplate(html, engine, "arch_row_item");
	}
	tmp = tmp->nextArch;
    }
    parseNwriteTemplate(html, engine, "arch_row_end");

#else	// ifdef HAVE_LIBTEMPLATE

    fprintf(html, "<tr><td width=\"%d\">", PACKAGE_FIELD_WIDTH);
    fprintf(html, "%s</td>\n", rpmSoftwareName(cur));
    fprintf(html, "<td width=\"%d\">%s</td>\n",
            DESCRIPTION_FIELD_WIDTH, convertHTML(cur->summary));
    /* dump the archs list */
    tmp = cur;
    while (tmp != NULL) {
	if (strcmp(tmp->arch, "src")) {
	    if (tmp->dir != NULL)
		fprintf(html, "<td bgcolor=\"%s\" width=\"%d\">",
			tmp->dir->color, SYSTEM_FIELD_WIDTH);
	    else
		fprintf(html, "<td width=\"%d\">",
			SYSTEM_FIELD_WIDTH);
	    generateHtmlRpmAnchor(html, tmp);
	    fprintf(html, "%s/%s</a>", tmp->os, tmp->arch);
	    if ((currentTime - tmp->date) < (24 * 60 * 60 * 15))
		fprintf(html, "<img src=\"%s/new.png\" alt=\"New\">",
		        rpm2html_url);
	    fprintf(html, "</td>");
	}
	tmp = tmp->nextArch;
    }
    fprintf(html, "\n</tr>\n");
#endif	// ifdef HAVE_LIBTEMPLATE
}

/*
 * Generate the Links for the main pages
 */

void generateLinks(FILE *html, int installed) {
    int i;

#ifdef HAVE_LIBTEMPLATE
    tpl_element_set(engine, "url", rpm2html_url);

    parseNwriteTemplate(html, engine, "main_header");

#else	// ifdef HAVE_LIBTEMPLATE

    fprintf(html, "<table border=5 cellspacing=5 cellpadding=5>\n");
    fprintf(html, "<tbody>\n<tr>\n");

    fprintf(html, "<td><a href=\"%s/%s\">%s</a></td>\n",
            rpm2html_url, localizedStrings[LANG_INDEX_HTML],
	    localizedStrings[LANG_INDEX]);
    fprintf(html, "<td><a href=\"%s/%s\">%s</a></td>\n",
            rpm2html_url, localizedStrings[LANG_GROUP_HTML],
	    localizedStrings[LANG_SORTED_BY_GROUP]);
    fprintf(html, "<td><a href=\"%s/%s\">%s</a></td>\n",
            rpm2html_url, localizedStrings[LANG_DISTRIB_HTML],
	    localizedStrings[LANG_SORTED_BY_DISTRIB]);
    fprintf(html, "<td><a href=\"%s/%s\">%s</a></td>\n",
            rpm2html_url, localizedStrings[LANG_VENDOR_HTML],
	    localizedStrings[LANG_SORTED_BY_VENDOR]);
    if (installed)
	fprintf(html, "<td><a href=\"%s/%s\">%s</a></td>\n",
		rpm2html_url, localizedStrings[LANG_BYDATE_HTML],
		localizedStrings[LANG_SORTED_BY_IDATE]);
    else
	fprintf(html, "<td><a href=\"%s/%s\">%s</a></td>\n",
		rpm2html_url, localizedStrings[LANG_BYDATE_HTML],
		localizedStrings[LANG_SORTED_BY_CDATE]);
    fprintf(html, "<td><a href=\"%s/%s\">%s</a></td>\n",
            rpm2html_url, localizedStrings[LANG_BYNAME_HTML],
	    localizedStrings[LANG_SORTED_BY_NAME]);
#endif	// ifdef HAVE_LIBTEMPLATE

#ifdef HAVE_LIBTEMPLATE
    if (buffer == NULL) {
        buffer = (char *) xmlMalloc(buffer_size * sizeof(char));
        if (buffer == NULL) {
            perror("xmlMalloc failed");
            exit(1);
        }
    }
#endif
    for (i = 0;i < rpm2html_nb_extra_headers;i++) {
#ifdef HAVE_LIBTEMPLATE
        if ((*rpm2html_headers_url[i] == '/') ||
	    (!strncmp(rpm2html_headers_url[i], "http://", 7)) ||
	    (!strncmp(rpm2html_headers_url[i], "ftp://", 6)) ||
	    (!strncmp(rpm2html_headers_url[i], "mailto", 6)))
	    tpl_element_set(engine, "url", rpm2html_headers_url[i]);
	else {
	    /* carefull: initial value for buffer_size is 2000 so if URI generated here is longer,
	       it'll be truncated thus very likely invalid */
	    snprintf(buffer, buffer_size, "%s/%s", rpm2html_url, rpm2html_headers_url[i]);
	    tpl_element_set(engine, "url", buffer);
	}
	tpl_element_set(engine, "name", rpm2html_headers_name[i]);

        parseNwriteTemplate(html, engine, "main_item");

#else	// ifdef HAVE_LIBTEMPLATE

        if ((*rpm2html_headers_url[i] == '/') ||
	    (!strncmp(rpm2html_headers_url[i], "http://", 7)) ||
	    (!strncmp(rpm2html_headers_url[i], "ftp://", 6)) ||
	    (!strncmp(rpm2html_headers_url[i], "mailto", 6)))
	    fprintf(html, "<td><a href=\"%s\">%s</a></td>\n",
	            rpm2html_headers_url[i], rpm2html_headers_name[i]);
	else
	    fprintf(html, "<td><a href=\"%s/%s\">%s</a></td>\n",
	            rpm2html_url, rpm2html_headers_url[i],
		    rpm2html_headers_name[i]);
#endif	// ifdef HAVE_LIBTEMPLATE
    }

#ifdef HAVE_LIBTEMPLATE
    parseNwriteTemplate(html, engine, "main_footer");
#else	// ifdef HAVE_LIBTEMPLATE
    fprintf(html, "</tr>\n</tbody></table>\n");
#endif	// ifdef HAVE_LIBTEMPLATE
}

/*
 * Generate a color indicator
 */

void generateColorIndicator(FILE *html) {
#ifdef SHOW_DATE
    int nb = 0;
    rpmDirPtr dir = dirList;

#ifdef HAVE_LIBTEMPLATE
    parseNwriteTemplate(html, engine, "color_indicator_header");
    while (dir != NULL) {
	if (strcasecmp(dir->color, "#ffffff")) {
	    if ((nb > 0) && ((nb % MAX_COLOR_PER_LINE) == 0))
		parseNwriteTemplate(html, engine, "color_indicator_eol");
	    tpl_element_set(engine, "color", dir->color);
	    if (dir->name != NULL)
		tpl_element_set(engine, "item", dir->name);
	    else if (dir->mirrors[0] != NULL)
		tpl_element_set(engine, "item", dir->mirrors[0]);
	    else
		tpl_element_set(engine, "item", dir->ftp));
            parseNwriteTemplate(html, engine, "color_indicator_item");
	    nb++;
	}
	dir = dir->next;
    }
    parseNwriteTemplate(html, engine, "color_indicator_footer");

#else	// ifdef HAVE_LIBTEMPLATE

    fprintf(html, "<table align=\"center\"><tbody>\n<tr>\n");
    while (dir != NULL) {
	if (strcasecmp(dir->color, "#ffffff")) {
	    if ((nb > 0) && ((nb % MAX_COLOR_PER_LINE) == 0))
		fprintf(html, "</tr><tr>\n");
	    fprintf(html, "<td bgcolor=\"%s\">", dir->color);
	    if (dir->name != NULL)
		fprintf(html, "%s</td>", dir->name);
	    else if (dir->mirrors[0] != NULL)
		fprintf(html, "%s</td>", dir->mirrors[0]);
	    else
		fprintf(html, "%s</td>", dir->ftp);
	    nb++;
	}
	dir = dir->next;
    }
    fprintf(html, "</tr>\n</tbody></table>\n");
#endif	// ifdef HAVE_LIBTEMPLATE
#endif
}

/*
 * Dump a subtree in an HTML page with all the links
 */

void generateHtmlTree(FILE *html, rpmSubdirPtr tree, int level, int full) {
    int i;

    if (tree->html == 0) return;

#ifdef HAVE_LIBTEMPLATE
    if (buffer == NULL) {
        buffer = (char *) xmlMalloc(buffer_size * sizeof(char));
	if (buffer == NULL) {
	    perror("xmlMalloc failed");
	    exit(1);
	}
    }
    /* carefull: initial value for buffer_size is 2000 so if URI generated here is longer,
       it'll be truncated thus very likely invalid */

    if ((tree->nb_subdirs > 0) || (tree->parent != NULL)) {
	if (level == 0) {
	    parseNwriteTemplate(html, engine, "html_tree_sublevel_header");
        }
	if (tree->color) {
            tpl_element_set(engine, "color", tree->color);
            tpl_parse(engine, "html_tree_header_color", "p_html_tree_header_color", 0);	// XXX maybe not necessary to call this more than once? => optimize to call just once
            tpl_element_set(engine, "color", tpl_element_get(engine, "p_html_tree_header_color"));
        }
        else
            tpl_element_set(engine, "color", "");
	parseNwriteTemplate(html, engine, "html_tree_color");
	if ((level == 0) && (tree->parent != NULL)) {
	    if (tree->parent->htmlpath[0] != '\0') {
                snprintf(buffer, buffer_size, "%s/%s", rpm2html_url, tree->parent->htmlpath);
	        tpl_element_set(engine, "url", buffer);
	    } else {
                tpl_element_set(engine, "url", rpm2html_url);
	    }
	    tpl_element_set(engine, "rpm2html_url", rpm2html_url);
	    parseNwriteTemplate(html, engine, "html_tree_header");
	}
	for (i = 0;i < tree->nb_subdirs;i++) {
	    if (tree->subdirs[i]->html == 0) continue;
	    tpl_element_set(engine, "url", rpm2html_url);
	    tpl_element_set(engine, "suburi", tree->subdirs[i]->htmlpath);
	    tpl_element_set(engine, "name", tree->subdirs[i]->name);
	    parseNwriteTemplate(html, engine, "html_tree_item");
	    if (full)
		generateHtmlTree(html, tree->subdirs[i], level + 1, full);
	}
	parseNwriteTemplate(html, engine, "html_tree_footer");
    }

#else	// ifdef HAVE_LIBTEMPLATE

    if ((tree->nb_subdirs > 0) || (tree->parent != NULL)) {
	if (level == 0)
	    fprintf(html, "<h3>%s</h3>\n", localizedStrings[LANG_SUBDIRS]);
	if (tree->color)
	    fprintf(html, "<blockquote style=\"background : %s\">\n",
	            tree->color);
	else
	    fprintf(html, "<blockquote>\n");
	if ((level == 0) && (tree->parent != NULL)) {
	    if (tree->parent->htmlpath[0] != '\0') {
		fprintf(html, "<p><strong><a href=\"%s/%s/%s\">",
			rpm2html_url, tree->parent->htmlpath,
			localizedStrings[LANG_INDEX_HTML]);
	    } else {
		fprintf(html, "<p><strong><a href=\"%s/%s\">",
			rpm2html_url, localizedStrings[LANG_INDEX_HTML]);
	    }
	    fprintf(html,
	        "<img src=\"%s/dir.png\" alt=\"parent-directory\" border=0>",
	            rpm2html_url);
	    fprintf(html, " ..</a></strong></p>\n");
	}
	for (i = 0;i < tree->nb_subdirs;i++) {
	    if (tree->subdirs[i]->html == 0) continue;
	    fprintf(html, "<p><strong><a href=\"%s/%s/%s\">",
		    rpm2html_url, tree->subdirs[i]->htmlpath,
		    localizedStrings[LANG_INDEX_HTML]);
	    fprintf(html,
	        "<img src=\"%s/dir.png\" alt=\"sub-directory\" border=0>",
	            rpm2html_url);
	    fprintf(html, " %s</a></strong></p>\n", tree->subdirs[i]->name);
	    if (full)
		generateHtmlTree(html, tree->subdirs[i], level + 1, full);
	}
	fprintf(html, "</blockquote>\n");
    }
#endif	// ifdef HAVE_LIBTEMPLATE
}

/*
 * Dump the parent names in an HTML page with all the links
 */

void generateHtmlParentsNames(FILE *html, rpmSubdirPtr tree) {
    int i, j, k;
    rpmSubdirPtr parent;

    if (tree->parent == NULL) return;

    /*
     * get the tree depth.
     */
    i = 0;
    parent = tree->parent;
    while ((parent != NULL) && (parent->parent != NULL)) {
        i++;
	parent = parent->parent;
    }

    /*
     * Dump each level in the parent tree.
     */
#ifdef HAVE_LIBTEMPLATE
    if (buffer == NULL) {
        buffer = (char *) xmlMalloc(buffer_size * sizeof(char));
	if (buffer == NULL) {
	    perror("xmlMalloc failed");
	    exit(1);
	}
    }
    /* carefull: initial value for buffer_size is 2000 so if URI generated here is longer,
       it'll be truncated thus very likely invalid */

    for (j = i; j >= 0;j--) {
	parent = tree;
        for (k = 0;k < j;k++) parent = parent->parent;
	if (parent->htmlpath[0] != '\0') {
            snprintf(buffer, buffer_size, "%s/%s", rpm2html_url, parent->htmlpath);
            tpl_element_set(engine, "url", buffer);
	} else {
            tpl_element_set(engine, "url", rpm2html_url);
	}
	tpl_element_set(engine, "name", parent->name);
	if (j != 0) {
	    tpl_parse(engine, "parent_names_separator", "p_parent_names_separator", 0);	// XXX maybe not necessary to call this more than once? => optimize to call just once
            tpl_element_set(engine, "separator", tpl_element_get(engine, "p_parent_names_separator"));
        }
        else
            tpl_element_set(engine, "separator", "");
	parseNwriteTemplate(html, engine, "parent_names");
    }

#else	// ifdef HAVE_LIBTEMPLATE

    for (j = i; j >= 0;j--) {
	parent = tree;
        for (k = 0;k < j;k++) parent = parent->parent;
	if (parent->htmlpath[0] != '\0') {
	    fprintf(html, "<a href=\"%s/%s/%s\">",
		    rpm2html_url, parent->htmlpath,
		    localizedStrings[LANG_INDEX_HTML]);
	} else {
	    fprintf(html, "<a href=\"%s/%s\">", rpm2html_url,
		    localizedStrings[LANG_INDEX_HTML]);
	}
	fprintf(html, "%s</a>\n", parent->name);
	if (j != 0) fprintf(html, " / ");
    }
#endif	// ifdef HAVE_LIBTEMPLATE
}

/*
 * Dump the whole index for a full complete config file.
 */
void dumpIndex(time_t start_time, int installed) {
    FILE *html;

    if (!rpm2html_dump_html) return;

    if (rpm2htmlVerbose > 1) {
    printf("Dumping %s/%s\n", rpm2html_dir, localizedStrings[LANG_INDEX_HTML]);
    }
    snprintf(buf, sizeof(buf), "%s/%s", rpm2html_dir, localizedStrings[LANG_INDEX_HTML]);

    html = fopen(buf, "w");
    if (html == NULL) {
        fprintf(stderr, "Couldn't save to file %s: %s\n",
	        buf, strerror(errno));
        return;
    }

#ifdef HAVE_LIBTEMPLATE
    tpl_element_set(engine, "host", rpm2html_host);
    if (installed) {
	generateHtmlHeader(html, "header_title_install", NULL);
	generateLinks(html, installed);
    } else {
	generateHtmlHeader(html, "header_title_repository", NULL);
	generateLinks(html, installed);
    }
#ifdef WITH_SQL
    tpl_element_set(engine, "search_host", rpm2html_host);
#else
    tpl_element_set(engine, "search_host", "rpmfind.net");
#endif

    if (rpm2html_install_files != 0) {
        snprintf(buf, sizeof(buf), "%d", rpm2html_install_files);
        tpl_element_set(engine, "count", buf);
        snprintf(buf, sizeof(buf), "%d", rpm2html_install_size / 1024);
        tpl_element_set(engine, "size", buf);
        tpl_parse(engine, "index_header_stats_installed", "p_stats", 0);
    }
    if (rpm2html_files != 0) {
        snprintf(buf, sizeof(buf), "%d", rpm2html_files);
        tpl_element_set(engine, "count", buf);
        snprintf(buf, sizeof(buf), "%d", rpm2html_size / 1024);
        tpl_element_set(engine, "size", buf);
        tpl_parse(engine, "index_header_stats_repository", "p_stats", 0);
    }
    tpl_element_set(engine, "stats", tpl_element_get(engine, "p_stats"));

    parseNwriteTemplate(html, engine, "index_header");  // XXX ugly hack: here we assume that generateHtmlHeader() alredy filled proper value into "title" element of engine
      // later reconsider usage of rpm2html only with libtemplate and then a) remove all those ifdefs and b) restructure html.c so as this ugy hack is not necessary

    generateHtmlTree(html, dirTree, 0, 0);

    snprintf(buf, sizeof(buf), "%d", (int) (time(NULL) - start_time));
    tpl_element_set(engine, "generation_time", buf);
    
    parseNwriteTemplate(html, engine, "index_footer");

#else	// ifdef HAVE_LIBTEMPLATE

    if (installed) {
	snprintf(buf, sizeof(buf), "%s %s", localizedStrings[LANG_WELCOME_INSTALL],
	        rpm2html_host);
	generateHtmlHeader(html, buf, NULL);
	generateLinks(html, installed);
	fprintf(html, "<h1 align=center>%s %s</h1>\n",
		localizedStrings[LANG_WELCOME_INSTALL], rpm2html_host);
    } else {
	snprintf(buf, sizeof(buf), "%s %s", localizedStrings[LANG_WELCOME_REPOSITORY],
	        rpm2html_host);
	generateHtmlHeader(html, buf, NULL);
	generateLinks(html, installed);
	fprintf(html, "<h1 align=center>%s %s</h1>\n",
		localizedStrings[LANG_WELCOME_REPOSITORY], rpm2html_host);
    }

    fprintf(html, "%s\n", localizedStrings[LANG_RPM2HTML_INTRO]);
#ifdef WITH_SQL
    snprintf(buf, sizeof(buf), "http://%s/linux/rpm2html", rpm2html_host);
    fprintf(html, localizedStrings[LANG_SEARCH_FORM], buf);
#else
    fprintf(html, localizedStrings[LANG_SEARCH_FORM],
	    "http://rpmfind.net/linux/rpm2html");
#endif
    if (rpm2html_install_files != 0) {
	fprintf(html, "<h3>");
	fprintf(html, localizedStrings[LANG_INSTALLED_STATS],
		rpm2html_install_files, rpm2html_install_size / 1024);
	fprintf(html, "</h3>\n");
    }
    if (rpm2html_files != 0) {
	fprintf(html, "<h3>");
	fprintf(html, localizedStrings[LANG_STATS],
		rpm2html_files, rpm2html_size / 1024);
	fprintf(html, "</h3>\n");
    }
    fprintf(html, "<ul>\n");
    fprintf(html, "<li>\n%s <a href=\"%s\">%s</a>\n",
            localizedStrings[LANG_LIST],
	    localizedStrings[LANG_GROUP_HTML],
	    localizedStrings[LANG_INDEX_GROUP]);
    fprintf(html, "<li>\n%s <a href=\"%s\">%s</a>\n",
            localizedStrings[LANG_LIST],
	    localizedStrings[LANG_BYDATE_HTML],
	    localizedStrings[LANG_INDEX_CREATION]);
    fprintf(html, "<li>\n%s <a href=\"%s\">%s</a>\n",
            localizedStrings[LANG_LIST],
	    localizedStrings[LANG_BYNAME_HTML],
	    localizedStrings[LANG_INDEX_NAME]);
    fprintf(html, "<li>\n%s <a href=\"%s\">%s</a>\n",
            localizedStrings[LANG_LIST],
	    localizedStrings[LANG_VENDOR_HTML],
	    localizedStrings[LANG_INDEX_VENDOR]);
    fprintf(html, "<li>\n%s <a href=\"%s\">%s</a>\n",
            localizedStrings[LANG_LIST],
	    localizedStrings[LANG_DISTRIB_HTML],
	    localizedStrings[LANG_INDEX_DISTRIB]);
    fprintf(html, "</ul>\n");

    generateHtmlTree(html, dirTree, 0, 0);

    fprintf(html, "<p>%s %d %s</p>\n",
            localizedStrings[LANG_GENERATION_TIME],
            (int) (time(NULL) - start_time),
            localizedStrings[LANG_SECONDS]);
#endif	// ifdef HAVE_LIBTEMPLATE
    generateHtmlFooter(html);
    fclose(html);
}

/*
 * Dump the whole index for a full complete config file.
 */
void dumpTopIndex(rpmDirPtr *sqlDirList) {
    FILE *html;
    int i;

    if (!rpm2html_dump_html) return;

    if (rpm2htmlVerbose > 1) {
    printf("Dumping %s/%s\n", rpm2html_dir, localizedStrings[LANG_INDEX_HTML]);
    }
    snprintf(buf, sizeof(buf), "%s/%s", rpm2html_dir, localizedStrings[LANG_INDEX_HTML]);

    html = fopen(buf, "w");
    if (html == NULL) {
        fprintf(stderr, "Couldn't save to file %s: %s\n",
	        buf, strerror(errno));
        return;
    }

#ifdef HAVE_LIBTEMPLATE
    tpl_element_set(engine, "host", rpm2html_host);
    generateHtmlHeader(html, "header_title_repository", NULL);
    generateLinks(html, 0);
#ifdef WITH_SQL
    tpl_element_set(engine, "search_host", rpm2html_host);
#else
    tpl_element_set(engine, "search_host", "rpmfind.net");
#endif

    if (rpm2html_install_files != 0) {
        snprintf(buf, sizeof(buf), "%d", rpm2html_install_files);
        tpl_element_set(engine, "count", buf);
        snprintf(buf, sizeof(buf), "%d", rpm2html_install_size / 1024);
        tpl_element_set(engine, "size", buf);
        tpl_parse(engine, "index_header_stats_installed", "p_stats", 0);
    }
    if (rpm2html_files != 0) {
        snprintf(buf, sizeof(buf), "%d", rpm2html_files);
        tpl_element_set(engine, "count", buf);
        snprintf(buf, sizeof(buf), "%d", rpm2html_size / 1024);
        tpl_element_set(engine, "size", buf);
        tpl_parse(engine, "index_header_stats_repository", "p_stats", 0);
    }
    tpl_element_set(engine, "stats", tpl_element_get(engine, "p_stats"));

    parseNwriteTemplate(html, engine, "index_header");  // XXX ugly hack: here we assume that generateHtmlHeader() alredy filled proper value into "title" element of engine
      // later reconsicer usage of rpm2html only with libtemplate and then a) remove all those ifdefs and b) restructure html.c so as this ugy hack is not necessary

    if (sqlDirList != NULL) {
	for (i = 0; i < 500;i++)
	    if (sqlDirList[i] != NULL) {
		/*
		 * Check first that the distro is still active and that
		 * the index has been generated
		 */
		if (!checkDirectory(sqlDirList[i]->rpmdir))
		    continue;
		snprintf(buf, sizeof(buf), "%s/%s/index.html",
			 sqlDirList[i]->dir, sqlDirList[i]->subdir);
		if (!checkFile(buf))
		    continue;

                tpl_element_set(engine, "url", sqlDirList[i]->url);
                tpl_element_set(engine, "suburl", sqlDirList[i]->subdir);
                tpl_element_set(engine, "name", sqlDirList[i]->name);
                parseNwriteTemplate(html, engine, "index_sql_dirlist");
	    }
    }

#else	// ifdef HAVE_LIBTEMPLATE

    snprintf(buf, sizeof(buf), "%s %s", localizedStrings[LANG_WELCOME_REPOSITORY],
	    rpm2html_host);
    generateHtmlHeader(html, buf, NULL);
    generateLinks(html, 0);
    fprintf(html, "<h1 align=center>%s %s</h1>\n",
	    localizedStrings[LANG_WELCOME_REPOSITORY], rpm2html_host);

    fprintf(html, "%s\n", localizedStrings[LANG_RPM2HTML_INTRO]);
#ifdef WITH_SQL
    snprintf(buf, sizeof(buf), "http://%s/linux/rpm2html", rpm2html_host);
    fprintf(html, localizedStrings[LANG_SEARCH_FORM], buf);
#else
    fprintf(html, localizedStrings[LANG_SEARCH_FORM],
	    "http://rpmfind.net/linux/rpm2html");
#endif
    if (rpm2html_install_files != 0) {
	fprintf(html, "<h3>");
	fprintf(html, localizedStrings[LANG_INSTALLED_STATS],
		rpm2html_install_files, rpm2html_install_size / 1024);
	fprintf(html, "</h3>\n");
    }
    if (rpm2html_files != 0) {
	fprintf(html, "<h3>");
	fprintf(html, localizedStrings[LANG_STATS],
		rpm2html_files, rpm2html_size / 1024);
	fprintf(html, "</h3>\n");
    }
    fprintf(html, "<ul>\n");
    fprintf(html, "<li>\n%s <a href=\"%s\">%s</a>\n",
            localizedStrings[LANG_LIST],
	    localizedStrings[LANG_GROUP_HTML],
	    localizedStrings[LANG_INDEX_GROUP]);
    fprintf(html, "<li>\n%s <a href=\"%s\">%s</a>\n",
            localizedStrings[LANG_LIST],
	    localizedStrings[LANG_BYDATE_HTML],
	    localizedStrings[LANG_INDEX_CREATION]);
    fprintf(html, "<li>\n%s <a href=\"%s\">%s</a>\n",
            localizedStrings[LANG_LIST],
	    localizedStrings[LANG_BYNAME_HTML],
	    localizedStrings[LANG_INDEX_NAME]);
    fprintf(html, "<li>\n%s <a href=\"%s\">%s</a>\n",
            localizedStrings[LANG_LIST],
	    localizedStrings[LANG_VENDOR_HTML],
	    localizedStrings[LANG_INDEX_VENDOR]);
    fprintf(html, "<li>\n%s <a href=\"%s\">%s</a>\n",
            localizedStrings[LANG_LIST],
	    localizedStrings[LANG_DISTRIB_HTML],
	    localizedStrings[LANG_INDEX_DISTRIB]);
    fprintf(html, "</ul>\n");

    if (sqlDirList != NULL) {
	for (i = 0; i < 500;i++)
	    if (sqlDirList[i] != NULL) {
		/*
		 * Check first that the distro is still active and that
		 * the index has been generated
		 */
		if (!checkDirectory(sqlDirList[i]->rpmdir))
		    continue;
		snprintf(buf, sizeof(buf), "%s/%s/index.html",
			 sqlDirList[i]->dir, sqlDirList[i]->subdir);
		if (!checkFile(buf))
		    continue;

		fprintf(html,
"<p><strong><a href=\"%s/%s/index.html\"><img src=\"/linux/RPM/dir.png\" alt=\"sub-directory\" border=0>%s</a></strong></p>\n",
                        sqlDirList[i]->url, sqlDirList[i]->subdir,
			sqlDirList[i]->name);

	    }
    }
#endif	// ifdef HAVE_LIBTEMPLATE

    generateHtmlFooter(html);
    fclose(html);
}

/*
 * Dump an RPM block as an HTML file.
 */

void dumpRpmHtml(rpmDataPtr rpm, rpmSubdirPtr tree) {
    struct tm * tstruct;
    rpmDirPtr dir = rpm->dir;
    int installed = dir->installbase;
    FILE *html;
    int i;

    if (!rpm2html_dump_html) return;

    /*
     * create the directory on the fly if needed.
     */
    if ((dir->subdir != NULL) && (dir->subdir[0] != '\0')) {
	if ((rpm->subdir != NULL) && (rpm->subdir[0] != '\0'))
	    snprintf(buf, sizeof(buf), "%s/%s/%s", dir->dir, dir->subdir,
	            rpm->subdir);
	else
	    snprintf(buf, sizeof(buf), "%s/%s", dir->dir, dir->subdir);
    } else {
	if ((rpm->subdir != NULL) && (rpm->subdir[0] != '\0'))
	    snprintf(buf, sizeof(buf), "%s/%s", dir->dir, rpm->subdir);
	else
	    snprintf(buf, sizeof(buf), "%s", dir->dir);
    }

    createDirectory(buf);
    strcat(buf, "/");
    strcat(buf, rpmName(rpm));
    strcat(buf, ".html");

    if (!needsDump(buf, rpm->extra->stamp))
    	return;

    if (rpm2htmlVerbose > 1) {
        printf("Dumping %s\n", buf);
    }

    html = fopen(buf, "w");
    if (html == NULL) {
        fprintf(stderr, "Couldn't save to file %s: %s\n",
	        buf, strerror(errno));
        return;
    }
#ifdef HAVE_LIBTEMPLATE
    tpl_element_set(engine, "name", rpmName(rpm));
    generateHtmlHeader(html, "rpm_title", NULL);
    generateLinks(html, installed);
    if ((rpm->subdir != NULL) && (rpm->subdir[0] != '\0')) {
	if (dir->mirrors[0] != NULL)
	    snprintf(buf, sizeof(buf), "%s/%s/%s", dir->mirrors[0], rpm->subdir, rpm->filename);
	else
	    snprintf(buf, sizeof(buf), "%s/%s/%s", dir->ftp, rpm->subdir, rpm->filename);
    } else {
	if (dir->mirrors[0] != NULL)
	    snprintf(buf, sizeof(buf), "%s/%s", dir->mirrors[0], rpm->filename);
	else
	    snprintf(buf, sizeof(buf), "%s/%s", dir->ftp, rpm->filename);
    }
    tpl_element_set(engine, "url", buf);
    tpl_element_set(engine, "name", rpm->name);
    tpl_element_set(engine, "version", rpm->version);
    tpl_element_set(engine, "release", rpm->release);
    if (rpm->arch) {
        if (!strcmp(rpm->arch, "src"))
            tpl_parse(engine, "rpm_subtitle_src", "p_subtitle", 0);
        else {
            tpl_element_set(engine, "arch", rpm->arch);
            tpl_parse(engine, "rpm_subtitle_arch", "p_subtitle", 0);
        }
    } else
        tpl_parse(engine, "rpm_subtitle_arch_unknown", "p_subtitle", 0);
    tpl_element_set(engine, "subtitle", tpl_element_get(engine, "p_subtitle"));
    parseNwriteTemplate(html, engine, "rpm_subtitle");

    parseNwriteTemplate(html, engine, "rpm_subsubtitle_begin");
    if ((tree != NULL) && (tree->parent != NULL)) {
	generateHtmlParentsNames(html, tree);
    } else if (dir->name) {
        tpl_element_set(engine, "name", dir->name);
        if (dir->ftp) {
            tpl_element_set(engine, "url", dir->ftp);
            parseNwriteTemplate(html, engine, "rpm_subsubtitle_body");
        }
        else
            parseNwriteTemplate(html, engine, "rpm_subsubtitle_body_nourl");
    }
    parseNwriteTemplate(html, engine, "rpm_subsubtitle_end");

    tpl_element_set(engine, "color", dir->color);
    tpl_element_set(engine, "name", rpm->name);
    tpl_element_set(engine, "url", rpm2html_url);
    tpl_element_set(engine, "distrib_url", distribName(rpm->distribution));
    tpl_element_set(engine, "distribution", convertHTML(rpm->distribution));
    tpl_element_set(engine, "version", rpm->version);
    tpl_element_set(engine, "vendor_url", vendorName(rpm->vendor));
    tpl_element_set(engine, "vendor", convertHTML(rpm->vendor));
    tstruct = localtime(&(rpm->date));
#ifdef HAVE_STRFTIME
    strftime(buf, sizeof(buf) - 1, "%c", tstruct);
#else
#error "no strftime, please check !"
#endif
    tpl_element_set(engine, "release", rpm->release);
    tpl_element_set(engine, "date", buf);
    if (installed)
        tpl_parse(engine, "rpm_body_date_install", "p_date_title", 0);
    else
        tpl_parse(engine, "rpm_body_date_build", "p_date_title", 0);
    tpl_element_set(engine, "date_title", tpl_element_get(engine, "p_date_title"));
    if (rpm2html_url != NULL) {
        snprintf(buf, sizeof(buf), "%s/%s", rpm2html_url, groupName(rpm->group));
        tpl_element_set(engine, "group_url", buf);
    }
    else
        tpl_element_set(engine, "group_url", groupName(rpm->group));
    tpl_element_set(engine, "group_url", buf);
    tpl_element_set(engine, "group", convertHTML(rpm->group));
    tpl_element_set(engine, "build_host", rpm->extra->host);
    snprintf(buf, sizeof(buf), "%d", rpm->size);
    tpl_element_set(engine, "size", buf);
    tpl_element_set(engine, "srcrpm_name", rpm->extra->srcrpm);
    if (dir->ftpsrc) {
        snprintf(buf, sizeof(buf), "%s/%s", dir->ftpsrc, rpm->extra->srcrpm);
        tpl_element_set(engine, "srcrpm_url", buf);
        tpl_parse(engine, "rpm_body_src_rpm_name_and_url", "p_source_rpm", 0);
    } else {
        tpl_parse(engine, "rpm_body_src_rpm_name", "p_source_rpm", 0);
    }
    tpl_element_set(engine, "source_rpm", tpl_element_get(engine, "p_source_rpm"));
    if (rpm->extra->packager) {
        tpl_element_set(engine, "packager", convertHTML(rpm->extra->packager));
        char *email = extractEMail(rpm->extra->packager);
	if (email == NULL)
	    tpl_parse(engine, "rpm_body_packager_without_email", "p_packager", 0);
        else {
	    tpl_element_set(engine, "packager_email", email);
	    tpl_parse(engine, "rpm_body_packager_with_email", "p_packager", 0);
        }
        tpl_element_set(engine, "packager", tpl_element_get(engine, "p_packager"));
    }
    else
        tpl_element_set(engine, "packager", "");
    if (rpm->url)
        tpl_element_set(engine, "home_url", rpm->url);
    else
        tpl_element_set(engine, "home_url", "");
    tpl_element_set(engine, "summary", convertHTML(rpm->summary));
    tpl_element_set(engine, "description", convertHTML(rpm->extra->description));
    parseNwriteTemplate(html, engine, "rpm_body");

    if ((rpm->extra->nb_resources + rpm->extra->nb_requires < 2) &&
        (!strstr(rpm->name, "lib")) &&
	(!strcmp(rpm->arch, "src"))) {

        if (rpm2htmlVerbose > 1)
            fprintf(stderr, "Resource lists problem : %s\n", rpmName(rpm));
        tpl_element_set(engine, "url", rpm2html_url);
        tpl_element_set(engine, "suburl", resourceName(rpm->name));
        parseNwriteTemplate(html, engine, "rpm_resource_list_problem");
    }
    if (rpm->extra->nb_resources > 0) {
       parseNwriteTemplate(html, engine, "rpm_provides_header");
       for (i = 0;i < rpm->extra->nb_resources;i++) {
#ifdef WITH_SQL
           if (rpm2html_search != NULL)
               snprintf(buf, sizeof(buf), "%s?query=%s", rpm2html_search,
                   escapeName(rpm->extra->resources[i]->name));
	   else
#endif
	   if (rpm2html_url != NULL)
	       snprintf(buf, sizeof(buf), "%s/%s", rpm2html_url,
	           resourceName(rpm->extra->resources[i]->name));
	   else
	       snprintf(buf, sizeof(buf), "%s", resourceName(rpm->extra->resources[i]->name));
           tpl_element_set(engine, "url", buf);
           tpl_element_set(engine, "name", rpm->extra->resources[i]->name);
           parseNwriteTemplate(html, engine, "rpm_provides_item");
       }
       parseNwriteTemplate(html, engine, "rpm_provides_footer");
    }
    if (rpm->extra->nb_requires > 0) {
       parseNwriteTemplate(html, engine, "rpm_requires_header");
       for (i = 0;i < rpm->extra->nb_requires;i++) {
	   if ((rpm->extra->requires[i]->flag != RPM2HTML_REQ_NONE) &&
	       (rpm->extra->requires[i]->version != NULL)) {
	       snprintf(buf, sizeof(buf), " %s %s", html_flags[rpm->extra->requires[i]->flag],
	           rpm->extra->requires[i]->version);
               tpl_element_set(engine, "version", buf);
           } else
               tpl_element_set(engine, "version", "");
#ifdef WITH_SQL
           if (rpm2html_search != NULL) {
               snprintf(buf, sizeof(buf), "%s?query=%s", rpm2html_search,
		   escapeName(rpm->extra->requires[i]->name));
               tpl_element_set(engine, "url", buf);
	   } else 
#endif
	   if (rpm2html_url != NULL) {
	       snprintf(buf, sizeof(buf), "%s/%s", rpm2html_url,
	           resourceName(rpm->extra->requires[i]->name));
               tpl_element_set(engine, "url", buf);
	   } else {
	       tpl_element_set(engine, "url", resourceName(rpm->extra->requires[i]->name));
	   }
           tpl_element_set(engine, "name", rpm->extra->requires[i]->name);
           parseNwriteTemplate(html, engine, "rpm_requires_item");
       }
       parseNwriteTemplate(html, engine, "rpm_requires_footer");
    }
    if (rpm->extra->copyright) {
       tpl_element_set(engine, "copyright", convertHTML(rpm->extra->copyright));
       parseNwriteTemplate(html, engine, "rpm_copyright");
    }
    if (rpm->extra->sigs != NULL) {
        parseNwriteTemplate(html, engine, "rpm_signatures_header");
	for(i = 0; i < rpm->extra->nb_sigs; i++) {
	    tpl_element_set(engine, "signature", convertHTML(convertSIG(rpm->extra->sigs[i])));
	    parseNwriteTemplate(html, engine, "rpm_signatures_item");
        }
    }
    if (rpm->extra->changelog) {
        tpl_element_set(engine, "changelog", convertCVE(convertHTML(rpm->extra->changelog)));
        parseNwriteTemplate(html, engine, "rpm_changelog");
    }
    parseNwriteTemplate(html, engine, "rpm_files_header");
    if (rpm->extra->filelist == NULL)
        parseNwriteTemplate(html, engine, "rpm_files_body_no_files");
    else {
        tpl_element_set(engine, "files", rpm->extra->filelist);
        parseNwriteTemplate(html, engine, "rpm_files_body");
    }

#else	// ifdef HAVE_LIBTEMPLATE

    snprintf(buf, sizeof(buf), "%s RPM", rpmName(rpm));
    generateHtmlHeader(html, buf, NULL);
    generateLinks(html, installed);
    if ((rpm->subdir != NULL) && (rpm->subdir[0] != '\0')) {
	if (dir->mirrors[0] != NULL)
	    fprintf(html, "<h1 align=center><a href=\"%s/%s/%s\">\n",
		dir->mirrors[0], rpm->subdir, rpm->filename);
	else
	    fprintf(html, "<h1 align=center><a href=\"%s/%s/%s\">\n",
		dir->ftp, rpm->subdir, rpm->filename);
    } else {
	if (dir->mirrors[0] != NULL)
	    fprintf(html, "<h1 align=center><a href=\"%s/%s\">\n",
		dir->mirrors[0], rpm->filename);
	else
	    fprintf(html, "<h1 align=center><a href=\"%s/%s\">\n",
		dir->ftp, rpm->filename);
    }
    if (rpm->arch) {
        if (!strcmp(rpm->arch, "src"))
	    fprintf(html, "%s-%s-%s Source RPM</a></h1>\n",
	            rpm->name, rpm->version, rpm->release);
        else
	    fprintf(html, "%s-%s-%s RPM for %s</a></h1>\n",
		rpm->name, rpm->version, rpm->release, rpm->arch);
    } else {
	fprintf(html, "%s-%s-%s RPM</a></h1>\n",
            rpm->name, rpm->version, rpm->release);
    }

    if ((tree != NULL) && (tree->parent != NULL)) {
	fprintf(html, "<h3 align=center>%s ", localizedStrings[LANG_FROM]);
	generateHtmlParentsNames(html, tree);
	fprintf(html, "</h3>\n");
    } else if (dir->name) {
        fprintf(html, "<h3 align=center>%s <a href=\"%s\">%s</a></h3>\n",
	      localizedStrings[LANG_FROM],
	      dir->ftp, dir->name);
    }

    fprintf(html, "<table align=center border=5 cellspacing=5 cellpadding=5 bgcolor=\"%s\">", dir->color);
    fprintf(html, "<tbody>\n");
    fprintf(html, "<tr><td>%s: %s</td>\n",
	    localizedStrings[LANG_NAME],
            rpm->name);
    fprintf(html, "<td>%s: <a href=\"%s/%s\">%s</a></td></tr>\n",
	    localizedStrings[LANG_DISTRIBUTION], rpm2html_url,
            distribName(rpm->distribution), convertHTML(rpm->distribution));
    fprintf(html, "<tr><td>%s: %s</td>\n",
	    localizedStrings[LANG_VERSION],
            rpm->version);
    fprintf(html, "<td>%s: <a href=\"%s/%s\">%s</a></td></tr>\n",
	    localizedStrings[LANG_VENDOR], rpm2html_url,
            vendorName(rpm->vendor), convertHTML(rpm->vendor));
    tstruct = localtime(&(rpm->date));
#ifdef HAVE_STRFTIME
    strftime(buf, sizeof(buf) - 1, "%c", tstruct);
#else
#error "no strftime, please check !"
#endif
    if (installed) {
	fprintf(html, "<tr><td>%s: %s</td>\n<td>%s: %s</td></tr>\n",
		localizedStrings[LANG_RELEASE],
		rpm->release,
		localizedStrings[LANG_INSTALL_DATE],
		buf);
    } else {
	fprintf(html, "<tr><td>%s: %s</td>\n<td>%s: %s</td></tr>\n",
		localizedStrings[LANG_RELEASE],
		rpm->release,
		localizedStrings[LANG_BUILD_DATE],
		buf);
    }
    if (rpm2html_url != NULL)
	fprintf(html, "<tr><td>%s: <a href=\"%s/%s\">%s</a></td>\n",
		localizedStrings[LANG_GROUP], rpm2html_url,
		groupName(rpm->group), convertHTML(rpm->group));
    else
	fprintf(html, "<tr><td>%s: <a href=\"%s\">%s</a></td>\n",
		localizedStrings[LANG_GROUP],
		groupName(rpm->group), convertHTML(rpm->group));
    fprintf(html, "<td>%s: %s</td></tr>\n",
	    localizedStrings[LANG_BUILD_HOST], rpm->extra->host);
    fprintf(html, "<tr><td>%s: %d</td>\n",
	    localizedStrings[LANG_SIZE],
            rpm->size);
    if (dir->ftpsrc) {
	fprintf(html, "<td>%s: <a href=\"%s/%s\">%s</a></td></tr>\n",
	        localizedStrings[LANG_RPM_SRC],
		dir->ftpsrc, rpm->extra->srcrpm, rpm->extra->srcrpm);
    } else {
	fprintf(html, "<td>%s: %s</td></tr>\n",
	        localizedStrings[LANG_RPM_SRC],
		rpm->extra->srcrpm);
    }
    if (rpm->extra->packager) {
        char *email = extractEMail(rpm->extra->packager);
	if (email == NULL)
	    fprintf(html, "<tr><td colspan=\"2\">%s: %s</td></tr>\n",
		    localizedStrings[LANG_PACKAGER],
		    convertHTML(rpm->extra->packager));
        else
	    fprintf(html, "<tr><td colspan=\"2\">%s: <a href=\"mailto:%s\">%s</a></td></tr>\n",
		    localizedStrings[LANG_PACKAGER],
		    email, convertHTML(rpm->extra->packager));
    }
    if (rpm->url)
	fprintf(html, "<tr><td colspan=\"2\">%s: <a href=\"%s\">%s</a></td></tr>\n",
		localizedStrings[LANG_URL],
		rpm->url, rpm->url);
    fprintf(html, "<tr><td colspan=\"2\">%s: %s</td></tr>\n",
	    localizedStrings[LANG_SUMMARY],
            convertHTML(rpm->summary));
    fprintf(html, "</tbody>\n</table>\n");
    fprintf(html, "<pre>%s\n</pre>\n", convertHTML(rpm->extra->description));
    if ((rpm->extra->nb_resources + rpm->extra->nb_requires < 2) &&
        (!strstr(rpm->name, "lib")) &&
	(!strcmp(rpm->arch, "src"))) {
       if (rpm2htmlVerbose > 1)
	   fprintf(stderr, "Resource lists problem : %s\n", rpmName(rpm));
       fprintf(html, "<h2 align=center style=\"color : #ff0000\">%s</h2>\n",
               localizedStrings[LANG_WARNING_RESOURCES]);
       fprintf(html, "<h2 align=center><a href=\"%s/%s\">%s</a></h2>\n",
               rpm2html_url, resourceName(rpm->name),
               localizedStrings[LANG_CHOOSE_ANOTHER]);
    }
    if (rpm->extra->nb_resources > 0) {
       fprintf(html, "<h3>%s</h3>\n",
	    localizedStrings[LANG_PROVIDE]);
       fprintf(html, "<ul>\n");
       for (i = 0;i < rpm->extra->nb_resources;i++) {
#ifdef WITH_SQL
           if (rpm2html_search != NULL)
	       fprintf(html, "<li><a href=\"%s?query=%s\">%s</a>\n",
		   rpm2html_search,
		   escapeName(rpm->extra->resources[i]->name),
		   rpm->extra->resources[i]->name);
	   else
#endif
	   if (rpm2html_url != NULL)
	       fprintf(html, "<li><a href=\"%s/%s\">%s</a>\n",
		   rpm2html_url,
		   resourceName(rpm->extra->resources[i]->name),
		   rpm->extra->resources[i]->name);
	   else
	       fprintf(html, "<li><a href=\"%s\">%s</a>\n",
		       resourceName(rpm->extra->resources[i]->name),
		       rpm->extra->resources[i]->name);
       }
       fprintf(html, "</ul>\n");
    }
    if (rpm->extra->nb_requires > 0) {
       fprintf(html, "<h3>%s</h3>\n",
	    localizedStrings[LANG_REQUIRE]);
       fprintf(html, "<ul>\n");
       for (i = 0;i < rpm->extra->nb_requires;i++) {
#ifdef WITH_SQL
           if (rpm2html_search != NULL) {
	       if ((rpm->extra->requires[i]->flag != RPM2HTML_REQ_NONE) &&
		   (rpm->extra->requires[i]->version != NULL)) {
		   fprintf(html, "<li><a href=\"%s?query=%s\">%s</a> %s %s\n",
			   rpm2html_search,
			   escapeName(rpm->extra->requires[i]->name), 
			   rpm->extra->requires[i]->name,
			   html_flags[rpm->extra->requires[i]->flag],
			   rpm->extra->requires[i]->version);
	       } else {
		   fprintf(html, "<li><a href=\"%s?query=%s\">%s</a>\n",
			   rpm2html_search,
			   escapeName(rpm->extra->requires[i]->name), 
			   rpm->extra->requires[i]->name);
	       }
	   } else 
#endif
	   if (rpm2html_url != NULL) {
	       if ((rpm->extra->requires[i]->flag != RPM2HTML_REQ_NONE) &&
		   (rpm->extra->requires[i]->version != NULL)) {
		   fprintf(html, "<li><a href=\"%s/%s\">%s</a> %s %s\n",
			   rpm2html_url,
			   resourceName(rpm->extra->requires[i]->name), 
			   rpm->extra->requires[i]->name,
			   html_flags[rpm->extra->requires[i]->flag],
			   rpm->extra->requires[i]->version);
	       } else {
		   fprintf(html, "<li><a href=\"%s/%s\">%s</a>\n",
			   rpm2html_url,
			   resourceName(rpm->extra->requires[i]->name), 
			   rpm->extra->requires[i]->name);
	       }
	   } else {
	       if ((rpm->extra->requires[i]->flag != RPM2HTML_REQ_NONE) &&
		   (rpm->extra->requires[i]->version != NULL)) {
		   fprintf(html, "<li><a href=\"%s\">%s</a> %s %s\n",
			   resourceName(rpm->extra->requires[i]->name),
			   rpm->extra->requires[i]->name,
			   html_flags[rpm->extra->requires[i]->flag],
			   rpm->extra->requires[i]->version);
	       } else {
		   fprintf(html, "<li><a href=\"%s\">%s</a>\n",
			   resourceName(rpm->extra->requires[i]->name),
			   rpm->extra->requires[i]->name);
	       }
	   }
       }
       fprintf(html, "</ul>\n");
    }
    if (rpm->extra->copyright) {
       fprintf(html, "<h3>%s</h3>\n",
	    localizedStrings[LANG_COPYRIGHT]);
	fprintf(html, "<pre>%s\n</pre>\n", convertHTML(rpm->extra->copyright));
    }
    if (rpm->extra->sigs != NULL) {
       fprintf(html, "<h3>%s</h3>\n",
	    localizedStrings[LANG_SIGNATURES]);
	for(i = 0; i < rpm->extra->nb_sigs; i++)
	    fprintf(html, "<pre>%s\n</pre>\n", convertHTML(convertSIG(rpm->extra->sigs[i])));
    }
    if (rpm->extra->changelog) {
        fprintf(html, "<h3>%s</h3>\n",
	    localizedStrings[LANG_CHANGELOG]);
	fprintf(html, "<pre>%s\n</pre>\n", convertCVE(convertHTML(rpm->extra->changelog)));
    }
    fprintf(html, "<h3>%s</h3>\n",
	localizedStrings[LANG_FILES]);
    if (rpm->extra->filelist == NULL) 
	fprintf(html, "<bold>%s</bold>\n",
	     localizedStrings[LANG_NO_FILES]);
    else
	fprintf(html, "<pre>%s\n</pre>\n", rpm->extra->filelist);
#endif	// ifdef HAVE_LIBTEMPLATE
    
    generateHtmlFooter(html);
    fclose(html);
}

#ifdef WITH_SQL
/*
 * Dump a resource HTML file redirecting to the search engine.
 */

void dumpRessRedirHtml(const char *name) {
    FILE *html;

    if (!rpm2html_dump_html) return;

    /*
     * Guess why ??? index.html gets ovewritten !
     */
    if (!strcmp(name, "index"))
	return;

    snprintf(buf, sizeof(buf), "%s/%s", rpm2html_dir, resourceName(name));

    if (rpm2htmlVerbose > 1) {
	printf("Dumping %s/%s\n", rpm2html_dir, resourceName(name));
    }

    html = fopen(buf, "w");
    if (html == NULL) {
        fprintf(stderr, "Couldn't save to file %s: %s\n",
	        buf, strerror(errno));
        return;
    }
    snprintf(buf, sizeof(buf), "%s %s",
	 localizedStrings[LANG_RPM_RESOURCE], name);
    fprintf(html, "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\">\n");
    fprintf(html, "<html>\n<head>\n<title>%s</title>\n", buf);
    fprintf(html, "<meta name=\"GENERATOR\" content=\"%s %s\">\n",
            rpm2html_rpm2html_name, rpm2html_rpm2html_ver);
    if (rpm2html_search != NULL)
	fprintf(html,
	    "<META HTTP-EQUIV=\"refresh\" CONTENT=\"0; URL=%s?query=%s\">\n", 
	    rpm2html_search, escapeName(name));
    else
	fprintf(html,
	    "<META HTTP-EQUIV=\"refresh\" CONTENT=\"0; URL=http://rpmfind.net/linux/rpm2html/search.php?query=%s\">\n", 
	        escapeName(name));
    fprintf(html, "</head>\n<body bgcolor=\"#ffffff\" text=\"#000000\">\n");
    generateLinks(html, 0);
    
    fprintf(html, "<h1 align=center>%s %s</h1>\n",
	 localizedStrings[LANG_RPM_RESOURCE], name);
    fprintf(html, "<p>This static page has been replaced\n");
    if (rpm2html_search != NULL)
	fprintf(html, "<a href=\"%s?query=%s\">",
		rpm2html_search, escapeName(name));
    else
	fprintf(html,
	  "<a href=\"http://rpmfind.net/linux/rpm2html/search.php?query=%s\">",
	        escapeName(name));
    fprintf(html, "by a dynamic one</a></p>\n");
    fprintf(html, "<p>Thank you for updating your bookmarks</p>\n");
    generateHtmlFooter(html);
    fclose(html);
}
#endif

/*
 * Dump a resource block as an HTML file.
 */

void dumpRessHtml(rpmRessPtr ress, int installed) {
    rpmDataPtr rpm;
    FILE *html;
    int i;
    const char *name = NULL;

    if (!rpm2html_dump_html) return;
#ifdef WITH_SQL
    if (rpm2html_search != NULL) {
	dumpRessRedirHtml(ress->name);
	return;
    }
#endif

    /*
     * Guess why ??? index.html gets ovewritten !
     */
    if (!strcmp(ress->name, "index"))
	return;

    rpmRessSort(ress);

    snprintf(buf, sizeof(buf), "%s/%s", rpm2html_dir,
           resourceName(ress->name));
    /*
     * !!!!!
     * if (checkDate(buf, ress->stamp)) return;
     */
    if (rpm2htmlVerbose > 1) {
    printf("Dumping %s/%s\n", rpm2html_dir,
           resourceName(ress->name));
    }

    html = fopen(buf, "w");
    if (html == NULL) {
        fprintf(stderr, "Couldn't save to file %s: %s\n",
	        buf, strerror(errno));
        return;
    }

#ifdef HAVE_LIBTEMPLATE
    tpl_element_set(engine, "name", ress->name);
    generateHtmlHeader(html, "resource_title", NULL);
    generateLinks(html, installed);
    generateColorIndicator(html);
    
    parseNwriteTemplate(html, engine, "resource_header");  // XXX ugly hack: here we assume that generateHtmlHeader() alredy filled proper value into "title" element of engine
      // later reconsicer usage of rpm2html only with libtemplate and then a) remove all those ifdefs and b) restructure html.c so as this ugy hack is not necessary

    parseNwriteTemplate(html, engine, "resource_body_begin");
    for (i = 0;i < ress->nb_provider;i++) {
        rpm = ress->provider[i];
	if ((name != NULL) && (strcmp(rpm->name, name))) {
	    parseNwriteTemplate(html, engine, "resource_body_end");
	    parseNwriteTemplate(html, engine, "resource_body_begin");
        }
	name = rpm->name;
	generateHtmlRpmRow(html, rpm, 1);
    }
    parseNwriteTemplate(html, engine, "resource_body_end");

#else	// ifdef HAVE_LIBTEMPLATE

    snprintf(buf, sizeof(buf), "%s %s",
	 localizedStrings[LANG_RPM_RESOURCE], ress->name);
    generateHtmlHeader(html, buf, NULL);
    generateLinks(html, installed);
    generateColorIndicator(html);
    fprintf(html, "<h1 align=center>%s %s</h1>\n",
	 localizedStrings[LANG_RPM_RESOURCE], ress->name);
    fprintf(html, "<h3>%s</h3>\n",
	 localizedStrings[LANG_PROVIDED_BY]);
    fprintf(html, "<table><tbody>\n");
    for (i = 0;i < ress->nb_provider;i++) {
        rpm = ress->provider[i];
	if ((name != NULL) && (strcmp(rpm->name, name)))
	    fprintf(html, "</tbody></table>\n<br>\n<table><tbody>\n");
	name = rpm->name;
	generateHtmlRpmRow(html, rpm, 1);
    }
    fprintf(html, "</tbody></table>\n");
#endif	// ifdef HAVE_LIBTEMPLATE
    
    generateHtmlFooter(html);
    fclose(html);
}

/*
 * Dump all RPM blocks in the HTML files.
 */
void dumpAllRessHtml(int installed) {
    rpmRessPtr cur;

    if (!rpm2html_dump_html) return;

    if (rpm2htmlVerbose)
      printf("Dumping resource HTML pages\n");

    if (installed) cur = ressInstalledList;
    else cur = ressList;

    while (cur != NULL) {
        dumpRessHtml(cur, installed);
	cur = cur->next;
    }
}

/*
 * Dump the Groups of all HTML files.
 * One expect that the RPM files have been sorted by group.
 */
void dumpDirRpmByGroups(rpmDataPtr list, rpmDirPtr dir,
                   char *subdir, int installed) {
    FILE *Groups;
    FILE *currentGroup = NULL;
    rpmDataPtr cur, prev = NULL;
    int count = 0;
    int pcount = 0;

    if (!rpm2html_dump_html) return;
    if ((dir != NULL) && (!dir->html)) return;

    cur = list;
    
    fullPathName(buf, sizeof(buf), dir, subdir, localizedStrings[LANG_GROUP_HTML]);
    if (rpm2htmlVerbose > 1) {
        printf("Dumping %s\n", buf);
    }

    Groups = fopen(buf, "w");
    if (Groups == NULL) {
        fprintf(stderr, "Couldn't save to file %s: %s\n",
	        buf, strerror(errno));
        return;
    }

#ifdef HAVE_LIBTEMPLATE
    generateHtmlHeader(Groups, "groups_title", NULL);
    generateLinks(Groups, installed);
    parseNwriteTemplate(Groups, engine, "groups_header");  // XXX ugly hack: here we assume that generateHtmlHeader() alredy filled proper value into "title" element of engine
      // later reconsicer usage of rpm2html only with libtemplate and then a) remove all those ifdefs and b) restructure html.c so as this ugy hack is not necessary

    rpmDataPtr lastCur = NULL;
    while (cur != NULL) {
        if ((cur->group != NULL) && (strlen(cur->group) > 0)) {
	    if ((prev == NULL) || (strcasecmp(prev->group, cur->group))) {
		if (pcount != 0) {
		    /* Add the current group in the Group list */
		    tpl_element_set(engine, "url", groupName(lastCur->group));
		    tpl_element_set(engine, "name", convertHTML(lastCur->group));
		    snprintf(buf, sizeof(buf), "%d", pcount);
		    tpl_element_set(engine, "count", buf);
		    parseNwriteTemplate(Groups, engine, "groups_item");
                }
		pcount = 0;
	        if (currentGroup != NULL) {
		    /* one need to close the current group list */
		    parseNwriteTemplate(currentGroup, engine, "group_end");
		    generateHtmlFooter(currentGroup);
		    fclose(currentGroup);
		}

		lastCur = cur;

		/* open the new HTML group file */
                fullPathName(buf, sizeof(buf), dir, subdir, groupName(cur->group));
		if (rpm2htmlVerbose > 1) {
		    printf("Dumping %s\n", buf);
		}

		currentGroup = fopen(buf, "w");
		if (currentGroup == NULL) {
		    fprintf(stderr, "Couldn't save to file %s: %s\n",
			    buf, strerror(errno));
		    return;
		}
		tpl_element_set(engine, "name", convertHTML(cur->group));
                generateHtmlHeader(currentGroup, "group_title", NULL);
		generateLinks(currentGroup, installed);
                parseNwriteTemplate(currentGroup, engine, "group_header");  // XXX ugly hack: here we assume that generateHtmlHeader() alredy filled proper value into "title" element of engine
                    // later reconsider usage of rpm2html only with libtemplate and then a) remove all those ifdefs and b) restructure html.c so as this ugy hack is not necessary
		generateColorIndicator(currentGroup);
		parseNwriteTemplate(currentGroup, engine, "group_begin");
		count = 0;
	    }
	    generateHtmlRpmArchRow(currentGroup, cur);
	    count++;
	    pcount++;
	    if ((count % MAX_TABLE_LENGHT) == 0) {
	        parseNwriteTemplate(currentGroup, engine, "group_end");
	        parseNwriteTemplate(currentGroup, engine, "group_begin");
            }
	}
	prev = cur;
	if ((dir == NULL) && (subdir == NULL))
	    cur = cur->nextSoft;
	else
	    cur = cur->next;
    }

    /* Add the last group in the Group list */
    if (pcount != 0) {
        tpl_element_set(engine, "url", groupName(lastCur->group));
        tpl_element_set(engine, "name", convertHTML(lastCur->group));
        snprintf(buf, sizeof(buf), "%d", pcount);
        tpl_element_set(engine, "count", buf);
        parseNwriteTemplate(Groups, engine, "groups_item");
    }

    if (currentGroup != NULL) {
	/* one need to close the current group list */
	parseNwriteTemplate(currentGroup, engine, "group_end");
	generateHtmlFooter(currentGroup);
	fclose(currentGroup);
    }
    parseNwriteTemplate(Groups, engine, "groups_footer");

#else	// ifdef HAVE_LIBTEMPLATE

    generateHtmlHeader(Groups, localizedStrings[LANG_SORTED_BY_GROUP], NULL);
    generateLinks(Groups, installed);
    fprintf(Groups, "<h1 align=center>%s</h1>\n",
        localizedStrings[LANG_SORTED_BY_GROUP]);
    fprintf(Groups, "<ul>\n");

    while (cur != NULL) {
        if ((cur->group != NULL) && (strlen(cur->group) > 0)) {
	    if ((prev == NULL) || (strcasecmp(prev->group, cur->group))) {
		if (pcount != 0)
		    fprintf(Groups, " (%d)</a></li>\n", pcount);
		pcount = 0;
	        if (currentGroup != NULL) {
		    /* one need to close the current group list */
		    fprintf(currentGroup,"</tbody></table>\n");
		    generateHtmlFooter(currentGroup);
		    fclose(currentGroup);
		}

		/* Add the current group in the Group list */
		fprintf(Groups, "<li><a href=\"%s\">%s",
		        groupName(cur->group), convertHTML(cur->group));

		/* open the new HTML group file */
                fullPathName(buf, sizeof(buf), dir, subdir, groupName(cur->group));
		if (rpm2htmlVerbose > 1) {
		    printf("Dumping %s\n", buf);
		}

		currentGroup = fopen(buf, "w");
		if (currentGroup == NULL) {
		    fprintf(stderr, "Couldn't save to file %s: %s\n",
			    buf, strerror(errno));
		    return;
		}
                snprintf(buf, sizeof(buf), "%s %s", localizedStrings[LANG_OF_GROUP],
		        convertHTML(cur->group));
                generateHtmlHeader(currentGroup, buf, NULL);
		generateLinks(currentGroup, installed);
		fprintf(currentGroup,
		  "<h1 align=center>%s %s</h1>\n",
		     localizedStrings[LANG_OF_GROUP], convertHTML(cur->group));
		generateColorIndicator(currentGroup);
		fprintf(currentGroup,"<table><tbody>\n");
		count = 0;
	    }
	    generateHtmlRpmArchRow(currentGroup, cur);
	    count++;
	    pcount++;
	    if ((count % MAX_TABLE_LENGHT) == 0)
		fprintf(currentGroup, "</tbody></table>\n<table><tbody>\n");
	}
	prev = cur;
	if ((dir == NULL) && (subdir == NULL))
	    cur = cur->nextSoft;
	else
	    cur = cur->next;
    }

    /*
     * finish the last group line.
     */
    if (pcount != 0)
	fprintf(Groups, " (%d)</a></li>\n", pcount);

    if (currentGroup != NULL) {
	/* one need to close the current group list */
	fprintf(currentGroup,"</tbody></table>\n");
	generateHtmlFooter(currentGroup);
	fclose(currentGroup);
    }
    fprintf(Groups, "</ul>\n");
#endif	// ifdef HAVE_LIBTEMPLATE

    generateHtmlFooter(Groups);
    fclose(Groups);
}

/*
 * Dump the full set of  RPM by Group HTML file.
 * One expect that the RPM files have been sorted by groups.
 */
void dumpRpmByGroups(rpmDataPtr list, int installed) {
    if (!rpm2html_dump_html) return;

    if (rpm2htmlVerbose)
      printf("Dumping \"by group\" HTML pages\n");

    dumpDirRpmByGroups(list, NULL, NULL, installed);
}

/*
 * Dump the Distribs HTML files.
 * One expect that the RPM files have been sorted by distribution.
 */
void dumpDirRpmByDistribs(rpmDataPtr list, rpmDirPtr dir,
                      char *subdir, int installed) {
    FILE *Distribs;
    FILE *currentDistrib = NULL;
    rpmDataPtr cur, prev = NULL;
    int count = 0;
    int pcount = 0;

    if (!rpm2html_dump_html) return;
    if ((dir != NULL) && (!dir->html)) return;

    cur = list;

    fullPathName(buf, sizeof(buf), dir, subdir, localizedStrings[LANG_DISTRIB_HTML]);
    if (rpm2htmlVerbose > 1) {
        printf("Dumping %s\n", buf);
    }

    Distribs = fopen(buf, "w");
    if (Distribs == NULL) {
        fprintf(stderr, "Couldn't save to file %s: %s\n",
	        buf, strerror(errno));
        return;
    }
    
#ifdef HAVE_LIBTEMPLATE
    generateHtmlHeader(Distribs, "distribs_title", NULL);
    generateLinks(Distribs, installed);
    parseNwriteTemplate(Distribs, engine, "distribs_header");  // XXX ugly hack: here we assume that generateHtmlHeader() alredy filled proper value into "title" element of engine
      // later reconsicer usage of rpm2html only with libtemplate and then a) remove all those ifdefs and b) restructure html.c so as this ugy hack is not necessary

    rpmDataPtr lastCur = NULL;
    while (cur != NULL) {
        if ((cur->distribution != NULL) && (strlen(cur->distribution) > 0)) {
	    if ((prev == NULL) || (strcasecmp(prev->distribution, cur->distribution))) {
		if (pcount != 0) {
		    /* Add the current distribution in the Distrib list */
		    tpl_element_set(engine, "url", distribName(lastCur->distribution));
		    tpl_element_set(engine, "name", convertHTML(lastCur->distribution));
		    snprintf(buf, sizeof(buf), "%d", pcount);
		    tpl_element_set(engine, "count", buf);
		    parseNwriteTemplate(Distribs, engine, "distribs_item");
                }
		pcount = 0;

	        if (currentDistrib != NULL) {
		    /* one need to close the current distribution list */
		    parseNwriteTemplate(currentDistrib, engine, "distrib_end");
		    generateHtmlFooter(currentDistrib);
		    fclose(currentDistrib);
		}

		lastCur = cur;

		/* open the new HTML distribution file */
                fullPathName(buf, sizeof(buf), dir, subdir, distribName(cur->distribution));
		if (rpm2htmlVerbose > 1) {
		    printf("Dumping %s\n", buf);
		}

		currentDistrib = fopen(buf, "w");
		if (currentDistrib == NULL) {
		    fprintf(stderr, "Couldn't save to file %s: %s\n",
			    buf, strerror(errno));
		    return;
		}
		tpl_element_set(engine, "name", convertHTML(cur->distribution));
                generateHtmlHeader(currentDistrib, "distrib_title", NULL);
		generateLinks(currentDistrib, installed);
                parseNwriteTemplate(currentDistrib, engine, "distrib_header");  // XXX ugly hack: here we assume that generateHtmlHeader() alredy filled proper value into "title" element of engine
                    // later reconsider usage of rpm2html only with libtemplate and then a) remove all those ifdefs and b) restructure html.c so as this ugy hack is not necessary
		if (!installed) generateColorIndicator(currentDistrib);
		parseNwriteTemplate(currentDistrib, engine, "distrib_begin");
		count = 0;
	    }
	    generateHtmlRpmRow(currentDistrib, cur, 1);
	    count++;
	    pcount++;
	    if ((count % MAX_TABLE_LENGHT) == 0) {
	        parseNwriteTemplate(currentDistrib, engine, "distrib_end");
	        parseNwriteTemplate(currentDistrib, engine, "distrib_begin");
            }
	}
	prev = cur;
	cur = cur->next;
    }
    /* Add the last distribution in the Distrib list */
    if (pcount != 0) {
        tpl_element_set(engine, "url", distribName(lastCur->distribution));
        tpl_element_set(engine, "name", convertHTML(lastCur->distribution));
        snprintf(buf, sizeof(buf), "%d", pcount);
        tpl_element_set(engine, "count", buf);
        parseNwriteTemplate(Distribs, engine, "distribs_item");
    }

    if (currentDistrib != NULL) {
	/* one need to close the current distribution list */
	parseNwriteTemplate(currentDistrib, engine, "distrib_end");
	generateHtmlFooter(currentDistrib);
	fclose(currentDistrib);
    }
    parseNwriteTemplate(Distribs, engine, "distribs_footer");

#else	// ifdef HAVE_LIBTEMPLATE

    generateHtmlHeader(Distribs,
        localizedStrings[LANG_SORTED_BY_DISTRIB], NULL);
    generateLinks(Distribs, installed);
    fprintf(Distribs, "<h1 align=center>%s</h1>\n",
        localizedStrings[LANG_SORTED_BY_DISTRIB]);
    fprintf(Distribs, "<ul>\n");

    while (cur != NULL) {
        if ((cur->distribution != NULL) && (strlen(cur->distribution) > 0)) {
	    if ((prev == NULL) || (strcasecmp(prev->distribution, cur->distribution))) {
		if (pcount != 0)
		    fprintf(Distribs, " (%d)</a></li>\n", pcount);
		pcount = 0;

	        if (currentDistrib != NULL) {
		    /* one need to close the current distribution list */
		    fprintf(currentDistrib,"</tbody></table>\n");
		    generateHtmlFooter(currentDistrib);
		    fclose(currentDistrib);
		}

		/* Add the current distribution in the Distrib list */
		fprintf(Distribs, "<li><a href=\"%s\">%s",
		        distribName(cur->distribution),
			convertHTML(cur->distribution));

		/* open the new HTML distribution file */
                fullPathName(buf, sizeof(buf), dir, subdir, distribName(cur->distribution));
		if (rpm2htmlVerbose > 1) {
		    printf("Dumping %s\n", buf);
		}

		currentDistrib = fopen(buf, "w");
		if (currentDistrib == NULL) {
		    fprintf(stderr, "Couldn't save to file %s: %s\n",
			    buf, strerror(errno));
		    return;
		}
                snprintf(buf, sizeof(buf), "%s %s",
		    localizedStrings[LANG_OF_DISTRIB],
		    convertHTML(cur->distribution));
                generateHtmlHeader(currentDistrib, buf, NULL);
		generateLinks(currentDistrib, installed);
		fprintf(currentDistrib,
		  "<h1 align=center>%s %s</h1>\n",
		  localizedStrings[LANG_OF_DISTRIB],
		  convertHTML(cur->distribution));
		if (!installed) generateColorIndicator(currentDistrib);
		fprintf(currentDistrib,"<table><tbody>\n");
		count = 0;
	    }
	    generateHtmlRpmRow(currentDistrib, cur, 1);
	    count++;
	    pcount++;
	    if ((count % MAX_TABLE_LENGHT) == 0)
		fprintf(currentDistrib, "</tbody></table>\n<table><tbody>\n");
	}
	prev = cur;
	cur = cur->next;
    }
    /*
     * finish the last group line.
     */
    if (pcount != 0)
	fprintf(Distribs, " (%d)</a></li>\n", pcount);

    if (currentDistrib != NULL) {
	/* one need to close the current distribution list */
	fprintf(currentDistrib,"</tbody></table>\n");
	generateHtmlFooter(currentDistrib);
	fclose(currentDistrib);
    }
    fprintf(Distribs, "</ul>\n");
#endif	// ifdef HAVE_LIBTEMPLATE

    generateHtmlFooter(Distribs);
    fclose(Distribs);
}

/*
 * Dump the full set of  RPM by Distribution HTML file.
 * One expect that the RPM files have been sorted by distributions.
 */
void dumpRpmByDistribs(rpmDataPtr list, int installed) {
    if (!rpm2html_dump_html) return;

    if (rpm2htmlVerbose)
      printf("Dumping \"by distribution\" HTML pages\n");

    dumpDirRpmByDistribs(list, NULL, NULL, installed);
}

/*
 * Dump the Vendors HTML files.
 * One expect that the RPM files have been sorted by vendors.
 */
void dumpDirRpmByVendors(rpmDataPtr list, rpmDirPtr dir,
                      char *subdir, int installed) {
    FILE *Vendors;
    FILE *currentVendor = NULL;
    rpmDataPtr cur, prev = NULL;
    int count = 0;
    int pcount = 0;

    if (!rpm2html_dump_html) return;

    cur = list;

    fullPathName(buf, sizeof(buf), dir, subdir, localizedStrings[LANG_VENDOR_HTML]);
    if (rpm2htmlVerbose > 1) {
        printf("Dumping %s\n", buf);
    }

    Vendors = fopen(buf, "w");
    if (Vendors == NULL) {
        fprintf(stderr, "Couldn't save to file %s: %s\n",
	        buf, strerror(errno));
        return;
    }
#ifdef HAVE_LIBTEMPLATE
    generateHtmlHeader(Vendors, "vendors_title", NULL);
    generateLinks(Vendors, installed);
    parseNwriteTemplate(Vendors, engine, "vendors_header");  // XXX ugly hack: here we assume that generateHtmlHeader() alredy filled proper value into "title" element of engine
      // later reconsicer usage of rpm2html only with libtemplate and then a) remove all those ifdefs and b) restructure html.c so as this ugy hack is not necessary

    rpmDataPtr lastCur = NULL;
    while (cur != NULL) {
        if ((cur->vendor != NULL) && (strlen(cur->vendor) > 0)) {
	    if ((prev == NULL) || (strcasecmp(prev->vendor, cur->vendor))) {
		if (pcount != 0) {
		    /* Add the current vendor in the Vendor list */
		    tpl_element_set(engine, "url", vendorName(lastCur->vendor));
		    tpl_element_set(engine, "name", convertHTML(lastCur->vendor));
		    snprintf(buf, sizeof(buf), "%d", pcount);
		    tpl_element_set(engine, "count", buf);
		    parseNwriteTemplate(Vendors, engine, "vendors_item");
                }
                pcount = 0;

	        if (currentVendor != NULL) {
		    /* one need to close the current vendor list */
		    parseNwriteTemplate(currentVendor, engine, "vendor_end");
		    generateHtmlFooter(currentVendor);
		    fclose(currentVendor);
		}

		lastCur = cur;

		/* open the new HTML vendor file */
                fullPathName(buf, sizeof(buf), dir, subdir, vendorName(cur->vendor));
		if (rpm2htmlVerbose > 1) {
		    printf("Dumping %s\n", buf);
		}

		currentVendor = fopen(buf, "w");
		if (currentVendor == NULL) {
		    fprintf(stderr, "Couldn't save to file %s: %s\n",
			    buf, strerror(errno));
		    return;
		}
		tpl_element_set(engine, "name", convertHTML(cur->vendor));
                generateHtmlHeader(currentVendor, "vendor_title", NULL);
		generateLinks(currentVendor, installed);
                parseNwriteTemplate(currentVendor, engine, "vendor_header");  // XXX ugly hack: here we assume that generateHtmlHeader() alredy filled proper value into "title" element of engine
                    // later reconsider usage of rpm2html only with libtemplate and then a) remove all those ifdefs and b) restructure html.c so as this ugy hack is not necessary
		if (!installed) generateColorIndicator(currentVendor);
		parseNwriteTemplate(currentVendor, engine, "vendor_begin");
		count = 0;
	    }
	    generateHtmlRpmRow(currentVendor, cur, 1);
	    count++;
	    pcount++;
	    if ((count % MAX_TABLE_LENGHT) == 0) {
                parseNwriteTemplate(currentVendor, engine, "vendor_end");
                parseNwriteTemplate(currentVendor, engine, "vendor_begin");
            }
	}
	prev = cur;
	cur = cur->next;
    }

    /* Add the last vendor in the Vendor list */
    if (pcount != 0) {
	tpl_element_set(engine, "url", vendorName(lastCur->vendor));
	tpl_element_set(engine, "name", convertHTML(lastCur->vendor));
	snprintf(buf, sizeof(buf), "%d", pcount);
	tpl_element_set(engine, "count", buf);
	parseNwriteTemplate(Vendors, engine, "vendors_item");
    }

    if (currentVendor != NULL) {
	/* one need to close the current vendor list */
	parseNwriteTemplate(currentVendor, engine, "vendor_end");
	generateHtmlFooter(currentVendor);
	fclose(currentVendor);
    }
    parseNwriteTemplate(Vendors, engine, "vendors_footer");

#else	// ifdef HAVE_LIBTEMPLATE
    generateHtmlHeader(Vendors, localizedStrings[LANG_SORTED_BY_VENDOR], NULL);
    generateLinks(Vendors, installed);
    fprintf(Vendors, "<h1 align=center>%s</h1>\n",
        localizedStrings[LANG_SORTED_BY_VENDOR]);
    fprintf(Vendors, "<ul>\n");

    while (cur != NULL) {
        if ((cur->vendor != NULL) && (strlen(cur->vendor) > 0)) {
	    if ((prev == NULL) || (strcasecmp(prev->vendor, cur->vendor))) {
		if (pcount != 0)
		    fprintf(Vendors, " (%d)</a></li>\n", pcount);
		pcount = 0;

	        if (currentVendor != NULL) {
		    /* one need to close the current vendor list */
		    fprintf(currentVendor,"</tbody></table>\n");
		    generateHtmlFooter(currentVendor);
		    fclose(currentVendor);
		}

		/* Add the current vendor in the Vendor list */
		fprintf(Vendors, "<li><a href=\"%s\">%s",
		        vendorName(cur->vendor), convertHTML(cur->vendor));

		/* open the new HTML vendor file */
                fullPathName(buf, sizeof(buf), dir, subdir, vendorName(cur->vendor));
		if (rpm2htmlVerbose > 1) {
		    printf("Dumping %s\n", buf);
		}

		currentVendor = fopen(buf, "w");
		if (currentVendor == NULL) {
		    fprintf(stderr, "Couldn't save to file %s: %s\n",
			    buf, strerror(errno));
		    return;
		}
                snprintf(buf, sizeof(buf), "%s %s", localizedStrings[LANG_OF_VENDOR],
		    convertHTML(cur->vendor));
                generateHtmlHeader(currentVendor, buf, NULL);
		generateLinks(currentVendor, installed);
		fprintf(currentVendor,
		  "<h1 align=center>%s %s</h1>\n",
		  localizedStrings[LANG_OF_VENDOR], convertHTML(cur->vendor));
		if (!installed) generateColorIndicator(currentVendor);
		fprintf(currentVendor,"<table><tbody>\n");
		count = 0;
	    }
	    generateHtmlRpmRow(currentVendor, cur, 1);
	    count++;
	    pcount++;
	    if ((count % MAX_TABLE_LENGHT) == 0)
		fprintf(currentVendor, "</tbody></table>\n<table><tbody>\n");
	}
	prev = cur;
	cur = cur->next;
    }

    /*
     * finish the last group line.
     */
    if (pcount != 0)
	fprintf(Vendors, " (%d)</a></li>\n", pcount);

    if (currentVendor != NULL) {
	/* one need to close the current vendor list */
	fprintf(currentVendor,"</tbody></table>\n");
	generateHtmlFooter(currentVendor);
	fclose(currentVendor);
    }
    fprintf(Vendors, "</ul>\n");
#endif	// ifdef HAVE_LIBTEMPLATE

    generateHtmlFooter(Vendors);
    fclose(Vendors);
}

/*
 * Dump the full set of  RPM by Vendor HTML file.
 * One expect that the RPM files have been sorted by vendors.
 */
void dumpRpmByVendors(rpmDataPtr list, int installed) {
    if (!rpm2html_dump_html) return;

    if (rpm2htmlVerbose)
      printf("Dumping \"by vendor\" HTML pages\n");

    dumpDirRpmByVendors(list, NULL, NULL, installed);
}

#ifdef HAVE_LIBTEMPLATE
/*
 * Helper function for dumpDirRpmByDate(): it performs subdumps for given package age.
 * Returns actualised page counter.
 */
int subdumpDirRpmByDate(rpmDataPtr list, rpmDirPtr dir,
                      char *subdir, int installed,
                      FILE **html, FILE *RSS, int *rdfcount,
                      char *title, int maxAge, rpmDataPtr *cur, int *page, int *count) {

    tpl_parse(engine, title, "p_title", 0);
    tpl_element_set(engine, "title", tpl_element_get(engine, "p_title"));
    parseNwriteTemplate((*html), engine, "dates_subheader");
    parseNwriteTemplate((*html), engine, "dates_begin");
    while (((*cur) != NULL) && (((currentTime - (*cur)->date) < maxAge) || (maxAge < 0))) {
	generateHtmlRpmRow((*html), (*cur), 0);
	if ((RSS != NULL) && ((*rdfcount)++ < rpm2html_rdf_count_limit)) {
	    generateRSSItem(RSS, (*cur));
	}
	(*cur) = (*cur)->next;
	(*count)++;
	if (((*count) % MAX_TABLE_LENGHT) == 0) {
	    parseNwriteTemplate((*html), engine, "dates_end");
	    parseNwriteTemplate((*html), engine, "dates_begin");
        }
	if ((*count) > MAX_PAGE_LENGHT) {
	    (*count) = 0;
	    parseNwriteTemplate((*html), engine, "dates_end");
	    snprintf(buf, sizeof(buf), "%d", (*page));
	    tpl_element_set(engine, "next_page", buf);
	    parseNwriteTemplate((*html), engine, "dates_next_page");
	    generateHtmlFooter((*html));
	    fclose((*html));
	    fullPathNameNr(buf, sizeof(buf), dir, subdir, localizedStrings[LANG_BYDATE_HTML], (*page)++);
	    if (rpm2htmlVerbose > 1) {
		printf("Dumping %s\n", buf);
	    }
	    (*html) = fopen(buf, "w");
	    if ((*html) == NULL) {
		fprintf(stderr, "Couldn't save to file %s: %s\n",
			buf, strerror(errno));
		if (RSS) {
		    fprintf(RSS, "  </channel>\n");
		    fprintf(RSS, "</rss>\n");
		    fclose(RSS);
		}
		return;
	    }
	    if (installed) {
	        generateHtmlHeader((*html), "idates_title", NULL);
	        generateLinks((*html), installed);
	        parseNwriteTemplate((*html), engine, "dates_header");  // XXX ugly hack: here we assume that generateHtmlHeader() alredy filled proper value into "title" element of engine
	          // later reconsicer usage of rpm2html only with libtemplate and then a) remove all those ifdefs and b) restructure html.c so as this ugy hack is not necessary
            } else {
                generateHtmlHeader((*html), "cdates_title", NULL);
                generateLinks((*html), installed);
                parseNwriteTemplate((*html), engine, "dates_header");  // XXX ugly hack: here we assume that generateHtmlHeader() alredy filled proper value into "title" element of engine
                  // later reconsicer usage of rpm2html only with libtemplate and then a) remove all those ifdefs and b) restructure html.c so as this ugy hack is not necessary
            }
            //tpl_parse(engine, title, "p_title", 0);
            //tpl_element_set(engine, "title", tpl_element_get(engine, "p_title"));	- I hope it is still preserved from the begining of the function?
            parseNwriteTemplate((*html), engine, "dates_subheader");
            parseNwriteTemplate((*html), engine, "dates_begin");
	}
    }
    parseNwriteTemplate((*html), engine, "dates_end");

    return (*page);
}
#endif

/*
 * Dump the RPM by Date HTML file.
 * One expect that the RPM files have been sorted by date.
 */
void dumpDirRpmByDate(rpmDataPtr list, rpmDirPtr dir,
                      char *subdir, int installed) {
    FILE *ByDate;
    FILE *RSS;
    rpmDataPtr cur;
    int count = 0;
    int rdfcount = 0;
    int page = 1;

    if (!rpm2html_dump_html) return;
    if ((dir != NULL) && (!dir->html)) return;

    cur = list;

    fullPathName(buf, sizeof(buf), dir, subdir, localizedStrings[LANG_BYDATE_HTML]);
    if (rpm2htmlVerbose > 1) {
	printf("Dumping %s\n", buf);
    }

    ByDate = fopen(buf, "w");
    if (ByDate == NULL) {
        fprintf(stderr, "Couldn't save to file %s: %s\n",
	        buf, strerror(errno));
        return;
    }
    fullPathName(buf, sizeof(buf), dir, subdir, "rdf");
    if (rpm2htmlVerbose > 1) {
	printf("Dumping RSS channel %s\n", buf);
    }

    RSS = fopen(buf, "w");
    if (RSS == NULL) {
        fprintf(stderr, "Couldn't save to file %s: %s\n",
	        buf, strerror(errno));
	fclose(ByDate);
        return;
    }
#ifdef HAVE_LIBTEMPLATE
    if (installed) {
	generateHtmlHeader(ByDate, "idates_title", NULL);
	generateLinks(ByDate, installed);
        parseNwriteTemplate(ByDate, engine, "dates_header");  // XXX ugly hack: here we assume that generateHtmlHeader() alredy filled proper value into "title" element of engine
          // later reconsicer usage of rpm2html only with libtemplate and then a) remove all those ifdefs and b) restructure html.c so as this ugy hack is not necessary
    } else {
	generateHtmlHeader(ByDate, "cdates_title", NULL);
	generateLinks(ByDate, installed);
        parseNwriteTemplate(ByDate, engine, "dates_header");  // XXX ugly hack: here we assume that generateHtmlHeader() alredy filled proper value into "title" element of engine
          // later reconsicer usage of rpm2html only with libtemplate and then a) remove all those ifdefs and b) restructure html.c so as this ugy hack is not necessary
    }

#else	// ifdef HAVE_LIBTEMPLATE

    if (installed) {
	generateHtmlHeader(ByDate, localizedStrings[LANG_SORTED_BY_IDATE], NULL);
	generateLinks(ByDate, installed);
	fprintf(ByDate, "<h1 align=center>%s</h1>\n",
	    localizedStrings[LANG_SORTED_BY_IDATE]);
    } else {
	generateHtmlHeader(ByDate, localizedStrings[LANG_SORTED_BY_CDATE], NULL);
	generateLinks(ByDate, installed);
	fprintf(ByDate, "<h1 align=center>%s</h1>\n",
	    localizedStrings[LANG_SORTED_BY_CDATE]);
    }
#endif	// ifdef HAVE_LIBTEMPLATE
    if (RSS) {
	fprintf(RSS, "<?xml version=\"1.0\"?>\n");
	fprintf(RSS,
    "<!DOCTYPE rss PUBLIC \"-//Netscape Communications//DTD RSS 0.91//EN\"\n");
	fprintf(RSS,
    "		 \"http://my.netscape.com/publish/formats/rss-0.91.dtd\">\n"); 
	fprintf(RSS, "<rss version=\"0.91\">\n");
	fprintf(RSS, "  <channel>\n");
	if (dir != NULL) {
	    fprintf(RSS, "    <title>Rpmfind %s index on %s</title>\n",
		    dir->name, rpm2html_host);
	    fullURL(buf, sizeof(buf), dir, subdir,
		    localizedStrings[LANG_INDEX_HTML]);
	    fprintf(RSS, "    <link>%s</link>\n", buf);
	    fprintf(RSS, "    <description>Rpmfind %s News on %s</description>\n",
		    rpm2html_host, dir->name);
	} else {
	    fprintf(RSS, "    <title>Rpmfind index on %s</title>\n",
		    rpm2html_host);
	    fullURL(buf, sizeof(buf), dir, subdir,
		    localizedStrings[LANG_INDEX_HTML]);
	    fprintf(RSS, "    <link>%s</link>\n", buf);
	    fprintf(RSS, "    <description>Rpmfind News on %s</description>\n",
		    rpm2html_host);
	}
#ifdef HAVE_LIBTEMPLATE
        parseNwriteTemplate(ByDate, engine, "dates_rdf");
#else
	fprintf(ByDate, localizedStrings[LANG_RDF_CHANNEL], "rdf");
#endif
    }

    /*
     * Skip all the RPMs with date in the futur :-(
     */
    while ((cur != NULL) && ((cur->date < 0) || (currentTime < cur->date))) {
	if (rpm2htmlVerbose > 1) {
	    fprintf(stderr, "dropping %s, date %d > current time %d\n",
		    rpmSoftwareName(cur), (int) cur->date, (int) currentTime);
	}
	cur = cur->next;
    }

#ifdef HAVE_LIBTEMPLATE
    #define MAX_AGE_LESS_3D (3 * 24 * 60 * 60)
    #define MAX_AGE_LESS_1W (7 * 24 * 60 * 60)
    #define MAX_AGE_LESS_2W (14 * 24 * 60 * 60)
    #define MAX_AGE_LESS_1M (30 * 24 * 60 * 60)
    #define MAX_AGE_MORE_1M -1

    /*
     * First dump RPMs less than 3 days old.
     */
    if (!installed) generateColorIndicator(ByDate);
    if (installed)
	subdumpDirRpmByDate(list, dir, subdir, installed,
	    &ByDate, RSS, &rdfcount,
	    "idates_title_less_3d_old", MAX_AGE_LESS_3D, &cur, &page, &count);
    else
        subdumpDirRpmByDate(list, dir, subdir, installed,
            &ByDate, RSS, &rdfcount,
            "cdates_title_less_3d_old", MAX_AGE_LESS_3D, &cur, &page, &count);

    /*
     * Then dump RPMs less than one week old.
     */
    if (installed)
        subdumpDirRpmByDate(list, dir, subdir, installed,
            &ByDate, RSS, &rdfcount,
            "idates_title_less_1w_old", MAX_AGE_LESS_1W, &cur, &page, &count);
    else
	subdumpDirRpmByDate(list, dir, subdir, installed,
	    &ByDate, RSS, &rdfcount,
            "cdates_title_less_1w_old", MAX_AGE_LESS_1W, &cur, &page, &count);

    /*
     * Then dump RPMs less than two weeks old.
     */
    if (installed)
        subdumpDirRpmByDate(list, dir, subdir, installed,
            &ByDate, RSS, &rdfcount,
            "idates_title_less_2w_old", MAX_AGE_LESS_2W, &cur, &page, &count);
    else
        subdumpDirRpmByDate(list, dir, subdir, installed,
            &ByDate, RSS, &rdfcount,
            "cdates_title_less_2w_old", MAX_AGE_LESS_2W, &cur, &page, &count);

    /*
     * Then dump RPMs less than one month old.
     */
    if (installed)
	subdumpDirRpmByDate(list, dir, subdir, installed,
	    &ByDate, RSS, &rdfcount,
            "idates_title_less_1m_old", MAX_AGE_LESS_1M, &cur, &page, &count);
    else
	subdumpDirRpmByDate(list, dir, subdir, installed,
	    &ByDate, RSS, &rdfcount,
            "cdates_title_less_1m_old", MAX_AGE_LESS_1M, &cur, &page, &count);

    /*
     * Then dump RPMs more than one month old.
     */
    if (installed)
	subdumpDirRpmByDate(list, dir, subdir, installed,
	    &ByDate, RSS, &rdfcount,
            "idates_title_more_1m_old", MAX_AGE_MORE_1M, &cur, &page, &count);
    else
	subdumpDirRpmByDate(list, dir, subdir, installed,
	    &ByDate, RSS, &rdfcount,
            "cdates_title_more_1m_old", MAX_AGE_MORE_1M, &cur, &page, &count);

#else	// ifdef HAVE_LIBTEMPLATE

    /*
     * First dump RPMs less than 3 days old.
     */
    if (!installed) generateColorIndicator(ByDate);
    if (installed)
	fprintf(ByDate, "<h2>%s</h2>\n", localizedStrings[LANG_I_LESS_3D_OLD]);
    else
	fprintf(ByDate, "<h2>%s</h2>\n", localizedStrings[LANG_LESS_3D_OLD]);
    fprintf(ByDate, "<table><tbody>\n");
    while ((cur != NULL) && ((currentTime - cur->date) < (3 * 24 * 60 * 60))) {
	generateHtmlRpmRow(ByDate, cur, 0);
	if ((RSS != NULL) && (rdfcount++ < rpm2html_rdf_count_limit)) {
	    generateRSSItem(RSS, cur);
	}
	cur = cur->next;
	count++;
	if ((count % MAX_TABLE_LENGHT) == 0)
	    fprintf(ByDate, "</tbody></table>\n<table><tbody>\n");
	if (count > MAX_PAGE_LENGHT) {
	    count = 0;
	    fprintf(ByDate, "</tbody></table>\n");
	    fprintf(ByDate, "<a href=\"%d%s\">...</a>\n",
	            page, localizedStrings[LANG_BYDATE_HTML]);
	    generateHtmlFooter(ByDate);
	    fclose(ByDate);
	    fullPathNameNr(buf, sizeof(buf), dir, subdir, localizedStrings[LANG_BYDATE_HTML], page++);
	    if (rpm2htmlVerbose > 1) {
		printf("Dumping %s\n", buf);
	    }
	    ByDate = fopen(buf, "w");
	    if (ByDate == NULL) {
		fprintf(stderr, "Couldn't save to file %s: %s\n",
			buf, strerror(errno));
		if (RSS) {
		    fprintf(RSS, "  </channel>\n");
		    fprintf(RSS, "</rss>\n");
		    fclose(RSS);
		}
		return;
	    }
	    if (installed) {
		generateHtmlHeader(ByDate,
		          localizedStrings[LANG_SORTED_BY_IDATE], NULL);
		generateLinks(ByDate, installed);
		fprintf(ByDate, "<h1 align=center>%s</h1>\n",
		    localizedStrings[LANG_SORTED_BY_IDATE]);
	    } else {
		generateHtmlHeader(ByDate,
		          localizedStrings[LANG_SORTED_BY_CDATE], NULL);
		generateLinks(ByDate, installed);
		fprintf(ByDate, "<h1 align=center>%s</h1>\n",
		    localizedStrings[LANG_SORTED_BY_CDATE]);
	    }
	    if (installed)
		fprintf(ByDate, "<h2>%s</h2>\n", localizedStrings[LANG_I_LESS_3D_OLD]);
	    else
		fprintf(ByDate, "<h2>%s</h2>\n", localizedStrings[LANG_LESS_3D_OLD]);
	    fprintf(ByDate, "<table><tbody>\n");
	}
    }
    fprintf(ByDate, "</tbody></table>\n");

    /*
     * Then dump RPMs less than one week old.
     */
    if (installed)
	fprintf(ByDate, "<h2>%s</h2>\n", localizedStrings[LANG_I_LESS_1W_OLD]);
    else
	fprintf(ByDate, "<h2>%s</h2>\n", localizedStrings[LANG_LESS_1W_OLD]);
    fprintf(ByDate, "<table><tbody>\n");
    while ((cur != NULL) && ((currentTime - cur->date) < (7 * 24 * 60 * 60))) {
	generateHtmlRpmRow(ByDate, cur, 0);
	if ((RSS != NULL) && (rdfcount++ < rpm2html_rdf_count_limit)) {
	    generateRSSItem(RSS, cur);
	}
	cur = cur->next;
	count++;
	if ((count % MAX_TABLE_LENGHT) == 0)
	    fprintf(ByDate, "</tbody></table>\n<table><tbody>\n");
	if (count > MAX_PAGE_LENGHT) {
	    count = 0;
	    fprintf(ByDate, "</tbody></table>\n");
	    fprintf(ByDate, "<a href=\"%d%s\">...</a>\n",
	            page, localizedStrings[LANG_BYDATE_HTML]);
	    generateHtmlFooter(ByDate);
	    fclose(ByDate);
	    fullPathNameNr(buf, sizeof(buf), dir, subdir,
	                   localizedStrings[LANG_BYDATE_HTML], page++);
	    if (rpm2htmlVerbose > 1) {
		printf("Dumping %s\n", buf);
	    }
	    ByDate = fopen(buf, "w");
	    if (ByDate == NULL) {
		fprintf(stderr, "Couldn't save to file %s: %s\n",
			buf, strerror(errno));
		if (RSS) {
		    fprintf(RSS, "  </channel>\n");
		    fprintf(RSS, "</rss>\n");
		    fclose(RSS);
		}
		return;
	    }
	    if (installed) {
		generateHtmlHeader(ByDate,
		          localizedStrings[LANG_SORTED_BY_IDATE], NULL);
		generateLinks(ByDate, installed);
		fprintf(ByDate, "<h1 align=center>%s</h1>\n",
		    localizedStrings[LANG_SORTED_BY_IDATE]);
	    } else {
		generateHtmlHeader(ByDate,
		          localizedStrings[LANG_SORTED_BY_CDATE], NULL);
		generateLinks(ByDate, installed);
		fprintf(ByDate, "<h1 align=center>%s</h1>\n",
		    localizedStrings[LANG_SORTED_BY_CDATE]);
	    }
	    if (installed)
		fprintf(ByDate, "<h2>%s</h2>\n", localizedStrings[LANG_I_LESS_1W_OLD]);
	    else
		fprintf(ByDate, "<h2>%s</h2>\n", localizedStrings[LANG_LESS_1W_OLD]);
	    fprintf(ByDate, "<table><tbody>\n");
	}
    }
    fprintf(ByDate, "</tbody></table>\n");

    /*
     * Then dump RPMs less than two weeks old.
     */
    if (installed)
	fprintf(ByDate, "<h2>%s</h2>\n", localizedStrings[LANG_I_LESS_2W_OLD]);
    else
	fprintf(ByDate, "<h2>%s</h2>\n", localizedStrings[LANG_LESS_2W_OLD]);
    fprintf(ByDate, "<table><tbody>\n");
    while ((cur != NULL) && ((currentTime - cur->date) < (14 * 24 * 60 * 60))) {
	generateHtmlRpmRow(ByDate, cur, 0);
	if ((RSS != NULL) && (rdfcount++ < rpm2html_rdf_count_limit)) {
	    generateRSSItem(RSS, cur);
	}
	cur = cur->next;
	count++;
	if ((count % MAX_TABLE_LENGHT) == 0)
	    fprintf(ByDate, "</tbody></table>\n<table><tbody>\n");
	if (count > MAX_PAGE_LENGHT) {
	    count = 0;
	    fprintf(ByDate, "</tbody></table>\n");
	    fprintf(ByDate, "<a href=\"%d%s\">...</a>\n",
	            page, localizedStrings[LANG_BYDATE_HTML]);
	    generateHtmlFooter(ByDate);
	    fclose(ByDate);
	    fullPathNameNr(buf, sizeof(buf), dir, subdir,
	                   localizedStrings[LANG_BYDATE_HTML], page++);
	    if (rpm2htmlVerbose > 1) {
		printf("Dumping %s\n", buf);
	    }
	    ByDate = fopen(buf, "w");
	    if (ByDate == NULL) {
		fprintf(stderr, "Couldn't save to file %s: %s\n",
			buf, strerror(errno));
		if (RSS) {
		    fprintf(RSS, "  </channel>\n");
		    fprintf(RSS, "</rss>\n");
		    fclose(RSS);
		}
		return;
	    }
	    if (installed) {
		generateHtmlHeader(ByDate,
		          localizedStrings[LANG_SORTED_BY_IDATE], NULL);
		generateLinks(ByDate, installed);
		fprintf(ByDate, "<h1 align=center>%s</h1>\n",
		    localizedStrings[LANG_SORTED_BY_IDATE]);
	    } else {
		generateHtmlHeader(ByDate,
		          localizedStrings[LANG_SORTED_BY_CDATE], NULL);
		generateLinks(ByDate, installed);
		fprintf(ByDate, "<h1 align=center>%s</h1>\n",
		    localizedStrings[LANG_SORTED_BY_CDATE]);
	    }
	    if (installed)
		fprintf(ByDate, "<h2>%s</h2>\n", localizedStrings[LANG_I_LESS_2W_OLD]);
	    else
		fprintf(ByDate, "<h2>%s</h2>\n", localizedStrings[LANG_LESS_2W_OLD]);
	    fprintf(ByDate, "<table><tbody>\n");
	}
    }
    fprintf(ByDate, "</tbody></table>\n");

    /*
     * Then dump RPMs less than one month old.
     */
    if (installed)
	fprintf(ByDate, "<h2>%s</h2>\n", localizedStrings[LANG_I_LESS_1M_OLD]);
    else
	fprintf(ByDate, "<h2>%s</h2>\n", localizedStrings[LANG_LESS_1M_OLD]);
    fprintf(ByDate, "<table><tbody>\n");
    while ((cur != NULL) && ((currentTime - cur->date) < (30 * 24 * 60 * 60))) {
	generateHtmlRpmRow(ByDate, cur, 0);
	if ((RSS != NULL) && (rdfcount++ < rpm2html_rdf_count_limit)) {
	    generateRSSItem(RSS, cur);
	}
	cur = cur->next;
	count++;
	if ((count % MAX_TABLE_LENGHT) == 0)
	    fprintf(ByDate, "</tbody></table>\n<table><tbody>\n");
	if (count > MAX_PAGE_LENGHT) {
	    count = 0;
	    fprintf(ByDate, "</tbody></table>\n");
	    fprintf(ByDate, "<a href=\"%d%s\">...</a>\n",
	            page, localizedStrings[LANG_BYDATE_HTML]);
	    generateHtmlFooter(ByDate);
	    fclose(ByDate);
	    fullPathNameNr(buf, sizeof(buf), dir, subdir,
	                   localizedStrings[LANG_BYDATE_HTML], page++);
	    if (rpm2htmlVerbose > 1) {
		printf("Dumping %s\n", buf);
	    }
	    ByDate = fopen(buf, "w");
	    if (ByDate == NULL) {
		fprintf(stderr, "Couldn't save to file %s: %s\n",
			buf, strerror(errno));
		if (RSS) {
		    fprintf(RSS, "  </channel>\n");
		    fprintf(RSS, "</rss>\n");
		    fclose(RSS);
		}
		return;
	    }
	    if (installed) {
		generateHtmlHeader(ByDate,
		          localizedStrings[LANG_SORTED_BY_IDATE], NULL);
		generateLinks(ByDate, installed);
		fprintf(ByDate, "<h1 align=center>%s</h1>\n",
		    localizedStrings[LANG_SORTED_BY_IDATE]);
	    } else {
		generateHtmlHeader(ByDate,
		          localizedStrings[LANG_SORTED_BY_CDATE], NULL);
		generateLinks(ByDate, installed);
		fprintf(ByDate, "<h1 align=center>%s</h1>\n",
		    localizedStrings[LANG_SORTED_BY_CDATE]);
	    }
	    if (installed)
		fprintf(ByDate, "<h2>%s</h2>\n", localizedStrings[LANG_I_LESS_1M_OLD]);
	    else
		fprintf(ByDate, "<h2>%s</h2>\n", localizedStrings[LANG_LESS_1M_OLD]);
	    fprintf(ByDate, "<table><tbody>\n");
	}
    }
    fprintf(ByDate, "</tbody></table>\n");

    /*
     * Then dump RPMs more than one month old.
     */
    if (installed)
	fprintf(ByDate, "<h2>%s</h2>\n", localizedStrings[LANG_I_MORE_1M_OLD]);
    else
	fprintf(ByDate, "<h2>%s</h2>\n", localizedStrings[LANG_MORE_1M_OLD]);
    fprintf(ByDate, "<table><tbody>\n");
    while (cur != NULL) {
	generateHtmlRpmRow(ByDate, cur, 0);
	if ((RSS != NULL) && (rdfcount++ < rpm2html_rdf_count_limit)) {
	    generateRSSItem(RSS, cur);
	}
	cur = cur->next;
	count++;
	if ((count % MAX_TABLE_LENGHT) == 0)
	    fprintf(ByDate, "</tbody></table>\n<table><tbody>\n");
	if (count > MAX_PAGE_LENGHT) {
	    count = 0;
	    fprintf(ByDate, "</tbody></table>\n");
	    fprintf(ByDate, "<a href=\"%d%s\">...</a>\n",
	            page, localizedStrings[LANG_BYDATE_HTML]);
	    generateHtmlFooter(ByDate);
	    fclose(ByDate);
	    fullPathNameNr(buf, sizeof(buf), dir, subdir,
	                   localizedStrings[LANG_BYDATE_HTML], page++);
	    if (rpm2htmlVerbose > 1) {
		printf("Dumping %s\n", buf);
	    }
	    ByDate = fopen(buf, "w");
	    if (ByDate == NULL) {
		fprintf(stderr, "Couldn't save to file %s: %s\n",
			buf, strerror(errno));
		if (RSS) {
		    fprintf(RSS, "  </channel>\n");
		    fprintf(RSS, "</rss>\n");
		    fclose(RSS);
		}
		return;
	    }
	    if (installed) {
		generateHtmlHeader(ByDate,
		          localizedStrings[LANG_SORTED_BY_IDATE], NULL);
		generateLinks(ByDate, installed);
		fprintf(ByDate, "<h1 align=center>%s</h1>\n",
		    localizedStrings[LANG_SORTED_BY_IDATE]);
	    } else {
		generateHtmlHeader(ByDate,
		          localizedStrings[LANG_SORTED_BY_CDATE], NULL);
		generateLinks(ByDate, installed);
		fprintf(ByDate, "<h1 align=center>%s</h1>\n",
		    localizedStrings[LANG_SORTED_BY_CDATE]);
	    }
	    if (installed)
		fprintf(ByDate, "<h2>%s</h2>\n", localizedStrings[LANG_I_MORE_1M_OLD]);
	    else
		fprintf(ByDate, "<h2>%s</h2>\n", localizedStrings[LANG_MORE_1M_OLD]);
	    fprintf(ByDate, "<table><tbody>\n");
	}
    }
    fprintf(ByDate, "</tbody></table>\n");
#endif	// ifdef HAVE_LIBTEMPLATE

    generateHtmlFooter(ByDate);
    fclose(ByDate);
    if (RSS) {
	fprintf(RSS, "  </channel>\n");
	fprintf(RSS, "</rss>\n");
	fclose(RSS);
    }
}

/*
 * Dump the full set of  RPM by Date HTML file.
 * One expect that the RPM files have been sorted by date.
 */
void dumpRpmByDate(rpmDataPtr list, int installed) {
    if (!rpm2html_dump_html) return;

    if (rpm2htmlVerbose)
      printf("Dumping \"by date\" HTML pages\n");

    dumpDirRpmByDate(list, NULL, NULL, installed);
}

/*
 * Dump the RPM in a flat list sorted by Name.
 * One expect that the RPM files have been sorted by name.
 */
void dumpDirRpmByNameFlat(rpmDataPtr list, rpmDirPtr dir,
                      char *subdir, int installed) {
    FILE *ByName;
    rpmDataPtr cur;

    if (!rpm2html_dump_html) return;

    cur = list;

    fullPathName(buf, sizeof(buf), dir, subdir, localizedStrings[LANG_BYNAME_HTML]);
    if (rpm2htmlVerbose > 1) {
        printf("Dumping %s\n", buf);
    }

    ByName = fopen(buf, "w");
    if (ByName == NULL) {
        fprintf(stderr, "Couldn't save to file %s: %s\n",
	        buf, strerror(errno));
        return;
    }
#ifdef HAVE_LIBTEMPLATE
    generateHtmlHeader(ByName, "names_title", NULL);
    generateLinks(ByName, installed);
    if (!installed) generateColorIndicator(ByName);

    parseNwriteTemplate(ByName, engine, "names_header");  // XXX ugly hack: here we assume that generateHtmlHeader() alredy filled proper value into "title" element of engine
      // later reconsicer usage of rpm2html only with libtemplate and then a) remove all those ifdefs and b) restructure html.c so as this ugy hack is not necessary

    while (cur != NULL) {
	generateHtmlRpmArchRow(ByName, cur);
	if ((dir == NULL) && (subdir == NULL))
	    cur = cur->nextSoft;
	else
	    cur = cur->next;
    }

    parseNwriteTemplate(ByName, engine, "name_end");

#else	// ifdef HAVE_LIBTEMPLATE

    generateHtmlHeader(ByName, localizedStrings[LANG_SORTED_BY_NAME], NULL);
    generateLinks(ByName, installed);
    fprintf(ByName, "<h1 align=center>%s</h1>\n",
        localizedStrings[LANG_SORTED_BY_NAME]);
    if (!installed) generateColorIndicator(ByName);
    fprintf(ByName, "<table><tbody>\n");

    while (cur != NULL) {
	generateHtmlRpmArchRow(ByName, cur);
	if ((dir == NULL) && (subdir == NULL))
	    cur = cur->nextSoft;
	else
	    cur = cur->next;
    }

    fprintf(ByName, "</tbody></table>\n");
#endif	// ifdef HAVE_LIBTEMPLATE

    generateHtmlFooter(ByName);
    fclose(ByName);
}

/*
 * Dump the RPM in a subtree by Name HTML file.
 * One expect that the RPM files have been sorted by name.
 */
void dumpDirRpmByName(rpmDataPtr list, rpmDirPtr dir,
                      char *subdir, int installed) {
    FILE *ByName;
    FILE *CurrName = NULL;
    rpmDataPtr cur;
    int i = 0;
    char last_letter = '\0', letter;

    if (!rpm2html_dump_html) return;

    if ((dir == NULL) && (subdir == NULL))
	for (cur = list; cur != NULL; cur = cur->nextSoft) i++;
    else
	for (cur = list; cur != NULL; cur = cur->next) i++;
    if (i < MAX_NAME_LIST_LENGHT) {
        dumpDirRpmByNameFlat(list, dir, subdir, installed);
        return;
    }
    i = 0;
    cur = list;

    fullPathName(buf, sizeof(buf), dir, subdir, localizedStrings[LANG_BYNAME_HTML]);
    if (rpm2htmlVerbose > 1) {
        printf("Dumping %s\n", buf);
    }

    ByName = fopen(buf, "w");
    if (ByName == NULL) {
        fprintf(stderr, "Couldn't save to file %s: %s\n",
	        buf, strerror(errno));
        return;
    }

#ifdef HAVE_LIBTEMPLATE
    generateHtmlHeader(ByName, "names_title", NULL);
    generateLinks(ByName, installed);
    parseNwriteTemplate(ByName, engine, "names_header");  // XXX ugly hack: here we assume that generateHtmlHeader() alredy filled proper value into "title" element of engine
      // later reconsicer usage of rpm2html only with libtemplate and then a) remove all those ifdefs and b) restructure html.c so as this ugy hack is not necessary

    while (cur != NULL) {
	letter = toupper(cur->name[0]);
	if (letter != last_letter) {
	    if (CurrName != NULL) {
		/*
		 * Finish the previous file.
		 */
		parseNwriteTemplate(CurrName, engine, "name_end");
		generateHtmlFooter(CurrName);
		fclose(CurrName);
	    }
	    if (i != 0) {
	        snprintf(buf, sizeof(buf), "%c", last_letter);
	        tpl_element_set(engine, "letter", buf);
	        snprintf(buf, sizeof(buf), "%d", i);
	        tpl_element_set(engine, "count", buf);
	        parseNwriteTemplate(ByName, engine, "names_item");
	    }
	    fullPathNameLr(buf, sizeof(buf), dir, subdir,
	                   localizedStrings[LANG_BYNAME_HTML], letter);
	    if (rpm2htmlVerbose > 1) {
		printf("Dumping %s\n", buf);
	    }

	    CurrName = fopen(buf, "w");
	    if (CurrName == NULL) {
		fprintf(stderr, "Couldn't save to file %s: %s\n",
			buf, strerror(errno));
		return;
	    }
	    snprintf(buf, sizeof(buf), "%c", letter);
	    tpl_element_set(engine, "letter", buf);
	    generateHtmlHeader(CurrName, "name_title", NULL);
	    generateLinks(CurrName, installed);
            parseNwriteTemplate(CurrName, engine, "name_header");  // XXX ugly hack: here we assume that generateHtmlHeader() alredy filled proper value into "title" element of engine
              // later reconsider usage of rpm2html only with libtemplate and then a) remove all those ifdefs and b) restructure html.c so as this ugy hack is not necessary
	    if (!installed) generateColorIndicator(CurrName);
	    parseNwriteTemplate(CurrName, engine, "name_begin");
	    i = 0;
	}
	i++;
	if ((i % MAX_TABLE_LENGHT) == 0) {
	    parseNwriteTemplate(CurrName, engine, "name_end");
	    parseNwriteTemplate(CurrName, engine, "name_begin");
        }
	last_letter = letter;
	generateHtmlRpmArchRow(CurrName, cur);
	if ((dir == NULL) && (subdir == NULL))
	    cur = cur->nextSoft;
	else
	    cur = cur->next;
    }

    if (i != 0) {
	snprintf(buf, sizeof(buf), "%c", last_letter);
	tpl_element_set(engine, "letter", buf);
	snprintf(buf, sizeof(buf), "%d", i);
	tpl_element_set(engine, "count", buf);
	parseNwriteTemplate(ByName, engine, "names_item");
    }
    if (CurrName != NULL) {
	/*
	 * Finish the previous file.
	 */
	parseNwriteTemplate(CurrName, engine, "name_end");
	generateHtmlFooter(CurrName);
	fclose(CurrName);
    }

#else	// ifdef HAVE_LIBTEMPLATE

    generateHtmlHeader(ByName, localizedStrings[LANG_SORTED_BY_NAME], NULL);
    generateLinks(ByName, installed);
    fprintf(ByName, "<h1 align=center>%s</h1>\n",
        localizedStrings[LANG_SORTED_BY_NAME]);

    while (cur != NULL) {
	letter = toupper(cur->name[0]);
	if (letter != last_letter) {
	    if (CurrName != NULL) {
		/*
		 * Finish the previous file.
		 */
		fprintf(CurrName, "</tbody></table>\n");
		generateHtmlFooter(CurrName);
		fclose(CurrName);
	    }
	    if (i != 0)
		fprintf(ByName, "<p><a href=\"%c%s\">%d %s %c</a></p>\n",
		        last_letter, localizedStrings[LANG_BYNAME_HTML],
			i, localizedStrings[LANG_BEGINNING_LETTER],
			last_letter);
	    fullPathNameLr(buf, sizeof(buf), dir, subdir,
	                   localizedStrings[LANG_BYNAME_HTML], letter);
	    if (rpm2htmlVerbose > 1) {
		printf("Dumping %s\n", buf);
	    }

	    CurrName = fopen(buf, "w");
	    if (CurrName == NULL) {
		fprintf(stderr, "Couldn't save to file %s: %s\n",
			buf, strerror(errno));
		return;
	    }
	    snprintf(buf, sizeof(buf), "%s %c", localizedStrings[LANG_BEGINNING_LETTER],
	            letter);
	    generateHtmlHeader(CurrName, buf, NULL);
	    generateLinks(CurrName, installed);
	    fprintf(CurrName, "<h1 align=center>%s</h1>\n", buf);
	    if (!installed) generateColorIndicator(CurrName);
	    fprintf(CurrName, "<table><tbody>\n");
	    i = 0;
	}
	i++;
	if ((i % MAX_TABLE_LENGHT) == 0)
	    fprintf(CurrName, "</tbody></table>\n<table><tbody>\n");
	last_letter = letter;
	generateHtmlRpmArchRow(CurrName, cur);
	if ((dir == NULL) && (subdir == NULL))
	    cur = cur->nextSoft;
	else
	    cur = cur->next;
    }

    if (i != 0)
	fprintf(ByName, "<p><a href=\"%c%s\">%d %s %c</a></p>\n",
		last_letter, localizedStrings[LANG_BYNAME_HTML],
		i, localizedStrings[LANG_BEGINNING_LETTER],
		last_letter);
    if (CurrName != NULL) {
	/*
	 * Finish the previous file.
	 */
	fprintf(CurrName, "</tbody></table>\n");
	generateHtmlFooter(CurrName);
	fclose(CurrName);
    }
#endif	// ifdef HAVE_LIBTEMPLATE

    generateHtmlFooter(ByName);
    fclose(ByName);
}

/*
 * Dump the full set of  RPM by Name HTML file.
 * One expect that the RPM files have been sorted by name.
 */
void dumpRpmByName(rpmDataPtr list, int installed) {
    if (!rpm2html_dump_html) return;

    if (rpm2htmlVerbose)
      printf("Dumping \"by name\" HTML pages\n");

    dumpDirRpmByName(list, NULL, NULL, installed);
}

/*
 * Dump the index for a directory and its subdirectories.
 */
rpmDataPtr dumpDirIndex(rpmDirPtr dir, rpmSubdirPtr tree, rpmDataPtr list) {
    FILE *html;
    int i;

    if (!rpm2html_dump_html) return(list);
    if ((dir != NULL) && (!dir->html)) return(list);

    if (tree->htmlpath[0] != '\0')
	snprintf(buf, sizeof(buf), "%s/%s/%s", rpm2html_dir, tree->htmlpath,
		localizedStrings[LANG_INDEX_HTML]);
    else
	snprintf(buf, sizeof(buf), "%s/%s", rpm2html_dir,
		localizedStrings[LANG_INDEX_HTML]);

    if (rpm2htmlVerbose > 1) {
	printf("Dumping %s\n", buf);
    }

    html = fopen(buf, "w");
    if (html == NULL) {
        fprintf(stderr, "Couldn't save to file %s: %s\n",
	        buf, strerror(errno));
        return(list);
    }
    if (tree->htmlpath[0] != '\0')
        snprintf(buf, sizeof(buf), "%s : %s", dir->name, tree->htmlpath);
    else
        snprintf(buf, sizeof(buf), "%s", dir->name);

#ifdef HAVE_LIBTEMPLATE
    tpl_element_set(engine, "dir", buf);
    generateHtmlHeader(html, "dirs_title", NULL);
    generateLinks(html, 0);
    if (((tree->rpmpath == NULL) || (tree->rpmpath[0] == '\0')) &&
        (dir->build_tree)) {
        tpl_parse(engine, "dirs_parent", "p_title", 0);
        tpl_element_set(engine, "title", tpl_element_get(engine, "p_title"));
    }
    else
        tpl_element_set(engine, "title", "");
    parseNwriteTemplate(html, engine, "dirs_header");  // XXX ugly hack: here we assume that generateHtmlHeader() alredy filled proper value into "title" element of engine
      // later reconsicer usage of rpm2html only with libtemplate and then a) remove all those ifdefs and b) restructure html.c so as this ugy hack is not necessary

    if (((tree->rpmpath == NULL) || (tree->rpmpath[0] == '\0')) &&
        (dir->build_tree)) {
	if (tree->color) {
            tpl_element_set(engine, "color", tree->color);
            tpl_parse(engine, "dirs_tree_color", "p_html_tree_header_color", 0);	// XXX maybe not necessary to call this more than once? => optimize to call just once
            tpl_element_set(engine, "color", tpl_element_get(engine, "p_html_tree_header_color"));
        }
	else
	    tpl_element_set(engine, "color", "");
	tpl_element_set(engine, "url", rpm2html_url);
	parseNwriteTemplate(html, engine, "dirs_tree");
    }

    generateHtmlTree(html, tree, 0, 1);

    tpl_element_set(engine, "color", dir->color);
    tpl_element_set(engine, "url", dir->ftp);
    if (dir->name == NULL)
        tpl_element_set(engine, "name", dir->ftp);
    else
        tpl_element_set(engine, "name", dir->name);
    parseNwriteTemplate(html, engine, "dir_begin");

    if ((dir->ftpsrc != NULL) && (strcmp(dir->ftp, dir->ftpsrc))) {
        tpl_element_set(engine, "color", dir->color);
        tpl_element_set(engine, "url", dir->ftpsrc);
        tpl_element_set(engine, "name", dir->ftpsrc);
        parseNwriteTemplate(html, engine, "dir_src");
    }
    if (dir->nb_mirrors > 0) {
        tpl_element_set(engine, "color", dir->color);
        tpl_element_set(engine, "url", dir->mirrors[0]);
        tpl_element_set(engine, "name", dir->mirrors[0]);
        parseNwriteTemplate(html, engine, "dir_mirrors");
    }
    if (dir->nb_mirrors > 1) {
        tpl_element_set(engine, "color", dir->color);
        parseNwriteTemplate(html, engine, "dir_mirrors_list_begin");
	for (i = 1;i < dir->nb_mirrors;i++) {
	    //tpl_element_set(engine, "color", dir->color);	- we've still got it
	    tpl_element_set(engine, "url", dir->mirrors[i]);
	    tpl_element_set(engine, "name", dir->mirrors[i]);
	    parseNwriteTemplate(html, engine, "dir_mirrors_list_item");
	}
    }
    parseNwriteTemplate(html, engine, "dir_mirrors_list_end");

#else	// ifdef HAVE_LIBTEMPLATE

    generateHtmlHeader(html, buf, NULL);
    generateLinks(html, 0);
    if (((tree->rpmpath == NULL) || (tree->rpmpath[0] == '\0')) &&
        (dir->build_tree))
        snprintf(buf, sizeof(buf), "<a href=\"Root%s\">%s</a>",
	        localizedStrings[LANG_TREE_HTML],
		localizedStrings[LANG_INDEX_HTML]);
    fprintf(html, "<h1 align=center>%s</h1>\n", buf);

    fprintf(html, "<ul>\n");
    fprintf(html, "<li>\n%s <a href=\"%s\">%s</a>\n",
            localizedStrings[LANG_LIST],
	    localizedStrings[LANG_GROUP_HTML],
	    localizedStrings[LANG_INDEX_GROUP]);
    fprintf(html, "<li>\n%s <a href=\"%s\">%s</a>\n",
            localizedStrings[LANG_LIST],
	    localizedStrings[LANG_BYDATE_HTML],
	    localizedStrings[LANG_INDEX_CREATION]);
    fprintf(html, "<li>\n%s <a href=\"%s\">%s</a>\n",
            localizedStrings[LANG_LIST],
	    localizedStrings[LANG_BYNAME_HTML],
	    localizedStrings[LANG_INDEX_NAME]);
    fprintf(html, "<li>\n%s <a href=\"%s\">%s</a>\n",
            localizedStrings[LANG_LIST],
	    localizedStrings[LANG_VENDOR_HTML],
	    localizedStrings[LANG_INDEX_VENDOR]);
    fprintf(html, "<li>\n%s <a href=\"%s\">%s</a>\n",
            localizedStrings[LANG_LIST],
	    localizedStrings[LANG_DISTRIB_HTML],
	    localizedStrings[LANG_INDEX_DISTRIB]);
    fprintf(html, "</ul>\n\n");
    if (((tree->rpmpath == NULL) || (tree->rpmpath[0] == '\0')) &&
        (dir->build_tree)) {
	fprintf(html, "<h3>%s</h3>\n",
	    localizedStrings[LANG_BROWSE_TREE]);
	if (tree->color)
	    fprintf(html, "<blockquote style=\"background : %s\">\n",
	            tree->color);
	else
	    fprintf(html, "<blockquote>\n");
	fprintf(html, "<p><strong><a href=\"%s\">",
	        localizedStrings[LANG_TREE_HTML]);
	fprintf(html,
	        "<img src=\"%s/dir.png\" alt=\"Directory\" border=0>",
	            rpm2html_url);
	fprintf(html, " ..</a></strong></p></blockquote>\n");
    }

    generateHtmlTree(html, tree, 0, 1);
    
    fprintf(html, "<table bgcolor=\"%s\"><tbody>\n<tr>\n",
	    dir->color);
    if (dir->name == NULL)
	fprintf(html, "<td bgcolor=\"%s\" colspan=2><a href=\"%s\">%s</a></td>\n",
		dir->color, dir->ftp, dir->ftp);
    else
	fprintf(html, "<td bgcolor=\"%s\" colspan=2><a href=\"%s\">%s</a></td>\n",
		dir->color, dir->ftp, dir->name);
    fprintf(html, "</tr>\n");
    if ((dir->ftpsrc != NULL) && (strcmp(dir->ftp, dir->ftpsrc)))
	fprintf(html, 
	    "<tr><td bgcolor=\"%s\">%s:</td><td bgcolor=\"%s\"><a href=\"%s\">%s</a></td></tr>\n",
	    dir->color, localizedStrings[LANG_SOURCES_REPOSITORY],
	    dir->color, dir->ftpsrc, dir->ftpsrc);
    if (dir->nb_mirrors > 0)
	fprintf(html,
	    "<tr><td bgcolor=\"%s\">%s:</td><td bgcolor=\"%s\"><a href=\"%s\">%s</a></td></tr>\n",
	    dir->color, localizedStrings[LANG_LOCAL_MIRROR],
	    dir->color, dir->mirrors[0], dir->mirrors[0]);
    if (dir->nb_mirrors > 1) {
	fprintf(html, "<tr><td bgcolor=\"%s\" colspan=2>%s</td></tr>\n",
		dir->color, localizedStrings[LANG_MIRRORS]);
	for (i = 1;i < dir->nb_mirrors;i++) {
	    fprintf(html,
		"<tr><td bgcolor=\"%s\" colspan=2><a href=\"%s\">%s</a></td></tr>\n",
		dir->color, dir->mirrors[i], dir->mirrors[i]);
	}
    }
    fprintf(html, "</tbody></table>\n");
#endif	// ifdef HAVE_LIBTEMPLATE

    generateHtmlFooter(html);
    fclose(html);

    rpmDistribSort(&list, 0);
    dumpDirRpmByDistribs(list, dir, tree->htmlpath, 0);
    rpmGroupSort(&list, 0);
    dumpDirRpmByGroups(list, dir, tree->htmlpath, 0);
    rpmVendorSort(&list, 0);
    dumpDirRpmByVendors(list, dir, tree->htmlpath, 0);
    rpmDateSort(&list, 0);
    dumpDirRpmByDate(list, dir, tree->htmlpath, 0);
    rpmNameSort(&list, 0);
    dumpDirRpmByName(list, dir, tree->htmlpath, 0);
    return(list);
}

/*
 * Dump a real tree subdir title with links to parents and whole tree.
 */

void rpmDumpHtmlRealTreeTitle(FILE *subdir, rpmDirPtr dir, char *buffer) {
#ifndef HAVE_LIBTEMPLATE
    char buf[3000];
#endif
    char tmp[1000];
    char path[1000];
    char loc[200];
    int i = 0, j = 0, k = 0; /* indexes for buffer, loc and path resp. */
    int l = 0; /* index for directory depth */

    /* link for the root */
#ifdef HAVE_LIBTEMPLATE
    tpl_element_set(engine, "name", dir->name);
    parseNwriteTemplate(subdir, engine, "realtree_title_header");
    while (buffer[i] != '\0') {
        if (buffer[i] == '_') {
	    loc[j] = '\0';
	    path[k++] = '_';
	    path[k] = '\0';
	    tpl_element_set(engine, "path", path);
	    if (l++) {
	    	snprintf(tmp, sizeof(tmp), "/%s</a>", loc);
	    	tpl_element_set(engine, "name", tmp);
            }
	    else
	        tpl_element_set(engine, "name", loc);
            parseNwriteTemplate(subdir, engine, "realtree_title_link");
	    j = 0;
	} else {
	    loc[j++] = buffer[i];
	    path[k++] = buffer[i];
	}
        i++;
    }

    parseNwriteTemplate(subdir, engine, "realtree_title_footer");

#else	// ifdef HAVE_LIBTEMPLATE

    snprintf(buf, sizeof(buf), "<a href=\"%s\">/</a>",
	     localizedStrings[LANG_TREE_HTML]);
    while (buffer[i] != '\0') {
        if (buffer[i] == '_') {
	    loc[j] = '\0';
	    path[k++] = '_';
	    path[k] = '\0';
	    if (l++)
	    	snprintf(tmp, sizeof(tmp), " <a href=\"%s%s\">/%s</a>", path,
	            	localizedStrings[LANG_TREE_HTML], loc);
	    else
	    	snprintf(tmp, sizeof(tmp), " <a href=\"%s%s\">%s</a>", path,
	            	localizedStrings[LANG_TREE_HTML], loc);
            strcat(buf, tmp);
	    j = 0;
	} else {
	    loc[j++] = buffer[i];
	    path[k++] = buffer[i];
	}
        i++;
    }

    fprintf(subdir, "<h1 align=center><a href=\"Root%s\">%s</a> : %s</h1>\n",
            localizedStrings[LANG_TREE_HTML], dir->name, buf);
#endif	// ifdef HAVE_LIBTEMPLATE
}

/*
 * Dump a real tree subdir.
 */

void rpmDumpHtmlRealTree(FILE *html, rpmDirPtr dir, rpmRealDirPtr tree,
                         char *buffer, int index) {
    FILE *subdir;
    char buf[1000];
    int has_subdir = 0;
    int has_file = 0;
    int i;

    if (!rpm2html_dump_html) return;
    if ((dir != NULL) && (!dir->html)) return;

    if (dir->subdir)
	snprintf(buf, sizeof(buf), "%s/%s/%s%s", rpm2html_dir,
	        dir->subdir, buffer,
		localizedStrings[LANG_TREE_HTML]);
    else
	snprintf(buf, sizeof(buf), "%s/%s%s", rpm2html_dir, buffer,
	        localizedStrings[LANG_TREE_HTML]);

    if (rpm2htmlVerbose > 1) {
	printf("Dumping %s\n", buf);
    }

    subdir = fopen(buf, "w");
    if (subdir == NULL) {
        fprintf(stderr, "Couldn't save to file %s: %s\n",
	        buf, strerror(errno));
        return;
    }



#ifdef HAVE_LIBTEMPLATE
    tpl_element_set(engine, "dir", buffer);	// XXX maybe use a nices title than 'usr_share_xyz_' etc.
    generateHtmlHeader(subdir, "realtree_title", NULL);
    generateLinks(subdir, 1);
    rpmDumpHtmlRealTreeTitle(subdir, dir, buffer);

    for (i = 0;i < tree->nb_elem;i++) {
        if (tree->flags[i] & RPM_ELEM_DIRECTORY) {
	    if (!has_subdir)
	        parseNwriteTemplate(html, engine, "realtree_item_begin");
	    has_subdir++;
	    tpl_element_set(engine, "url", buffer);
	    tpl_element_set(engine, "suburl", tree->names[i]);
	    tpl_element_set(engine, "name", tree->names[i]);
	    parseNwriteTemplate(html, engine, "realtree_item_item");
	    snprintf(&buffer[index], 500, "%s_", tree->names[i]);
	    rpmDumpHtmlRealTree(html, dir, tree->elems[i],
	                        buffer, strlen(buffer));
	    buffer[index] = '\0';
	    if (!has_file)
	        parseNwriteTemplate(subdir, engine, "realtree_subitem_begin");
	    has_file++;
	    tpl_element_set(engine, "url", buffer);
	    tpl_element_set(engine, "suburl", tree->names[i]);
	    tpl_element_set(engine, "rpm2html_url", rpm2html_url);
	    tpl_element_set(engine, "name", tree->names[i]);
	    parseNwriteTemplate(subdir, engine, "realtree_subitem_dir");

	} else {
	    if (!has_file)
	        parseNwriteTemplate(subdir, engine, "realtree_subitem_begin");
	    has_file++;
	    tpl_element_set(engine, "url", generateHtmlRpmAnchor(tree->elems[i]));
	    tpl_element_set(engine, "name", tree->names[i]);
	    parseNwriteTemplate(subdir, engine, "realtree_subitem_file");
	} 
    }
    if (has_subdir)
        parseNwriteTemplate(html, engine, "realtree_item_end");
    if (has_file)
        parseNwriteTemplate(subdir, engine, "realtree_subitem_end");

#else	// ifdef HAVE_LIBTEMPLATE

    generateHtmlHeader(subdir, buffer, NULL);	// XXX maybe use a nices title than 'usr_share_xyz_' etc.
    generateLinks(subdir, 1);
    rpmDumpHtmlRealTreeTitle(subdir, dir, buffer);

    for (i = 0;i < tree->nb_elem;i++) {
        if (tree->flags[i] & RPM_ELEM_DIRECTORY) {
	    if (!has_subdir)
		fprintf(html, "<ul>\n");
	    has_subdir++;
	    fprintf(html, "<li> <a href=\"%s%s_%s\">%s</a>\n",
	            buffer, tree->names[i],
		    localizedStrings[LANG_TREE_HTML], tree->names[i]);
	    snprintf(&buffer[index], 500, "%s_", tree->names[i]);
	    rpmDumpHtmlRealTree(html, dir, tree->elems[i],
	                        buffer, strlen(buffer));
	    buffer[index] = '\0';
	    if (!has_file)
		fprintf(subdir, "<ul>\n");
	    has_file++;
	    fprintf(subdir, "<li><a href=\"%s%s_%s\">",
	            buffer, tree->names[i],
		    localizedStrings[LANG_TREE_HTML]);
	    fprintf(subdir,
	        "<img src=\"%s/dir.png\" alt=\"subdir\" border=0>",
	            rpm2html_url);
	    fprintf(subdir, " %s</a>\n", tree->names[i]);

	} else {
	    if (!has_file)
		fprintf(subdir, "<ul>\n");
	    has_file++;
	    fprintf(subdir, "<li>");
	    generateHtmlRpmAnchor(subdir, tree->elems[i]);
	    fprintf(subdir, "%s</a>\n", tree->names[i]);
	} 
    }
    if (has_subdir)
	fprintf(html, "</ul>\n");
    if (has_file)
	fprintf(subdir, "</ul>\n");
#endif	// ifdef HAVE_LIBTEMPLATE

    generateHtmlFooter(subdir);
    fclose(subdir);
}

/*
 * Dump a real tree root.
 */

void rpmDumpHtmlRealRoot(rpmDirPtr dir) {
    FILE *html;
    char buf[1000];
    rpmRealDirPtr tree;

    if (!rpm2html_dump_html) return;
    if ((dir != NULL) && (!dir->html)) return;

    if (dir == NULL) return;
    tree = dir->root;
    if (tree == NULL) return;
    if (dir->subdir)
	snprintf(buf, sizeof(buf), "%s/%s/Root%s", rpm2html_dir,
	        dir->subdir, localizedStrings[LANG_TREE_HTML]);
    else
	snprintf(buf, sizeof(buf), "%s/Root%s", rpm2html_dir,
	        localizedStrings[LANG_TREE_HTML]);

    if (rpm2htmlVerbose > 1) {
	printf("Dumping %s\n", buf);
    }

    html = fopen(buf, "w");
    if (html == NULL) {
        fprintf(stderr, "Couldn't save to file %s: %s\n",
	        buf, strerror(errno));
        return;
    }

    snprintf(buf, sizeof(buf), "%s", dir->name);

#ifdef HAVE_LIBTEMPLATE
    tpl_element_set(engine, "dir", buf);
    generateHtmlHeader(html, "realroot_title", NULL);
    generateLinks(html, 1);
    parseNwriteTemplate(html, engine, "realroot_header");  // XXX ugly hack: here we assume that generateHtmlHeader() alredy filled proper value into "title" element of engine
      // later reconsicer usage of rpm2html only with libtemplate and then a) remove all those ifdefs and b) restructure html.c so as this ugy hack is not necessary

    buf[0] = 0;
    rpmDumpHtmlRealTree(html, dir, tree, buf, 0);

#else	// ifdef HAVE_LIBTEMPLATE

    generateHtmlHeader(html, buf, NULL);
    generateLinks(html, 1);
    fprintf(html, "<h1 align=center>%s</h1>\n", buf);

    buf[0] = 0;
    rpmDumpHtmlRealTree(html, dir, tree, buf, 0);
#endif	// ifdef HAVE_LIBTEMPLATE
    
    generateHtmlFooter(html);
    fclose(html);
}

