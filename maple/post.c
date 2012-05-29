/*-------------------------------------------------------*/
/* post.c       ( NTHU CS MapleBBS Ver 2.39 )		 */
/*-------------------------------------------------------*/
/* target : bulletin boards' routines		 	 */
/* create : 95/03/29				 	 */
/* update : 96/04/05				 	 */
/*-------------------------------------------------------*/


#include "bbs.h"


extern BCACHE *bshm;
extern XZ xz[];


extern int wordsnum;		/* itoc.010408: �p��峹�r�� */
extern int TagNum;
extern char xo_pool[];
extern char brd_bits[];


#ifdef HAVE_ANONYMOUS
extern char anonymousid[];	/* itoc.010717: �۩w�ΦW ID */
#endif

static void
change_stamp(folder, hdr)
  char *folder;
  HDR *hdr;
{
  /* hrs.100104: �٬O�� hdr_stamp �h�T�w stamp ���ߤ@�� */
  HDR buf;

  /* ���F�T�w�s�y�X�Ӫ� stamp �]�O unique (���M�J���� chrono ����)�A
     �N���ͤ@�ӷs���ɮסA���ɮ��H�K link �Y�i�C
     �o�Ӧh���ͥX�Ӫ��U���|�b expire �Q sync �� (�]�����b .DIR ��) */
  hdr_stamp(folder, HDR_LINK | 'A', &buf, "etc/stamp");
  hdr->stamp = buf.chrono;
}

int
cmpchrono(hdr)
  HDR *hdr;
{
  return hdr->chrono == currchrono;
}


/* ----------------------------------------------------- */
/* ��} innbbsd ��X�H��B�s�u��H���B�z�{��		 */
/* ----------------------------------------------------- */


void
btime_update(bno)
  int bno;
{
  if (bno >= 0)
    (bshm->bcache + bno)->btime = -1;	/* �� class_item() ��s�� */
}


#ifndef HAVE_NETTOOL
static 			/* �� enews.c �� */
#endif
void
outgo_post(hdr, board)
  HDR *hdr;
  char *board;
{
  bntp_t bntp;

  memset(&bntp, 0, sizeof(bntp_t));

  if (board)		/* �s�H */
  {
    bntp.chrono = hdr->chrono;
  }
  else			/* cancel */
  {
    bntp.chrono = -1;
    board = currboard;
  }
  strcpy(bntp.board, board);
  strcpy(bntp.xname, hdr->xname);
  strcpy(bntp.owner, hdr->owner);
  strcpy(bntp.nick, hdr->nick);
  strcpy(bntp.title, hdr->title);
  rec_add("innd/out.bntp", &bntp, sizeof(bntp_t));
}

void
cancel_post(hdr)
  HDR *hdr;
{
  if ((hdr->xmode & POST_OUTGO) &&		/* �~��H�� */
    (hdr->chrono > ap_start - 7 * 86400))	/* 7 �Ѥ������� */
  {
    outgo_post(hdr, NULL);
  }
}


static inline int		/* �^�Ǥ峹 size �h���� */
move_post(hdr, folder, by_bm)	/* �N hdr �q folder �h��O���O */
  HDR *hdr;
  char *folder;
  int by_bm;
{
  HDR post;
  int xmode;
  char fpath[64], fnew[64], *board;
  char buf[64];
  int len;
  BRD *brd;

  brd = bshm->bcache + currbno;

  xmode = hdr->xmode;
  hdr_fpath(fpath, folder, hdr);

  if (!(xmode & POST_BOTTOM)
	  && brd->bmode != BMODE_HIDE && brd->bmode != BMODE_PAL
	  &&!(xmode & POST_RESTRICT))
				/* �m����Q�夣�� move_post */
  {				/* hrs.071220: ���O�峹��move post */	
				/* hrs.080103: �K��N����i�Ӱ�    */

      board = by_bm ? BN_DELETED : BN_JUNK;

      if (*fpath == 'b')
      {
	  FILE *fpnew;
	  sprintf(buf, "tmp/delete.%s%ld", cuser.userid, time(NULL));
	  fpnew = fopen(buf, "w");
	  f_suck(fpnew, fpath);
	  fprintf (fpnew, DELETE_BANNER, cuser.userid, str_ip(cutmp->in_addr), Now());
          fclose (fpnew);
	  f_mv(buf, fpath);
      }


    brd_fpath(fnew, board, fn_dir);
    hdr_stamp(fnew, HDR_LINK | 'A', &post, fpath);

    /* �����ƻs trailing data�Gowner(�t)�H�U�Ҧ���� */

    memcpy(post.owner, hdr->owner, sizeof(HDR) -
      (sizeof(post.chrono) + sizeof(post.xmode) + sizeof(post.xid) + sizeof(post.xname)));

    len = 43 - strlen(currboard) + 
	  (!strncmp(hdr->title, "Fw: ", 4) || !strncmp(hdr->title, "Re: ", 4) ?
	  4 : 0);
      sprintf(post.title, "%-*.*s.%s", len, len, hdr->title, currboard);

    rec_bot(fnew, &post, sizeof(HDR));
    btime_update(brd_bno(board));
  }

  unlink(fpath);
  btime_update(currbno);
  cancel_post(hdr);

  return hdr->money;
}

/* ----------------------------------------------------- */
/* �R����O�峹�ƥ� (ALLPOST)                            */
/* ----------------------------------------------------- */

char currxname[32];

int cmpxname(hdr)
HDR * hdr;
{
    return !strcmp(hdr->xname, currxname);
}

static void
post_delhdr(hdr, brdname)
HDR * hdr;
char * brdname;
{
    register int i;
    int fsize;
    char fpath[256], folder[256], *fimage;

    brd_fpath(folder, brdname, fn_dir);
    fimage = f_map(folder, &fsize);

    if (fimage == (char *) -1)
        return;

    for (i = fsize / sizeof(HDR) - 1; i >= 0; i--)
    {
	HDR * nhdr = (HDR *) fimage + i;
	if (nhdr->xid == hdr->chrono) // ��O�峹�� xid �P��� chrono �ۦP
	{
	    hdr_fpath(fpath, folder, nhdr);
	    unlink(fpath);

	    strcpy(currxname, nhdr->xname);
	    rec_del(folder, sizeof(HDR), i, cmpxname);

	    break; // ���]�u�|���@�g (�q�`�]�O)
	}
    }

    munmap(fimage, fsize);
    btime_update(brd_bno(brdname)); // ��s�ݪO�\Ū����
}


static void
post_delcrosspost(hdr)
HDR * hdr;
{

#ifdef HAVE_ALLPOST
    post_delhdr(hdr, BN_ALLPOST);
    post_delhdr(hdr, BN_ALLHIDPOST);
#endif

#ifdef HAVE_LUCKYPOST
    post_delhdr(hdr, BN_LUCKYPOST);
#endif
    return;
}


#ifdef HAVE_DETECT_CROSSPOST
/* ----------------------------------------------------- */
/* ��} cross post ���v					 */
/* ----------------------------------------------------- */


#define MAX_CHECKSUM_POST	20	/* �O���̪� 20 �g�峹�� checksum */
#define MAX_CHECKSUM_LINE	6	/* �u���峹�e 6 ��Ӻ� checksum */


typedef struct
{
  int sum;			/* �峹�� checksum */
  int total;			/* ���峹�w�o��X�g */
}      CHECKSUM;


static CHECKSUM checksum[MAX_CHECKSUM_POST];
static int checknum = 0;


static inline int
checksum_add(str)		/* �^�ǥ��C��r�� checksum */
  char *str;
{
  int i, len, sum;

  len = strlen(str);

  sum = len;	/* ��r�ƤӤ֮ɡA�e�|�����@�ܥi�৹���ۦP�A�ҥH�N�r�Ƥ]�[�J sum �� */
  for (i = len >> 2; i > 0; i--)	/* �u��e�|�����@�r���� sum �� */
    sum += *str++;

  return sum;
}


static inline int		/* 1:�Ocross-post 0:���Ocross-post */
checksum_put(sum)
  int sum;
{
  int i;

  if (sum)
  {
    for (i = 0; i < MAX_CHECKSUM_POST; i++)
    {
      if (checksum[i].sum == sum)
      {
	checksum[i].total++;

	if (checksum[i].total > MAX_CROSS_POST)
	  return 1;
	return 0;	/* total <= MAX_CROSS_POST */
      }
    }

    if (++checknum >= MAX_CHECKSUM_POST)
      checknum = 0;
    checksum[checknum].sum = sum;
    checksum[checknum].total = 1;
  }
  return 0;
}


static int			/* 1:�Ocross-post 0:���Ocross-post */
checksum_find(fpath)
  char *fpath;
{
  int i, sum;
  char buf[ANSILINELEN];
  FILE *fp;

  sum = 0;
  if (fp = fopen(fpath, "r"))
  {
    for (i = -(LINE_HEADER + 1);;)	/* �e�X�C�O���Y */
    {
      if (!fgets(buf, ANSILINELEN, fp))
	break;

      if (i < 0)	/* ���L���Y */
      {
	i++;
	continue;
      }

      if (*buf == QUOTE_CHAR1 || *buf == '\n' || !strncmp(buf, "��", 2))	 /* ���L�ި� */
	continue;

      sum += checksum_add(buf);
      if (++i >= MAX_CHECKSUM_LINE)
	break;
    }

    fclose(fp);
  }

  return checksum_put(sum);
}


static int
check_crosspost(fpath, bno)
  char *fpath;
  int bno;			/* �n��h���ݪO */
{
  char *blist, folder[64];
  ACCT acct;
  HDR hdr;

  if (HAS_PERM(PERM_ALLADMIN))
    return 0;

  /* �O�D�b�ۤv�޲z���ݪO���C�J��K�ˬd */
  blist = (bshm->bcache + bno)->BM;
  if (HAS_PERM(PERM_BM) && blist[0] > ' ' && is_bm(blist, cuser.userid))
    return 0;

  if (checksum_find(fpath))
  {
    /* �p�G�O cross-post�A������h BN_SECURITY �ê������v */
    brd_fpath(folder, BN_SECURITY, fn_dir);
    hdr_stamp(folder, HDR_COPY | 'A', &hdr, fpath);
    strcpy(hdr.owner, cuser.userid);
    strcpy(hdr.nick, cuser.username);
    sprintf(hdr.title, "%s %s Cross-Post", cuser.userid, Now());
    rec_bot(folder, &hdr, sizeof(HDR));
    btime_update(brd_bno(BN_SECURITY));

    bbstate &= ~STAT_POST;
    cuser.userlevel &= ~PERM_POST;
    cuser.userlevel |= PERM_DENYPOST;
    if (acct_load(&acct, cuser.userid) >= 0)
    {
      acct.tvalid = time(NULL) + CROSSPOST_DENY_DAY * 86400;
      acct_setperm(&acct, PERM_DENYPOST, PERM_POST);
    }
    board_main();
    mail_self(FN_ETC_CROSSPOST, str_sysop, "Cross-Post ���v", 0);
    vmsg("�z�]���L�� Cross-Post �w�Q���v");
    return 1;
  }
  return 0;
}
#endif	/* HAVE_DETECT_CROSSPOST */

/* ----------------------------------------------------- */
/* �ˬd�峹�o���v��                                      */
/* ----------------------------------------------------- */

int CheckPostPerm(brd)
    BRD *brd;
{
  int bno = brd_bno(brd->brdname);

  if (bno < 0)
    return 0;

  if (HAS_PERM(PERM_ALLADMIN))
    return 1;

  if (!(brd_bits[bno] & BRD_W_BIT))
    return 0;

  if (cuser.numposts < brd->limit_posts)
    return 0;

  if (cuser.numlogins < brd->limit_logins)
    return 0;
  
  if ((time(NULL) - cuser.firstlogin) / (86400 * 30) <  brd->limit_regtime)
    return 0;

  return 1;
}


/* ----------------------------------------------------- */
/* �t�~����峹�s���v�� (�]�t����M���D)                 */
/* ----------------------------------------------------- */

int CheckEditPerm(BRD *brd)
{

// ALLPOST ���峹����s��
#ifdef HAVE_ALLPOST
    if (!strcmp(brd->brdname, BN_ALLPOST) || !strcmp(brd->brdname, BN_ALLHIDPOST))
	return 0;
#endif

// LUCKYPOST ���峹����s��
#ifdef HAVE_LUCKYPOST
    if (!strcmp(brd->brdname, BN_LUCKYPOST))
	return 0;
#endif

// �S���o���v�����H����s��
    if (!CheckPostPerm(brd)) 
	return 0;

// guest ����s��
    if (!cuser.userlevel)   
	return 0;

    return 1;
}


/* ----------------------------------------------------- */
/* �o��B�^���B�s��B����峹				 */
/* ----------------------------------------------------- */


int
is_author(hdr)
  HDR *hdr;
{
  /* �o�̨S���ˬd�O���O guest�A�`�N�ϥΦ��禡�ɭn�S�O�Ҽ{ guest ���p */

  /* itoc.070426: ��b���Q�M����A�s���U�ۦP ID ���b���ä��֦��L�h�� ID �o���峹���Ҧ��v */
  return !strcmp(hdr->owner, cuser.userid) && (hdr->chrono > cuser.firstlogin);
}


#ifdef HAVE_REFUSEMARK
static int
post_canseelist(xo)            /* hrs.090803: �[�K�峹�i���W�� */
  XO *xo;
{
  char fpath[256];
  HDR *hdr;
  XO *xt;

  hdr = (HDR *) xo_pool + (xo->pos - xo->top);

  if  (!(hdr->xmode & POST_RESTRICT) || !is_author(hdr))
      return XO_NONE;

  hdr_fpath(fpath, xo->dir, hdr);
  strcat(fpath, ".vis");

  xz[XZ_PAL - XO_ZONE].xo = xt = xo_new(fpath);
  xt->key = PALTYPE_CANSEE;
  xover(XZ_PAL);                /* Thor: �ixover�e, pal_xo �@�w�n ready */

  free(xt);
  return XO_INIT;
}

static int
belong_canseelist(HDR *hdr, int userno)
{
    char folder[256], fpath[256];

    brd_fpath(folder, currboard, fn_dir);
    hdr_fpath(fpath, folder, hdr);
    strcat(fpath, ".vis");

    return pal_find(fpath, userno);
}

void
unlink_canseelist(char *brdname, HDR *hdr)
{
    char folder[256], fpath[256];

    if (!(hdr->xmode & POST_RESTRICT))
	return;

    brd_fpath(folder, brdname, fn_dir);
    hdr_fpath(fpath, folder, hdr);
    strcat(fpath, ".vis");

    unlink(fpath);
}

inline int
chkrestrict(hdr)
  HDR *hdr;
{
  return !(hdr->xmode & POST_RESTRICT) || is_author(hdr) || (bbstate & STAT_BM) 
      || belong_canseelist(hdr, cuser.userno);
}
#endif  

/* hrs.081101: �ˬd��H�峹�O���O�w�g��X�h�F */
int
chkoutgo(hdr, brd)	/* 1: ��X�h�F; 0: �٨S��X�h */
  HDR *hdr;
  BRD *brd;
{
    bntp_t BNTP;
    register int i, size;

    if (!(hdr->xmode & POST_OUTGO))  /* �����峹�S���O�_��X�����D */
	return 0;

    if (brd->battr & BRD_NOTRAN)    /* �D��H�O�]�S���O�_��X�����D */
	return 0;

    size = rec_num("innd/out.bntp", sizeof(bntp_t));
    for (i = 0; i < size; i++)
    {
	rec_get("innd/out.bntp", &BNTP, sizeof(bntp_t), i);
	if (!strcmp(brd->brdname, BNTP.board)&&
		!strcmp(hdr->xname, BNTP.xname)&&
		    !strcmp(hdr->owner, BNTP.owner))
			return 0;
    }

    return 1;
}

#ifdef HAVE_ANONYMOUS
static void
log_anonymous(fname)
  char *fname;
{
  char buf[512];

  sprintf(buf, "%s %-13s(%s)\n%-13s %s %s\n", 
    Now(), cuser.userid, fromhost, currboard, fname, ve_title);
  f_cat(FN_RUN_ANONYMOUS, buf);
}
#endif


#ifdef HAVE_UNANONYMOUS_BOARD
static void
do_unanonymous(fpath)
  char *fpath;
{
  HDR hdr;
  char folder[64];

  brd_fpath(folder, BN_UNANONYMOUS, fn_dir);
  hdr_stamp(folder, HDR_LINK | 'A', &hdr, fpath);

  strcpy(hdr.owner, cuser.userid);
  strcpy(hdr.title, ve_title);

  rec_bot(folder, &hdr, sizeof(HDR));
  btime_update(brd_bno(BN_UNANONYMOUS));
}
#endif


/* �o���ݪO */
void add_post(brdname, fpath, title, owner, nick, xmode) 
  char *brdname, *fpath, *title, *owner, *nick;        
  /* �ت��ݪO/�ɮ׸��|/�峹���D/�@��ID/�@�̼ʺ� */
  int xmode;
  /* �ݩ� */
{
  HDR hdr;
  char folder[64];

  brd_fpath(folder, brdname, fn_dir);
  hdr_stamp(folder, HDR_LINK | 'A', &hdr, fpath);

  str_ncpy(hdr.owner, owner, sizeof(hdr.owner));
  str_ncpy(hdr.nick, nick, sizeof(hdr.nick));
  str_ncpy(hdr.title, title, sizeof(hdr.title));
  hdr.xmode = xmode;

  rec_bot(folder, &hdr, sizeof(HDR));
  btime_update(brd_bno(brdname));
}

static void
cross_post(fpath, owner, nick, mode, chrono, brdname)
  char *fpath;
  char *owner;
  char *nick;
  int mode;
  time_t chrono;
  char *brdname;
{
  HDR hdr;
  char folder[64];
  int len;

  brd_fpath(folder, brdname, fn_dir);
  hdr_stamp(folder, HDR_LINK | 'A', &hdr, fpath);

  hdr.xmode = mode & ~POST_OUTGO;  /* ���� POST_OUTGO */
  hdr.xid = chrono;
  strcpy(hdr.owner, owner);
  strcpy(hdr.nick, nick);
  len = 43 - strlen(currboard) +
       (!strncmp(ve_title, "Fw: ", 4) || !strncmp(ve_title, "Re: ", 4) ?
           4 : 0);
  sprintf(hdr.title, "%-*.*s.%s", len, len, ve_title, currboard);

  rec_bot(folder, &hdr, sizeof(HDR));

  btime_update(brd_bno(brdname));
}

static int
do_post(xo, title)
  XO *xo;
  char *title;
{
  /* Thor.981105: �i�J�e�ݳ]�n curredit �� quote_file */
  HDR hdr;
  char fpath[64], afpath[64], *folder, *nick, *rcpt;
  int mode;
  time_t spendtime;
  char myprefix[20],tmp[20];
  BRD *brd;
  int isprefix = -1;
  FILE *fp;
  char prefix[NUM_PREFIX][20], sample[20], dpath[64], lucky;
  srand(time(NULL));

  if (!(bbstate & STAT_POST))
  {
#ifdef NEWUSER_LIMIT
    if (cuser.lastlogin - cuser.firstlogin < 3 * 86400)
      vmsg("�s��W���A�T���l�i�i�K�峹");
    else
#endif      
      vmsg("�藍�_�A�z�S���b���o��峹���v��");
    return XO_FOOT;
  }

  brd = bshm->bcache + currbno;

  if (!CheckPostPerm(brd))
  {
      vmsg("�藍�_�A�z�S���b���o��峹���v�� (�j�gI�i�ݭ���)");
      return XO_FOOT;
  }

  if (brd->battr & BRD_COSIGN)
  {
    if (bbstate & STAT_BOARD)
    {
	if (vans("�s�p���A�z�n�o��G 1) �@��峹 2) �s�p�峹 [2] ") != '1')
	return do_cosign();
    }
    else
    return do_cosign();
  }

  brd_fpath(fpath, currboard, FN_POSTLAW);
  if (more(fpath, (char *) -1) == -1)
    film_out(FILM_POST, 0);
  
  move(19,0);
  prints("�o��峹��i %s �j�ݪO", currboard);

#ifdef POST_PREFIX
  /* �ɥ� mode�Brcpt�Bfpath */

  if (title)
  {
    rcpt = NULL;
  }
  else		/* itoc.020113: �s�峹��ܼ��D���� */
  {
      brd_fpath(fpath, currboard, FN_PREFIX);
      if (fp = fopen(fpath, "r"))
      {
	  move(21, 0);
	  clrtobot();
	  outs("���O�G");
	  for (mode = 0; mode < NUM_PREFIX; mode++)
	  {
	      char *ptr;

	      // fpath �Q�ɥΨӦs prefix
	      if (fgets(fpath, 14, fp) == NULL)
		  break;

	      /* hrs:�]���̫�@��Ū�J���r���p�G�O\n  ����r��  �n�h���� */
	      ptr = strchr(fpath, '\n');
	      if (ptr)
		  *ptr = '\0';

	      strcpy(prefix[mode], fpath);
	      if(mode == 5)  move(23, 6);
	      prints(ANSI_COLOR(1;33) "%d." ANSI_COLOR(1;37) "%s " ANSI_RESET, mode, fpath);
	  }

	  fclose(fp);
	  vget(20, 0, "�п�ܤ峹���O�]�Φۦ��J�^�G",myprefix, 15, DOECHO) ;
	  if (myprefix[0]-'0' >= 0 && myprefix[0]-'0' < NUM_PREFIX && !myprefix[1])
	  {
	      isprefix = myprefix[0] - '0';
	      sprintf(myprefix, "%s ", prefix[isprefix]);
	  }
	  else if (myprefix[0])
	  {                                      
	      sprintf(tmp,"[%s] ", myprefix);
	      strcpy(myprefix, tmp);
	  }
      }
      else
      {
	  myprefix[0] = '\0';
      }
  }
  if (!ve_subject(21, title, myprefix))
#else
      if (!ve_subject(21, title, NULL))
#endif
	  return XO_HEAD;

  /* ����� Internet �v���̡A�u��b�����o��峹 */
  /* Thor.990111: �S��H�X�h���ݪO, �]�u��b�����o��峹 */

  if (!HAS_PERM(PERM_INTERNET) || (currbattr & BRD_NOTRAN))
      curredit &= ~EDIT_OUTGO;

  utmp_mode(M_POST);
  if (isprefix >= 0)
  {
      sprintf(sample, FN_SAMPLE ".%d", isprefix);
      sprintf(fpath, "tmp/sample.%s.%ld", cuser.userid, time(NULL));
      brd_fpath(dpath, currboard, sample);
      f_cp(dpath, fpath, O_APPEND);
  }
  else
      fpath[0] = '\0';

  time(&spendtime);
  if (vedit(fpath, 1) < 0)
  {
      unlink(fpath);
      vmsg(msg_cancel);
      return XO_HEAD;
  }
  spendtime = time(0) - spendtime;	/* itoc.010712: �`�@�᪺�ɶ�(���) */

  /* build filename */
  folder = xo->dir;
  hdr_stamp(folder, HDR_LINK | 'A', &hdr, fpath);

  /* set owner to anonymous for anonymous board */

#ifdef HAVE_ANONYMOUS
  /* Thor.980727: lkchu�s�W��[²�檺��ܩʰΦW�\��] */
  if (curredit & EDIT_ANONYMOUS)
  {
      rcpt = anonymousid;	/* itoc.010717: �۩w�ΦW ID */
      nick = STR_ANONYMOUS;

      /* Thor.980727: lkchu patch: log anonymous post */
      /* Thor.980909: gc patch: log anonymous post filename */
      log_anonymous(hdr.xname);

#ifdef HAVE_UNANONYMOUS_BOARD
      do_unanonymous(fpath);
#endif
  }
  else
#endif
  {
      rcpt = cuser.userid;
      nick = cuser.username;
  }
  title = ve_title;
  mode = (curredit & EDIT_OUTGO) ? POST_OUTGO : 0;
#ifdef HAVE_REFUSEMARK
  if (curredit & EDIT_RESTRICT)
      mode |= POST_RESTRICT;
#endif

#ifdef HAVE_ALLPOST
  /* hrs.071223:ALLPOST */
  if ((brd->readlevel) < (PERM_VALID << 1) 
	  && brd->bmode != BMODE_HIDE
	  && brd->bmode != BMODE_PAL
	  && !(brd->battr & BRD_18X)
	  && !(brd->battr & BRD_HIDEALLPOST))
      cross_post(fpath, rcpt, nick, mode, hdr.chrono, BN_ALLPOST);
  else
      cross_post(fpath, rcpt, nick, mode, hdr.chrono, BN_ALLHIDPOST);
#endif

  hdr.money = (BMIN(wordsnum, spendtime) / 10 > 1000 ?
	  1000 : BMIN(wordsnum, spendtime)) / 10;
  /* hrs.080315 :�������n�A��KLuckyPost */

#ifdef HAVE_LUCKYPOST

  /* hrs.080107:LuckyPost 
   * ���v����k: percent =  (money / 1000) * 100 %   
   * (�H�۽Z�S�u���ܤ�)
   * 1000 ��   ->   1000/1000 = 1.0 (100  %)
   *  100 ��   ->    100/1000 = 0.1 (10.0 %)
   *    0 ��   ->      0/1000 =   0 (0.00 %)		    */

  lucky = 0;

  if (!(currbattr & BRD_NOCOUNT) &&(rand() % 100 < (double)hdr.money / 1000 * 100))
  {
      hdr.xmode = ((((brd->readlevel) < (PERM_VALID << 1)) 
		  && brd->bmode != BMODE_HIDE
		  && brd->bmode != BMODE_PAL) ?
	      mode : mode | POST_RESTRICT);
      cross_post(fpath, rcpt, nick, hdr.xmode, hdr.chrono, BN_LUCKYPOST);

      hdr_fpath(afpath, folder, &hdr);
      FILE *fpw = fopen(fpath, "a+");
      fprintf (fpw, LUCKY_BANNER);
      fclose (fpw);

      lucky++;
      hdr.money += atoi(LUCKYPOST_REWARD);
  }

#endif /* HAVE_LUCKYPOST */

  hdr.xmode = mode;
  strcpy(hdr.owner, rcpt);
  strcpy(hdr.nick, nick);
  strcpy(hdr.title, title);

  rec_bot(folder, &hdr, sizeof(HDR));
  btime_update(currbno);

  if (mode & POST_OUTGO)
      outgo_post(&hdr, currboard);

  post_history(xo, &hdr);
#ifdef HAVE_ALLPOST
  allpost_history(xo, &hdr);
#endif


  clear();

#ifdef HAVE_LUCKYPOST
  if (lucky)
      outs(ANSI_COLOR(1) 
	      "���ߤ����F�A���� " LUCKYPOST_REWARD " ���A"
	      "�z�i�H�b" BN_LUCKYPOST "�O�ݨ�o�g�峹\n\n" ANSI_RESET);
#endif

  outs("���Q�K�X�峹�A");

  /* hrs.100108: ���q�L�{�ҨϥΪ̤��W�[�峹/������ */
  /* hrs.100109: �L�Z�S�峹���C�J���� */
  if (currbattr & BRD_NOCOUNT || wordsnum < 30 || !HAS_PERM(PERM_VALID) || !hdr.money)
  {				
      outs("�峹���C�J�����A�q�Х]�[�C");
  }
  else
  {
      prints("�o�O�z���� %d �g�峹�A�o %d �ȡC", ++cuser.numposts, hdr.money);
#ifdef HAVE_LUCKYPOST
      if (lucky)
	  outs(" (�t����)");
#endif
      addmoney(hdr.money);
  }

  if (mode & POST_RESTRICT)
  {
      outs("\n\n" ANSI_COLOR(1;35) "���g���[�K�峹�A�󦹤峹�e�� o �i�]�w�峹�i���W��C" ANSI_RESET);
  }

  /* �^�����@�̫H�c */

  if (curredit & EDIT_BOTH)
  {
      rcpt = quote_user;

      if (strchr(rcpt, '@'))	/* ���~ */
	  mode = bsmtp(fpath, title, rcpt, 0);
      else			/* �����ϥΪ� */
	  mode = mail_him(fpath, rcpt, title, 0);

      outs(mode >= 0 ? "\n\n���\\�^���ܧ@�̫H�c" : "\n\n�@�̵L�k���H");
  }

  unlink(fpath);

  vmsg(NULL);

  return XO_INIT;
}


int
do_reply(xo, hdr)
    XO *xo;
    HDR *hdr;
{
    curredit = 0;
    BRD *brd;

    brd = bshm->bcache + currbno;

    if (hdr->xmode & POST_COSIGN)
	return cosign_reply(xo, hdr);

    if (brd->battr & BRD_NOREPLY)
    {
	vmsg("���ݪO���o�^�Ф峹�C");
	return XO_FOOT;
    }

    switch (vans("�� �^���� (F)�ݪO (M)�@�̫H�c (B)�G�̬ҬO (Q)�����H[F] "))
    {
	case 'm':
	    hdr_fpath(quote_file, xo->dir, hdr);
	    return do_mreply(hdr, 0);

	case 'q':
	    return XO_FOOT;

	case 'b':
	    /* �Y�L�H�H���v���A�h�u�^�ݪO */
	    if (HAS_PERM(strchr(hdr->owner, '@') ? PERM_INTERNET : PERM_LOCAL))
		curredit = EDIT_BOTH;
	    break;
    }

    /* Thor.981105: ���׬O��i��, �άO�n��X��, ���O�O���i�ݨ쪺, �ҥH�^�H�]��������X */
    if (hdr->xmode & (POST_INCOME | POST_OUTGO))
	curredit |= EDIT_OUTGO;

    hdr_fpath(quote_file, xo->dir, hdr);
    strcpy(quote_user, hdr->owner);
    strcpy(quote_nick, hdr->nick);
    return do_post(xo, hdr->title);
}


static int
post_reply(xo)
    XO *xo;
{
    if (bbstate & STAT_POST)
    {
	HDR *hdr;

	hdr = (HDR *) xo_pool + (xo->pos - xo->top);

	if ((hdr->xmode & POST_MARKED)&& (hdr->xmode & POST_DONE))
	{
	    vmsg("����w�Q��w�C");
	    return XO_HEAD;
	}

#ifdef HAVE_REFUSEMARK
	if (!chkrestrict(hdr))
	    return XO_NONE;
#endif
	return do_reply(xo, hdr);
    }
    return XO_NONE;
}


static int
post_add(xo)
    XO *xo;
{
    curredit = EDIT_OUTGO;
    *quote_file = '\0';
    return do_post(xo, NULL);
}


/* ----------------------------------------------------- */
/* �L�X hdr ���D					 */
/* ----------------------------------------------------- */


int
tag_char(chrono)
    int chrono;
{
    return TagNum && !Tagger(chrono, 0, TAG_NIN) ? '*' : ' ';
}


#ifdef HAVE_DECLARE
static inline int
cal_day(date)		/* itoc.010217: �p��P���X */
    char *date;
{
#if 0
    ���Ǥ����O�@�ӱ�����@�ѬO�P���X������.
	�o�����O:
	c                y       26(m+1)
	W= [---] - 2c + y + [---] + [---------] + d - 1
	4                4         10
	W �� ���ҨD������P����. (�P����: 0  �P���@: 1  ...  �P����: 6)
	c �� ���w�������~�����e���Ʀr.
	y �� ���w�������~��������Ʀr.
	m �� �����
	d �� �����
	[] �� ��ܥu���Ӽƪ���Ƴ��� (�a�O���)
	ps.�ҨD������p�G�O1���2��,�h�������W�@�~��13���14��.
	�ҥH������m�����Ƚd�򤣬O1��12,�ӬO3��14
#endif

	/* �A�� 2000/03/01 �� 2099/12/31 */

	int y, m, d;

    y = 10 * ((int) (date[0] - '0')) + ((int) (date[1] - '0'));
    d = 10 * ((int) (date[6] - '0')) + ((int) (date[7] - '0'));
    if (date[3] == '0' && (date[4] == '1' || date[4] == '2'))
    {
	y -= 1;
	m = 12 + (int) (date[4] - '0');
    }
    else
    {
	m = 10 * ((int) (date[3] - '0')) + ((int) (date[4] - '0'));
    }
    return (-1 + y + y / 4 + 26 * (m + 1) / 10 + d) % 7;
}
#endif

int money_mode = 0;

void
hdr_outs(hdr, cc, zone)		/* print HDR's subject */
    HDR *hdr;
    int cc;			/* �L�X�̦h cc - 1 �r�����D */
    int zone;			/* zone : 0)post 1)mail */
{
    /* �^��/���/���/�\Ū�����P�D�D�^��/�\Ū�����P�D�D���/�\Ū�����P�D�D��� */
static char *type[8] = {"Re", "Fw", "��",  "��"
	, ANSI_COLOR(1;33)"=>", ANSI_COLOR(1;33)"=>"
	    , ANSI_COLOR(1;31)"��", ANSI_COLOR(1;31) "��"};
    uschar *title, *mark;
    int ch, len, tlen;
    int in_chi, inchi2;	/* 1: �b����r�� */
#ifdef HAVE_DECLARE
    int square;		/* 1: �n�B�z��A */
#endif
#ifdef CHECK_ONLINE
    UTMP *online;
#endif

    /* --------------------------------------------------- */
    /* �L�X���						 */
    /* --------------------------------------------------- */

    if (money_mode && zone)
    {
	if (POST_SPECIAL(hdr->xmode))
	    outs(ANSI_COLOR(1;30) "----- " ANSI_RESET);
	else
	    prints(ANSI_COLOR(1;30) "%5d " ANSI_RESET, hdr->money);
    }
    else
    {
#ifdef HAVE_DECLARE
	/* itoc.010217: ��άP���X�ӤW�� */
	prints("\033[1;3%dm%s\033[m ", cal_day(hdr->date) + 1, hdr->date + 3);
#else
	outs(hdr->date + 3);
	outc(' ');
#endif
    }
    /* --------------------------------------------------- */
    /* �L�X�@��						 */
    /* --------------------------------------------------- */

#ifdef CHECK_ONLINE
    if (online = utmp_seek(hdr))
	outs(COLOR7);
#endif

    mark = hdr->owner;
    len = IDLEN + 1;
    in_chi = inchi2 = 0;

    while (ch = *mark)
    {
	if (--len <= 0)
	{
	    /* ��W�L len ���ת������������� */
	    /* itoc.060604.����: �p�G��n���b����r���@�b�N�|�X�{�ýX�A���L�o���p�ܤֵo�͡A�ҥH�N���ޤF */
	    ch = '.';
	}
	else
	{
	    /* ���~���@�̧� '@' ���� '.' */
	    if (in_chi || IS_ZHC_HI(ch))	/* ����r���X�O '@' ������ */
		in_chi ^= 1;
	    else if (ch == '@')
		ch = '.';
	}

	outc(ch);

	if (ch == '.')
	    break;

	mark++;
    }

    while (len--)
	outc(' ');

#ifdef CHECK_ONLINE
    if (online)
	outs(str_ransi);
#endif

    /* --------------------------------------------------- */
    /* �L�X���D������					 */
    /* --------------------------------------------------- */

    if (!chkrestrict(hdr))
    {
	title = "\033[1;30m<< �����[�K�峹�A����ܼ��D >>\033[m";
	ch = 1;
    }
    else
    {
	/* len: ���D�O type[] �̭������@�� */
	title = str_ttl(mark = hdr->title);
	len = (hdr->xmode & POST_COSIGN && zone ? 3 : ((title == mark) ? 2 : ((*mark == 'R') ? 0 : 1)));
	if (!strcmp(currtitle, title))
	    len += 4;
	outs(type[len]);
	outc(' ');
    }

    /* --------------------------------------------------- */
    /* �L�X���D						 */
    /* --------------------------------------------------- */

    tlen = strlen(title);
    mark = title + cc;

#ifdef HAVE_DECLARE	/* Thor.980508: Declaration, ���ըϬY��title����� */
    square = in_chi = 0;
    if (len < 3)
    {
	if (*title == '[')
	{
	    outs("\033[1m");
	    square = 1;
	}
    }
#endif

    /* ��W�L cc ���ת������������� */
    /* itoc.060604.����: �p�G��n���b����r���@�b�N�|�X�{�ýX�A���L�o���p�ܤֵo�͡A�ҥH�N���ޤF */
    while ((ch = *title++) && (title < mark))
    {
#ifdef HAVE_DECLARE
	if (square)
	{
	    if (in_chi || IS_ZHC_HI(ch))	/* ����r���ĤG�X�Y�O ']' ����O��A */
	    {
		in_chi ^= 1;
	    }
	    else if (ch == ']')
	    {
		outs("]\033[m");
		square = 0;			/* �u�B�z�@�դ�A�A��A�w�g�B�z���F */
		continue;
	    }
	}
	else
#endif
	{
	    if ((in_chi % 2) || IS_ZHC_HI(ch))
	    {
		in_chi++;
	    }

	    if (((in_chi % 2) && title >= mark - 2) ||  (tlen > cc && title == mark - 1))
	    {
		outs(ANSI_COLOR(1) ">");
		len = 3;
		break;
	    }
	}

	outc(ch);
    }

#ifdef HAVE_DECLARE
    if (square || len >= 3)	/* Thor.980508: �ܦ��٭�� */
#else
	if (len >= 3)
#endif
	    outs("\033[m");

    outc('\n');
}


/* ----------------------------------------------------- */
/* �ݪO�\���						 */
/* ----------------------------------------------------- */


static int post_body();
static int post_head();


static int
post_init(xo)
    XO *xo;
{
#ifdef HAVE_BRD_EXPLAIN
    XO_TALL = b_lines - 3;
#endif
    xo_load(xo, sizeof(HDR));
    return post_head(xo);
}


static int
post_load(xo)
    XO *xo;
{
    xo_load(xo, sizeof(HDR));
    return post_body(xo);
}


static int
post_attr(hdr)
    HDR *hdr;
{
    int mode, attr;
    int unread, later;

    unread = brh_unread(BMAX(hdr->chrono, hdr->stamp));
    later = !brh_unread(hdr->chrono) && brh_unread(hdr->stamp);
    mode = hdr->xmode;

    /* �ѩ�m����S���\Ū�O���A�ҥH�����wŪ */
    attr = !(mode & POST_BOTTOM) && unread
	? 0 : 0x20;   /* �w�\Ū���p�g�A���\Ū���j�g */

#ifdef HAVE_REFUSEMARK
    if (!chkrestrict(hdr)) // �ݤ��쪺�K��Хܬ��wŪ
	return 'x';
    else if (mode & POST_RESTRICT)
	attr |= 'X';
    else
#endif
    if (mode & POST_WIKI)
	    attr |= 'W';
    else if (mode & POST_DONE)
		attr |= 'S';        /* qazq.030815: ����B�z���аO�� */
    else
#ifdef HAVE_LABELMARK
    if (mode & POST_DELETE)
		    attr |= 'T';
    else
#endif
    if (mode & POST_MARKED)
	attr |= 'M';


    if ((mode & POST_MARKED) && (mode & POST_DONE))
	attr = '!';

    if (!(cuser.ufo & UFO_UNREADCOLOR) && later)
    {
	if (attr)
	    attr = '=';
	else
	    attr = '~';
    }

    if (unread && !attr)
	attr = '+';

    return attr;
}

static char * post_itemcolor(HDR *hdr)
{
    if (cuser.ufo & UFO_UNREADCOLOR && !brh_unread(hdr->chrono) && brh_unread(hdr->stamp) && chkrestrict(hdr)) // �ݪ��쪺�K��~�Хܥ�Ū
	// TODO: �Ҽ{�� cache �קK�����ˬd�i���W��
	return ANSI_COLOR(0;36);
    else if (hdr->xmode & POST_DONE)
	return ANSI_COLOR(1;37);
    else if (hdr->xmode & POST_MARKED)
	return ANSI_COLOR(1;33);
    else if (hdr->xmode & POST_DELETE)
	return ANSI_COLOR(1;32);
    else
	return "";
}

static void
post_item(num, hdr)
    int num;
    HDR *hdr;
{
#ifdef HAVE_SCORE
    char pre[40];

    if (hdr->xmode & POST_BOTTOM)
	strcpy(pre, "   "ANSI_COLOR(1;33)"�� "ANSI_RESET);
    else
	sprintf(pre, "%6d", num);

    prints("%s%c%s%c" ANSI_RESET, pre, tag_char(hdr->chrono), post_itemcolor(hdr), post_attr(hdr));

    if (hdr->xmode & POST_SCORE)
    {
	num = hdr->score;
	if (num <= 99 && num >= -99)         /* qazq.031013: �i�H����"�z"*/
	    prints("\033[1;3%cm%2d\033[m", num ? num > 0 ? '1' : '2' : '3', abs(num));
	else
	    prints("\033[1;3%s\033[m", num >= 0 ? "1m�z" : "2m��");
    }
    else
    {
	outs("  ");
    }

    outc(' ');			/* hrs:�A�֤@��Ӥ��}���ƩM���D */
    hdr_outs(hdr, d_cols + 46, 1);   /* �֤@��ө���� */
#else
    prints("%6d%c%c ", (hdr->xmode & POST_BOTTOM) ? -1 : num, tag_char(hdr->chrono), post_attr(hdr));
    hdr_outs(hdr, d_cols + 47, 1);
#endif
}

static int
post_body(xo)
    XO *xo;
{
    HDR *hdr;
    int num, max, tail;

    max = xo->max;
    if (max <= 0)
    {
	if (bbstate & STAT_POST)
	{
	    if (vans("�n�s�W��ƶ�(Y/N)�H[N] ") == 'y')
		return post_add(xo);
	}
	else
	{
	    vmsg("���ݪO�|�L�峹");
	}
	return XO_QUIT;
    }

    hdr = (HDR *) xo_pool;
    num = xo->top;
    tail = num + XO_TALL;
    if (max > tail)
	max = tail;

    move(3, 0);
    do
    {
	post_item(++num, hdr++);
    } while (num < max);
    clrtobot();

    /* return XO_NONE; */
    return XO_FOOT;	/* itoc.010403: �� b_lines ��W feeter */
}


static int
post_head(xo)
    XO *xo;
{ 
    BRD *brd;
    char string[80];
    int len;
    brd = bshm->bcache + currbno;
    len = b_cols - strlen(brd->title) - strlen(currboard) - 2 * 5;

    sprintf(string, "%-.*s%s", len, currBM, strlen(currBM) < len ? "" : "..");
    vs_head(string, xo->xyz);
    prints(NECKER_POST, money_mode ? "�Z �S" : "�� ��", d_cols, "", bshm->mantime[currbno]);
    return post_body(xo);
}


/* ----------------------------------------------------- */
/* ��Ƥ��s���Gbrowse / history				 */
/* ----------------------------------------------------- */


static int
post_visit(xo)
    XO *xo;
{
    int ans, row, max;
    HDR *hdr;

    ans = vans("�]�w�Ҧ��峹 (U)��Ū (V)�wŪ (W)�e�wŪ�᥼Ū (Q)�����H[Q] ");
    if (ans == 'v' || ans == 'u' || ans == 'w')
    {
	row = xo->top;
	max = xo->max - row + 3;
	if (max > b_lines)
	    max = b_lines;

	hdr = (HDR *) xo_pool + (xo->pos - row);
	/* brh_visit(ans == 'w' ? hdr->chrono : ans == 'u'); */
	/* weiyu.041010: �b�m����W�� w ���������wŪ */
	brh_visit((ans == 'u') ? 1 : (ans == 'w' && !(hdr->xmode & POST_BOTTOM)) ? hdr->chrono : 0);

	hdr = (HDR *) xo_pool;
	return post_body(xo);
    }
    return XO_FOOT;
}

void
post_chrono_history(char * dir, time_t chrono) /* �N chrono �o�� timestamp �[�J brh */
{
    int fd;
    time_t prev, next, this;
    HDR buf;

    if (!brh_unread(chrono)) /* �p�G�w�b brh ���A�N�L�ݰʧ@ */
	return;

    if ((fd = open(dir, O_RDONLY)) >= 0)
    {
	prev = chrono + 1;
	next = chrono - 1;
	while (read(fd, &buf, sizeof(HDR)) == sizeof(HDR))
	{
	    this = BMAX(buf.chrono, buf.stamp);

	    if (chrono - this < chrono - prev)
		prev = this;
	    else if (this - chrono < next - chrono)
		next = this;
	}
	close(fd);

	if (prev > chrono)      /* �S���U�@�g */
	    prev = chrono;
	if (next < chrono)      /* �S���W�@�g */
	    next = chrono;

	brh_add(prev, chrono, next);
    }
}

void
post_history(xo, hdr)          /* �N hdr �o�g�[�J brh */
    XO *xo;
    HDR *hdr;
{
    post_chrono_history(xo->dir, hdr->chrono);
    post_chrono_history(xo->dir, hdr->stamp);
}


#if 0
hrs113355:
    �����ݪO�����D���|�O "����D      .brdname"�A
    �]���q���ݩ��e�M�� '.' �Y�i���O�W�A�Y�S���O�W�H�P��������άO�䤣��]�L���A
    �|�b ptr == hdr->title - 1 �M�䤣�� bno ���ɭԳQ�L�o���C

    ��Φ���k�i�`�ٶ}��Ū�ɮɩүӶO�� I/O
#endif

static int find_board(char * title)
{
    char *ptr;

    for (ptr = title + strlen(title) - 1; ptr >= title; ptr--)
	if (*ptr == '.')
	    break;

    if (ptr == title - 1)
	return -1;

    return brd_bno(ptr + 1);
}


#ifdef HAVE_ALLPOST
void
allpost_history(xo, hdr)
    XO *xo;
    HDR *hdr;
{
    int fd, bno;
    BRD *brd;
    char folder[64];
    HDR old;

    if (strcmp(currboard, BN_ALLPOST))/* �b�@��OŪ���A�]�h All_Post �ХܤwŪ */
    {
	brd_fpath(folder, BN_ALLPOST, fn_dir);
	if ((fd = open(folder, O_RDONLY)) >= 0)
	{
	    lseek(fd, 0, SEEK_SET);
	    while (read(fd, &old, sizeof(HDR)) == sizeof(HDR))
	    {
		if (hdr->chrono == old.xid)
		{
		    if ((bno = find_board(old.title)) >= 0)
		    {
			brd = bshm->bcache + bno;
			if (!strcmp(currboard, brd->brdname))
			{
			    bno = brd_bno(BN_ALLPOST);
			    brd = bshm->bcache + bno;

			    brh_get(brd->bstamp, bno);
			    bno = hdr->chrono;
			    brh_add(bno - 1, bno, bno + 1);

			    /* ��_��ӬݪO */
			    brd = bshm->bcache + currbno;
			    brh_get(brd->bstamp, currbno);
			    break;
			}
		    }
		}
	    }
	    close(fd);
	}
    }
    else                              /* �b All_Post Ū���A�]�h�O�O�ХܤwŪ */
    {
	if ((bno = find_board(hdr->title)) >= 0)
	{
	    brd = bshm->bcache + bno;
	    brh_get(brd->bstamp, bno);
	    bno = hdr->xid;
	    brh_add(bno - 1, bno, bno + 1);

	    /* ��_��ӬݪO */
	    brd = bshm->bcache + currbno;
	    brh_get(brd->bstamp, currbno);
	}
    }
}
#endif

#if defined(HAVE_ALLPOST) || defined(HAVE_LUCKYPOST)
static int
post_jump(xo)       /* �ݪO���D(�q BN_ALLPOST/BN_ALLHIDPOST/BN_LUCKYPOST) */
    XO *xo;
{
    int tmpbno, bno;
    HDR *hdr;
    static int p_jump = 0;  /* �i�J�X�h */
    int cross_board = 0;

#ifdef HAVE_ALLPOST
    if (!strcmp(currboard, BN_ALLPOST) || !strcmp(currboard, BN_ALLHIDPOST))
	cross_board++;
#endif

#ifdef HAVE_LUCKYPOST
    if (!strcmp(currboard, BN_LUCKYPOST))
	cross_board++;
#endif

    if (!cross_board)
	return XO_NONE;
    
    if (p_jump >= 1)  /* �̦h jump �@�h */
	return XO_NONE;

    hdr = (HDR *) xo_pool + (xo->pos - xo->top);
    if (!chkrestrict(hdr))
	return XO_NONE;
    bno = find_board(hdr->title);

    if (bno >= 0 && bno != currbno)    /* �ݪO�s�b && �n����O���ݪO */
    {
	tmpbno = currbno; /* �����W�@�ӬݪO */

	if (!XoPost(bno))      /* �i�H�i�J�~ xover() */
	{
	    p_jump++;
	    xover(XZ_POST);
#ifndef ENHANCED_VISIT
	    time(&brd_visit[currbno]);
#endif
	    p_jump--;

	    XoPost(tmpbno);        /* ���^��Ӫ��ݪO */
	    return XZ_POST;
	}
    }
    return XO_NONE;
}
#endif

static int
post_browse(xo)
    XO *xo;
{
    HDR *hdr;
    int xmode, pos, key;
    char *dir, fpath[64];

    dir = xo->dir;

    for (;;)
    {
	pos = xo->pos;
	hdr = (HDR *) xo_pool + (pos - xo->top);
	xmode = hdr->xmode;

#ifdef HAVE_REFUSEMARK
	if (!chkrestrict(hdr))
	    return XO_NONE;
#endif

	hdr_fpath(fpath, dir, hdr);

	post_history(xo, hdr);
#ifdef HAVE_ALLPOST
	allpost_history(xo, hdr);
#endif

	/* Thor.990204: ���Ҽ{more �Ǧ^�� */
	if ((key = more(fpath, FOOTER_POST)) == -1)
	    break;

	strcpy(currtitle, str_ttl(hdr->title));

re_key:
	switch (xo_getch(xo, key))
	{
	    case XO_BODY:
		continue;

	    case 'y':
	    case 'r':
		if (bbstate & STAT_POST)
		{
		    if ((hdr->xmode & POST_MARKED)&& (hdr->xmode & POST_DONE))
		    {
			vmsg("����w�Q��w�C");
			return XO_HEAD;
		    }

		    if (do_reply(xo, hdr) == XO_INIT)	/* �����\�a post �X�h�F */
			return post_init(xo);
		}
		break;

	    case 'm':
		if ((bbstate & STAT_BOARD) && !(xmode & POST_MARKED | POST_DELETE))
		{
		    /* hdr->xmode = xmode ^ POST_MARKED; */
		    /* �b post_browse �ɬݤ��� m �O���A�ҥH����u�� mark */
		    hdr->xmode = xmode | POST_MARKED;
		    currchrono = hdr->chrono;
		    rec_put(dir, hdr, sizeof(HDR), pos, cmpchrono);
		}
		break;

#ifdef HAVE_SCORE
	    case '%':
		post_score(xo);
		return post_init(xo);
#endif

	    case '/':
		if (vget(b_lines, 0, "�j�M�G", hunt, sizeof(hunt), DOECHO))
		{
		    more(fpath, FOOTER_POST);
		    goto re_key;
		}
		continue;

	    case 'E':
		return post_edit(xo);

	    case 'C':	/* itoc.000515: post_browse �ɥi�s�J�Ȧs�� */
		{
		    FILE *fp;
		    if (fp = tbf_open())
		    { 
			f_suck(fp, fpath); 
			fclose(fp);
		    }
		}
		break;

	    case 'Q':	/* hrs.091011: ��ܤ峹��T */
		p_fileinfobt(xo);
		break;

	    case 's':
		outl(0, ANSI_COLOR(7) "�i��ܬݪO�j" ANSI_RESET);
		if (Select() >= 0)
		    return XO_QUIT;
		break;

	    case 'h':
#ifdef M3_USE_PMORE
		xo_help("pmore");
#else
		xo_help("post");
#endif
		break;
	}
	break;
    }

    return post_head(xo);
}


/* ----------------------------------------------------- */
/* ��ذ�						 */
/* ----------------------------------------------------- */


static int
post_gem(xo)
    XO *xo;
{
    int level;
    char fpath[64], fname[64];

    strcpy(fpath, "gem/");
    strcpy(fpath + 4, xo->dir);

    level = 0;
    if (bbstate & STAT_BOARD)
	level ^= GEM_W_BIT;
    if (HAS_PERM(PERM_SYSOP))
	level ^= GEM_X_BIT;
    if (bbstate & STAT_BM)
	level ^= GEM_M_BIT;

    brd_fpath(fname, currboard, fn_gemnote);
    more(fname, NULL);

    XoGem(fpath, "��ذ�", level);
    return post_init(xo);
}


/* ----------------------------------------------------- */
/* �i�O�e��						 */
/* ----------------------------------------------------- */


static int
post_memo(xo)
    XO *xo;
{
    char fpath[64];

    brd_fpath(fpath, currboard, fn_note);
    /* Thor.990204: ���Ҽ{more �Ǧ^�� */   
    if (more(fpath, NULL) == -1)
    {
	vmsg("���ݪO�|�L�u�i�O�e���v");
	return XO_FOOT;
    }
 
    return post_head(xo);
}


/* ----------------------------------------------------- */
/* �\��Gtag / switch / cross / forward			 */
/* ----------------------------------------------------- */


static int
post_tag(xo)
    XO *xo;
{
    HDR *hdr;
    int tag, pos, cur;

    pos = xo->pos;
    cur = pos - xo->top;
    hdr = (HDR *) xo_pool + cur;

    if (xo->key == XZ_XPOST)
	pos = hdr->xid;

    if (tag = Tagger(hdr->chrono, pos, TAG_TOGGLE))
    {
	move(3 + cur, 0);
	post_item(pos + 1, hdr);
    }

    /* return XO_NONE; */
    return xo->pos + 1 + XO_MOVE; /* lkchu.981201: ���ܤU�@�� */
}


static int
post_switch(xo)
    XO *xo;
{
    int bno;
    BRD *brd;
    char bname[BNLEN + 1];

    if (brd = ask_board(bname, BRD_R_BIT, NULL))
    {
	if ((bno = brd - bshm->bcache) >= 0 && currbno != bno)
	{
	    XoPost(bno);
	    return XZ_POST;
	}
    }
    else
    {
	vmsg(err_bid);
    }
    return post_head(xo);
}


int
post_cross(xo)
    XO *xo;
{
    /* �ӷ��ݪO */
    char *dir, *ptr;
    HDR *hdr, xhdr;

    /* ����h���ݪO */
    int xbno;
    usint xbattr;
    char xboard[BNLEN + 1], xfolder[64];
    HDR xpost;

    int tag, rc, locus, finish;
    int method;		/* 0:������ 1:�q���}�ݪO/��ذ�/�H�c����峹 2:�q���K�ݪO����峹 */
    usint tmpbattr;
    char tmpboard[BNLEN + 1];
    char fpath[64], buf[ANSILINELEN];
    FILE *fpr, *fpw;
    BRD  *brd;

    if (!cuser.userlevel)	/* itoc.000213: �קK guest ����h sysop �O */
	return XO_NONE;

    tag = AskTag("���");
    if (tag < 0)
	return XO_FOOT;

    dir = xo->dir;
    hdr = tag ? &xhdr : (HDR *) xo_pool + (xo->pos - xo->top);    /* lkchu.981201: ������ */

    if (!chkrestrict(hdr))
    {
	vmsg("�[�K�峹���o����C");
	return XO_FOOT;
    }
    if (!ask_board(xboard, BRD_W_BIT, "\n\n\033[1;33m�ЬD��A���ݪO�A��������W�L�T�O�C\033[m\n\n") ||
	    (*dir == 'b' && !strcmp(xboard, currboard)))	/* �H�c�B��ذϤ��i�H�����currboard */
	return XO_HEAD;

    brd = bshm->bcache + currbno;

    if (method && *dir == 'b')    /* �q�ݪO��X�A���ˬd���ݪO�O�_�����K�O */
    {
	if (brd->bmode == BMODE_HIDE || brd->bmode == BMODE_PAL)
	    method = 2;
    }

    xbno = brd_bno(xboard);
    brd = bshm->bcache + xbno;
    xbattr = brd->battr;

    int xmethod = 0;
    /* �ɥ� tmpbattr */
    if (brd->battr & BRD_COSIGN && !(brd_bits[xbno] & BRD_M_BIT))
    {
	vmsg("�z�L�o���v���C");
	return XO_HEAD;
    }

    if (!CheckPostPerm(brd))
    {
	vmsg("�藍�_�A�b�ӬݪO�z�S���o��峹���v�� (�j�gI�i�ݭ���)");
	return XO_HEAD;
    }

    if (brd->bmode == BMODE_HIDE || brd->bmode == BMODE_PAL)
	xmethod = 2;

    /* ��@������ۤv�峹�ɡA�i�H��ܡu�������v */
    method = (HAS_PERM(PERM_ALLBOARD) || (!tag && is_author(hdr))) &&
	(vget(2, 0, "(1)������ (2)����峹�H[1] ", buf, 3, DOECHO) != '2') ? 0 : 1;

    if (!tag)	/* lkchu.981201: �������N���n�@�@�߰� */
    {
	if (method && strncmp(hdr->title, "[���]", 6))
	    sprintf(ve_title, "[���] %.66s", str_ttl(hdr->title));	
	/* �w�� Re:/Fw: �r�˴N�u�n�@�� Fw: */
	else
	    strcpy(ve_title, hdr->title);

	if (!vget(2, 0, "���D�G", ve_title, TTLEN + 1, GCARRY))
	    return XO_HEAD;
    }

#ifdef HAVE_REFUSEMARK    
    rc = vget(2, 0, "(S)�s�� (L)���� (X)�K�� (Q)�����H[Q] ", buf, 3, LCECHO);
    if (rc != 'l' && rc != 's' && rc != 'x')
#else
	rc = vget(2, 0, "(S)�s�� (L)���� (Q)�����H[Q] ", buf, 3, LCECHO);
    if (rc != 'l' && rc != 's')
#endif
	return XO_HEAD;

    /* Thor.990111: �b�i�H��X�e�A�n�ˬd���S����X���v�O? */
    if ((rc == 's') && (!HAS_PERM(PERM_INTERNET) || (xbattr & BRD_NOTRAN)))
	rc = 'l';

    /* �ƥ� currboard */
    if (method)
    {
	/* itoc.030325: �@������I�s ve_header�A�|�ϥΨ� currboard�Bcurrbattr�A���ƥ��_�� */
	strcpy(tmpboard, currboard);
	strcpy(currboard, xboard);
	tmpbattr = currbattr;
	currbattr = xbattr;
    }

    locus = 0;
    do	/* lkchu.981201: ������ */
    {
	if (tag)
	{
	    EnumTag(hdr, dir, locus, sizeof(HDR));

	    if (method)
		sprintf(ve_title, "Fw: %.68s", str_ttl(hdr->title));	/* �w�� Re:/Fw: �r�˴N�u�n�@�� Fw: */
	    else
		strcpy(ve_title, hdr->title);
	}

	if (hdr->xmode & GEM_FOLDER)	/* �D plain text ������ */
	    continue;

#ifdef HAVE_REFUSEMARK
	if (hdr->xmode & POST_RESTRICT)
	    continue;
#endif

	hdr_fpath(fpath, dir, hdr);

#ifdef HAVE_DETECT_CROSSPOST
	if (check_crosspost(fpath, xbno))
	    break;
#endif

	brd_fpath(xfolder, xboard, fn_dir);

	if (method)		/* �@����� */
	{
	    /* itoc.030325: �@������n���s�[�W header */
	    fpw = fdopen(hdr_stamp(xfolder, 'A', &xpost, buf), "w");
	    ve_header(fpw);

	    /* itoc.040228: �p�G�O�q��ذ�����X�Ӫ��ܡA�|�������� [currboard] �ݪO�A
	       �M�� currboard �����O�Ӻ�ذϪ��ݪO�C���L���O�ܭ��n�����D�A�ҥH�N���ޤF :p */
	    fprintf(fpw, "�� ��������� [%s] %s\n\n", 
		    *dir == 'u' ? cuser.userid : method == 2 ? "���K" : tmpboard, 
		    *dir == 'u' ? "�H�c" : "�ݪO");

	    /* Kyo.051117: �Y�O�q���K�ݪO��X���峹�A�R���峹�Ĥ@��ҰO�����ݪO�W�� */
	    finish = 0;
	    if ((method == 2) && (fpr = fopen(fpath, "r")))
	    {
		if (fgets(buf, sizeof(buf), fpr) && 
			((ptr = strstr(buf, str_post1)) || (ptr = strstr(buf, str_post2))) && (ptr > buf))
		{
		    ptr[-1] = '\n';
		    *ptr = '\0';

		    do
		    {
			fputs(buf, fpw);
		    } while (fgets(buf, sizeof(buf), fpr));
		    finish = 1;
		}
		fclose(fpr);
	    }
	    if (!finish)
		f_suck(fpw, fpath);

	    fclose(fpw);

	    strcpy(xpost.owner, cuser.userid);
	    strcpy(xpost.nick, cuser.username);
	    if (*dir == 'b' && !(hdr->xmode & POST_COSIGN))
	    {
		fpw = fopen(fpath, "a+");
		fprintf (fpw, CROSS_BANNER, cuser.userid, xmethod == 2 ? "�Y����" : xboard, Now());
		fclose (fpw);
	    }

	}
	else		/* ������ */
	{
	    /* itoc.030325: ���������� copy �Y�i */
	    hdr_stamp(xfolder, HDR_COPY | 'A', &xpost, fpath);

	    strcpy(xpost.owner, hdr->owner);
	    strcpy(xpost.nick, hdr->nick);
	    strcpy(xpost.date, hdr->date);	/* �������O�d���� */
	}

	strcpy(xpost.title, ve_title);

	if (rc == 's')
	    xpost.xmode = POST_OUTGO;
#ifdef HAVE_REFUSEMARK
	else if (rc == 'x')
	    xpost.xmode = POST_RESTRICT;
#endif

	rec_bot(xfolder, &xpost, sizeof(HDR));

	if (rc == 's')
	    outgo_post(&xpost, xboard);
    } while (++locus < tag);

    btime_update(xbno);

    /* Thor.981205: check �Q�઺�O���S���C�J����? */
    if (!(xbattr & BRD_NOCOUNT))
	cuser.numposts += tag ? tag : 1;	/* lkchu.981201: �n�� tag */

    /* �_�� currboard�Bcurrbattr */
    if (method)
    {
	strcpy(currboard, tmpboard);
	currbattr = tmpbattr;
    }

    vmsg("�������(�[�K�峹���|�Q���)");
    return XO_HEAD;
}


int
post_forward(xo)
    XO *xo;
{
    ACCT muser;
    HDR *hdr;

    if (!HAS_PERM(PERM_LOCAL))
	return XO_NONE;

    hdr = (HDR *) xo_pool + (xo->pos - xo->top);

    if (hdr->xmode & GEM_FOLDER)	/* �D plain text ������ */
	return XO_NONE;

#ifdef HAVE_REFUSEMARK
    if (!chkrestrict(hdr))
	return XO_NONE;
#endif

    if (acct_get("��F�H�󵹡G", &muser) > 0)
    {
	strcpy(quote_user, hdr->owner);
	strcpy(quote_nick, hdr->nick);
	hdr_fpath(quote_file, xo->dir, hdr);
	sprintf(ve_title, "%.64s (fwd)", hdr->title);
	move(1, 0);
	clrtobot();
	prints("��F��: %s (%s)\n��  �D: %s\n", muser.userid, muser.username, ve_title);

	mail_send(muser.userid);
	*quote_file = '\0';
    }
    return XO_HEAD;
}


/* ----------------------------------------------------- */
/* �O�D�\��Gmark / delete / label			 */
/* ----------------------------------------------------- */


static int
post_mark(xo)
    XO *xo;
{
    if (bbstate & STAT_BOARD)
    {
	HDR *hdr;
	int pos, cur, xmode;

	pos = xo->pos;
	cur = pos - xo->top;
	hdr = (HDR *) xo_pool + cur;
	xmode = hdr->xmode;

#ifdef HAVE_LABELMARK
	if (xmode & POST_DELETE)	/* �ݬ媺�峹���� mark */
	    return XO_NONE;
#endif

	hdr->xmode = xmode ^ POST_MARKED;
	currchrono = hdr->chrono;
	rec_put(xo->dir, hdr, sizeof(HDR), xo->key == XZ_XPOST ? hdr->xid : pos, cmpchrono);

	move(3 + cur, 0);
	post_item(pos + 1, hdr);
	return xo->pos + 1 + XO_MOVE; /* lkchu.981201: ���ܤU�@�� */

	/* hrs.080203:�O�Dm���~����U�@���A��L�S���v��m���N���n�ʧ@�F */
    }
    return XO_NONE; 
}

static int          /* qazq.030815: ����B�z�����аO��*/
post_done_mark(xo)
    XO *xo;
{
    if ((bbstate & STAT_BM) || HAS_PERM(PERM_SYSOP)) /* hrs.060812:�O�D�N�i�H�аO�F */
    {
	HDR *hdr;
	int pos, cur, xmode;

	pos = xo->pos;
	cur = pos - xo->top;
	hdr = (HDR *) xo_pool + cur;
	xmode = hdr->xmode;

#ifdef HAVE_LABELMARK
	if (xmode & POST_DELETE)    /* �ݬ媺�峹���� mark */
	    return XO_NONE;
#endif

	hdr->xmode = xmode ^ POST_DONE;
	currchrono = hdr->chrono;

	rec_put(xo->dir, hdr, sizeof(HDR), xo->key == XZ_XPOST ?
		hdr->xid : pos, cmpchrono);
	move(3 + cur, 0);
	post_item(pos + 1, hdr);

	return xo->pos + 1 + XO_MOVE; /* lkchu.981201: ���ܤU�@�� */
	/* hrs.080203:�O�Dl���~����U�@���A��L�S���v��m���N���n�ʧ@�F */
    }
    return XO_NONE; 
}

static int
post_wiki(xo)
    XO *xo;
{
    HDR *hdr;
    int pos, cur, xmode;

    if (!cuser.userlevel) /* itoc.020114: guest ������L guest ���峹���� */
	return XO_NONE;

    pos = xo->pos;
    cur = pos - xo->top;
    hdr = (HDR *) xo_pool + cur;

    if (!strcmp(hdr->owner, cuser.userid) || (bbstate & STAT_BOARD))
    {
	xmode = hdr->xmode;

#ifdef HAVE_LABELMARK
	if (xmode & POST_DELETE)    /* �ݬ媺�峹���� wiki */
	    return XO_NONE;
#endif
#ifdef HAVE_REFUSEMARK
	if (hdr->xmode & POST_RESTRICT)
	    return XO_NONE;
#endif

	if (vansf("�T�w�n%s����峹 (����H���i�H�s��) �H [y/N] ", 
		    (hdr->xmode & POST_WIKI) ? "����" : "�аO��" ) != 'y')
	    return XO_FOOT;

	hdr->xmode = xmode ^ POST_WIKI;
	currchrono = hdr->chrono;
	rec_put(xo->dir, hdr, sizeof(HDR), pos, cmpchrono);
	move(3 + cur, 0);
	post_item(pos + 1, hdr);
    }
    return XO_FOOT;
}

static int
post_bottom(xo)
    XO *xo;
{
    if (bbstate & STAT_BOARD)
    {
	int pos, cur;
	HDR *hdr, post;
	char fpath[64];

	pos = xo->pos;
	cur = pos - xo->top;
	hdr = (HDR *) xo_pool + cur;

	if (hdr->xmode & POST_BOTTOM)
	{
	    if (vans("�T�w�n�����m���峹�H [y/N] ") == 'y')
	    {
		currchrono = hdr->chrono;

		rec_del(xo->dir, sizeof(HDR), xo->key == XZ_XPOST ? hdr->xid : pos, cmpchrono);
		return XO_LOAD;
	    }
	    else
		return XO_FOOT;
	}

	if (vans("�T�w�n�m���峹�H [y/N] ") != 'y')
	    return XO_FOOT;

	hdr_fpath(fpath, xo->dir, hdr);
	hdr_stamp(xo->dir, HDR_LINK | 'A', &post, fpath);
#ifdef HAVE_REFUSEMARK
	post.xmode = POST_BOTTOM | (hdr->xmode & POST_RESTRICT);
#else
	post.xmode = POST_BOTTOM;
#endif
	strcpy(post.owner, hdr->owner);
	strcpy(post.nick, hdr->nick);
	strcpy(post.title, hdr->title);

	rec_add(xo->dir, &post, sizeof(HDR));
	/* btime_update(currbno); */	/* ���ݭn�A�]���m���峹���C�J��Ū */

	return post_load(xo);	/* �ߨ���ܸm���峹 */
    }
    return XO_NONE;
}


#ifdef HAVE_REFUSEMARK
static int
post_refuse(xo)		/* itoc.010602: �峹�[�K */
    XO *xo;
{
    HDR *hdr;
    int pos, cur;
    BRD *brd;

    if (!cuser.userlevel)	/* itoc.020114: guest ������L guest ���峹�[�K */
	return XO_NONE;

    pos = xo->pos;
    cur = pos - xo->top;
    hdr = (HDR *) xo_pool + cur;
    brd = bshm->bcache + currbno;

    if (is_author(hdr) || (bbstate & STAT_BM))
    {
	if (!(hdr->xmode & POST_RESTRICT))  /* hrs.090302: �[�K�峹�P�ɧR����O�ƥ� */
	    post_delcrosspost(hdr);
	else /* hrs.090803: �ѱK�ɬ尣�i���W�� */
	{
	    if (vans("�Ѱ��峹�O�K�|�R�������i���W��A�z�T�w��(Y/N)�H[N] ") != 'y')
	    {
		move(3 + cur, 0);
		post_item(pos + 1, hdr);
		return XO_FOOT;
	    }
	    unlink_canseelist(currboard, hdr);
	}

	hdr->xmode ^= POST_RESTRICT;
	currchrono = hdr->chrono;
	rec_put(xo->dir, hdr, sizeof(HDR), xo->key == XZ_XPOST ? hdr->xid : pos, cmpchrono);


	move(3 + cur, 0);
	post_item(pos + 1, hdr);
    }

    return XO_FOOT;
}
#endif


#ifdef HAVE_LABELMARK
static int
post_label(xo)
    XO *xo;
{
    if (bbstate & STAT_BOARD)
    {
	HDR *hdr;
	int pos, cur, xmode;

	pos = xo->pos;
	cur = pos - xo->top;
	hdr = (HDR *) xo_pool + cur;
	xmode = hdr->xmode;

	if (xmode & (POST_MARKED | POST_RESTRICT | POST_DONE | POST_WIKI | POST_BOTTOM))	/* mark �� �[�K �� done ���峹����ݬ� */
	    return XO_NONE;

	hdr->xmode = xmode ^ POST_DELETE;
	currchrono = hdr->chrono;
	rec_put(xo->dir, hdr, sizeof(HDR), xo->key == XZ_XPOST ? hdr->xid : pos, cmpchrono);

	move(3 + cur, 0);
	post_item(pos + 1, hdr);

	return pos + 1 + XO_MOVE;	/* ���ܤU�@�� */
    }

    return XO_NONE;
}

static int
post_delabel(xo)
    XO *xo;
{
    int fdr, fsize, xmode;
    char fnew[64], fold[64], *folder;
    HDR *hdr;
    FILE *fpw;

    if (!(bbstate & STAT_BOARD))
	return XO_NONE;

    if (vans("�T�w�n�R���ݬ�峹��(Y/N)�H[N] ") != 'y')
	return XO_FOOT;

    folder = xo->dir;
    if ((fdr = open(folder, O_RDONLY)) < 0)
	return XO_FOOT;

    if (!(fpw = f_new(folder, fnew)))
    {
	close(fdr);
	return XO_FOOT;
    }

    fsize = 0;
    mgets(-1);
    while (hdr = mread(fdr, sizeof(HDR)))
    {
	xmode = hdr->xmode;

	if (!(xmode & POST_DELETE))
	{
	    if ((fwrite(hdr, sizeof(HDR), 1, fpw) != 1))
	    {
		close(fdr);
		fclose(fpw);
		unlink(fnew);
		return XO_FOOT;
	    }
	    fsize++;
	}
	else
	{
	    /* �s�u��H */
	    cancel_post(hdr);

	    /* hrs.090302: �R����O�ƥ� */
	    post_delcrosspost(hdr);

#ifdef HAVE_REFUSEMARK
	    /* hrs.090803: �R���i���W�� */
	    unlink_canseelist(currboard, hdr);
#endif

	    /* �R����� */
	    hdr_fpath(fold, folder, hdr);
	    unlink(fold);
	}
    }
    close(fdr);
    fclose(fpw);

    sprintf(fold, "%s.o", folder);
    rename(folder, fold);
    if (fsize)
	rename(fnew, folder);
    else
	unlink(fnew);

    btime_update(currbno);

    return post_load(xo);
}
#endif


static int
post_delete(xo)
    XO *xo;
{
    int pos, cur, by_BM;
    HDR *hdr;

    if (!cuser.userlevel ||
	    !strcmp(currboard, BN_DELETED) ||
	    !strcmp(currboard, BN_JUNK))
	return XO_NONE;

    pos = xo->pos;
    cur = pos - xo->top;
    hdr = (HDR *) xo_pool + cur;

    if ((hdr->xmode & (POST_MARKED | POST_BOTTOM)) || (!(bbstate & STAT_BOARD) && !is_author(hdr)))
	return XO_NONE;

    if ((hdr->xmode & POST_COSIGN) && !(bbstate & STAT_BOARD))
	return XO_NONE;

    /* hrs.080205:�D�O�D���i�H��s�p�峹 */
    by_BM = !is_author(hdr);

    if (vans(msg_del_ny) == 'y')
    {
	currchrono = hdr->chrono;

	if (!rec_del(xo->dir, sizeof(HDR), xo->key == XZ_XPOST ? hdr->xid : pos, cmpchrono))
	{

	    /* hrs.090302: �R����O�ƥ� */
	    post_delcrosspost(hdr);

	    /* hrs.090803: �R���i���W�� */
#ifdef HAVE_REFUSEMARK
	    unlink_canseelist(currboard, hdr);
#endif
	    pos = move_post(hdr, xo->dir, by_BM);

	    if (!by_BM && !(currbattr & BRD_NOCOUNT) && !(hdr->xmode & POST_BOTTOM) && hdr->money)
	    {
		pos = (cuser.money > pos ? pos : cuser.money);

		/* itoc.010830.����: �|�}: �Y multi-login �夣��t�@������ */
		cuser.money -= pos;

		if (cuser.numposts > 0)
		    cuser.numposts--;
		vmsgf("%s�A�z���峹� %d �g�Aú�ǲM��O %d ��", MSG_DEL_OK, cuser.numposts, pos);
	    }

	    if (xo->key == XZ_XPOST)
	    {
		vmsg("��C��g�R����V�áA�Э��i�걵�Ҧ��I");
		return XO_QUIT;
	    }
	    return XO_LOAD;
	}
    }
    return XO_FOOT;
}


static int
chkpost(hdr)
    HDR *hdr;
{
    return (hdr->xmode & (POST_MARKED | POST_BOTTOM));
}


static int
vfypost(hdr, pos)
    HDR *hdr;
    int pos;
{
    return (Tagger(hdr->chrono, pos, TAG_NIN) || chkpost(hdr));
}


static void
delpost(xo, hdr)
    XO *xo;
    HDR *hdr;
{
    char fpath[64];

    cancel_post(hdr);
    hdr_fpath(fpath, xo->dir, hdr);
#ifdef HAVE_REFUSEMARK
    unlink_canseelist(currboard, hdr);
#endif
    unlink(fpath);
}


static int
post_rangedel(xo)
    XO *xo;
{
    if (!(bbstate & STAT_BOARD))
	return XO_NONE;

    btime_update(currbno);

    return xo_rangedel(xo, sizeof(HDR), chkpost, delpost);
}


static int
post_prune(xo)
    XO *xo;
{
    int ret;

    if (!(bbstate & STAT_BOARD))
	return XO_NONE;

    ret = xo_prune(xo, sizeof(HDR), vfypost, delpost);

    btime_update(currbno);

    if (xo->key == XZ_XPOST && ret == XO_LOAD)
    {
	vmsg("��C��g�妸�R����V�áA�Э��i�걵�Ҧ��I");
	return XO_QUIT;
    }

    return ret;
}


static int
post_copy(xo)	   /* itoc.010924: ���N gem_gather */
    XO *xo;
{
    int tag;

    tag = AskTag("�ݪO�峹����");

    if (tag < 0)
	return XO_FOOT;

#ifdef HAVE_REFUSEMARK
    gem_buffer(xo->dir, tag ? NULL : (HDR *) xo_pool + (xo->pos - xo->top), chkrestrict);
#else
    gem_buffer(xo->dir, tag ? NULL : (HDR *) xo_pool + (xo->pos - xo->top), NULL);
#endif

    if (bbstate & STAT_BOARD)
    {
#ifdef XZ_XPOST
	if (xo->key == XZ_XPOST)
	{
	    zmsg("�ɮ׼аO�����C[�`�N] �z���������}�걵�Ҧ��~��i�J��ذϡC");
	    return XO_FOOT;
	}
	else
#endif
	{
	    zmsg("���������C[�`�N] �K�W��~��R�����I");
	    return post_gem(xo);	/* �����������i��ذ� */
	}
    }

    zmsg("�ɮ׼аO�����C[�`�N] �z�u��b���(�p)�O�D�Ҧb�έӤH��ذ϶K�W�C");
    return XO_FOOT;
}


/* ----------------------------------------------------- */
/* �����\��Gedit / title				 */
/* ----------------------------------------------------- */

/* append data from tail of src (starting point=off) to dst */
int
AppendTail(const char *src, const char *dst, int off)
{
    int fi, fo, bytes;
    char buf[8192];

    fi=open(src, O_RDONLY);
    if(fi<0) return -1;

    fo=open(dst, O_WRONLY | O_APPEND | O_CREAT, 0600);
    if(fo<0) {close(fi); return -1;}
    // flock(dst, LOCK_SH);

    if(off > 0)
	lseek(fi, (off_t)off, SEEK_SET);

    while((bytes=read(fi, buf, sizeof(buf)))>0)
    {
	write(fo, buf, bytes);
    }
    // flock(dst, LOCK_UN);
    close(fo);
    close(fi);
    return 0;
}


// return: 0 - ok; otherwise - fail.
static int
hash_partial_file( char *path, size_t sz, unsigned char output[SMHASHLEN] )
{
    int fd;
    size_t n;
    unsigned char buf[1024];

    Fnv64_t fnvseed = FNV1_64_INIT;
    assert(SMHASHLEN == sizeof(fnvseed));

    fd = open(path, O_RDONLY);
    if (fd < 0)
	return 1;

    while(  sz > 0 &&
	    (n = read(fd, buf, sizeof(buf))) > 0 )
    {
	if (n > sz) n = sz;
	fnvseed = fnv_64_buf(buf, (int) n, fnvseed);
	sz -= n;
    }
    close(fd);

    if (sz > 0) // file is different
	return 2;

    memcpy(output, (void*) &fnvseed, sizeof(fnvseed));
    return HASHPF_RET_OK;
}

int
post_edit(xo)
    XO *xo;
{
    char fpath[64], genbuf[64];
    HDR *hdr;
    FILE *fp;
    BRD *brd;

    struct stat st;
    time_t oldmt, newmt;
    off_t oldsz, newsz;
    char SmartMerge = 1, DoRecord = 0, MergeDone = 0;

    brd = bshm->bcache + currbno;
    hdr = (HDR *) xo_pool + (xo->pos - xo->top);

    hdr_fpath(genbuf, xo->dir, hdr);
    sprintf(fpath, "tmp/edit.%s.%ld", cuser.userid, time(NULL));

#ifdef HAVE_REFUSEMARK
    if (!chkrestrict(hdr))
	return XO_NONE;
#endif
 
    if (!cuser.userlevel)       /* hrs.090928: ���� guest �ק�峹 */
        return XO_NONE;

    if ((hdr->xmode & POST_MARKED)&& (hdr->xmode & POST_DONE))
    {
	vmsg("����w�Q��w�C");
	return XO_HEAD;
    }

    /* hrs.080205:���i�H��s�p�峹 */
    if (hdr->xmode & POST_COSIGN && !HAS_PERM(PERM_SYSOP)) 
	return XO_HEAD;

    if (chkoutgo(hdr, brd))
    {
	outl(b_lines - 1, ANSI_COLOR(1;31)"�� ����w�g�o�G�ܷs�D�s�զ��A���A�z���ק�N�L�k�Q��L���x�\\Ū�C "
		ANSI_RESET);
	if (vans("�z�٬O�Ʊ�n�ק�峹�ܡH [Y/n] ") == 'n')
	    return XO_HEAD;
    }

    f_cp(genbuf, fpath, O_APPEND);		/* do Copy */
    stat(fpath, &st);	oldsz = st.st_size;
    stat(genbuf, &st);	oldmt = st.st_mtime;

    while (1)
    { 
	unsigned char oldsum[SMHASHLEN] = {0}, newsum[SMHASHLEN] = {0};

	if (hash_partial_file(fpath, oldsz, oldsum) != HASHPF_RET_OK)
	    SmartMerge = 0;

	if (HAS_PERM(PERM_ALLBOARD))			/* �����ק� */
	{ 
	    if(!vedit(fpath, 0))
	    {
		if(vans("�O�_�d�U�ק�峹�O���H[Y/n] ") != 'n')
		    DoRecord = 1;
	    }
	    else
		return XO_HEAD;
	}
	else if (is_author(hdr) && CheckEditPerm(brd) || (bbstate & STAT_BOARD)
		|| hdr->xmode & POST_WIKI)		/* ��@�̭ק� */
	{
	    if (!vedit(fpath, 0))	/* �Y�D�����h�[�W�ק��T */
	    {
		DoRecord = 1;
	    }
	    else
		return XO_HEAD;
	}
	else		/* itoc.010301: ���ѨϥΪ̭ק�(�������x�s)��L�H�o���峹 */
	{
#ifdef HAVE_REFUSEMARK
	    if (hdr->xmode & POST_RESTRICT)
		return XO_NONE;
#endif
	    vedit(fpath, -1);
	    return XO_HEAD;
	}

	stat(genbuf, &st); newmt = st.st_mtime; newsz = st.st_size;
	if (SmartMerge && newmt == oldmt || newsz < oldsz)
	    SmartMerge = 0;

	if (SmartMerge && hash_partial_file(genbuf, oldsz, newsum) != HASHPF_RET_OK)
	    SmartMerge = 0;

	if (SmartMerge && memcmp(oldsum, newsum, sizeof(newsum)) != 0)
	    SmartMerge = 0;

	if (SmartMerge)
	{
	    SmartMerge = 0; 

	    clear();
	    move(b_lines-7, 0);
	    outs(ANSI_COLOR(1;33) "�� �ɮפw�Q�ק�L! ��" ANSI_RESET "\n\n");
	    outs("�i��۰ʦX�� [Smart Merge]...\n");

	    if (AppendTail(genbuf, fpath, oldsz) == 0)
	    {

		oldmt = newmt;
		outs(ANSI_COLOR(1)
			"�X�֦��\\�A�s�ק�(�α���)�w�[�J�z���峹���C\n"
			ANSI_RESET "\n");
		MergeDone = 1;
	//	vmsg("�X�֧���");
	    } 
	    else 
	    {
		outs(ANSI_COLOR(31)
			"�۰ʦX�֥��ѡC �Ч�ΤH�u��ʽs��X�֡C" ANSI_RESET);
		vmsg("�X�֥���");
	    }
	}
	if (oldmt != newmt)
	{
	    int c = 0;

	    clear();
	    move(b_lines-7, 0);
	    outs(ANSI_COLOR(1;31) "�� �ɮפw�Q�ק�L! ��" ANSI_RESET "\n\n");

	    outs("�i��O�z�b�s�誺�L�{�����H�i�����έפ�C\n"
		    "�z�i�H��ܪ����л\\�ɮ�(y)�B���(n)�A\n"
		    " �άO" ANSI_COLOR(1)"���s�s��" ANSI_RESET
		    "(�s��|�Q�K���s���ɮ׫᭱)(e)�C\n");
	    c = vans("�n�����л\\�ɮ�/����/���s��? [Y/n/e]");

	    if (c == 'n')
		return XO_HEAD;

	    if (c == 'e')
	    {
		FILE *fp, *src;

		/* merge new and old stuff */
		fp = fopen(fpath, "at");
		src = fopen(genbuf, "rt");

		if(!fp)
		{
		    vmsg("��p�A�ɮפw�l���C");
		    if(src) fclose(src);
		    unlink(fpath); // fpath is a temp file
		    return XO_HEAD;
		}

		if(src)
		{
		    int c = 0;

		    fprintf(fp, MSG_SEPERATOR "\n");
		    fprintf(fp, "�H�U���Q�ק�L���̷s���e: ");
		    fprintf(fp,
			    " (%s)\n",
			    Btime(&newmt));
		    fprintf(fp, MSG_SEPERATOR "\n");
		    while ((c = fgetc(src)) >= 0)
			fputc(c, fp);
		    fclose(src);

		    // update oldsz, old mt records
		    stat(genbuf, &st); oldmt = st.st_mtime; oldsz = st.st_size;
		}
		fclose(fp);
		continue;
	    }
	}
	

	// let's do something instead to keep the link with hard-linking posts
	{
	    FILE *fp = fopen(genbuf, "w");
	    f_suck(fp, fpath);
	    fclose(fp);
	    break;
	
	    //f_mv(fpath, genbuf);
	    //break;
	}
    } 

    if (DoRecord)
	if (fp = fopen(genbuf, "a"))
	{
	    ve_banner(fp, 1);
	    fclose(fp);
	}

    change_stamp(xo->dir, hdr);
    currchrono = hdr->chrono;
    rec_put(xo->dir, hdr, sizeof(HDR), xo->pos, cmpchrono);
    post_history(xo, hdr);
#ifdef HAVE_ALLPOST
    allpost_history(xo, hdr);
#endif

    btime_update(currbno);

    if (hdr->xmode & POST_WIKI)
    {
	/* todo WIKI�M�� */
	/* bluesway.070323: ��fpath �Ӧs��ذϸ��| */
	gem_fpath(fpath, currboard, fn_dir);
	gem_log(fpath, "Wiki", hdr);
    }

    if (MergeDone)
	vmsg("�X�֧���");

    return XO_HEAD;	/* itoc.021226: XZ_POST �M XZ_XPOST �@�� post_edit() */
}


void
header_replace(xo, hdr)		/* itoc.010709: �ק�峹���D���K�ק鷺�媺���D */
    XO *xo;
    HDR *hdr;
{
    FILE *fpr, *fpw;
    char srcfile[64], tmpfile[64], buf[ANSILINELEN];

    hdr_fpath(srcfile, xo->dir, hdr);
    strcpy(tmpfile, "tmp/");
    strcat(tmpfile, hdr->xname);
    f_cp(srcfile, tmpfile, O_TRUNC);

    if (!(fpr = fopen(tmpfile, "r")))
	return;

    if (!(fpw = fopen(srcfile, "w")))
    {
	fclose(fpr);
	return;
    }

    fgets(buf, sizeof(buf), fpr);		/* �[�J�@�� */
    fputs(buf, fpw);

    fgets(buf, sizeof(buf), fpr);		/* �[�J���D */
    if (!str_ncmp(buf, "��", 2))		/* �p�G�� header �~�� */
    {
	strcpy(buf, buf[2] == ' ' ? "��  �D: " : "���D: ");
	strcat(buf, hdr->title);
	strcat(buf, "\n");
    }
    fputs(buf, fpw);

    while(fgets(buf, sizeof(buf), fpr))	/* �[�J��L */
	fputs(buf, fpw);

    fclose(fpr);
    fclose(fpw);
    f_rm(tmpfile);
}


static int
post_title(xo)
    XO *xo;
{
    HDR *fhdr, mhdr;
    int pos, cur;

    if (!cuser.userlevel)	/* itoc.000213: �קK guest �b sysop �O����D */
	return XO_NONE;

    pos = xo->pos;
    cur = pos - xo->top;
    fhdr = (HDR *) xo_pool + cur;
    memcpy(&mhdr, fhdr, sizeof(HDR));

    if (!chkrestrict(&mhdr))
	return XO_NONE;

    if (!is_author(&mhdr) && !(bbstate & STAT_BOARD))
	return XO_NONE;

    vget(b_lines, 0, "���D�G", mhdr.title, TTLEN + 1, GCARRY);

    if (HAS_PERM(PERM_ALLBOARD))  /* itoc.000213: ��@�̥u�����D */
    {
	vget(b_lines, 0, "�@�̡G", mhdr.owner, 73 /* sizeof(mhdr.owner) */, GCARRY);
	/* Thor.980727: sizeof(mhdr.owner) = 80 �|�W�L�@�� */
	vget(b_lines, 0, "�ʺ١G", mhdr.nick, sizeof(mhdr.nick), GCARRY);
	vget(b_lines, 0, "����G", mhdr.date, sizeof(mhdr.date), GCARRY);
    }

    if (memcmp(fhdr, &mhdr, sizeof(HDR)) && vans(msg_sure_ny) == 'y')
    {
	memcpy(fhdr, &mhdr, sizeof(HDR));
	currchrono = fhdr->chrono;
	rec_put(xo->dir, fhdr, sizeof(HDR), xo->key == XZ_XPOST ? fhdr->xid : pos, cmpchrono);

	move(3 + cur, 0);
	post_item(++pos, fhdr);

	/* itoc.010709: �ק�峹���D���K�ק鷺�媺���D */
	/* hrs.080205: �s�p�峹�h�����F */
	if (!(fhdr->xmode & POST_COSIGN))
	    header_replace(xo, fhdr);
    }
    return XO_FOOT;
}


/* ----------------------------------------------------- */
/* �B�~�\��Gwrite / score				 */
/* ----------------------------------------------------- */


int
post_write(xo)			/* itoc.010328: ��u�W�@�̤e�w */
    XO *xo;
{
    if (HAS_PERM(PERM_PAGE))
    {
	HDR *hdr;
	UTMP *up;

	hdr = (HDR *) xo_pool + (xo->pos - xo->top);

	if (!(hdr->xmode & POST_INCOME) && (up = utmp_seek(hdr)))
	    do_write(up);
    }
    return XO_NONE;
}


#ifdef HAVE_SCORE

static int curraddscore;

static void
addscore(hdd, ram)
    HDR *hdd, *ram;
{
    hdd->xmode |= POST_SCORE;
    hdd->stamp = ram->stamp;

    if (curraddscore > 0)
    {
	if (hdd->score <= 99)
	    hdd->score++;
    }
    else if (curraddscore < 0)
    {
	if (hdd->score >= -99)
	    hdd->score--;
    }
}

int
post_score(xo)
    XO *xo;
{
static time_t next = 0;   /* �U���i�����ɶ� */
    HDR *hdr;
    int pos, cur, ans, vtlen, maxlen;
    char *dir, userid[IDLEN + 1], *verb, fpath[64], reason[80], vtbuf[12], prompt[256], msg[80];
    FILE *fp;
#ifdef HAVE_ANONYMOUS
    char uid[IDLEN + 1];
#endif
    int msgpos;		    /* hrs.080103:�T����m */
    BRD *brd;

    brd = bshm->bcache + currbno;

    if (brd->battr & BRD_NOSCORE)
    {
	vmsg("���ݪO���o�����C");
	return XO_FOOT;
    }

    if (!cuser.userlevel || !(bbstate & STAT_POST))
    {
	vmsg("�v�������C");
	return XO_FOOT;
    }

    if (!CheckPostPerm(brd))
    {
	vmsg("�藍�_�A�z�S���b�����ˤ峹���v�� (�j�gI�i�ݭ���)");
	return XO_FOOT;
    }

    pos = xo->pos;
    cur = pos - xo->top;
    hdr = (HDR *) xo_pool + cur;

    if ((hdr->xmode & POST_MARKED)&& (hdr->xmode & POST_DONE))
    {
	vmsg("����w�Q��w�C");
	return XO_HEAD;
    }

    if (hdr->xmode & POST_COSIGN)
	return XO_NONE;
    /* hrs.080205:�s�p�峹������ */

#ifdef HAVE_REFUSEMARK
    if (!chkrestrict(hdr))
	return XO_NONE;
#endif

    utmp_mode(M_SCORE);
    if (next < time(NULL))
    {
	switch (ans = vbar(brd->battr & BRD_NOBOO ?
		    ANSI_COLOR(1)  "�� ���� "
		    ANSI_COLOR(33) "0)���� "
		    ANSI_COLOR(31) "1)���� "
		    ANSI_COLOR(31) "3)�ۭq�� "
		    ANSI_RESET "[1]? ":

		    ANSI_COLOR(1)  "�� ���� "
		    ANSI_COLOR(33) "0)���� "
		    ANSI_COLOR(31) "1)���� "
		    ANSI_COLOR(32) "2)��� "
		    ANSI_COLOR(31) "3)�ۭq�� "
		    ANSI_COLOR(32) "4)�ۭq�A "
		    ANSI_RESET "[1]? "))
	{
	    case '0':
		verb = "3m��";
		vtlen = 2;
		break;

	    default:
	    case '1':
		ans = '1';
		verb = "1m��";
		vtlen = 2;
		break;

	    case '2':
		if (brd->battr & BRD_NOBOO)
		    return XO_FOOT;
		verb = "2m�A";
		vtlen = 2;
		break;

	    case '3':
	    case '4':
		if (brd->battr & BRD_NOBOO && ans == '4')
		    return XO_FOOT;
		if (!vget(b_lines, 0, "�п�J�ʵ��G", fpath, 7, DOECHO))
		    return XO_FOOT;
		vtlen = strlen(fpath);
		sprintf(verb = vtbuf, "%cm%s", ans - 2, fpath);
		break;
	}
    }
    else{
	verb = "3m��";
	vtlen = 2;
	ans= '9' ; //magic number
    }

#ifdef HAVE_ANONYMOUS
    if (brd->battr & BRD_ANONYMOUS)
    {
	strcpy(userid, uid);
	if (!vget(b_lines, 0, "�п�J�z�Q�Ϊ�ID�A�]�i������[Enter]�A�άO��[r]�ίu�W�G", userid, IDLEN,
		    DOECHO))
	    strcpy(userid, STR_ANONYMOUS);
	else if (userid[0] == 'r' && userid[1] == '\0')
	    strcpy(userid, cuser.userid);
	else
	    strcat(userid, ".");      /* �۩w���ܡA�̫�[ '.' */
    }
    else
#endif
	strcpy(userid, cuser.userid);

    maxlen = 64 - strlen(userid) - vtlen;

    if (brd->battr & BRD_SCOREALIGN)
    {
	sprintf(prompt, "�� " ANSI_COLOR(1;33) "%-*s " ANSI_COLOR(1;3%c) "%*s" ANSI_RESET ":",
		                     IDLEN, userid, *verb, 6, verb + 2);
	maxlen -= IDLEN - strlen(userid) + 6 - (strlen(verb) - 2);
    }
    else
	sprintf(prompt,"�� " ANSI_COLOR(1;33) "%s \033[1;3%s" ANSI_RESET ":", userid, verb);

    msgpos = b_lines - 1;

    if(ans=='9')
    {
	sprintf(msg, ANSI_COLOR(1;33) "�� ���嶡�j�ӱ���(�٦� %ld ��)�A��Ρu���v�G " ANSI_RESET
		, next - time(NULL) > 0 ? next - time(NULL) : 0);
	outl(msgpos--, msg); 
    }

    if(strstr(hdr->owner,"@"))
	outl(msgpos--, ANSI_COLOR(1;31) "�� �o�g�峹�Ӧۥ~����H�O�A��@�̥i��L�k�ݨ����C" ANSI_RESET); 
    if (chkoutgo(hdr, brd))
	outl(msgpos--, ANSI_COLOR(1;31) "�� ����w�g�o�G�ܷs�D�s�զ��A���A�z������N�L�k�Q��L���x�\\Ū�C" ANSI_RESET);

    if (!vget(b_lines, 0, prompt, reason, maxlen, DOECHO | CTECHO))
	return XO_HEAD;

    while(*reason == '1' && !*(reason + 1) && ans=='9')
    {

	outl(b_lines - 1, ANSI_COLOR(1;31) "�� �ѩ���������j�ӱ����Ρu���v�A�z�i�ण�p�߫���" ANSI_RESET); 
	if (vans("�����F? [Y/n] ") == 'n')
	    break;
	vget(b_lines, 0, prompt, reason, maxlen, DOECHO | CTECHO);
	if(!*reason)
	    return XO_HEAD;
    }

    dir = xo->dir;
    hdr_fpath(fpath, dir, hdr);

    if (fp = fopen(fpath, "a"))
    {
	time_t now;
	struct tm *ptime;

	time(&now);
	ptime = localtime(&now);

	if (brd->battr & BRD_SCOREALIGN)
	    fprintf(fp, "�� " ANSI_COLOR(1;33) "%-*s " ANSI_COLOR(1;3%c) "%*s" ANSI_RESET,
		    IDLEN, userid, *verb, 6, verb + 2);
	else
	    fprintf(fp, "�� " ANSI_COLOR(1;33) "%s \033[1;3%s" ANSI_RESET, userid, verb);

	fprintf(fp, ":%-*s" ANSI_COLOR(1;30) "%02d%02d %02d:%02d" ANSI_RESET "\n",
		maxlen, reason, ptime->tm_mon + 1, ptime->tm_mday, ptime->tm_hour, ptime->tm_min);
	fclose(fp);
    }

#ifdef HAVE_ANONYMOUS
    if (currbattr & BRD_ANONYMOUS && strcmp(userid, cuser.userid) != 0)
    {
	char buf[256];
	sprintf(buf, "%s %-13s(%s)\n%-13s %s ����:%s\n",
	    Now(), cuser.userid, str_ip(cutmp->in_addr), currboard, hdr->xname, reason);
	f_cat(FN_RUN_ANONYMOUS, buf);
    }
#endif

    ans -= '0';
    curraddscore = 0;
    if (ans == 0 || ans == 9)     /* ���� */
    {
	curraddscore = 0;
    }
    else if (ans == 1 || ans == 3)     /* �[�� */
    {
	if (hdr->score <= 99)
	    curraddscore = 1;
    }
    else if (ans == 2 || ans == 4)    /* ���� */
    {
	if (hdr->score >= -99)
	    curraddscore = -1;
    }

    change_stamp(xo->dir, hdr);
    currchrono = hdr->chrono;
    rec_ref(dir, hdr, sizeof(HDR), xo->key == XZ_XPOST ?
	    hdr->xid : pos, cmpchrono, addscore);
    post_history(xo, hdr);
#ifdef HAVE_ALLPOST
    allpost_history(xo, hdr);
#endif

    btime_update(currbno);

    if (brd_bits[currbno] & BRD_M_BIT)
	next = time(NULL) + 10;  /* �O�D�C 10 ���i�����@�� */
    else
	next = time(NULL) + 60;  /* �C 60 ���i�����@�� */

    return XO_LOAD;
}
#endif

#ifdef HAVE_SCORE
static int
post_resetscore(xo)
    XO *xo;
{
    if (bbstate & STAT_BOARD)
    {
	HDR *hdr;
	int pos, cur, xmode;

	if (vans("�T�w�n�N�o�g�峹��������k�s�ܡH [y/N] ") != 'y')
	    return XO_FOOT;

	pos = xo->pos;
	cur = pos - xo->top;
	hdr = (HDR *) xo_pool + cur;
	xmode = hdr->xmode;

	hdr->xmode = xmode & ~POST_SCORE;
	hdr->score = 0;
	currchrono = hdr->chrono;

	rec_put(xo->dir, hdr, sizeof(HDR), xo->key == XZ_XPOST ?
		hdr->xid : pos, cmpchrono);
	move(3 + cur, 0);
	post_item(pos + 1, hdr);

    }
    return XO_FOOT;
}
#endif

static int
post_help(xo)
    XO *xo;
{
    xo_help("post");
    /* return post_head(xo); */
    return XO_HEAD;		/* itoc.001029: �P xpost_help �@�� */
}


KeyFunc post_cb[] =
{
    XO_INIT, post_init,
    XO_LOAD, post_load,
    XO_HEAD, post_head,
    XO_BODY, post_body,

    'r', post_browse,
    's', post_switch,
    KEY_TAB, post_gem,
    'z', post_gem,

    'y', post_reply,
    'd', post_delete,
    'v', post_visit,
    'x', post_cross,		/* �b post/mbox �����O�p�g x ��ݪO�A�j�g X ��ϥΪ� */
    'X', post_forward,
    't', post_tag,
    'E', post_edit,
    'T', post_title,
    'm', post_mark,		/* mark m */
    'l', post_done_mark,    	/* mark s */
    'W', post_wiki,		/* mark w */
    '_', post_bottom,
    '*', post_bottom,
    'D', post_rangedel,
#if defined(HAVE_ALLPOST) || defined(HAVE_LUCKYPOST)
    'J', post_jump,     /* �ݪO���D(�q BN_ALLPOST/BN_ALLHIDPOST/BN_LUCKYPOST) */
#endif
    'Q', p_fileinfo,
    'I', p_brdinfo,
    'o', post_canseelist,

#ifdef HAVE_SCORE
    '%', post_score,
    'i', post_resetscore,
#endif

    'w', post_write,

    'b', post_memo,
    'c', post_copy,
    'g', gem_gather,

    Ctrl('P'), post_add,
    Ctrl('D'), post_prune,
    Ctrl('Q'), xo_uquery,
    Ctrl('O'), xo_usetup,

#ifdef HAVE_REFUSEMARK
    Ctrl('Y'), post_refuse,
#endif

#ifdef HAVE_LABELMARK
    'n', post_label,
    Ctrl('N'), post_delabel,
#endif

    'B' | XO_DL, (void *) "bin/manage.so:post_manage",
    'R' | XO_DL, (void *) "bin/vote.so:vote_result",
    'V' | XO_DL, (void *) "bin/vote.so:XoVote",

#ifdef HAVE_TERMINATOR
    Ctrl('X') | XO_DL, (void *) "bin/manage.so:post_terminator",
#endif

    '~', XoXselect,		/* itoc.001220: �j�M�@��/���D */
    'S', XoXsearch,		/* itoc.001220: �j�M�ۦP���D�峹 */
    'a', XoXauthor,		/* itoc.001220: �j�M�@�� */
    '/', XoXtitle,		/* itoc.001220: �j�M���D */
    'f', XoXfull,			/* itoc.030608: ����j�M */
    'G', XoXmark,			/* itoc.010325: �j�M mark �峹 */
    'L', XoXlocal,		/* itoc.010822: �j�M���a�峹 */
    '#', XoXfilename,		/* hrs: �j�M�峹�N�X(�ɦW) */
    'Z', XoXscore,		/* hrs.080419: �j�M���� */
#ifdef HAVE_XYNEWS
    'u', XoNews,			/* itoc.010822: �s�D�\Ū�Ҧ� */
#endif

    'h', post_help
};


KeyFunc xpost_cb[] =
{
    XO_INIT, xpost_init,
    XO_LOAD, xpost_load,
    XO_HEAD, xpost_head,
    XO_BODY, post_body,		/* Thor.980911: �@�ΧY�i */

    'r', xpost_browse,
    'y', post_reply,
    't', post_tag,
    'x', post_cross,
    'X', post_forward,
    'c', post_copy,
    'g', gem_gather,
    'm', post_mark,               /* mark m */
    'l', post_done_mark,          /* mark s */
    'W', post_wiki,               /* mark w */
    'd', post_delete,		/* Thor.980911: ��K�O�D */
    'E', post_edit,		/* itoc.010716: ���� XPOST ���i�H�s����D�B�峹�A�[�K */
    'T', post_title,
#if defined(HAVE_ALLPOST) || defined(HAVE_LUCKYPOST)
    'J', post_jump,     /* �ݪO���D(�q BN_ALLPOST/BN_ALLHIDPOST/BN_LUCKYPOST) */
#endif
    'Q', p_fileinfo,
    'I', p_brdinfo,
    'o', post_canseelist,

#ifdef HAVE_SCORE
    '%', post_score,
    'i', post_resetscore,
#endif
    'w', post_write,
#ifdef HAVE_REFUSEMARK
    Ctrl('Y'), post_refuse,
#endif
#ifdef HAVE_LABELMARK
    'n', post_label,
#endif

    '~', XoXselect,
    'S', XoXsearch,
    'a', XoXauthor,
    '/', XoXtitle,
    'f', XoXfull,
    'G', XoXmark,
    'L', XoXlocal,
    '#', XoXfilename,
    'Z', XoXscore,                /* hrs.080419: �j�M���� */

    Ctrl('P'), post_add,
    Ctrl('D'), post_prune,
    Ctrl('Q'), xo_uquery,
    Ctrl('O'), xo_usetup,

    'h', post_help		/* itoc.030511: �@�ΧY�i */
};


#ifdef HAVE_XYNEWS
KeyFunc news_cb[] =
{
    XO_INIT, news_init,
    XO_LOAD, news_load,
    XO_HEAD, news_head,
    XO_BODY, post_body,

    'r', XoXsearch,
    '#', XoXfilename,

    'h', post_help		/* itoc.030511: �@�ΧY�i */
};
#endif	/* HAVE_XYNEWS */
