NAME		= cnet
VERSION		= 3.3.1
SHELL		= /bin/sh
#
# ---------------------------------------------------------------------
# Comment/uncomment the following 4 constants to suit your system:
#
# PREFIX defines the directory below which cnet will be installed.
PREFIX		= /usr/local
#PREFIX		= $(HOME)
#
# BINDIR defines the directory where the cnet binary will be installed.
BINDIR		= $(PREFIX)/bin
#
# LIBDIR defines the directory where cnet's support files will be installed.
LIBDIR		= $(PREFIX)/lib/cnet
#
# WWWDIR defines the directory to hold cnet's web-based documentation
WWWDIR		= /home/httpd/html/cnet
#WWWDIR		= $(HOME)/WWW/cnet
#
# ---------------------------------------------------------------------
#
it:
	make -C src
	make -C support
	@ls -l bin/$(NAME) lib/*.a

install:
	make
	@mkdir -p  $(BINDIR) $(LIBDIR)
	@chmod 755 $(BINDIR) $(LIBDIR)
	@cp bin/$(NAME) $(BINDIR)/$(NAME)
	@chmod 755 $(BINDIR)/$(NAME)
	@rm -rf $(LIBDIR)/*
	@cp lib/* $(LIBDIR)
	@chmod 644 $(LIBDIR)/*
	@ls -l $(BINDIR)/$(NAME) $(LIBDIR)/*.a

AND	= html/CLICK html/KEYBOARD html/STOPANDWAIT html/TICKTOCK
www:
	@cd html && ./INSTALL
	@mkdir -p $(WWWDIR) $(WWWDIR)/images
	@chmod 711 $(WWWDIR) $(WWWDIR)/images
	cp -pR html/cnet.css html/[a-z]*.html $(AND) $(WWWDIR)
	cp -pR html/images/*.* $(WWWDIR)/images

# ---------------------------------------------------------------------

clean:
	make -C src clean
	make -C support clean
	make -C examples clean
	rm -rf bin/$(NAME) lib/*.a f? *.o *.cnet             

uninstall:
	rm -rf $(BINDIR)/$(NAME) $(LIBDIR) $(WWWDIR)

