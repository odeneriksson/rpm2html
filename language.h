/*
 * language.h : definitions for the Localization support.
 *
 * See Copyright for the status of this software.
 *
 * $Id: language.h,v 1.13 2000/10/29 22:51:03 veillard Exp $
 */

#ifndef __RPM2HTML_LANGUAGE_H__
#define __RPM2HTML_LANGUAGE_H__

#define LANG_HTML_SUFFIX	0
#define LANG_GENERATED		1
#define LANG_INDEX_HTML		2
#define LANG_GROUP_HTML		3
#define LANG_BYDATE_HTML	4
#define LANG_BYNAME_HTML	5
#define LANG_VENDOR_HTML	6
#define LANG_DISTRIB_HTML	7
#define LANG_WELCOME_REPOSITORY	8
#define LANG_RPM2HTML_INTRO	9
#define LANG_STATS		10
#define LANG_INSTALLED_STATS	11
#define LANG_LIST		12
#define LANG_INDEX_GROUP	13
#define LANG_INDEX_CREATION	14
#define LANG_INDEX_NAME		15
#define LANG_INDEX_VENDOR	16
#define LANG_INDEX_DISTRIB	17
#define LANG_INDEX_INSTALL	18
#define LANG_SOURCES_REPOSITORY	19
#define LANG_LOCAL_MIRROR	20
#define LANG_MIRRORS		21
#define LANG_GENERATION_TIME	22
#define LANG_SECONDS		23
#define LANG_WELCOME_INSTALL	24
#define LANG_FROM		25
#define LANG_NAME		26
#define LANG_DISTRIBUTION	27
#define LANG_VERSION		28
#define LANG_VENDOR		29
#define LANG_RELEASE		30
#define LANG_BUILD_DATE		31
#define LANG_INSTALL_DATE	32
#define LANG_GROUP		33
#define LANG_BUILD_HOST		34
#define LANG_SIZE		35
#define LANG_RPM_SRC		36
#define LANG_PACKAGER		37
#define LANG_URL		38
#define LANG_SUMMARY		39
#define LANG_PROVIDE		40
#define LANG_REQUIRE		41
#define LANG_COPYRIGHT		42
#define LANG_FILES		43
#define LANG_NO_FILES		44
#define LANG_NO_SUMMARY		45
#define LANG_RPM_RESOURCE	46
#define LANG_PROVIDED_BY	47
#define LANG_SORTED_BY_GROUP	48
#define LANG_OF_GROUP		49
#define LANG_SORTED_BY_DISTRIB  50
#define LANG_OF_DISTRIB		51
#define LANG_SORTED_BY_VENDOR	52
#define LANG_OF_VENDOR		53
#define LANG_SORTED_BY_CDATE	54
#define LANG_SORTED_BY_IDATE	55
#define LANG_LESS_3D_OLD	56
#define LANG_LESS_1W_OLD	57
#define LANG_LESS_2W_OLD	58
#define LANG_LESS_1M_OLD	59
#define LANG_MORE_1M_OLD	60
#define LANG_I_LESS_3D_OLD	61
#define LANG_I_LESS_1W_OLD	62
#define LANG_I_LESS_2W_OLD	63
#define LANG_I_LESS_1M_OLD	64
#define LANG_I_MORE_1M_OLD	65
#define LANG_SORTED_BY_NAME	66
#define LANG_NO_DESCRIPTION	67
#define LANG_UNKNOWN		68
#define LANG_NONE		69
#define LANG_NO_GROUP		70
#define LANG_NO_HOST		71
#define LANG_INDEX		72
#define LANG_BEGINNING_LETTER	73
#define LANG_WARNING_RESOURCES	74
#define LANG_CHOOSE_ANOTHER	75
#define LANG_MORE		76
#define LANG_CHANGELOG		77
#define LANG_SUBDIRS		78
#define LANG_TREE_HTML		79
#define LANG_BROWSE_TREE	80
#define LANG_SEARCH_FORM	81
#define LANG_SIGNATURES		82
#define LANG_RDF_CHANNEL	83

/*
 * Array containing the localized language strings.
 */

extern char *localizedStrings[];

/*
 * Functions.
 */

extern int readLanguageFile(char *filename);
extern int writeLanguageFile(char *filename);

#endif /* __RPM2HTML_LANGUAGE_H__ */

