/*-------------------------------------------------------*/
/* visio.c	( NTHU CS MapleBBS Ver 3.00 )		 */
/*-------------------------------------------------------*/
/* target : VIrtual Screen Input Output routines 	 */
/* create : 95/03/29				 	 */
/* update : 96/10/10				 	 */
/*-------------------------------------------------------*/


#include <stdarg.h>
#include <arpa/telnet.h>


#include "bbs.h"


#define VO_MAX		5120	/* output buffer �j�p */
#define VI_MAX		256	/* input buffer �j�p */


#define INPUT_ACTIVE	0
#define INPUT_IDLE	1


#define	STANDOUT	attrsetbg(FILEDBG); attrsetfg(FILEDFG);	// filed color (defined in theme.h)
#define	STANDEND	attrsetbg(0); attrsetfg(7);		// default color (ANSI_COLOR(37;40))

/* ----------------------------------------------------- */
/* �~�r (zh-char) �P�_					 */
/* ----------------------------------------------------- */

int			/* 1:�O 0:���O */
is_zhc_low(str, n)	/* hightman.060504: �P�_�r�ꤤ���� n �Ӧr�ŬO�_���~�r����b�r */
  char *str;
  int n;
{
  char *end;

  end = str + n;
  while (str < end)
  {
    if (!*str)
      return 0;
    if (IS_ZHC_HI(*str))
      str++;
    str++;
  }

  return (str - end);
}


/* ----------------------------------------------------- */
/* output routines					 */
/* ----------------------------------------------------- */


static uschar vo_pool[VO_MAX];
static int vo_size;


#ifdef	VERBOSE
static void
telnet_flush(data, size)
  char *data;
  int size;
{
  int oset;

  oset = 1;

  if (select(1, NULL, &oset, NULL, NULL) <= 0)
  {
    abort_bbs();
  }

  xwrite(0, data, size);
}

#else

# define telnet_flush(data, size)	send(0, data, size, 0)
#endif


void
oflush()
{
  int size;

  if (size = vo_size)
  {
    telnet_flush(vo_pool, size);
    vo_size = 0;
  }
}

void
ochar(ch)
  int ch;
{
  uschar *data;
  int size;

  data = vo_pool;
  size = vo_size;
  if (size > VO_MAX - 2)
  {
    telnet_flush(data, size);
    size = 0;
  }
  data[size++] = ch;
  vo_size = size;
}


void
bell()
{
  static char sound[1] = {Ctrl('G')};

  telnet_flush(sound, sizeof(sound));
}


/* ----------------------------------------------------- */
/* virtual screen					 */
/* ----------------------------------------------------- */


static void
vs_redraw()
{
  redrawwin(); refresh(); return;
}


void
outs_n(uschar *str, int n)
{
    while(n--)
	outs(str);
}


void gety(int *y)
{
    int x;
    getyx(y, &x);
}

/* ----------------------------------------------------- */
/* eXtended output: �q�X user �� name �M nick		 */
/* ----------------------------------------------------- */


#ifdef SHOW_USER_IN_TEXT

static char deltatimebuf[50];

char *
delta_time(Y,y,M,m,D,d)
char Y,y,M,m,D,d;
{
    struct tm then;
    time_t interval;

    Y -= '0';
    y -= '0';
    M -= '0';
    m -= '0';
    D -= '0';
    d -= '0';

    then.tm_hour = then.tm_min = then.tm_sec = then.tm_isdst = 0;
    then.tm_year = Y * 10 + y + 2000 - 1900;
    then.tm_mon  = M * 10 + m - 1;
    then.tm_mday = D * 10 + d;

    if (mktime(&then)== -1)
	return "invaild time.";

    interval = mktime(&then) - time(NULL);
    
    if (interval < -86400)
	return "�w�L��";
    else if (interval < 0)
	return "���";

    sprintf(deltatimebuf, "%d �� %d �p�� %d �� %d ��\0"
	    , interval / 86400, (interval % 86400) / 3600
	    , (interval % 3600) / 60, interval % 60);

    return deltatimebuf;
}

int
expand_esc_star(char *buf, const char *src, int szbuf)
{
    int i;
    if (*src != KEY_ESC)
    {
        str_ncpy(buf, src, szbuf);
        return 0;
    }

    if (*++src != '*') // unknown escape... strip the ESC.
    {
        str_ncpy(buf, src, szbuf);
        return 0;
    }

    switch(*++src)
    {
        // insecure content
        case 's':   // current user id
            str_ncpy(buf, cuser.userid, szbuf);
            return 2;
        case 'n':   // current user nick
	    str_ncpy(buf, cuser.username, szbuf);
            return 2;
        case 'l':   // current user logins
            snprintf(buf, szbuf, "%d", cuser.numlogins);
            return 2;
        case 'p':   // current user posts
            snprintf(buf, szbuf, "%d", cuser.numposts);
            return 2;
        case 'm':   // current user money
            snprintf(buf, szbuf, "%d", cuser.money);
            return 2;
        case 'b':   // current user birthday
            snprintf(buf, szbuf, "%d/%d", cuser.month, cuser.day);
            return 2;

        case 't':   // current time
            str_ncpy(buf, Now(), szbuf);
            return 1;
        case 'u':   // current up users
            snprintf(buf, szbuf, "%d", total_user);
            return 1;
	case 'T':
	    for(i = 1;i < 7;i++)
		if(src[i] < '0' || src[i] > '9')
		{
		    str_ncpy(buf, src-1, szbuf);
		    return 0;
		}
	    str_ncpy(buf,
		    delta_time(src[1],src[2],src[3],src[4],src[5],src[6]),
		    szbuf);
	    return 1;
    }

    // unknown characters, return from star.
    str_ncpy(buf, src-1, szbuf);
    return 0;
}

int
outx(str)
  uschar *str;
{
  int ch;
  int control = 0;
  char buf[64] = "";

  while (ch = *str)
  {
    /* itoc.020301: ESC + * + s ������X */
    if (ch == KEY_ESC && str[1] == '*')
    {
      ch = expand_esc_star(buf, str, sizeof(buf));
      if (ch > 1)
         control = 1;
      outs(buf);
      str += (str[2] == 'T' && ch ? 8 : 2);
    }
    outc(ch);
    str++;
  }
  return control;
}

#endif

/* ----------------------------------------------------- */
/* clear the bottom line and show the message		 */
/* ----------------------------------------------------- */


void
outf(str)			    /* footer output */
  uschar *str;
{
  outz(str);
  prints("%*s\033[m", d_cols, "");
}

void outl (int line, uschar *msg)   /* line output */
{
    move (line, 0);
    clrtoeol();

    if (msg != NULL)
	outs(msg);
} 

void outlx (int line, uschar *msg)   /* line output */
{
    move (line, 0);
    clrtoeol();

    if (msg != NULL)
	outx(msg);
} 

inline void outz (uschar *msg)
{
    return outl (b_lines, msg);
}

inline void outzx (uschar *msg)
{
    return outlx (b_lines, msg);
}

#define ANSI_COLOR_CODE	    "[m;0123456789"
#define ANSI_COLOR_END	    "m"

void outr (uschar *str)		    /* restricted output (strip the ansi screen controlling code only) */
{
    uschar ch, buf[256], *p = NULL;
    int ansi = 0;

    while (ch = *str++)
    {
	if (ch == KEY_ESC)
	{
	    ansi = 1;
	    p = buf;
	    *p++ = ch;
	}
	else if (ansi)
	{
	    if (p)
		*p++ = ch;   

	    if (!strchr(ANSI_COLOR_CODE, ch))
	    {
		ansi = 0;
		buf[0] = '\0';
		p = NULL;
	    }
	    else if (strchr(ANSI_COLOR_END, ch))
	    {
		*p++ = '\0';
		ansi = 0;
		p = NULL;
		outs(buf);
		
	    }
	}
	else
	    outc(ch);
    }
}


void
prints(char *fmt, ...)
{
  va_list args;
  uschar buf[512], *str;	/* �̪��u��L 512 �r */
  int cc;

  va_start(args, fmt);
  vsprintf(buf, fmt, args);
  va_end(args);
  for (str = buf; cc = *str; str++)
    outc(cc);
}

static int save_y, save_x;

void		/* Thor.981028: ���F�� talk.c ���H�I�s�ɷ|show�r */
cursor_save()
{
  getyx(&save_y, &save_x);
}


/* static void */
void		/* Thor.981028: ���F�� talk.c ���H�I�s�ɷ|show�r */
cursor_restore()
{
  move(save_y, save_x);
}

int
vmsg(msg)
  char *msg;			/* length <= 54 */
{
    static int old_b_cols = 23;
    static char foot[512] = VMSG_NULL;

    move(b_lines, 0);
    if (msg)
	prints(ANSI_COLOR(1;44) " " ANSI_COLOR(33) "�� " ANSI_COLOR(37) "%-*s " ANSI_COLOR(40) 
		" [�Ы����N���~��] " ANSI_RESET, b_cols - 23, msg);
    else
    {
	if (b_cols != old_b_cols)
	{
	    old_b_cols = b_cols;
	    int i = 0;

	    str_ncpy(foot, ANSI_COLOR(1;34;44)" ", sizeof(foot));
	    for (i = 0; i < (b_cols - 2 - 22) / 4 + 1; i++)  // 22 = strlen(" �� �Ы����N���~�� �� ")
		strcat(foot, "�e");
	    strcat(foot, ANSI_COLOR(37) " �� �Ы����N���~�� �� "  ANSI_COLOR(34));
	    for (i = 0; i < (b_cols - 2 - 22) / 4 + 1; i++)  // 22 = strlen(" �� �Ы����N���~�� �� ")
		strcat(foot, "�e");
	    strcat(foot, " " ANSI_RESET);

	}
	outs(foot);
    }
    move(b_lines, 0);	/* itoc.010127: �ץ��b�������k����ΤU�A������|�����G�h��檺���D */
    return vkey();
}

/* hrs.080303: ��vmsgf��²��code */
int
vmsgf(char *fmt, ...)
{
  va_list args;
  char buf[512];        /* �̪��u��L 512 �r */

  va_start(args, fmt);
  vsprintf(buf, fmt, args);
  va_end(args);
  return vmsg(buf);
}

static inline void
zkey()				/* press any key or timeout */
{
  /* static */ struct timeval tv = {1, 100};  
  /* Thor.980806: man page ���] timeval struct�O�|���ܪ� */

  int rset;

  rset = 1;
  select(1, (fd_set *) &rset, NULL, NULL, &tv);

#if 0
  if (select(1, &rset, NULL, NULL, &tv) > 0)
  {
    recv(0, &rset, sizeof(&rset), 0);
  }
#endif
}


void
zmsg(msg)			/* easy message */
  char *msg;
{
  outz(msg);
  move(b_lines, 0);	/* itoc.031029: �ץ��b�������k����ΤU�A������|�����G�h��檺���D */

  refresh();
  zkey();
}


void
vs_bar(title)
  char *title;
{
  clear();
  prints("\033[1;33;44m�i %s �j\033[m\n", title);
}


static void
vs_line(msg)
  char *msg;
{
  int head, tail;

  if (msg)
    head = (strlen(msg) + 1) >> 1;
  else
    head = 0;

  tail = head;

  while (head++ < 38)
    outc('-');

  if (tail)
  {
    outc(' ');
    outs(msg);
    outc(' ');
  }

  while (tail++ < 38)
    outc('-');
  outc('\n');
}


/* ----------------------------------------------------- */
/* input routines					 */
/* ----------------------------------------------------- */


static uschar vi_pool[VI_MAX];
static int vi_size;
static int vi_head;


static int vio_fd;


#ifdef EVERY_Z

static int holdon_fd;		 /* Thor.980727: ���Xchat&talk�Ȧsvio_fd�� */


void
vio_save()
{
  holdon_fd = vio_fd;
  vio_fd = 0;
}


void
vio_restore()
{
  vio_fd = holdon_fd;
  holdon_fd = 0;
}


int
vio_holdon()
{
  return holdon_fd;
}
#endif


#if 0
struct timeval
{
  int tv_sec;		/* timeval second */
  int tv_usec;		/* timeval micro-second */
};
#endif

static struct timeval vio_to = {60, 0};


void
add_io(fd, timeout)
  int fd;
  int timeout;
{
  vio_fd = fd;
  vio_to.tv_sec = timeout;
}


static inline int
iac_count(current)
  uschar *current;
{
  switch (*(current + 1))
  {
  case DO:
  case DONT:
  case WILL:
  case WONT:
    return 3;

  case SB:			/* loop forever looking for the SE */
    {
      uschar *look = current + 2;

      /* fuse.030518: �u�W�վ�e���j�p�A���� b_lines */
      if ((*look) == TELOPT_NAWS)
      {
	b_lines = ntohs(* (short *) (look + 3)) - 1;
	b_cols = ntohs(* (short *) (look + 1)) - 1;
	if (b_lines >= T_LINES)
	  b_lines = T_LINES - 1;
	else if (b_lines < 23)
	  b_lines = 23;
	if (b_cols >= T_COLS)
	  b_cols = T_COLS - 1;
	else if (b_cols < 79)
	  b_cols = 79;
	d_cols = b_cols - 79;
	
	resizeterm(b_lines + 1, b_cols + 1);
      }

      for (;;)
      {
	if ((*look++) == IAC)
	{
	  if ((*look++) == SE)
	  {
	    return look - current;
	  }
	}
      }
    }
  }
  return 1;
}


int
igetch()
{

#define	IM_TRAIL	0x01
#define	IM_REPLY	0x02	/* ^R */
#define	IM_TALK		0x04

  static int imode = 0;
  static int idle = 0;

  int cc, fd, nfds, rset;
  uschar *data;

  data = vi_pool;
  nfds = 0;

  for (;;)
  {
    if (vi_size <= vi_head)
    {
      if (nfds == 0)
      {
	refresh();
	fd = (imode & IM_REPLY) ? 0 : vio_fd;
	nfds = fd + 1;
	if (fd)
	  fd = 1 << fd;
      }

      for (;;)
      {
	struct timeval tv = vio_to;
	/* Thor.980806: man page ���] timeval �O�|���ܪ� */

	rset = 1 | fd;
	cc = select(nfds, (fd_set *) & rset, NULL, NULL, &tv /*&vio_to*/);
			/* Thor.980806: man page ���] timeval �O�|���ܪ� */

	if (cc > 0)
	{
	  if (fd & rset)
	    return I_OTHERDATA;

	  cc = recv(0, data, VI_MAX, 0);
	  if (cc > 0)
	  {
	    vi_head = (*data) == IAC ? iac_count(data) : 0;
	    if (vi_head >= cc)
	      continue;
	    vi_size = cc;

#ifdef DETAIL_IDLETIME
	    if (cutmp)
#else
	    if (idle && cutmp)
#endif
	    {
	      idle = 0;

#ifdef DETAIL_IDLETIME
	      time(&cutmp->idle_time);	/* �Y #define DETAIL_IDLETIME�A�h idle_time ��ܶ}�l���m���ɶ�(��) */
#else
	      cutmp->idle_time = 0;	/* �Y #undef DETAIL_IDLETIME�A�h idle_time ��ܤw�g���m�F�h�[(��) */
#endif

#ifdef BMW_COUNT
	      /* itoc.010421: �����@��ᱵ���e�w�Ʀ^�k 0 */
	      cutmp->bmw_count = 0;
#endif
	    }
	    break;
	  }
	  if ((cc == 0) || (errno != EINTR))
	    abort_bbs();
	}
	else if (cc == 0)
	{
	  cc = vio_to.tv_sec;
	  if (cc < 60)		/* paging timeout : �C 60 ���s�@�� idle */
	    return I_TIMEOUT;

	  idle += cc / 60;
	  vio_to.tv_sec = cc + 60;	
	  /* Thor.980806: �C��timeout���W�[60��A�ҥH���l�U���U�C�A�n�i:p */
	  /* Thor.990201.����: ���Ftalk_rqst�Bchat���~�A�ݭn�b�ʤ@�ʤ���A���]tv_sec��60���? (�w�]��) */

          if (idle >= 9 && !cutmp)  /* �n�J�� idle �W�L 9 �����N�_�u */
          {
		outz("�n�J�O�ɡA�Э��s�W��");
		refresh();
		abort_bbs();
          }

#ifdef TIME_KICKER
	  if (idle > IDLE_TIMEOUT)
	  {
	    outs("�� �W�L���m�ɶ��I");
	    refresh();
	    abort_bbs();
  	  }
	  else if (idle >= IDLE_TIMEOUT - IDLE_WARNOUT)	/* itoc.001222: ���m�L�[ĵ�i */
	  {
	    bell();		/* itoc.010315: �s�@�U :p */
	    prints("\033[1;5;31mĵ�i\033[m�z�w�g���m�L�[�A�t�αN�b %d �������z�𰣡I", IDLE_WARNOUT);
	    refresh();
	  }	  
#endif

#ifndef DETAIL_IDLETIME
	  cutmp->idle_time = idle;
#endif

	  if (bbsmode < M_XMENU)	/* �b menu �̭��n�� movie */
	  {
	    movie();
	    refresh();
	  }
	}
	else
	{
	  if (errno != EINTR)
	    abort_bbs();
	}
      }
    }

    cc = data[vi_head++];
    if (imode & IM_TRAIL)
    {
      imode ^= IM_TRAIL;
      if (cc == 0 || cc == 0x0a)
	continue;
    }

    if (cc == 0x0d)
    {
      imode |= IM_TRAIL;
      return '\n';
    }

    if (cc == 0x7f)
    {
      return KEY_BKSP;
    }

    if (cc == Ctrl('L'))
    {
      vs_redraw();
      continue;
    }

    if ((cc == Ctrl('R')) && (bbstate & STAT_STARTED) && !(bbstate & STAT_LOCK) && !(imode & IM_REPLY))
						/* lkchu.990513: ��w�ɤ��i�^�T */
    {
      signal(SIGUSR1, SIG_IGN);

      /* Thor.980307: �b ^R �� talk �|�]�S�� vio_fd �ݤ��� I_OTHERDATA�A�Ӭݤ����襴���r�A�ҥH�b ^R �ɸT�� talk */
      imode |= IM_REPLY;
      bmw_reply();
      imode ^= IM_REPLY;

      signal(SIGUSR1, (void *) talk_rqst);

#ifdef BMW_COUNT
      /* itoc.010907: �����@��ᱵ���e�w�Ʀ^�k 0 */
      cutmp->bmw_count = 0;
#endif
      continue;
    }

    return (cc);
  }
}


#define	MATCH_END	0x8000
/* Thor.990204.����: �N��MATCH����, �n���N�ɨ�, �n���N�����쪬, ���q�X�i�઺�ȤF */

static void
match_title()
{
  move(2, 0);
  clrtobot();
  vs_line("������T�@����");
}


static int
match_getch()
{
  int ch;

  outs("\n�� �C��(C)�~�� (Q)�����H[C] ");
  ch = vkey();
  if (ch == 'q' || ch == 'Q')
    return ch;

  move(3, 0);
  clrtobot();
  return 0;
}


/* ----------------------------------------------------- */
/* ��� board	 					 */
/* ----------------------------------------------------- */


static BRD *xbrd;


BRD *
ask_board(board, perm, msg)
  char *board;
  int perm;
  char *msg;
{
  if (msg)
    outl(2, msg);

  if (vget(1, 0, "�п�J�ݪO�W��(���ť���۰ʷj�M)�G", board, BNLEN + 1, GET_BRD | perm))
    return xbrd;

  return NULL;
}


static int
vget_match(prefix, len, op)
  char *prefix;
  int len;
  int op;
{
  char *data, *hit;
  char newprefix[BNLEN + 1];	/* �~��ɧ����O�W */
  int row, col, match;
  int rlen;			/* �i�ɧ����Ѿl���� */

  row = 3;
  col = match = rlen = 0;

  if (op & GET_BRD)
  {
    usint perm;
    int i;
    char *bits, *n, *b;
    BRD *head, *tail;

    extern BCACHE *bshm;
    extern char brd_bits[];

    perm = op & (BRD_L_BIT | BRD_R_BIT | BRD_W_BIT);
    bits = brd_bits;
    head = bshm->bcache;
    tail = head + bshm->number;

    do
    {
      if (perm & *bits++)
      {
	data = head->brdname;
	int datalen = strlen(data);

	if (str_ncmp(prefix, data, len) &&
		(datalen < 2 || data[1] != '_' || str_ncmp(prefix, data + 2, len)))
	  continue;

	xbrd = head;

	if ((op & MATCH_END) && (!data[len] || (datalen > 2 && data[1] == '_' && !data[len+2])))
	{
	  strcpy(prefix, data);
	  return len;
	}

	match++;
	hit = data;

	if (op & MATCH_END)
	  continue;

	if (match == 1)
	{
	  match_title();
	  if (data[len])
	  {
	    strcpy(newprefix, data);
	    rlen = strlen(data + len);
	  }
	}
	else if (rlen)	/* LHD.051014: �٦��i�ɧ����l�a */
	{
	  n = newprefix + len;
	  b = data + len;
	  for (i = 0; i < rlen && ((*n | 0x20) == (*b | 0x20)); i++, n++, b++)
	    ;
	  *n = '\0';
	  rlen = i;
	}

	move(row, col);
	outs(data);

	col += BNLEN + 1;
	if (col > b_cols + 1 - BNLEN - 1)	/* �`�@�i�H�� (b_cols + 1) / (BNLEN + 1) �� */
	{
	  col = 0;
	  if (++row >= b_lines)
	  {
	    if (match_getch() == 'q')
	      break;

	    move(row = 3, 0);
	    clrtobot();
	  }
	}
      }
    } while (++head < tail);
  }
  else if (op & GET_USER)
  {
    struct dirent *de;
    DIR *dirp;
    int cc;
    char fpath[16];

    /* Thor.981203: USER name�ܤ֥��@�r, ��"<="�|����n��? */
    if (len == 0)
      return 0;

    cc = *prefix;
    if (cc >= 'A' && cc <= 'Z')
      cc |= 0x20;
    if (cc < 'a' || cc > 'z')
      return 0;

    sprintf(fpath, "usr/%c", cc);
    dirp = opendir(fpath);
    while (de = readdir(dirp))
    {
      data = de->d_name;
      if (str_ncmp(prefix, data, len))
	continue;

      if (!match++)
      {
	match_title();
	strcpy(hit = fpath, data);	/* �Ĥ@���ŦX����� */
      }

      move(row, col);
      outs(data);

      col += IDLEN + 1;
      if (col > b_cols + 1 - IDLEN - 1)	/* �`�@�i�H�� (b_cols + 1) / (IDLEN + 1) �� */
      {
	col = 0;
	if (++row >= b_lines)
	{
	  if (match_getch())
	    break;
	  row = 3;
	}
      }
    }

    closedir(dirp);
  }
  else /* Thor.990203.����: GET_LIST */
  {
    LinkList *list;
    extern LinkList *ll_head;

    for (list = ll_head; list; list = list->next)
    {
      data = list->data;

      if (str_ncmp(prefix, data, len))
	continue;

      if ((op & MATCH_END) && !data[len])
      {
	strcpy(prefix, data);
	return len;
      }

      match++;
      hit = data;

      if (op & MATCH_END)
	continue;

      if (match == 1)
	match_title();

      move(row, col);
      outs(data);

      col += IDLEN + 1;
      if (col > b_cols + 1 - IDLEN - 1)	/* �`�@�i�H�� (b_cols + 1) / (IDLEN + 1) �� */
      {
	col = 0;
	if (++row >= b_lines)
	{
	  if (match_getch())
	    break;
	  row = 3;
	}
      }
    }
  }

  if (match == 1)
  {
    strcpy(prefix, hit);
    return strlen(hit);
  }
  else if (rlen)
  {
    strcpy(prefix, newprefix);
    return len + rlen;
  }

  return 0;
}


char lastcmd[MAXLASTCMD][80];


/* Flags to getdata input function */
/* NOECHO  0x0000  ����ܡA�Ω�K�X���o */
/* DOECHO  0x0100  �@����� */
/* LCECHO  0x0200  low case echo�A�����p�g */
/* GCARRY  0x0400  �|��ܤW�@��/�ثe���� */

int
vget(line, col, prompt, data, max, echo)
  int line, col;
  uschar *prompt, *data;
  int max, echo;
{
  int ch, len;
  int x, y;
  int i, next;
  int vlen, hlen;

  if (line == b_lines)
  isgetdata = 1;

  /* itoc.010312: ��������m �]���᭱ line �M prompt ���Q���F */ 
  vlen = line;
  hlen = col + strlen(prompt);

  if (prompt)
  {
    move(line, col);
    clrtoeol();
    outs(prompt);
  }
  else
  {
    clrtoeol();
  }

  STANDOUT;

  getyx (&x, &y);

  if (echo & GCARRY){
    if (len = strlen(data))
      outs(data);
  }
  else
  {
    len = 0;
  }

  /* --------------------------------------------------- */
  /* ���o board / userid / on-line user			 */
  /* --------------------------------------------------- */

  ch = len;
  do
  {
    outc(' ');
  } while (++ch < max);

  STANDEND;

  line = -1;
  col = len;
  max--;

  for (;;)
  {
    if (echo & CTECHO)
    {
      move(x, 74);
      prints(ANSI_COLOR(1;37;40) "%02d/%02d" ANSI_RESET, len, max - len);
    }

    move(x, y + col);
    ch = vkey();

    if (ch == '\n')
    {
      data[len] = '\0';
      if ((echo & (GET_BRD | GET_LIST)) && len > 0)
      /* Thor.990204:�n�D��J���@�r�~�N��۰� match, �_�h��cancel */
      {
	ch = len;
	len = vget_match(data, len, echo | MATCH_END);

	STANDOUT;
	if (len > ch)
	{
	  move(x, y);
	  outs(data);
	}
	else if (len == 0)
	{
	  data[0] = '\0';
	}
	STANDEND;
      }
      break;
    }

    if (isprint2(ch))
    {
      if (ch == ' ' && (echo & (GET_USER | GET_BRD | GET_LIST)))
      {
	ch = vget_match(data, len, echo);

	STANDOUT;
	if (ch > len)
	{
	  move(x, y);
	  outs(data);
	  col = len = ch;
	}
	STANDEND;
	continue;
      }

      if (len >= max)
	continue;

      /* ----------------------------------------------- */
      /* insert data and display it			 */
      /* ----------------------------------------------- */

      prompt = &data[col];
      i = col;
      move(x, y + col);

      STANDOUT;
      for (;;)
      {
	outc(echo ? ch : '*');
	next = *prompt;
	*prompt++ = ch;
	if (i >= len)
	  break;
	i++;
	ch = next;
      }
      STANDEND;

      col++;
      len++;
      continue;
    }

    /* ----------------------------------------------- */
    /* ��J password �ɥu��� BackSpace		       */
    /* ----------------------------------------------- */

    if (!echo && ch != KEY_BKSP)
      continue;

    STANDOUT;
    switch (ch)
    {
    case Ctrl('D'):

      if (col >= len)
	continue;

      col++;

    case KEY_BKSP:

      if (!col)
	continue;

      /* ----------------------------------------------- */
      /* remove data and display it			 */
      /* ----------------------------------------------- */

      len--;
      col--;
#ifdef HAVE_MULTI_BYTE
      /* hightman.060504: �P�_�{�b�R������m�O�_���~�r����b�q�A�Y�O�R�G�r�� */
      if ((cuser.ufo & UFO_ZHC) && echo && col && IS_ZHC_LO(data, col))
      {
	len--;
	col--;
	next = 2;
      }
      else
#endif
	next = 1;
      move(x, y + col);
      for (i = col; i < len; i++)
      {
	data[i] = ch = data[i + next];
	outc(echo ? ch : '*');
      }
      while (next--)
	outc(' ');
      break;

    case KEY_DEL:

      if (col >= len)
	continue;

      /* ----------------------------------------------- */
      /* remove data and display it			 */
      /* ----------------------------------------------- */

      len--;
#ifdef HAVE_MULTI_BYTE
      /* hightman.060504: �P�_�{�b�R������m�O�_���~�r���e�b�q�A�Y�O�R�G�r�� */
      if ((cuser.ufo & UFO_ZHC) && col < len && IS_ZHC_HI(data[col]))
      {
	len--;
	next = 2;
      }
      else
#endif
	next = 1;
      for (i = col; i < len; i++)
      {
	data[i] = ch = data[i + next];
	outc(ch);
      }
      while (next--)
	outc(' ');
      break;

    case KEY_LEFT:
    case Ctrl('B'):
      if (col)
      {
	col--;
#ifdef HAVE_MULTI_BYTE
	/* hightman.060504: �����ɸI��~�r������ */
	if ((cuser.ufo & UFO_ZHC) && col && IS_ZHC_LO(data, col))
	  col--;
#endif
      }
      break;

    case KEY_RIGHT:
    case Ctrl('F'):
      if (col < len)
      {
	col++;
#ifdef HAVE_MULTI_BYTE
	/* hightman.060504: �k���ɸI��~�r������ */
	if ((cuser.ufo & UFO_ZHC) && col < len && IS_ZHC_HI(data[col - 1]))
	  col++;
#endif
      }
      break;

    case KEY_HOME:
    case Ctrl('A'):
      col = 0;
      break;

    case KEY_END:
    case Ctrl('E'):
      col = len;
      break;

    case Ctrl('C'):		/* clear / reset */
      if (len)
      {
	move(x, y);
	for (ch = 0; ch < len; ch++)
	  outc(' ');
	col = len = 0;
      }
      break;

    case KEY_DOWN:
    case Ctrl('N'):

      line += MAXLASTCMD - 2;

    case KEY_UP:
    case Ctrl('P'):

      line = (line + 1) % MAXLASTCMD;
      prompt = lastcmd[line];
      col = 0;
      move(x, y);

      do
      {
	if (!(ch = *prompt++))
	{
	  /* clrtoeol */

	  for (ch = col; ch < len; ch++)
	    outc(' ');
	  break;
	}

#if 0	/* ���ݭn�A�]�� bmtad/receive_article �| strip �� ansi code */
	if (ch == KEY_ESC)	/* itoc.020601: ���o�ϥα���X */
	  ch = '*';
#endif

	outc(ch);
	data[col] = ch;
      } while (++col < max);

      len = col;
      break;

    case Ctrl('K'):		/* delete to end of line */
      if (col < len)
      {
	move(x, y + col);
	for (ch = col; ch < len; ch++)
	  outc(' ');
	len = col;
      }
      break;

    /* itoc.030619: �� vget ���ٯ�� ^R ��e�w */
    case Ctrl('R'):
    case Ctrl('T'):
      if (bbsmode == M_BMW_REPLY)
      {
	if (bmw_reply_CtrlRT(ch))	/* ���h���e�w */
	{
	  data[len] = '\0';
	  return ch;
	}
      }
      break;
    }
    STANDEND;
  }

  if (len >= 2 && echo)
  {
    for (line = MAXLASTCMD - 1; line; line--)
      strcpy(lastcmd[line], lastcmd[line - 1]);
    strcpy(lastcmd[0], data);
  }

  move(vlen, strlen(data) + hlen);	/* itoc.010312: �T�w����ӦC�C���A�L�X'\n' */
  outc('\n');

  ch = data[0];
  if ((echo & LCECHO) && (ch >= 'A' && ch <= 'Z'))
    data[0] = (ch |= 0x20);

  if (vlen == b_lines)
    isgetdata = 0;

  STANDEND;

  return ch;
}

/* hrs.080303: ��vansf��²��code */
int
vansf(char *fmt, ...)
{
  va_list args;
  char buf[512];        /* �̪��u��L 512 �r */

  va_start(args, fmt);
  vsprintf(buf, fmt, args);
  va_end(args);
  return vans(buf);
}


int
vans(prompt)
  char *prompt;
{
  char ans[3];

  /* itoc.010812.����: �|�۰ʴ����p�g�� */
  return vget(b_lines, 0, prompt, ans, sizeof(ans), LCECHO);
}


int
vkey()
{
  int mode;
  int ch, last;

  mode = last = 0;
  for (;;)
  {
    ch = igetch();
    if (mode == 0)		/* Normal Key */
    {
      if (ch == KEY_ESC)
	mode = 1;
      else
	return ch;
    }
    else if (mode == 1)		/* Escape sequence */
    {
      if (ch == '[' || ch == 'O')
	mode = 2;
      else if (ch == '1' || ch == '4')
	mode = 3;
      else
	return Esc(ch);
    }
    else if (mode == 2)		/* Cursor key */
    {
      if (ch >= 'A' && ch <= 'D')
	return KEY_UP - (ch - 'A');
      else if (ch >= '1' && ch <= '6')
	mode = 3;
      else
	return ch;
    }
    else if (mode == 3)		/* Ins Del Home End PgUp PgDn */
    {
      if (ch == '~')
	return KEY_HOME - (last - '1');
      else
	return ch;
    }
    last = ch;
  }
}


int vbar (char * prompt)
{
    int ch;

    outz(prompt);
    ch = vkey();
    if (ch >= 'A' && ch <= 'Z')
	ch |= 0x20;
    return ch;
}
