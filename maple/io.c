/* $Id: io.c 4232 2008-04-22 12:19:57Z piaip $ */
#include "bbs.h"

//kcwu: 80x24 一般使用者名單 1.9k, 含 header 2.4k
// 一般文章推文頁約 2590 bytes
#define OBUFSIZE  3072
#define IBUFSIZE  128

/* realXbuf is Xbuf+3 because hz convert library requires buf[-2]. */
#define CVTGAP	  (3)

#ifdef DEBUG
#define register
#endif

static unsigned char real_outbuf[OBUFSIZE + CVTGAP*2] = "   ", 
		     real_inbuf [IBUFSIZE + CVTGAP*2] = "   ";

// use defines instead - it is discovered that sometimes the input/output buffer was overflow,
// without knowing why.
// static unsigned char *outbuf = real_outbuf + 3, *inbuf = real_inbuf + 3;
#define inbuf  (real_inbuf +CVTGAP)
#define outbuf (real_outbuf+CVTGAP)

static int      obufsize = 0, ibufsize = 0;
static int      icurrchar = 0;

#ifdef DBG_OUTRPT
// output counter
static unsigned long szTotalOutput = 0, szLastOutput = 0;
extern unsigned char fakeEscape;
unsigned char fakeEscape = 0;

unsigned char fakeEscFilter(unsigned char c)
{
    if (!fakeEscape) return c;
    if (c == ESC_CHR) return '*';
    else if (c == '\n') return 'N';
    else if (c == '\r') return 'R';
    else if (c == '\b') return 'B';
    else if (c == '\t') return 'I';
    return c;
}
#endif // DBG_OUTRPT

/* ----------------------------------------------------- */
/* convert routines                                      */
/* ----------------------------------------------------- */
#ifdef CONVERT

extern read_write_type write_type;
extern read_write_type read_type;
extern convert_type    input_type;

inline static ssize_t input_wrapper(void *buf, ssize_t count) {
    /* input_wrapper is a special case.
     * because we may do nothing,
     * a if-branch is better than a function-pointer call.
     */
    if(input_type) return (*input_type)(buf, count);
    else return count;
}

inline static int read_wrapper(int fd, void *buf, size_t count) {
    return (*read_type)(fd, buf, count);
}

inline static int write_wrapper(int fd, void *buf, size_t count) {
    return (*write_type)(fd, buf, count);
}
#endif

/* ----------------------------------------------------- */
/* output routines                                       */
/* ----------------------------------------------------- */
void
oflush(void)
{
    if (obufsize) {
#ifdef CONVERT
	write_wrapper(1, outbuf, obufsize);
#else
	write(1, outbuf, obufsize);
#endif
	obufsize = 0;
    }

#ifdef DBG_OUTRPT
    // if (0)
    {
	static char xbuf[128];
	sprintf(xbuf, ESC_STR "[s" ESC_STR "[H" " [%lu/%lu] " ESC_STR "[u",
		szLastOutput, szTotalOutput);
	write(1, xbuf, strlen(xbuf));
	szLastOutput = 0; 
    }
#endif // DBG_OUTRPT

    fsync(1);
}

void
output(const char *s, int len)
{
#ifdef DBG_OUTRPT
    int i = 0;
    if (fakeEscape)
	for (i = 0; i < obufsize; i++)
	    outbuf[i] = fakeEscFilter(outbuf[i]);

    szTotalOutput += len; 
    szLastOutput  += len;
#endif // DBG_OUTRPT

    /* Invalid if len >= OBUFSIZE */
    assert(len<OBUFSIZE);

    if (obufsize + len > OBUFSIZE) {
#ifdef CONVERT
	write_wrapper(1, outbuf, obufsize);
#else
	write(1, outbuf, obufsize);
#endif
	obufsize = 0;
    }
    memcpy(outbuf + obufsize, s, len);
    obufsize += len;
}

int
ochar(int c)
{

#ifdef DBG_OUTRPT
    c = fakeEscFilter(c);
    szTotalOutput ++; 
    szLastOutput ++;
#endif // DBG_OUTRPT

    if (obufsize > OBUFSIZE - 1) {
	/* suppose one byte data doesn't need to be converted. */
	write(1, outbuf, obufsize);
	obufsize = 0;
    }
    outbuf[obufsize++] = c;
    return 0;
}

/* ----------------------------------------------------- */
/* input routines                                        */
/* ----------------------------------------------------- */

static int      i_newfd = 0;
static struct timeval i_to, *i_top = NULL;
static int      (*flushf) () = NULL;

void
add_io(int fd, int timeout)
{
    i_newfd = fd;
    if (timeout) {
	i_to.tv_sec = timeout;
	i_to.tv_usec = 16384;	/* Ptt: 改成16384 避免不按時for loop吃cpu
				 * time 16384 約每秒64次 */
	i_top = &i_to;
    } else
	i_top = NULL;
}

int
num_in_buf(void)
{
    if (ibufsize <= icurrchar)
	return 0;
    return ibufsize - icurrchar;
}

int
input_isfull(void)
{
    return ibufsize >= IBUFSIZE;
}

/*
 * dogetch() is not reentrant-safe. SIGUSR[12] might happen at any time, and
 * dogetch() might be called again, and then ibufsize/icurrchar/inbuf might
 * be inconsistent. We try to not segfault here...
 */

static int
dogetch(void)
{
    ssize_t         len;
    static time_t  lastact;
    if (ibufsize <= icurrchar) {

	if (flushf)
	    (*flushf) ();

	refresh();

	if (i_newfd) {

	    struct timeval  timeout;
	    fd_set          readfds;

	    if (i_top)
		timeout = *i_top;	/* copy it because select() might
					 * change it */

	    FD_ZERO(&readfds);
	    FD_SET(0, &readfds);
	    FD_SET(i_newfd, &readfds);

	    /* jochang: modify first argument of select from FD_SETSIZE */
	    /* since we are only waiting input from fd 0 and i_newfd(>0) */

	    while ((len = select(i_newfd + 1, &readfds, NULL, NULL,
			    i_top ? &timeout : NULL)) < 0) {
		if (errno != EINTR)
		    abort_bbs();
		/* raise(SIGHUP); */
	    }

	    if (len == 0){
		syncnow();
		return I_TIMEOUT;
	    }

	    if (i_newfd && FD_ISSET(i_newfd, &readfds)){
		syncnow();
		return I_OTHERDATA;
	    }
	}

	do {
	    len = tty_read(inbuf, IBUFSIZE);
	    /* tty_read will handle abort_bbs.
	     * len <= 0: read more */
#ifdef CONVERT
	    if(len > 0)
		len = input_wrapper(inbuf, len);
#endif
#ifdef DBG_OUTRPT
	    // if (0)
	    {
		static char xbuf[128];
		sprintf(xbuf, ESC_STR "[s" ESC_STR "[2;1H [%ld] " 
			ESC_STR "[u", len);
		write(1, xbuf, strlen(xbuf));
		fsync(1);
	    }
#endif // DBG_OUTRPT

	} while (len <= 0);

	ibufsize = len;
	icurrchar = 0;
    }

    // CRLF Handle:
    //
    // (UNIX) LF
    // (WIN)  CRLF
    // (MAC)  CR
    //
    // to work in a compatible way, (see KEY_ENTER definition)
    // let KEY_ENTER = CR

    {
	unsigned char c = (unsigned char) inbuf[icurrchar++];

	// CR LF are treated as one.
	if (c == KEY_CR)
	{
	    // peak next character.
	    if (icurrchar < ibufsize && inbuf[icurrchar] == KEY_LF)
		icurrchar ++;
	    return KEY_ENTER;
	} 
	else if (c == KEY_LF)
	{
	    // XXX it is reported that still some users
	    // experience double ENTERs. We are not sure if there
	    // are still any stupid clients. But if any, you
	    // can reject the LFs. According the the compatibility
	    // test, most clients send CR only so it should be fine.
#ifdef ACCEPT_LF
	    return KEY_ENTER;
#else
	    return KEY_UNKNOWN;
#endif
	}

	// XXX also treat ^H and 127 (KEY_BS2) the same one?
	// else if (c == KEY_BS2)
	// {
	//   return KEY_BS;
	// }
	
	return c;
    }
}

#ifdef DEBUG
/*
 * These are for terminal keys debug
 */
void
_debug_print_ibuffer()
{
    static int y = 0;
    int i = 0;

    move(y % b_lines, 0);
    for (i = 0; i < t_columns; i++) 
	outc(' ');
    move(y % b_lines, 0);
    prints("%d. Current Buffer: %d/%d, ", y+1, icurrchar, ibufsize);
    outs(ANSI_COLOR(1) "[" ANSI_RESET);
    for (i = 0; i < ibufsize; i++)
    {
	int c = (unsigned char)inbuf[i];
	if(c < ' ')
	{
	    prints(ANSI_COLOR(1;33) "0x%02x" ANSI_RESET, c);
	} else {
	    outc(c);
	}
    }
    outs(ANSI_COLOR(1) "]" ANSI_RESET);
    y++;
    move(y % b_lines, 0);
    for (i = 0; i < t_columns; i++) 
	outc(' ');
}

int 
_debug_check_keyinput()
{
    int dbcsaware = 0;
    int flExit = 0;

    clear();
    while(!flExit)
    {
	int i = 0;
	move(b_lines, 0);
	for(i=0; i<t_columns; i++)
	    outc(' ');
	move(b_lines, 0);
	if(dbcsaware)
	{
	    outs( ANSI_REVERSE "游標在此" ANSI_RESET
		    " 測試中文模式會不會亂送鍵。 'q' 離開, 'd' 回英文模式 ");
	    move(b_lines, 4);
	} else {
	    outs("Waiting for key input. 'q' to exit, 'd' to try dbcs-aware");
	}
	refresh();
	wait_input(-1, 0);
	switch(dogetch())
	{
	    case 'd':
		dbcsaware = !dbcsaware;
		break;
	    case 'q':
		flExit = 1;
		break;
	}
	_debug_print_ibuffer();
	while(num_in_buf() > 0)
	    dogetch();
    }
    return 0;
}

#endif

/*
 * wait user input anything for f seconds.
 * if f < 0, then wait forever.
 * Return 1 if anything available.
 */
int 
wait_input(float f, int bIgnoreBuf)
{
    int sel = 0;
    fd_set readfds;
    struct timeval tv, *ptv = &tv;

    if(!bIgnoreBuf && num_in_buf() > 0)
	return 1;

    FD_ZERO(&readfds);
    FD_SET(0, &readfds);

    if(f > 0)
    {
	tv.tv_sec = (long) f;
	tv.tv_usec = (f - (long)f) * 1000000L;
    } else
	ptv = NULL;

    do {
	if(!bIgnoreBuf && num_in_buf() > 0)
	    return 1;
	sel = select(1, &readfds, NULL, NULL, ptv);
    } while (sel < 0 && errno == EINTR);
    /* EINTR, interrupted. I don't care! */

    if(sel == 0)
	return 0;

    return 1;
}

void 
drop_input(void)
{
    icurrchar = ibufsize = 0;
}

int 
peek_input(float f, int c)
{
    int i = 0;
    assert (c > 0 && c < ' '); // only ^x keys are safe to be detected.
    // other keys may fall into escape sequence.

    if (wait_input(f, 1) && (IBUFSIZE > ibufsize))
    {
	int len = tty_read(inbuf + ibufsize, IBUFSIZE - ibufsize);
#ifdef CONVERT
	if(len > 0)
	    len = input_wrapper(inbuf+ibufsize, len);
#endif
	if (len > 0)
	    ibufsize += len;
    }
    for (i = icurrchar; i < ibufsize; i++)
    {
	if (inbuf[i] == c)
	    return 1;
    }
    return 0;
}

