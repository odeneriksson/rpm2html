Summary: Translates an RPM database and dependency information into HTML
Name: rpm2html 
%define version 1.11.2
Version: %{version}
Release: 1%{?dist}
Group: Applications/System
Source0: http://savannah.nongnu.org/download/rpm2html/rpm2html-%{version}.tar.gz
URL: http://www.nongnu.org/rpm2html/
License: MIT
BuildRoot: %{_tmppath}/%{name}-root
BuildPreReq: rpm-devel
BuildRequires: automake
BuildRequires: bzip2-devel
BuildRequires: libtemplate-devel
BuildRequires: libxml2-devel
BuildRequires: popt
BuildRequires: zlib-devel
Requires: gnupg

%description
The rpm2html utility automatically generates web pages that describe a
set of RPM packages.  The goals of rpm2html are to identify the
dependencies between various packages, and to find the package(s) that
will provide the resources needed to install a given package.
Rpm2html analyzes the provides and requires of the given set of RPMs,
and then shows the dependency cross-references using hypertext links.
Rpm2html can now dump the meta data associated with RPM files into
standard RDF files.

Install rpm2html if you want a utility for translating information
from an RPM database into HTML.

%prep
%setup -q

%build
./autogen.sh
%configure --sysconfdir=/etc \
	--with-gpg \
	--with-template
make %{?_smp_mflags}

# enable 'html_template' option
sed -i -e 's#^;html_template=rpm2html.tpl#html_template=%{_sysconfdir}/%{name}.tpl#g' rpm2html.config*

%install
rm -rf $RPM_BUILD_ROOT
install -d ${RPM_BUILD_ROOT}%{_bindir}
install -d ${RPM_BUILD_ROOT}%{_sysconfdir}
install -d ${RPM_BUILD_ROOT}%{_datadir}/rpm2html
install -d ${RPM_BUILD_ROOT}%{_mandir}/man1

install -m 0755 rpm2html ${RPM_BUILD_ROOT}%{_bindir}

install -m 0644 msg.* ${RPM_BUILD_ROOT}%{_datadir}/rpm2html/

install -m 0644 rpm2html.config rpm2html.tpl ${RPM_BUILD_ROOT}%{_sysconfdir}
install -m 0644 rpm2html.1  ${RPM_BUILD_ROOT}%{_mandir}/man1/rpm2html.1

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc CHANGES BUGS Copyright PRINCIPLES README TODO 
%doc rpm2html-cdrom.config rpm2html-en.config rpm2html-rdf.config
%{_datadir}/rpm2html
%{_bindir}/rpm2html*
%{_datadir}/rpm2html/msg.*
%{_mandir}/man1/*
%config(noreplace) %{_sysconfdir}/rpm2html.config
%config(noreplace) %{_sysconfdir}/rpm2html.tpl

%changelog
* Tue Nov  9 2010 Peter Hanecak <hany@hany.sk> 1.11.2-1
- configure.in rpm2html.h: release 1.11.1 (development release)
- NEWS rpm2html.spec.in: updated
- config.c, html.c, rpm2html.config.test, rpm2html-en.config, rpm2html.h,
  rpmopen.c: added global option 'dump_html_only_if_rpm_newer' (off by default) 

* Thu Sep 30 2010 Peter Hanecak <hany@hany.sk> 1.11.1-1
- configure.in rpm2html.h: release 1.11.0 (development release)
- NEWS rpm2html.spec.in: updated
- rpmopen.c: removed double open - should speed things up around 20%
- INSTALL: added "./autogen.sh" tip
- rpm2html.1: fixed typos - patch from Michal Čihař <nijel@debian.org>
- rpm2html.spec.in: run ./autogen.sh before ./configure to properly
  deal with various version of autotools we may encounter on various
  build machines
- rpmopen.c: reenabled support for package signature processing for
  RPM prior to 4.6

* Tue Apr 28 2009 Peter Hanecak <hany@hany.sk> 1.11.0-1
- config.h.in configure.in rpmdata.h rpmopen.c: preliminary support for
  compilation against RPM.org's rpm 4.6 using rpmlegacy.h

* Tue Apr 28 2009 Peter Hanecak <hany@hany.sk> 1.10.0-1
- configure.in rpm2html.h: release 1.10.0 (stable release)
- NEWS rpm2html.spec.in: updated

* Sat Nov 29 2008 Peter Hanecak <hany@hany.sk> 1.9.7-1
- html.c: do not use whole local directory name in page title
- rpmopen.c: protect even email addresses from PGP/GPG signatures
- rpmdata.c: protect more email addresses in changelog, not just those enclosed in <>
- Makefile.am: fixed usage of configure's --with-gpg parameter

* Wed Aug 29 2007 Peter Hanecak <hany@hany.sk> 1.9.6-1
- rdf.c: fixed error messages like "couldn't open <RDF base>/usr/share/man/man5/aliases.5.gz.rdf
  for writing !"
- rpm2html.spec.in: added BuildRequires: automake
- config.c: fixed error when compiling without libtemplate support

* Tue May 15 2007 Peter Hanecak <hany@hany.sk> 1.9.5-1
- ChangeLog: fixed typo
- html.c: fixed handling of "rdf_count_limit" option introduced
  in 1.9.2 - option has been so far ignored by mistake
- html.c: fixed setting of 'next_page' template variable for
  'dates_next_page' template item

* Sat Apr 14 2007 Peter Hanecak <hany@hany.sk> 1.9.4-1
- config.c, rpmopen.c: fixed some memory leaks
- html.c, rdf.c: little more verbose messages
- html.c, rpm2html.tpl: fixed segmentation fault occuring when
  generating pages describing local configuration with libtemplate
  enabled
- html.c: fixed minor typos
- rpm2html.spec.in: added missing BuildRequires

* Sun Apr  1 2007 Peter Hanecak <hany@hany.sk> 1.9.3-1
- build using Lazarus libtemplate
- accept '(' and ')' in resource names thus fixing the Requires list for 64-bit packages
- accept '(' and ')' in groups thus reduced the amount of "garbled group" warnings
- added hyperlinking to any CVE-xxxx-yyyy and CAN-xxxx-yyyy numbers occuring
  in the changelog
- little documentation clean-up and update
- %%{dist} added to release
- configs are now noreplace
- removed crude autogen.sh hack
- do not strip binary
- properly install translations

* Sun Sep 25 2005 Peter Hanecak <hanecak@megaloman.sk> 1.9.2-1
- option 'rdf_count_limit' has been added so users can customize the limit  
  of RDF export (default value was and still is 20)
- disabled length check for "garbled" warnings

* Thu Aug 18 2005 Peter Hanecak <hanecak@megaloman.sk> 1.9.1-1
- 'Mirrors' link can be configured in configuration file
  (see 'header=... Mirrors' line in example configuration files)
- home page URLs has been updated
- moderated "Invalid package <name> : garbled release" error message:
  release is considered garbled if it is longer than 65 characters
  (instead of 20)
- fixed coredumps caused by large changelogs

* Tue Apr  5 2005 Peter Hanecak <hanecak@megaloman.sk> 1.9.0-1
- added new feature: e-mail protection (mangle e-mail adresses in HTML
  output) - see 'protectemails' entry in manuel.html or example
  configuration files
- fixed changelog parsing
- fixed handling of empty filelists
- home page URL has been updated in manuel.html

* Mon Mar 14 2005 Peter Hanecak <hanecak@megaloman.sk> 1.8.3-1
- GIFs transformed to PNGs
- fixed bugs in configure.in affecting usage of 'configure --with-gpg'
- project maintenance taken over by me (Peter Hanecak)

* Fri Sep 26 2003 Daniel Veillard <veillard@redhat.com> 1.8.2-1
- made a new release based on the patches from Peter Hanecak

* Fri Sep 26 2003 Peter Hanecak <hanecak@megaloman.sk> 1.8.1-2
- synchronized with Daniel Veillard's <veillard@redhat.com> rpm2html-1.8.1-1
  package
- rpm2html-1.8.1-rpm42.patch based on rpm42 patch for apt
  by Gary Peck <gbpeck@sbcglobal.net>

* Mon Aug 11 2003 Peter Hanecak <hanecak@megaloman.sk> 1.7-8.1
- synchronized with RedHat's rpm2html-1.7-8 package

* Thu Nov  7 2002 Daniel Veillard <veillard@redhat.com>
- release of 1.8
- a bunch of cleanups w.r.t. spec, Makefiles, etc...
- make sure RDF files are indented

* Wed Aug 14 2002 Jeff Johnson <jbj@redhat.com> 1.7-8
- initial port to rpm-4.1.

* Tue Apr 30 2002 Peter Hanecak <hanecak@megaloman.sk>
- rebuild against rpm-4.0.4
- droped old unused patches

* Wed Feb  6 2002 Peter Hanecak <hanecak@megaloman.sk>
- update to 1.7
- merged some stuff from RedHat's rpm2html-1.7-4 package
- s/Copyright/License
- dropped rpm2html-1.5-sig-5.patch and rpm2html-dup.patch patches

* Wed Jan 02 2002 Florian La Roche <Florian.LaRoche@redhat.de>
- rebuild against new libs
- update config.guess and config.sub for IA64

* Tue Jul 24 2001 Daniel Veillard <veillard@redhat.com>
- upgraded to 1.7
- cleaned up a potential problem with libxml2
- fixed BuildPrereqs

* Wed Jul 18 2001 Daniel Veillard <veillard@redhat.com>
- upgraded to version 1.6
- removed the mysql support, this is too specific

* Tue Jul 10 2001 Elliot Lee <sopwith@redhat.com>
- Rebuild against rpm-4.0.3-0.65
- Use %%{?_smp_mflags}

* Fri Jun 29 2001 Jeff Johnson <jbj@redhat.com>
- rebuild against rpm-4.0.3-0.54.

* Wed May 23 2001 Jeff Johnson <jbj@redhat.com>
- rebuild against rpm-4.0.3-0.26.

* Tue Dec 26 2000 Jeff Johnson <jbj@redhat.com>
- build against rpm-4.0.2-0.9.

* Tue Dec 12 2000 Jeff Johnson <jbj@redhat.com>
- rebuild to remove 0777 directories.

* Mon Dec 11 2000 Peter Hanecak <hanecak@megaloman.sk>
- rpm2html-dup.patch by Alexey Nogin <ayn2@cornell.edu>

* Wed Nov  8 2000 Peter Hanecak <hanecak@megaloman.sk>
- rpm2html-1.5-sig-5.patch: summary of previous patches plus typo fixes

* Tue Nov  7 2000 Peter Hanecak <hanecak@megaloman.sk>
- rpm2html-1.5-sig-speedup.patch to avoid unnecessary signature resolving

* Sun Nov  5 2000 Peter Hanecak <hanecak@megaloman.sk>
- rpm2html-1.5-sig-4.patch: summary of rpm2html-1.4-sig.patch and
  rpm2html-1.5-sig-memleak.patch plus configurable "resolving" feature
  using gnupg

* Thu Nov  2 2000 Jeff Johnson <jbj@redhat.com>
- update to 1.5.
- enable mysql support by default.

* Wed Oct 25 2000 Peter Hanecak <hanecak@megaloman.sk>
- rpm2html-1.5-sig-memleak.patch by Daniel Veillard <Daniel.Veillard@w3.org>

* Tue Oct  3 2000 Peter Hanecak <hanecak@megaloman.sk>
- rpm2html-1.4-sig.patch - experimental enhancement to include package
  signature info in .html pages

* Mon Sep 25 2000 Peter Hanecak <hanecak@megaloman.sk>
- update to 1.5
- .bz2 source archive
- merged some stuff from Daniel Veillard's <veillard@w3.org> 1.5-1 spec

* Fri Aug  4 2000 Jeff Johnson <jbj@redhat.com>
- remove exec permissions on data files.

* Fri Jul 28 2000 Peter Hanecak <hanecak@megaloman.sk>
- update to 1.4
- spec improvements

* Thu Jul 13 2000 Prospector <bugzilla@redhat.com>
- automatic rebuild

* Sun Jun 25 2000 Peter Hanecak <hanecak@megaloman.sk>
- merged stuff from RedHat's rpm2html-1.2-8
- patches ported forward
- rpm-4.0 patch
- build against rpm-4.0-0.45

* Thu Jun 22 2000 Jeff Johnson <jbj@redhat.com>
- update to 1.4.

* Tue Jun 20 2000 Jeff Johnson <jbj@redhat.com>
- coagulate rpm310/rpm40 patch into one.
- better autoconf tests.

* Mon Jun 19 2000 Jeff Johnson <jbj@redhat.com>
- rebuild against rpm-4.0-0.40 (needs -ldb-3.1).

* Thu Jun  1 2000 Matt Wilson <msw@redhat.com>
- use _mandir macros
- build against new librpm to get correct libbz2 deps

* Sat May 13 2000 Jeff Johnson <jbj@redhat.com>
- use rpm-3.1 API (hack).

* Tue Feb 15 2000 Jeff Johnson <jbj@redhat.com>
- fix filename lossage with rpm-3.0.4.

* Mon Feb  7 2000 Jeff Johnson <jbj@redhat.com>
- compress man pages.

* Sat Oct 16 1999 Peter Hanecak <hanecak@megaloman.sk>
- updated to 1.3
- some spec stuff from Daniel Veillard's spec

* Sat Sep 25 1999 Jeff Johnson <jbj@redhat.com>
- recompile with rpm-3.0.3-1.

* Thu Aug 12 1999 Jeff Johnson <jbj@redhat.com>
- move to Red Hat 6.1 distribution.

* Wed Jul 21 1999 Tim Powers <timp@redhat.com>
- rebuilt for 6.1

* Fri Apr  9 1999 Jeff Johnson <jbj@redhat.com>
- update to 1.2.

* Fri Apr  2 1999 Jeff Johnson <jbj@redhat.com>
- repackage for Red Hat 6.0.
