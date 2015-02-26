# Modify this directory to suit your system
WWWDIR	= $(HOME)/WWW/cnet

AND	= CLICK KEYBOARD STOPANDWAIT TICKTOCK

it:
	@./INSTALL
	@mkdir -p $(WWWDIR) $(WWWDIR)/images
	@chmod 711 $(WWWDIR) $(WWWDIR)/images
	cp -pR cnet.css [a-z]*.html $(AND) $(WWWDIR)
	cp -pR images/*.* $(WWWDIR)/images

