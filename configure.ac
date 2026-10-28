AC_PREREQ([2.69])
AC_INIT([rpm2html], [1.11.2])
AC_CONFIG_SRCDIR([rpm2html.c])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([foreign])

AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
PKG_PROG_PKG_CONFIG
PKG_CHECK_MODULES([RPM], [rpm])
PKG_CHECK_MODULES([XML], [libxml-2.0])
AC_CHECK_LIB([z], [compress], [], [AC_MSG_ERROR([zlib development files missing])])

AC_CHECK_HEADERS([fcntl.h unistd.h ctype.h dirent.h errno.h malloc.h stdarg.h sys/stat.h sys/types.h time.h zlib.h])
AC_CHECK_FUNCS([gethostname strdup strndup strerror snprintf strftime unlink])

AC_PATH_PROG([CP], [cp], [/bin/cp])
AC_PATH_PROG([RM], [rm], [/bin/rm])
AC_PATH_PROG([MV], [mv], [/bin/mv])
AC_PATH_PROG([TAR], [tar], [/bin/tar])
AC_PATH_PROG([GZIP], [gzip], [/bin/gzip])

AC_ARG_WITH([sql], [AS_HELP_STRING([--with-sql], [MySQL support (not ported)])],
            [AS_IF([test "x$withval" = xyes], [AC_MSG_ERROR([SQL mode not ported to RPM 6])])])
AC_ARG_WITH([gpg], [AS_HELP_STRING([--with-gpg], [Legacy GPG mode (not ported)])],
            [AS_IF([test "x$withval" = xyes], [AC_MSG_ERROR([GPG mode not ported to RPM 6])])])
AC_ARG_WITH([template], [AS_HELP_STRING([--with-template], [External libtemplate mode (not ported)])],
            [AS_IF([test "x$withval" = xyes], [AC_MSG_ERROR([libtemplate mode not ported])])])
AM_CONDITIONAL([WITH_MYSQL], [false])
AC_DEFINE([HAVE_RPM42], [1], [Modern RPM file iteration API])
AC_DEFINE([HAVE_RPM_RPMIO_H], [1], [Use rpmio.h])
AC_DEFINE([HAVE_SNPRINTF], [1], [snprintf supported])
AC_SUBST([SQL_FLAGS], [])
AC_SUBST([GPG_FLAGS], [])
AC_SUBST([SQL_OBJ], [])
AC_SUBST([SQL_PROGS], [])
AC_CONFIG_FILES([Makefile rpm2html.spec])
AC_OUTPUT
