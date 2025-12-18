AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = b0rken m4 icons

bin_PROGRAMS = wmudmount

BUILT_SOURCES = chars.c

wmudmount_SOURCES = asklabel.c asklabel.h askpass.c askpass.h chars.h chars-mask.xpm conf.c conf.h die.c die.h dock.c dock.h fsm.c fsm.h make-chars.pl menu.c menu.h misc.h notify.c notify.h wmudisks.c wmudisks.h wmudmount.c button_images.xpm
nodist_wmudmount_SOURCES = chars.c
wmudmount_CFLAGS = @X_CFLAGS@
wmudmount_LDADD = b0rken/libb0rken.a @X_LIBS@

man_MANS = wmudmount.1
EXTRA_DIST = bootstrap $(man_MANS)
EXTRA_DIST += debian/changelog debian/compat debian/control debian/copyright debian/menu debian/README.Debian debian/rules debian/source/format debian/source/options debian/watch


chars.c: chars-mask.xpm make-chars.pl
	./make-chars.pl

reallyclean: distclean
	-rm -rf autom4te.cache
	find . -name Makefile.in -exec rm -f {} +
	-rm aclocal.m4 config.h.in* config.guess config.sub configure depcomp install-sh ltmain.sh missing mkinstalldirs stamp-h.in chars.c 
