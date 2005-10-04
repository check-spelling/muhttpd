PACKAGE = muhttpd
VERSION = 1.0.1

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

install : install-package install-bin install-etc install-log install-man

install-package : all
	[ -d "$(PACKAGEDIR)" ] || mkdir -p "$(PACKAGEDIR)"
	[ -d "$(PACKAGEDIR)/sbin" ] || mkdir -p "$(PACKAGEDIR)/sbin"
	[ -d "$(PACKAGEDIR)/etc" ] || mkdir -p "$(PACKAGEDIR)/etc"
	[ -d "$(PACKAGEDIR)/man/man5" ] || mkdir -p "$(PACKAGEDIR)/man/man5"
	[ -d "$(PACKAGEDIR)/man/man8" ] || mkdir -p "$(PACKAGEDIR)/man/man8"
	cp $(TARGETS) "$(PACKAGEDIR)/sbin"
	cp conf/* "$(PACKAGEDIR)/etc"
	cp muhttpd.conf.5 "$(PACKAGEDIR)/man/man5"
	cp muhttpd.8 "$(PACKAGEDIR)/man/man8"

install-bin :
	[ -d "$(SBINDIR)" ] || mkdir -p "$(SBINDIR)"
	if [ ! "$(PACKAGEDIR)/sbin" = "$(SBINDIR)" ]; \
	then \
		ln -s "$(TRUEPACKAGEDIR)/sbin/muhttpd" "$(SBINDIR)/"; \
	fi

install-etc :
	[ -d "$(SYSCONFDIR)" ] || mkdir -p "$(SYSCONFDIR)"
	if [ ! "$(PACKAGEDIR)/etc" = "$(SYSCONFDIR)" ]; \
	then \
		confdir="$(SYSCONFDIR)/muhttpd"; \
		[ -e "$${confdir}" ] || mkdir -p "$${confdir}"; \
		for i in "$(PACKAGEDIR)/etc/"*; \
		do \
			name=`basename "$$i"`; \
			if [ -e "$${confdir}/$$name" ]; \
			then \
				echo "*** $${confdir}/$$name exists, leaving untouched" >&2; \
				echo "*** To use the supplied $$name, manually replace $${confdir}/$$name by $(TRUEPACKAGEDIR)/etc/$$name" >&2; \
			else \
				cp "$(PACKAGEDIR)/etc/$$name" "$${confdir}/$$name"; \
			fi; \
		done; \
	fi

install-log :
	if [ ! -e "$(LOGDIR)/muhttpd/logfile" ]; \
	then \
		mkdir -p "$(LOGDIR)/muhttpd" && \
			touch "$(LOGDIR)/muhttpd/logfile"; \
	fi

install-man :
	if [ ! "$(PACKAGEDIR)/man" = "$(MANDIR)" ]; then \
 		[ -d "$(MANDIR)/man5" ] || mkdir -p "$(MANDIR)/man5"; \
		ln -sf "$(TRUEPACKAGEDIR)/man/man5/muhttpd.conf.5" \
			"$(MANDIR)/man5/"; \
 		[ -d "$(MANDIR)/man8" ] || mkdir -p "$(MANDIR)/man8"; \
		ln -sf "$(TRUEPACKAGEDIR)/man/man8/muhttpd.8" \
			"$(MANDIR)/man8/"; \
  	fi
	
love :
	#unzip; strip; touch; finger; mount; fsck; more; yes; umount; sleep

.PHONY : all clean distclean install install-package install-bin install-etc \
	install-man love
