AUTOMAKE_OPTIONS=no-dependencies
NAME=rpm2html

FIND_FILES= cleanup.c compressor.c config.c html.c language.c		\
    memory.c rdf_api.c rdf.c rpm2html.c rpmdata.c rpmopen.c stringbuf.c	\
    sql.c sqltools.c config.h html.h language.h memory.h rdf_api.h	\
    rdf.h rpm2html.h rpmdata.h sql.h stringbuf.h
MAKE_FILES=configure configure.in config.h.in Makefile.am Makefile.in \
           autogen.sh install-sh config.guess config.sub acconfig.h

CONFIG= rpm2html.config rpm2html-rdf.config rpm2html-cdrom.config rpm2html-en.config
USELESS=COPYING depcomp

FILES= Copyright BUGS README TODO INSTALL CHANGES PRINCIPLES \
       missing NEWS AUTHORS \
       rpm2html.spec rpm2html.spec.in rpm2html.1 ChangeLog \
       msg.fr msg.es msg.de msg.pl msg.cz dir.png new.png \
       rpm2html.tpl \
       $(MAKE_FILES) $(CONFIG) $(RDF_FILES) $(FIND_FILES) $(USELESS)

INCLUDES = -I$(top_srcdir) -I$(top_builddir) $(CFLAGS) $(XML_CFLAGS) \
           $(RPM_CFLAGS) $(SQL_FLAGS) $(GPG_FLAGS)

noinst_PROGRAMS = compressor
compressor_SOURCES = compressor.c
compressor_LDFLAGS =
compressor_LDADD = $(LIBS)

LDADDS = $(XML_LIBS) $(LIBS)

if WITH_MYSQL
bin_PROGRAMS = rpm2html sqltools

sqltools_SOURCES = sql.c cleanup.c config.c html.c language.c	\
    memory.c rdf_api.c rdf.c sqltools.c rpmdata.c rpmopen.c stringbuf.c \
    dir.png.h new.png.h
sqltools_LDFLAGS =
sqltools_LDADD = $(LDADDS)

rpm2html_SOURCES = sql.c cleanup.c config.c html.c language.c	\
    memory.c rdf_api.c rdf.c rpm2html.c rpmdata.c rpmopen.c stringbuf.c \
    dir.png.h new.png.h
rpm2html_LDFLAGS =
rpm2html_LDADD = $(LDADDS)
else
bin_PROGRAMS = rpm2html

rpm2html_SOURCES = cleanup.c config.c html.c language.c	\
    memory.c rdf_api.c rdf.c rpm2html.c rpmdata.c rpmopen.c stringbuf.c \
    dir.png.h new.png.h
rpm2html_LDFLAGS =
rpm2html_LDADD = $(LDADDS)
endif

all: rpm2html

clean:
	rm -f dir.png.h new.png.h

html.o: dir.png.h new.png.h

dir.png.h : $(srcdir)/dir.png $(top_builddir)/compressor
	if [ ! -f dir.png ] ; then $(CP) $(srcdir)/dir.png . ; fi
	$(top_builddir)/compressor dir.png

new.png.h : $(srcdir)/new.png $(top_builddir)/compressor
	if [ ! -f new.png ] ; then $(CP) $(srcdir)/new.png . ; fi
	$(top_builddir)/compressor new.png

distrib : clean pissmeoff
	@(rm -rf $(NAME)-$(VERSION); ln -s . $(NAME)-$(VERSION))
	@(cd $(srcdir) ; tar cvf - `for i in $(FILES); \
	  do echo $(NAME)-$(VERSION)/$$i ; \
	  done` | $(GZIP) > $(NAME)-$(VERSION).tar.gz)
	@$(RM) $(srcdir)/$(NAME)-$(VERSION)

rpm : distrib
	@(rpmbuild -ta $(srcdir)/$(NAME)-$(VERSION).tar.gz)

pissmeoff $(USELESS):
	@rm -f $(USELESS)
	@touch $(USELESS)

install : $(bin_PROGRAMS)
	install -c -d $(DESTDIR)$(bindir) $(DESTDIR)$(mandir)/man1/ \
		$(DESTDIR)$(datadir)/$(NAME) $(DESTDIR)$(sysconfdir)
	install -c rpm2html $(DESTDIR)$(bindir)
	install -c -m 644 rpm2html.1 $(DESTDIR)$(mandir)/man1/
	install -c -d $(DESTDIR)$(datadir)/$(NAME)
	install -c -m 644 msg.* $(DESTDIR)$(datadir)/$(NAME)
	install -c -m 644 rpm2html.config $(DESTDIR)$(sysconfdir)/rpm2html.config
	@echo "Don't forget to edit $(sysconfdir)/rpm2html.config"
