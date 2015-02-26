#
#  You DO NOT need to modify this file to suit your system.
#
NAME		= cnet
SHELL		= /bin/sh

# ---------------------------------------------------------------------

it:
	@if uname -s -r | grep -q "Linux" ; then \
		make -f Makefile.linux $(NAME) ; fi
	@if uname -s -r | grep -q "Darwin" ; then \
		make -f Makefile.macosx $(NAME) ; fi
	@if uname -s -r | grep -q "SunOS 4" ; then \
		make -f Makefile.sunos $(NAME) ; fi
	@if uname -s -r | grep -q "SunOS 5" ; then \
		make -f Makefile.solaris $(NAME) ; fi
	@if uname -s -r | grep -q "OSF1" ; then \
		make -f Makefile.osf1 $(NAME) ; fi
	@if uname -s -r | grep -q "IRIX" ; then \
		make -f Makefile.irix5 $(NAME) ; fi
	@if uname -s -r | grep -q "FreeBSD" ; then \
		make -f Makefile.freebsd $(NAME) ; fi
	@if uname -s -r | grep -q "NetBSD" ; then \
		make -f Makefile.netbsd $(NAME) ; fi
	@cp -p $(NAME) ../bin
	@cp -p cnet.h  ../lib

# ---------------------------------------------------------------------

clean:
	@rm -f $(NAME) *.o core *.cnet f?
	@rm -rf HTML GPATH GRTAGS GSYMS GTAGS

hide:
	make clean
	echo '#if	HIDE_GLOBAL_NAMES'		 > hidenames.h
	sort < HIDENAMES | \
	awk '{ if($$0 !~ /#/) printf("#define\t%-36s\t__CN%03d\n", $$0,++n);}'\
							>> hidenames.h
	echo '#endif'					>> hidenames.h

