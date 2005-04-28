PACKAGE = muhttpd
VERSION = 0.10.0

include Makefile.cfg

TARGETS = muhttpd

all : $(TARGETS)

muhttpd : $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@ $(LIBS)

%.o : %.c flags.h
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
			name=`basename "$$i"`; \
			ln -s "$(TRUEPACKAGEDIR)/sbin/$$name" "$(SBINDIR)/$$name"; \
		done; \
	fi
	if [ ! $(PACKAGEDIR)/etc = $(SYSCONFDIR) ]; \
	then \
		for i in $(PACKAGEDIR)/etc/*; \
		do \
			name=`basename "$$i"`; \
			ln -s "$(TRUEPACKAGEDIR)/etc/$$name" "$(SYSCONFDIR)/$$name"; \
		done; \
	fi
	
.PHONY : all clean distclean install
