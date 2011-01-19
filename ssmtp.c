
/*
 * sSMTP sendmail -- send messages via smtp to a mailhub for local delivery 
 *	or forwarding. This program is used in place of /usr/lib/sendmail,
 *      called by /bin/mail (et all).   sSMTP does a selected subset of 
 *      sendmail's standard tasks (including exactly one rewriting task), and
 *      explains if you ask it to do something it can't.  It then sends
 *      the mail to the mailhub via an smtp connection.  Believe it or not,
 *      this is nothing but a filter.  You can receive mail with inetd, an
 *	inverse filter and /bin/mail -d.
 */
#include <stdio.h>
#include <limits.h>
#include <pwd.h>       /* For getpwent. */
#include <sys/types.h> /* For sockets. */
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#ifdef SYSLOG
#include <syslog.h>    /* For logging. */
#else
#define LOG_ERR 0
#define LOG_INFO 0
#endif
#include <signal.h>    /* For the timer and signals. */
#include <setjmp.h>
#include <string.h>

#include "string_ext.h"   /* Local additions. */
#include "ssmtp.h"
#include "patchlevel.h"

#ifndef MAILHUB
#define MAILHUB "mailhost" /* A surprisingly usefull default. */
#endif
#define PORTNUMBER 25 /* From the Assigned Numbers RFC. */
#define DATELEN 30    /* Length of date/time string. */

char	*Version = VERSION;	/* The version of the program. */
char	*ProgName = NULL;       /* It's name. */
char	*MailHub = MAILHUB;	/* The place to send the mail. */
char	HostName[MAXLINE];	/* Our name, unless overridden. */
char	DateString[DATELEN];	/* Current date in RFC format. */
#ifdef REWRITE_DOMAIN
char *RewriteDomain = REWRITE_DOMAIN; /* Place to claim to be. */
#endif
char	*Root = "postmaster";	/* Person to send root's mail to. */
struct passwd *Sender = NULL;   /* The person sending the mail. */
jmp_buf TimeoutJmpBuf;		/* Timeout waiting for input from network. */
int     Verbose = NO;		/* Tell the user what's happening. */
int	LogLevel =		/* Tell the log what's happening. */
#ifdef DEBUG
		1;
#else
		0;
#endif

char	*fromLine(),
	*properRecipient();

/*
 * main -- make the program behave like sendmail, then call ssmtp.
 */
main(argc,argv) int argc; char *argv[]; {
	char **newArgv, **doOptions();

	/* Try to be bulletproof (:-)) */
	(void) signal(SIGHUP,SIG_IGN);
	(void) signal(SIGINT,SIG_IGN);
	(void) signal(SIGTTIN,SIG_IGN);
	(void) signal(SIGTTOU,SIG_IGN);

	/* Set the globals. */
	ProgName = argv[0];
	if (gethostname(HostName,sizeof(HostName)) == ERR) {
		die("can't find the name of this host, %s, exiting.",
			"(an impossible condition)");
	}
	if ((Sender= getpwuid(getuid())) == NULL) {
		die("couldn't find password entry for sender (uid %d).",
			getuid());
	}
	(void) getDate(DateString);

	newArgv = doOptions(argc,argv); 
	exit(ssmtp(newArgv));
	/*NOTREACHED*/
}

/*
 * doOptions -- pull the options out of the command-line, process them 
 *      (and special-case calls to mailq, etc), and return the rest.
 */
 char **
doOptions(argc,argv) int argc; char *argv[]; {
	int    i, 
		newArgC;
	static char *newArgV[MAXARGS];

	newArgV[0] = argv[0];
	newArgC = 1;

	if (strstr(argv[0],"mailq") != NULL) {
		/* Someone wants to know the queue state... */
		(void) printf("Mail queue is empty.\n");
		exit(0);
		}
	else if (strstr(argv[0],"newalias") != NULL) {
		/* Someone wanted to recompile aliases. */
		/* This is slightly more like to be a human... */
		die("newalias is meaningless to sSMTP: it doesn't do aliases.");
	}
	
	for (i=1; i < argc; i++) {
		if (argv[i][0] != '-') {
			newArgV[newArgC++] = argv[i];
			continue;
		}
		switch(argv[i][1]) {
		case 'b':
			switch (argv[i][2]) {
			case 'a': /* ARPANET mode. */
				die("-ba is not supported by sSMTP sendmail, nor is -t.");
			case 'd': /* Run as a daemon. */
				die("-bd is not supported by sSMTP sendmail. Use rSMTP under inetd instead.");
			case 'i': /* Initialize aliases. */	      continue;
			case 'm': /*  Default addr processing. */     continue;
			case 'p': /* Print mailqueue. */
				die("Mail queue is empty.");
			case 's': /* Read smtp from stdin. */
				die("-bs is not supported by sSMTP sendmail."); 
			case 't': /* Test mode. */
				die("-bt is meaningless to sSMTP sendmail. It doesn't route.");
			case 'v': /*  Verify names only. */
				die("-bv is meaningless to sSMTP sendmail. It doesn't route.");
			case 'z': /* Create  freeze file. */
				die("-bz is meaningless to sSMTP sendmail. It isn't programmable.");
			}
		case 'C': /* Configfile name. */		      continue;
		case 'd': /* Debug. */
			LogLevel = 1;
			Verbose = YES; /* Almost the same thing... */
			break;
		case 'E': /* insecure channel, don't trust userid. */ continue;
		case 'F': /* fullname of sender. */		      continue;
		case 'f': /* Set from/sender address. */	      continue;
				/* Should I support these??? When? */
		case 'h': /* Set hopcount. */			      continue;
		case 'm': /* Ignore originator in adress list. */     continue;
		case 'M': /* USe specified message-id. */	      continue;
		case 'n': /* No aliasing. */			      continue;
		case 'o':
			switch (argv[i][2]) {
			case 'A': /* Alternate aliases file. */	      continue;
			case 'c': /* Delay connections. */	      continue;
			case 'D': /* Run newaliases if rqd. */	      continue;
			case 'd': /* Deliver now, in background or queue. */
				/* This may warrant a diagnostic for b or q. */
				continue;
			case 'e': /* Errors: mail, write or none. */  continue;
			case 'F': /* Set tempfile mode. */	      continue;
			case 'f': /* Save ``From ' lines. */	      continue;
			case 'g': /* Set group id. */		      continue;
			case 'H': /* Helpfile name. */		      continue;
			case 'i': /* DATA ends at EOF, not \n.\n */   continue;
			case 'L': /* Log level. */		      continue;
			case 'm': /* Send to me if in the list. */    continue;
			case 'o': /* Old headers, spaces between adresses. */
				die("-oo (old header format) is not supported by sSMTP sendmail.");
			case 'Q': /* Queue dir. */		      continue;
			case 'r': /* Read timeout. */		      continue;
			case 's': /* Always init the queue. */	      continue;
			case 'S': /* Stats file. */		      continue;
			case 'T': /* Queue timeout. */		      continue;
			case 't': /* Set timezone. */		      continue;
			case 'u': /* Set uid. */		      continue;
			case 'v': /* Set verbose flag. */
				Verbose = YES;
				continue;
			}
			break;
		case 'q': /* Process the queue [at time] */
			die("Mail queue is empty.");
		case 'R': /* Process queue for recipient. */	      continue;
		case 'r': /* Obsolete -f flag. */		      continue;
		case 't': /* Read message's To/Cc/Bcc lines. */
			die("-t is NOT supported by sSMTP sendmail.");
		case 'v': /* Verbose (ditto -ov). */
			Verbose = YES;
			break;
		case 'V': /*  Say version and quit. */
			die("sSMTP version %s (not sendmail at all)",
				Version);
			break;
		}
	}
	newArgV[newArgC] = NULL;
	if (newArgC <= 1) {
		die("no recipients supplied: no mail will be sent.");
	}
	return &newArgV[0];
}

#ifdef REWRITE_DOMAIN 
int fixFromLines();
#else
#define fixFromLines(buffer) NO
#endif

/*
 * ssmtp -- send the message (exactly one) from stdin to the smtp
 *	port on the mailhub.
 */
ssmtp(argv) char *argv[];  {
	extern int alarmHandler();
	char buffer[MAXLINE];
	int fd, i;

	if (getConfig() == NO) {
		log_event(LOG_INFO,"No mail.conf in /etc or /usr/local/etc.");
	}

	(void) signal(SIGALRM,(void (*)())alarmHandler); /* Catch SIGALRMs. */ 
	(void) alarm((unsigned)MAXWAIT); /* Set initial timer. */
	if (setjmp(TimeoutJmpBuf) != 0) {
		/* Then the timer has gone off and we bail out. */
		die("connection lost in middle of processing, exiting."); 
	}
	if ((fd=openSocket(MailHub,PORTNUMBER)) == ERR) {
		die("can't open the smtp port (%d) on %s.",
			PORTNUMBER,MailHub);
	}
	else if (getOkFromSmtp(fd,buffer) == NO) {
    		die("didn't get initial OK message from smtp server.");
	}
	if (Verbose) {
		(void) fprintf(stderr,"Connected to smtp server %s\n",MailHub);
	}
	if (LogLevel > 0) {
		log_event(LOG_INFO,"Connected to smtp server %s\n",MailHub);
	}

	/* Send "HELO", hostname. */
	putToSmtp(fd,"HELO %s",HostName);
	(void) alarm((unsigned)MEDWAIT); 
	if (getOkFromSmtp(fd,buffer) == NO) {
    		die("server didn't accept hostname %s, replied \"%s\".",
			HostName,buffer);
	}

	/* Send "MAIL From:", Sender->pw_name. */
	putToSmtp(fd,"MAIL From:<%s@%s>",Sender->pw_name,HostName);
	(void) alarm((unsigned)MEDWAIT); 
	if (getOkFromSmtp(fd,buffer) == NO) {
    		die("smtp server didn't accept MAIL From, replied \"%s\".",
			buffer);
	}
	if (Verbose) {
		(void) fprintf(stderr,"Server accepted From: %s@%s line.\n",
			  Sender->pw_name,HostName);
	}
	if (LogLevel > 0) {
		log_event(LOG_INFO,"Server accepted From: %s@%s line.\n",
			  Sender->pw_name,HostName);
	}

	/* Send all the To: adresses. */
	for (i=1; argv[i] != NULL; i++) {
		putToSmtp(fd,"RCPT To:<%s>",properRecipient(argv[i]));
		(void) alarm((unsigned)MEDWAIT); 
		if (getOkFromSmtp(fd,buffer) == NO) {
			die("smtp server didn't accept RCPT To: command, replied \"%s\".",
				buffer);
		}
	}
	if (Verbose) {
		(void) fprintf(stderr,"Server accepted To: line(s).\n");
	}
	if (LogLevel > 0) {
		log_event(LOG_INFO,"Server accepted To: line(s).\n");
	}

	/* Send DATA. */
	putToSmtp(fd,"DATA");
	(void) alarm((unsigned)MEDWAIT); 
	if (getFromSmtp(fd,buffer) != 3) {
		/* Oops, we were expecting "354 send your data". */
    		die("smtp server didn't accept DATA, replied \"%s\".",
			buffer);
	}
	if (Verbose) {
		(void) fprintf(stderr,"Message body transmission started.\n");
	}

	/* Send headers, with optional From: rewriting. */
	addInitialHeaders(fd);
	while (fgets(buffer,sizeof buffer,stdin) != NULL) {
		/* Trim off \n, double leading .'s */
		standardize(buffer);
		recordRequiredHeaders(buffer);
	        fixFromLines(buffer);
		putToSmtp(fd,"%s",buffer);
		(void) alarm((unsigned)MEDWAIT); 
		if (*buffer == '\0') {
			break;
		}
	}

	/* End of headers, add any reamining and start body. */
	addRequiredHeaders(fd);	
	putToSmtp(""); /* Seperate headers and body. */
	while (fgets(buffer,sizeof buffer,stdin) != NULL) {
		/* Trim off \n, double leading .'s */
		standardize(buffer);
		putToSmtp(fd,"%s",buffer);
		(void) alarm((unsigned)MEDWAIT); 
	}
	/* End of body. */
	putToSmtp(fd,".");

	(void) alarm((unsigned)MAXWAIT); 
	if (getOkFromSmtp(fd,buffer) == NO) {
		die("smtp server wouldn't accept message, replied \"%s\".",
			buffer);
	}
	if (Verbose) {
		(void) fprintf(stderr,"Message body transmission complete.\n");
	}
	/* Close conection. */
	(void) signal(SIGALRM,SIG_IGN); 
		putToSmtp(fd,"quit");
	(void) getOkFromSmtp(fd,buffer);
	(void) close(fd);
	(void) log_event(LOG_INFO,"%s sent mail for %s",
		ProgName,Sender->pw_name);
	return 0;
}


/*
** Supporting libraries -- i/o.
*/

/*
 * openSocket -- open a socket on a specified machine.
 *      Adapted from code by Blair P. Houghton:
 *      Copyright 1991 Blair P. Houghton, All Rights Reserved, 
 *      copying and distribution permitted with copyright intact.
 */
 int
openSocket(hostName,portNumber) char *hostName; int portNumber; {
	int fd;		/* socket to "plug" into the socket */
	struct sockaddr_in socketname;	/* mode, addr, and port data for */
						/* the socket */
	struct hostent *remote_host;	/* internet numbers, names */

	if ( (fd=socket(AF_INET,SOCK_STREAM,0)) < 0 ) {
		log_event(LOG_ERR,"unable to create a socket.\n");
		return ERR;
	}

	/* plug it into the listening socket */
	socketname.sin_family = AF_INET;
	if ((remote_host=gethostbyname(hostName)) == (struct hostent *)NULL) {
		log_event(LOG_ERR,"unable to locate host %s.\n",hostName);
		return ERR;
	}
	(void) bcopy((char *)remote_host->h_addr,(char *)&socketname.sin_addr,
		remote_host->h_length);
	socketname.sin_port = htons(portNumber);

	if (connect(fd,(struct sockaddr *)&socketname,sizeof socketname) < 0) {
		log_event(LOG_ERR,"unable to connect to \"%s\" port %d.\n",
			hostName,portNumber);
		return ERR;
	}
	return fd;
}

/*
 * getOkFromSmtp -- get a line and test the three-number string
 *      at the beginning.  If it starts with a 2, it's OK.
 */
getOkFromSmtp(fd,response) int fd; char *response; { 

	return (getFromSmtp(fd,response) == 2)? YES: NO;
}


/*
 * getFromSmtp -- get a line and return the initial digit.  Deal with
 *	continuation lines by reading to the last (non-continuation) line.
 */
 int
getFromSmtp(fd,response) int fd; char *response; { 
	char *getLine();

	do {
		if (getLine(response,MAXLINE,fd) == NULL) {
			*response = '\0';
			return NO;
		}
	} while (response[3] == '-');
	if (LogLevel > 0) {
		log_event(LOG_INFO,"Received \"%s\" from smtp port.\n",response);
	}			
	return atoi(response) / 100;
}
	

/*
 * putToSmtp -- a printf to an fd, which appends TCP/IP <CR/LF>.
 */
/*VARARGS*/
putToSmtp(fd,format,p1,p2,p3,p4,p5) int fd; char *format,*p1,*p2,*p3,*p4,*p5; {
	char line[MAXLINE];
/*	extern char *strcat(); */

	(void) sprintf(line,format,p1,p2,p3,p4,p5);
	if (LogLevel > 0) {
		log_event(LOG_INFO,"Sent \"%s\" to smtp port.\n",line);
	}
	(void) strcat(line,"\r\n");
	(void) write(fd,line,strlen(line));
}



/*
 * getLine -- get a line of text from a fd instead of an fp.
 */
 char *
getLine(line,size,fd) char *line; int size, fd; {
	int     i;
	char    ch;

	for (i=0; read(fd,&ch,1) == 1;) {
		if (i == size-1) {
			/* Truncate like fgets. */
			line[i] = '\0';
			return line;
		}
		else if (ch == '\r')
			; /* Strip it. */
		else if (ch == '\n') {
			break;
		}
		else {
			line[i++] = ch;
#ifdef DEBUG
			line [i] = '\0';
#endif
		}
	}
	line[i] = '\0';
	return line;
}

/*
** Supporting libraries -- header insertion.
*/
static int hasFrom = NO,
	hasTo = NO,
	hasDate = NO;
/*
 * recordRequiredHeaders -- note which ones we've seen.
 */
recordRequiredHeaders(line) char *line; {
	char 	*strncasecmp();

	if (*line == ' ' || *line == '\t') {
		return;
	}
	else if (strncasecmp(line,"From:",5)==0) {
		hasFrom = YES;
	}
	else if (strncasecmp(line,"To:",3)==0) {
		hasTo = YES;
	}
	else if (strncasecmp(line,"Date:",5)==0) {
		hasDate = YES;
	}
}

/*
 * addRequiredHeaders -- add ones that have been missed.
 */
addRequiredHeaders(fd) int fd; {
	char	*fromLine();

	if (hasFrom == NO) {
		putToSmtp(fd,fromLine());
	}
	if (hasTo == NO) {
		/* Can't happen, therefor... */
		putToSmtp(fd,"To: postmaster");
	}
	if (hasDate == NO) {
		putToSmtp("Date: %s",DateString);
	}
}

/*
 * addInitialHeaders -- prepend prerequisite timstamp
 *	and actual date lines.
 */
addInitialHeaders(fd) int fd;{ 

	putToSmtp(fd,"Received: by %s (sSMTP sendmail emulation); %s",
		  HostName,DateString);

}

#ifdef REWRITE_DOMAIN
/*
 * fixFromLine -- replace whole From: header with standardized pattern.
 *      Evil, nasty, immoral header-rewriting code (:-)).
 */
 int 
fixFromLines(line) char *line; {
        static int inHeaders = YES;

	if (strncasecmp(line,"From:",5) == 0) {
		(void) strcpy(line,fromLine());
	}
	if (*line == (char) NULL) {
	       inHeaders = NO;
	}
	return inHeaders;
}
#endif

/*
 * properRecipient -- alias systems-level users to the person who
 *	reads their mail.  This is variously the owner of a workstation,
 *	the sysadmin of a group of stations and the postmaster otherwise.
 *	We don't just mail stuff off to root on the mailhub (:-)).
 */
 char *
properRecipient(s) char *s; {
	struct passwd *p;

	if (strchr(s,'@') == NULL 
	   || (p= getpwnam(s)) == NULL 
	   || p->pw_uid > 10) {
		/* It's not a local systems-level user. */
		return s;
	}
	else {
		return Root;
	}
	/*NOTREACHED*/
}

/*
 * fromLine -- generate a from line is standard format. Used whenever
 *	we need one in standard format, "From: \"Real Name\" <id@site>" 
 */
 char *
fromLine() {
        static char buffer[MAXLINE];
	(void) sprintf(buffer,"From: \"%s\" <%s@%s>",
		strtok(Sender->pw_gecos,",;"),
		Sender->pw_name,
#ifdef REWRITE_DOMAIN
			RewriteDomain);
#else
			HostName);
#endif
	return &buffer[0];
}


/*
** Supporting libraries -- signals
*/
/*
 * alarmHandler -- a ``normal'' non-portable version of an alarm handler.
 *	Alas, setting a flag and returning is not fully functional in
 *      BSD: system calls don't fail when reading from a ``slow'' device
 *      like a socket. So we longjump instead, which is erronious on
 *      a small number of machines and ill-defined in the language.
 */
alarmHandler() {
	extern jmp_buf TimeoutJmpBuf;
	longjmp(TimeoutJmpBuf,(int)1);
}

/*
** emergency exit functions.
*/

/*
 * die -- say something and exit with a non-zero return code.
 *	Save the message on stdin in dead.letter.
 */
/*VARARGS*/
die(format,p1,p2,p3,p4,p5) char *format,*p1,*p2,*p3,*p4,*p5; {
	(void) fprintf(stderr,"%s: ",ProgName);
	(void) fprintf(stderr,format,p1,p2,p3,p4,p5);
	(void) putc('\n',stderr);
	flush(); /* Send message to dead.letter */
	log_event(LOG_ERR,format,p1,p2,p3,p4,p5);
	exit(1);
}

/*
 * flush -- save stdin to dead.letter, if you can.
 */
flush() {
	char	line[MAXLINE];
	FILE	*fp;

	if (isatty(fileno(stdin))) {
		log_event(LOG_ERR,"stdin appears to be a terminal. Not saving to dead.letter.");
		return;
	}
	if (Sender == NULL) {
		/* Far to early to save things. */
		log_event(LOG_ERR, "No sender (can't happen), failing horribly.");
		return;
	}
	(void) sprintf(line,"%s/dead.letter",Sender->pw_dir);
	if ((fp= fopen(line,"a")) == NULL) {
		/* Perhaps the person doesn't have a homedir... */
		log_event(LOG_ERR,"Can't open %s, failing horribly.",
			  line);
		return;
	}
	(void) putc('\n',fp); /* Make sure we start on a new line, */
	(void) putc('\n',fp); /* with a blank line separating messages. */
	while (fgets(line,sizeof line,stdin)) {
		(void) fputs(line,fp);
	}
	if (fclose(fp) == ERR) {
		log_event(LOG_ERR,"Can't close %s/dead.letter, possibly truncated.",
			  Sender->pw_dir);
	}
}

/*
** Reporting amd logging library functions 
*/

/*
 * log_event -- log something to syslog and the log file.
 */
/*VARARGS*/
log_event(syslog_code,format,p1,p2,p3,p4,p5)
     int syslog_code; char *format,*p1,*p2,*p3,*p4,*p5; 
{
#ifdef SYSLOG
 	static int syslogOpen = NO;
#endif
#ifdef LOGFILE
	FILE *fp;

	if ((fp= fopen("/tmp/ssmtp.log","a")) != NULL) {
		(void) fprintf(fp,format,p1,p2,p3,p4,p5);
		(void) putc('\n',fp);
		(void) fclose(fp);
	}
	else {
		/* oops! */
		(void) fprintf(stderr,"Can't write to /tmp/ssmtp.log\n");
	}
#endif
#ifdef SYSLOG
 	if (syslogOpen == NO) {
 	       syslogOpen = YES;
#ifdef OLDSYSLOG
		openlog("sSMTP mail", LOG_PID);
#else
		openlog("sSMTP mail", LOG_PID, LOG_MAIL);
#endif
	}
	(void) syslog(syslog_code,format,p1,p2,p3,p4,p5);
#endif
}


/*
** Local/peculiar string manipulation.
*/

/*
 * standardize -- trim off '\n's, double leading dots.
 */
standardize(p) char *p; {
	char *q;

	if (*p == '.' && *(p+1) == '\n') {
		/* Double it, in hopes smtp will single it. */
		*(p+1) = '.';
		return;
	}
	for (q=p; *q; q++)
		;
	*--q = '\0';
}

/*
 * getDate -- get a string form of a system date, and turn it
 *	into a RFC 822 date. Use a horrible, if portable, trick
 *	just like getpwd(2).
 */
getDate(s) char *s; {
	FILE *fp;
	char buffer[MAXLINE];

	if ((fp= popen("date","r")) == NULL) {
		(void) strcpy(s,"Date not set.");
		return NO;
	}
	(void) fgets(buffer,MAXLINE,fp);

	/* <dd> <SP>: s0-2 = buffer8-10. */
	(void) strncpy(&s[0],&buffer[8],3);

	/* <mmm> <SP>: s3-6 = buffer4-7. */
	(void) strncpy(&s[3],&buffer[4],4);

	/* <yy>: s7-8 = buffer26-27 */
	(void) strncpy(&s[7],&buffer[26],2);

	/* <SP> <time>: s9-22 = buffer10-23. */
	(void) strncpy(&s[9],&buffer[10],13);
	s[23] = '\0';

	return pclose(fp)? NO: YES;
}

/*
** Config file access routines.
*/
 int
getConfig() {
	FILE	*fp;
	static char *locations[] = {
			"/usr/local/etc/mail.conf",
			"/etc/mail.conf",
			NULL
	};
	char	**lp;

	for (lp= &locations[0]; *lp; lp++) {
		if ((fp= fopen(*lp,"r")) != NULL) {
			parseConfig(fp);
			(void) fclose(fp);
			return YES;
		}
	}
	return NO; /* We use the default for everything. */
}

/*
 * parseConfig -- parse config file, extract values of a few
 *	predefined variables.
 */
 int
parseConfig(fp) FILE *fp; {
	char	line[MAXLINE],
		*p;

	while (fgets(line,sizeof line,fp)) {
		/* Make comments invisible. */
		if ((p= strchr(line,'#'))) {
			*p = '\0';
		}
		/* Ignore malformed lines and comments. */
		if (strchr(line,'=') == NULL) 
			continue;
		/* Parse out keywords. */
		if ((p= strtok(line,"= \t\n")) != NULL) {
			if (strcasecmp(p,"Root")==0) {
				Root = strdup(strtok(NULL,"= \t\n"));
				if (LogLevel > 0) {
				        log_event(LOG_INFO,
					       "Set Root=\"%s\".\n",Root);
				 }
			}
			else if (strcasecmp(p,"MailHub")==0) {
				MailHub = strdup(strtok(NULL,"= \t\n"));
				if (LogLevel > 0) {
				        log_event(LOG_INFO,
					      "Set MailHub=\"%s\".\n",MailHub);
				 }
			}
			else if (strcasecmp(p,"HostName")==0) {
				(void) strcpy(HostName,strdup(strtok(NULL,"= \t\n")));
				if (LogLevel > 0) {
				        log_event(LOG_INFO,
					      "Set HostName=\"%s\".\n",HostName);
				 }
			}

#ifdef REWRITE_DOMAIN
			else if (strcasecmp(p,"RewriteDomain")==0) {
				RewriteDomain = strdup(strtok(NULL,"= \t\n"));
				if (LogLevel > 0) {
				        log_event(LOG_INFO,
					       "Set RwriteDomain=\"%s\".\n",
						  RewriteDomain);
				 }
			}
#endif
			else {
				  log_event(LOG_INFO,
					"Unable to set %s=\"%s\".\n",
						p, strtok(NULL,"= \t\n"));
				 }
		}
	}
	return;
}


#ifndef lint
static char *rcsid = "$Header: /team/davecb/Tools/ssmtp/RCS/ssmtp.c,v 1.19 1993/02/23 13:16:46 davecb Exp $";
#endif


/*
 * $Log: ssmtp.c,v $
 * Revision 1.19  1993/02/23  13:16:46  davecb
 * remove ^Ls
 *
 * Revision 1.18  1993/02/10  02:43:52  davecb
 * snapshot at 1.99
 *
 * Revision 1.17  1993/02/05  13:29:23  davecb
 * bugfix from Titan alpha-test
 *
 * Revision 1.16  1992/12/18  13:19:53  davecb
 * adding syslog of variables from mail.conf file
 *
 * Revision 1.14  1992/11/15  16:50:31  davecb
 * Snapshot after adding the redirection code.
 *
 * Revision 1.13  1992/11/15  03:36:44  davecb
 * A snapshot after adding confog-file code and better logging, but
 * before maiking config-file variables do much.
 *
 * Revision 1.12  1992/11/15  02:42:50  davecb
 * snapshot time: the file was 0 size...
 *
 * Revision 1.11  1992/10/20  01:33:03  davecb
 * working on options for sendmail emulation.
 *
 * Revision 1.10  1992/10/18  22:19:54  davecb
 * *** empty log message ***
 *
 * Revision 1.9  1992/10/18  22:15:13  davecb
 * applying next changes/improvements
 *
 * Revision 1.8  1992/09/25  01:52:06  davecb
 * Fix a stupid misuse of printf.
 *
 * Revision 1.7  1992/09/15  16:11:02  davecb
 * again
 *
 * Revision 1.6  1992/09/15  16:07:58  davecb
 * still chasing seg fault: only iff first lien is null
 *
 * Revision 1.5  1992/09/15  16:04:45  davecb
 * improving messages, looking for from-logic error
 *
 * Revision 1.4  1992/09/08  14:17:41  davecb
 * fix lint compliant
 *
 * Revision 1.3  1992/08/18  12:20:16  davecb
 * adding external patchlevel
 *
 * Revision 1.2  1992/06/11  16:49:44  davecb
 * adding a check for missing from-lines
 *
 * Revision 1.1  1992/06/11  14:50:04  marshal
 * Initial revision
 *
 * Revision 1.1  1992/06/09  16:11:20  davecb
 * Initial revision
 *
 *
 */

