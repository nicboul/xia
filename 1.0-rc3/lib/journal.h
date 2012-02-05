#ifndef __XIANA_H
#define __XIANA_H

#define EXIT_ERR	0x0001 /* default error */ 
#define EXIT_NOT_ROOT	0x0002
#define EXIT_NO_MYSQL	0x0004
#define EXIT_ERR_PARS	0x0006
#define TRUM_ERR	0x0008

#define ERR_NULPL	0x0001
#define ERR_ACL		0x0002

extern void journal_ftrace(const char *);
extern void journal_failure(int, char *, ...);
extern void journal_notice(char *, ...);
extern void log_set_lvl(int);

#endif
