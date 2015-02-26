#
#  You DO NOT need to modify this file to suit your system.
#
SHELL		= /bin/sh

# ---------------------------------------------------------------------

it:
	@if uname -s -r | grep -q "Linux" ; then \
		make -f Makefile.linux ; fi
	@if uname -s -r | grep -q "Darwin" ; then \
		make -f Makefile.macosx ; fi
	@if uname -s -r | grep -q "SunOS 4" ; then \
		make -f Makefile.sunos ; fi
	@if uname -s -r | grep -q "SunOS 5" ; then \
		make -f Makefile.solaris ; fi
	@if uname -s -r | grep -q "OSF1" ; then \
		make -f Makefile.osf1 ; fi
	@if uname -s -r | grep -q "IRIX" ; then \
		make -f Makefile.irix5 ; fi
	@if uname -s -r | grep -q "FreeBSD" ; then \
		make -f Makefile.freebsd ; fi
	@if uname -s -r | grep -q "NetBSD" ; then \
		make -f Makefile.netbsd ; fi
	@cp -p cnetsupport.h ../lib

# ---------------------------------------------------------------------

clean:
	@rm -f *.a *.o core *.cnet f?

