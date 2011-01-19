#
# A makefile with a complete set of defaults for ssmtp
#
# Things ssmtp needs to know as -D options:
#	MAILHUB, the machine to send all mail via. Default
#		is ``mailhub.yorku.ca''.
#	REWRITE_DOMAIN, the domain to rewrite mail as coming
#		from.  Only rewrites ``From:'' line if this
#		variable is set, and nothing whatsoever if unset.
#		NOT required if using zmailer or a sane sendmail
#		on the mailhub. Defaults to off.
#	LOGGING, the mechanism use to log problems and successes:
#		SYSLOG, logging is to be done to syslog(2).
#		LOGFILE, if a file gets it too.
#		Default is no logging, which is not smart.
#	LACKING, the flags used to select optional replacement code:
#		NOSTRDUP, NOSTRSTR, NOSTRNCASECMP, NOSTRTOK for strings,
#		OLDSYSLOG, to indicate obsolete syslog/openlog function.
#		Defaults to empty.
#	
#MAILHUB=ccs.yorku.ca
MAILHUB=mailhost
LOGGING=-DSYSLOG # I recommend this strongly.
#
#LACKING=-DNOSTRDUP -DNOSTRSTR -DNOSTRNCASECMP # For SunOS 3.5, Sun 3, 3x
#LACKING= -DOLDSYSLOG -DNOSTRDUP -DNOSTRSTR -DNOHERRNO # For Ultrix 3.1D
#LACKING= -DOLDSYSLOG -DNOSTRDUP # For Ultrix 4.2, 2100s and 5000/25s
LACKING= # For SunOS 4.1.1, SPARCs and 3/60s
#LACKING=-DNOSTRDUP  # For NeXTs
#LACKING= -DNOSTRDUP -DNOSTRSTR # For Mips machines (now owned by SGI)
#
#BASEFLAGS=-DMAILHUB=\"${MAILHUB}\" ${LOGGING} ${LACKING} \
#	-DREWRITE_DOMAIN=\"${REWRITE_DOMAIN}\" 
#	Do not define the above unless rewriting is necessary.
BASEFLAGS=-DMAILHUB=\"${MAILHUB}\" ${LOGGING} ${LACKING}
# (End of tuning section).
#
# Places to install things, used to relocate things for Ultrix kits.
# 	ROOT normally is the empty string...
ROOT=/team/project/ssmtp/S4C-SUNos4.1.1/input
#ROOT=
DESTDIR=${ROOT}/usr/lib
MANDIR=${ROOT}/usr/local/man/man8
ETCDIR=${ROOT}/usr/local/etc
# (End of relocation section)
#

SRCS= ssmtp.c string_ext.c
OBJS= ssmtp.o string_ext.o
HFILES=string_ext.h

LFLAGS= ${BASEFLAGS} -DNOSTRINGS # Turn on everything for lint to check
CFLAGS= -g ${BASEFLAGS}
TFLAGS= -a ${BASEFLAGS} 

all: ssmtp

install: ssmtp
	test `whoami` = root
	@echo "WARNING: this kit replaces ${DESTDIR}/sendmail."
	@echo "type ^C within 30 seconds to stop it..."
	sleep 30
	if test ! -f ${DESTDIR}/sendmail.stock; then \
		mv ${DESTDIR}/sendmail ${DESTDIR}/sendmail.stock; \
	else \
		mv ${DESTDIR}/sendmail ${DESTDIR}/$$; \
		rm ${DESTDIR}/$$; \
	fi
	install -s -m 755 ssmtp ${DESTDIR}/sendmail
	ln ${DESTDIR}/sendmail ${DESTDIR}/ssmtp
	cp ssmtp.8 ${MANDIR}/ssmtp.8
	cp mail.conf ${ETCDIR}/mail.conf
deinstall:
	test `whoami` = root
	rm -f ${DESTDIR}/ssmtp
	if test -f ${DESTDIR}/ssmtp; then \
		mv ${DESTDIR}/sendmail.stock ${DESTDIR}/sendmail; \
	else \
		echo "WARNING: no stock sendmail to replace, you have no sendmail at all."; \
		rm -f ${DESTDIR}/sendmail; \
	fi
	rm -f ${MANDIR}/ssmtp.8
	rm -f ${ETCDIR}/mail.conf

clean:
	rm -f ssmtp *.o core a.out *~ *.bak *.dvi *.ps *.BAK #* *.tcov

lint:
	lint ${LFLAGS} ${SRCS} 

tcov: clean 
	${CC} -c -o ssmtp.o ${TFLAGS} ssmtp.c
	${CC} -c -o string_ext.o  ${TFLAGS} string_ext.c
	${CC} -o ssmtp ${TFLAGS} ${OBJS}
	test_ssmtp
	@echo "Have a look in ssmtp.tcov for missed statements."
	
shar:
	makekit -nssmtp.shar. -iMANIFEST -oMANIFEST -h2
	mv MANIFEST.BAK MANIFEST

ssmtp_plm: ssmtp_plm.tex
	l2a <ssmtp_plm.tex | sed -e 's/@(label)@ reliable//' \
		-e 's/@(begin)@ description//' -e 's/@(begin)@ verbatim//' \
		-e 's/@(end)@ description//' -e 's/@(end)@ verbatim//' \
		-e 's/@(newpage)@//' >ssmtp_plm

# Binaries:
ssmtp: ${OBJS} ${HFILES}
	${CC} -o ssmtp ${CFLAGS} ${OBJS}

#
# $Header: /team/davecb/Tools/ssmtp/RCS/Makefile,v 1.22 1993/02/23 13:01:02 davecb Exp $
#
#
# $Log: Makefile,v $
# Revision 1.22  1993/02/23  13:01:02  davecb
# adding ref t oREWRITE_DOMAIN
#
# Revision 1.21  1993/02/10  03:16:28  davecb
# adding parme to makekit
#
# Revision 1.20  1993/02/10  02:56:37  davecb
# adding next hacks
#
# Revision 1.19  1992/11/16  16:43:56  davecb
# Adding more cleanup
#
# Revision 1.18  1992/11/16  03:48:40  davecb
# adding LACKING stuff
#
# Revision 1.17  1992/11/16  03:43:18  davecb
# ading makekit stuff
#
# Revision 1.16  1992/11/16  01:48:23  davecb
# adding config file
#
# Revision 1.15  1992/11/15  16:41:22  davecb
# typo
#
# Revision 1.14  1992/11/15  16:38:12  davecb
# improving messages
#
# Revision 1.13  1992/11/15  02:43:09  davecb
# undoing berklyisms
#
# Revision 1.12  1992/08/18  12:17:43  davecb
# adding NeXT
#
# Revision 1.11  1992/08/10  12:57:51  davecb
# Changing comments, mailhub declaration.
#
# Revision 1.10  1992/07/16  15:30:31  davecb
# cahnging mailhub to just ``ccs.yorku.ca''
#
# Revision 1.9  1992/06/29  19:21:42  davecb
# *** empty log message ***
#
# Revision 1.5  1992/06/17  13:37:39  davecb
# test of macros
#
# Revision 1.4  1992/06/10  13:25:35  davecb
# *** empty log message ***
#
# Revision 1.3  1992/06/10  13:24:21  davecb
# making it ultrix
#
# Revision 1.2  1992/06/10  13:11:52  davecb
# *** empty log message ***
#
# Revision 1.1  1992/06/09  16:11:15  davecb
# Initial revision
#
#

# I have a broken (IBM) printer...
kludge:
	for i in README MANIFEST INSTALL ssmtp.8 Makefile \
	mail.conf patchlevel.h ssmtp.c ssmtp.h string_ext.c \
	string_ext.h test.mail test_ssmtp; do \
		enscript -G $$i; \
		echo $$i; \
		sleep 300; \
	done
	latex ssmtp_plm && dvi2ps ssmtp_plm >ssmtp_plm.ps
	lpr ssmtp_plm.ps
