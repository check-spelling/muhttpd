PACKAGE = muhttpd
VERSION = 0.9.9

include Makefile.cfg

TARGETS = muhttpd

all : $(TARGETS)

muhttpd : $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@ $(LIBS)

%.o : %.c
	$(CC) $(CFLAGS) -c $<

clean :
	-rm $(OBJECTS)

distclean : clean
	-rm $(TARGETS)

install : all
	[ -d $(PACKAGEDIR) ] || mkdir -p $(PACKAGEDIR)
	[ -d $(PACKAGEDIR)/sbin ] || mkdir -p $(PACKAGEDIR)/sbin
	[ -d $(PACKAGEDIR)/etc ] || mkdir -p $(PACKAGEDIR)/etc
	[ -d $(SBINDIR) ] || mkdir -p $(SBINDIR)
	[ -d $(SYSCONFDIR) ] || mkdir -p $(SYSCONFDIR)
	cp $(TARGETS) $(PACKAGEDIR)/sbin
	cp conf/* $(PACKAGEDIR)/etc
	if [ ! $(PACKAGEDIR)/sbin = $(SBINDIR) ]; \
	then \
		for i in $(PACKAGEDIR)/sbin/*; \
		do \
			ln -s "$$i" $(SBINDIR)/`basename "$$i"`; \
		done; \
	fi
	if [ ! $(PACKAGEDIR)/etc = $(SYSCONFDIR) ]; \
	then \
		for i in $(PACKAGEDIR)/etc/*; \
		do \
			ln -s "$$i" $(SYSCONFDIR)/`basename "$$i"`; \
		done; \
	fi
	
.PHONY : all clean distclean install
