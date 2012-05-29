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


extern int wordsnum;		/* itoc.010408: 計算文章字數 */
extern int TagNum;
extern char xo_pool[];
extern char brd_bits[];


#ifdef HAVE_ANONYMOUS
extern char anonymousid[];	/* itoc.010717: 自定匿名 ID */
#endif

static void
change_stamp(folder, hdr)
  char *folder;
  HDR *hdr;
{
  /* hrs.100104: 還是用 hdr_stamp 去確定 stamp 的唯一性 */
  HDR buf;

  /* 為了確定新造出來的 stamp 也是 unique (不和既有的 chrono 重覆)，
     就產生一個新的檔案，該檔案隨便 link 即可。
     這個多產生出來的垃圾會在 expire 被 sync 掉 (因為不在 .DIR 中) */
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
/* 改良 innbbsd 轉出信件、連線砍信之處理程序		 */
/* ----------------------------------------------------- */


void
btime_update(bno)
  int bno;
{
  if (bno >= 0)
    (bshm->bcache + bno)->btime = -1;	/* 讓 class_item() 更新用 */
}


#ifndef HAVE_NETTOOL
static 			/* 給 enews.c 用 */
#endif
void
outgo_post(hdr, board)
  HDR *hdr;
  char *board;
{
  bntp_t bntp;

  memset(&bntp, 0, sizeof(bntp_t));

  if (board)		/* 新信 */
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
  if ((hdr->xmode & POST_OUTGO) &&		/* 外轉信件 */
    (hdr->chrono > ap_start - 7 * 86400))	/* 7 天之內有效 */
  {
    outgo_post(hdr, NULL);
  }
}


static inline int		/* 回傳文章 size 去扣錢 */
move_post(hdr, folder, by_bm)	/* 將 hdr 從 folder 搬到別的板 */
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
				/* 置底文被砍不用 move_post */
  {				/* hrs.071220: 隱板文章不move post */	
				/* hrs.080103: 密文就不丟進來啦    */

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

    /* 直接複製 trailing data：owner(含)以下所有欄位 */

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
/* 刪除跨板文章備份 (ALLPOST)                            */
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
	if (nhdr->xid == hdr->chrono) // 跨板文章的 xid 與原文 chrono 相同
	{
	    hdr_fpath(fpath, folder, nhdr);
	    unlink(fpath);

	    strcpy(currxname, nhdr->xname);
	    rec_del(folder, sizeof(HDR), i, cmpxname);

	    break; // 假設只會有一篇 (通常也是)
	}
    }

    munmap(fimage, fsize);
    btime_update(brd_bno(brdname)); // 更新看板閱讀紀錄
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
/* 改良 cross post 停權					 */
/* ----------------------------------------------------- */


#define MAX_CHECKSUM_POST	20	/* 記錄最近 20 篇文章的 checksum */
#define MAX_CHECKSUM_LINE	6	/* 只取文章前 6 行來算 checksum */


typedef struct
{
  int sum;			/* 文章的 checksum */
  int total;			/* 此文章已發表幾篇 */
}      CHECKSUM;


static CHECKSUM checksum[MAX_CHECKSUM_POST];
static int checknum = 0;


static inline int
checksum_add(str)		/* 回傳本列文字的 checksum */
  char *str;
{
  int i, len, sum;

  len = strlen(str);

  sum = len;	/* 當字數太少時，前四分之一很可能完全相同，所以將字數也加入 sum 值 */
  for (i = len >> 2; i > 0; i--)	/* 只算前四分之一字元的 sum 值 */
    sum += *str++;

  return sum;
}


static inline int		/* 1:是cross-post 0:不是cross-post */
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


static int			/* 1:是cross-post 0:不是cross-post */
checksum_find(fpath)
  char *fpath;
{
  int i, sum;
  char buf[ANSILINELEN];
  FILE *fp;

  sum = 0;
  if (fp = fopen(fpath, "r"))
  {
    for (i = -(LINE_HEADER + 1);;)	/* 前幾列是檔頭 */
    {
      if (!fgets(buf, ANSILINELEN, fp))
	break;

      if (i < 0)	/* 跳過檔頭 */
      {
	i++;
	continue;
      }

      if (*buf == QUOTE_CHAR1 || *buf == '\n' || !strncmp(buf, "※", 2))	 /* 跳過引言 */
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
  int bno;			/* 要轉去的看板 */
{
  char *blist, folder[64];
  ACCT acct;
  HDR hdr;

  if (HAS_PERM(PERM_ALLADMIN))
    return 0;

  /* 板主在自己管理的看板不列入跨貼檢查 */
  blist = (bshm->bcache + bno)->BM;
  if (HAS_PERM(PERM_BM) && blist[0] > ' ' && is_bm(blist, cuser.userid))
    return 0;

  if (checksum_find(fpath))
  {
    /* 如果是 cross-post，那麼轉去 BN_SECURITY 並直接停權 */
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
    mail_self(FN_ETC_CROSSPOST, str_sysop, "Cross-Post 停權", 0);
    vmsg("您因為過度 Cross-Post 已被停權");
    return 1;
  }
  return 0;
}
#endif	/* HAVE_DETECT_CROSSPOST */

/* ----------------------------------------------------- */
/* 檢查文章發表權限                                      */
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
/* 另外限制文章編輯權限 (包含內文和標題)                 */
/* ----------------------------------------------------- */

int CheckEditPerm(BRD *brd)
{

// ALLPOST 的文章不能編輯
#ifdef HAVE_ALLPOST
    if (!strcmp(brd->brdname, BN_ALLPOST) || !strcmp(brd->brdname, BN_ALLHIDPOST))
	return 0;
#endif

// LUCKYPOST 的文章不能編輯
#ifdef HAVE_LUCKYPOST
    if (!strcmp(brd->brdname, BN_LUCKYPOST))
	return 0;
#endif

// 沒有發文權限的人不能編輯
    if (!CheckPostPerm(brd)) 
	return 0;

// guest 不能編輯
    if (!cuser.userlevel)   
	return 0;

    return 1;
}


/* ----------------------------------------------------- */
/* 發表、回應、編輯、轉錄文章				 */
/* ----------------------------------------------------- */


int
is_author(hdr)
  HDR *hdr;
{
  /* 這裡沒有檢查是不是 guest，注意使用此函式時要特別考慮 guest 情況 */

  /* itoc.070426: 當帳號被清除後，新註冊相同 ID 的帳號並不擁有過去該 ID 發表的文章之所有權 */
  return !strcmp(hdr->owner, cuser.userid) && (hdr->chrono > cuser.firstlogin);
}


#ifdef HAVE_REFUSEMARK
static int
post_canseelist(xo)            /* hrs.090803: 加密文章可見名單 */
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
  xover(XZ_PAL);                /* Thor: 進xover前, pal_xo 一定要 ready */

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

/* hrs.081101: 檢查轉信文章是不是已經轉出去了 */
int
chkoutgo(hdr, brd)	/* 1: 轉出去了; 0: 還沒轉出去 */
  HDR *hdr;
  BRD *brd;
{
    bntp_t BNTP;
    register int i, size;

    if (!(hdr->xmode & POST_OUTGO))  /* 站內文章沒有是否轉出的問題 */
	return 0;

    if (brd->battr & BRD_NOTRAN)    /* 非轉信板也沒有是否轉出的問題 */
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


/* 發文到看板 */
void add_post(brdname, fpath, title, owner, nick, xmode) 
  char *brdname, *fpath, *title, *owner, *nick;        
  /* 目的看板/檔案路徑/文章標題/作者ID/作者暱稱 */
  int xmode;
  /* 屬性 */
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

  hdr.xmode = mode & ~POST_OUTGO;  /* 拿掉 POST_OUTGO */
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
  /* Thor.981105: 進入前需設好 curredit 及 quote_file */
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
      vmsg("新手上路，三日後始可張貼文章");
    else
#endif      
      vmsg("對不起，您沒有在此發表文章的權限");
    return XO_FOOT;
  }

  brd = bshm->bcache + currbno;

  if (!CheckPostPerm(brd))
  {
      vmsg("對不起，您沒有在此發表文章的權限 (大寫I可看限制)");
      return XO_FOOT;
  }

  if (brd->battr & BRD_COSIGN)
  {
    if (bbstate & STAT_BOARD)
    {
	if (vans("連署機，您要發表： 1) 一般文章 2) 連署文章 [2] ") != '1')
	return do_cosign();
    }
    else
    return do_cosign();
  }

  brd_fpath(fpath, currboard, FN_POSTLAW);
  if (more(fpath, (char *) -1) == -1)
    film_out(FILM_POST, 0);
  
  move(19,0);
  prints("發表文章於【 %s 】看板", currboard);

#ifdef POST_PREFIX
  /* 借用 mode、rcpt、fpath */

  if (title)
  {
    rcpt = NULL;
  }
  else		/* itoc.020113: 新文章選擇標題分類 */
  {
      brd_fpath(fpath, currboard, FN_PREFIX);
      if (fp = fopen(fpath, "r"))
      {
	  move(21, 0);
	  clrtobot();
	  outs("類別：");
	  for (mode = 0; mode < NUM_PREFIX; mode++)
	  {
	      char *ptr;

	      // fpath 被借用來存 prefix
	      if (fgets(fpath, 14, fp) == NULL)
		  break;

	      /* hrs:因為最後一個讀入的字元如果是\n  換行字元  要去除掉 */
	      ptr = strchr(fpath, '\n');
	      if (ptr)
		  *ptr = '\0';

	      strcpy(prefix[mode], fpath);
	      if(mode == 5)  move(23, 6);
	      prints(ANSI_COLOR(1;33) "%d." ANSI_COLOR(1;37) "%s " ANSI_RESET, mode, fpath);
	  }

	  fclose(fp);
	  vget(20, 0, "請選擇文章類別（或自行輸入）：",myprefix, 15, DOECHO) ;
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

  /* 未具備 Internet 權限者，只能在站內發表文章 */
  /* Thor.990111: 沒轉信出去的看板, 也只能在站內發表文章 */

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
  spendtime = time(0) - spendtime;	/* itoc.010712: 總共花的時間(秒數) */

  /* build filename */
  folder = xo->dir;
  hdr_stamp(folder, HDR_LINK | 'A', &hdr, fpath);

  /* set owner to anonymous for anonymous board */

#ifdef HAVE_ANONYMOUS
  /* Thor.980727: lkchu新增之[簡單的選擇性匿名功能] */
  if (curredit & EDIT_ANONYMOUS)
  {
      rcpt = anonymousid;	/* itoc.010717: 自定匿名 ID */
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
  /* hrs.080315 :先把錢算好，方便LuckyPost */

#ifdef HAVE_LUCKYPOST

  /* hrs.080107:LuckyPost 
   * 機率的算法: percent =  (money / 1000) * 100 %   
   * (隨著稿酬線性變化)
   * 1000 銀   ->   1000/1000 = 1.0 (100  %)
   *  100 銀   ->    100/1000 = 0.1 (10.0 %)
   *    0 銀   ->      0/1000 =   0 (0.00 %)		    */

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
	      "恭喜中獎了，獎金 " LUCKYPOST_REWARD " 元，"
	      "您可以在" BN_LUCKYPOST "板看到這篇文章\n\n" ANSI_RESET);
#endif

  outs("順利貼出文章，");

  /* hrs.100108: 未通過認證使用者不增加文章/金錢數 */
  /* hrs.100109: 無稿酬文章不列入紀錄 */
  if (currbattr & BRD_NOCOUNT || wordsnum < 30 || !HAS_PERM(PERM_VALID) || !hdr.money)
  {				
      outs("文章不列入紀錄，敬請包涵。");
  }
  else
  {
      prints("這是您的第 %d 篇文章，得 %d 銀。", ++cuser.numposts, hdr.money);
#ifdef HAVE_LUCKYPOST
      if (lucky)
	  outs(" (含獎金)");
#endif
      addmoney(hdr.money);
  }

  if (mode & POST_RESTRICT)
  {
      outs("\n\n" ANSI_COLOR(1;35) "此篇為加密文章，於此文章前按 o 可設定文章可見名單。" ANSI_RESET);
  }

  /* 回應到原作者信箱 */

  if (curredit & EDIT_BOTH)
  {
      rcpt = quote_user;

      if (strchr(rcpt, '@'))	/* 站外 */
	  mode = bsmtp(fpath, title, rcpt, 0);
      else			/* 站內使用者 */
	  mode = mail_him(fpath, rcpt, title, 0);

      outs(mode >= 0 ? "\n\n成功\回應至作者信箱" : "\n\n作者無法收信");
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
	vmsg("本看板不得回覆文章。");
	return XO_FOOT;
    }

    switch (vans("▲ 回應至 (F)看板 (M)作者信箱 (B)二者皆是 (Q)取消？[F] "))
    {
	case 'm':
	    hdr_fpath(quote_file, xo->dir, hdr);
	    return do_mreply(hdr, 0);

	case 'q':
	    return XO_FOOT;

	case 'b':
	    /* 若無寄信的權限，則只回看板 */
	    if (HAS_PERM(strchr(hdr->owner, '@') ? PERM_INTERNET : PERM_LOCAL))
		curredit = EDIT_BOTH;
	    break;
    }

    /* Thor.981105: 不論是轉進的, 或是要轉出的, 都是別站可看到的, 所以回信也都應該轉出 */
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
	    vmsg("本文已被鎖定。");
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
/* 印出 hdr 標題					 */
/* ----------------------------------------------------- */


int
tag_char(chrono)
    int chrono;
{
    return TagNum && !Tagger(chrono, 0, TAG_NIN) ? '*' : ' ';
}


#ifdef HAVE_DECLARE
static inline int
cal_day(date)		/* itoc.010217: 計算星期幾 */
    char *date;
{
#if 0
    蔡勒公式是一個推算哪一天是星期幾的公式.
	這公式是:
	c                y       26(m+1)
	W= [---] - 2c + y + [---] + [---------] + d - 1
	4                4         10
	W → 為所求日期的星期數. (星期日: 0  星期一: 1  ...  星期六: 6)
	c → 為已知公元年份的前兩位數字.
	y → 為已知公元年份的後兩位數字.
	m → 為月數
	d → 為日數
	[] → 表示只取該數的整數部分 (地板函數)
	ps.所求的月份如果是1月或2月,則應視為上一年的13月或14月.
	所以公式中m的取值範圍不是1到12,而是3到14
#endif

	/* 適用 2000/03/01 至 2099/12/31 */

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
    int cc;			/* 印出最多 cc - 1 字的標題 */
    int zone;			/* zone : 0)post 1)mail */
{
    /* 回覆/轉錄/原創/閱讀中的同主題回覆/閱讀中的同主題轉錄/閱讀中的同主題原創 */
static char *type[8] = {"Re", "Fw", "◇",  "ˇ"
	, ANSI_COLOR(1;33)"=>", ANSI_COLOR(1;33)"=>"
	    , ANSI_COLOR(1;31)"◆", ANSI_COLOR(1;31) "ˇ"};
    uschar *title, *mark;
    int ch, len, tlen;
    int in_chi, inchi2;	/* 1: 在中文字中 */
#ifdef HAVE_DECLARE
    int square;		/* 1: 要處理方括 */
#endif
#ifdef CHECK_ONLINE
    UTMP *online;
#endif

    /* --------------------------------------------------- */
    /* 印出日期						 */
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
	/* itoc.010217: 改用星期幾來上色 */
	prints("\033[1;3%dm%s\033[m ", cal_day(hdr->date) + 1, hdr->date + 3);
#else
	outs(hdr->date + 3);
	outc(' ');
#endif
    }
    /* --------------------------------------------------- */
    /* 印出作者						 */
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
	    /* 把超過 len 長度的部分直接切掉 */
	    /* itoc.060604.註解: 如果剛好切在中文字的一半就會出現亂碼，不過這情況很少發生，所以就不管了 */
	    ch = '.';
	}
	else
	{
	    /* 站外的作者把 '@' 換成 '.' */
	    if (in_chi || IS_ZHC_HI(ch))	/* 中文字尾碼是 '@' 的不算 */
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
    /* 印出標題的種類					 */
    /* --------------------------------------------------- */

    if (!chkrestrict(hdr))
    {
	title = "\033[1;30m<< 此為加密文章，不顯示標題 >>\033[m";
	ch = 1;
    }
    else
    {
	/* len: 標題是 type[] 裡面的那一種 */
	title = str_ttl(mark = hdr->title);
	len = (hdr->xmode & POST_COSIGN && zone ? 3 : ((title == mark) ? 2 : ((*mark == 'R') ? 0 : 1)));
	if (!strcmp(currtitle, title))
	    len += 4;
	outs(type[len]);
	outc(' ');
    }

    /* --------------------------------------------------- */
    /* 印出標題						 */
    /* --------------------------------------------------- */

    tlen = strlen(title);
    mark = title + cc;

#ifdef HAVE_DECLARE	/* Thor.980508: Declaration, 嘗試使某些title更明顯 */
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

    /* 把超過 cc 長度的部分直接切掉 */
    /* itoc.060604.註解: 如果剛好切在中文字的一半就會出現亂碼，不過這情況很少發生，所以就不管了 */
    while ((ch = *title++) && (title < mark))
    {
#ifdef HAVE_DECLARE
	if (square)
	{
	    if (in_chi || IS_ZHC_HI(ch))	/* 中文字的第二碼若是 ']' 不算是方括 */
	    {
		in_chi ^= 1;
	    }
	    else if (ch == ']')
	    {
		outs("]\033[m");
		square = 0;			/* 只處理一組方括，方括已經處理完了 */
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
    if (square || len >= 3)	/* Thor.980508: 變色還原用 */
#else
	if (len >= 3)
#endif
	    outs("\033[m");

    outc('\n');
}


/* ----------------------------------------------------- */
/* 看板功能表						 */
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

    /* 由於置底文沒有閱讀記錄，所以視為已讀 */
    attr = !(mode & POST_BOTTOM) && unread
	? 0 : 0x20;   /* 已閱讀為小寫，未閱讀為大寫 */

#ifdef HAVE_REFUSEMARK
    if (!chkrestrict(hdr)) // 看不到的密文標示為已讀
	return 'x';
    else if (mode & POST_RESTRICT)
	attr |= 'X';
    else
#endif
    if (mode & POST_WIKI)
	    attr |= 'W';
    else if (mode & POST_DONE)
		attr |= 'S';        /* qazq.030815: 站方處理完標記ｓ */
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
    if (cuser.ufo & UFO_UNREADCOLOR && !brh_unread(hdr->chrono) && brh_unread(hdr->stamp) && chkrestrict(hdr)) // 看的到的密文才標示未讀
	// TODO: 考慮用 cache 避免重複檢查可見名單
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
	strcpy(pre, "   "ANSI_COLOR(1;33)"★ "ANSI_RESET);
    else
	sprintf(pre, "%6d", num);

    prints("%s%c%s%c" ANSI_RESET, pre, tag_char(hdr->chrono), post_itemcolor(hdr), post_attr(hdr));

    if (hdr->xmode & POST_SCORE)
    {
	num = hdr->score;
	if (num <= 99 && num >= -99)         /* qazq.031013: 可以推到"爆"*/
	    prints("\033[1;3%cm%2d\033[m", num ? num > 0 ? '1' : '2' : '3', abs(num));
	else
	    prints("\033[1;3%s\033[m", num >= 0 ? "1m爆" : "2m爛");
    }
    else
    {
	outs("  ");
    }

    outc(' ');			/* hrs:再少一格來分開分數和標題 */
    hdr_outs(hdr, d_cols + 46, 1);   /* 少一格來放分數 */
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
	    if (vans("要新增資料嗎(Y/N)？[N] ") == 'y')
		return post_add(xo);
	}
	else
	{
	    vmsg("本看板尚無文章");
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
    return XO_FOOT;	/* itoc.010403: 把 b_lines 填上 feeter */
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
    prints(NECKER_POST, money_mode ? "稿 酬" : "日 期", d_cols, "", bshm->mantime[currbno]);
    return post_body(xo);
}


/* ----------------------------------------------------- */
/* 資料之瀏覽：browse / history				 */
/* ----------------------------------------------------- */


static int
post_visit(xo)
    XO *xo;
{
    int ans, row, max;
    HDR *hdr;

    ans = vans("設定所有文章 (U)未讀 (V)已讀 (W)前已讀後未讀 (Q)取消？[Q] ");
    if (ans == 'v' || ans == 'u' || ans == 'w')
    {
	row = xo->top;
	max = xo->max - row + 3;
	if (max > b_lines)
	    max = b_lines;

	hdr = (HDR *) xo_pool + (xo->pos - row);
	/* brh_visit(ans == 'w' ? hdr->chrono : ans == 'u'); */
	/* weiyu.041010: 在置底文上選 w 視為全部已讀 */
	brh_visit((ans == 'u') ? 1 : (ans == 'w' && !(hdr->xmode & POST_BOTTOM)) ? hdr->chrono : 0);

	hdr = (HDR *) xo_pool;
	return post_body(xo);
    }
    return XO_FOOT;
}

void
post_chrono_history(char * dir, time_t chrono) /* 將 chrono 這個 timestamp 加入 brh */
{
    int fd;
    time_t prev, next, this;
    HDR buf;

    if (!brh_unread(chrono)) /* 如果已在 brh 中，就無需動作 */
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

	if (prev > chrono)      /* 沒有下一篇 */
	    prev = chrono;
	if (next < chrono)      /* 沒有上一篇 */
	    next = chrono;

	brh_add(prev, chrono, next);
    }
}

void
post_history(xo, hdr)          /* 將 hdr 這篇加入 brh */
    XO *xo;
    HDR *hdr;
{
    post_chrono_history(xo->dir, hdr->chrono);
    post_chrono_history(xo->dir, hdr->stamp);
}


#if 0
hrs113355:
    此類看板的標題都會是 "原標題      .brdname"，
    因此從尾端往前尋找 '.' 即可找到板名，若沒有板名以致於找到錯的或是找不到也無妨，
    會在 ptr == hdr->title - 1 和找不到 bno 的時候被過濾掉。

    改用此方法可節省開檔讀檔時所耗費的 I/O
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

    if (strcmp(currboard, BN_ALLPOST))/* 在一般板讀完，也去 All_Post 標示已讀 */
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

			    /* 恢復原來看板 */
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
    else                              /* 在 All_Post 讀完，也去別板標示已讀 */
    {
	if ((bno = find_board(hdr->title)) >= 0)
	{
	    brd = bshm->bcache + bno;
	    brh_get(brd->bstamp, bno);
	    bno = hdr->xid;
	    brh_add(bno - 1, bno, bno + 1);

	    /* 恢復原來看板 */
	    brd = bshm->bcache + currbno;
	    brh_get(brd->bstamp, currbno);
	}
    }
}
#endif

#if defined(HAVE_ALLPOST) || defined(HAVE_LUCKYPOST)
static int
post_jump(xo)       /* 看板跳躍(從 BN_ALLPOST/BN_ALLHIDPOST/BN_LUCKYPOST) */
    XO *xo;
{
    int tmpbno, bno;
    HDR *hdr;
    static int p_jump = 0;  /* 進入幾層 */
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
    
    if (p_jump >= 1)  /* 最多 jump 一層 */
	return XO_NONE;

    hdr = (HDR *) xo_pool + (xo->pos - xo->top);
    if (!chkrestrict(hdr))
	return XO_NONE;
    bno = find_board(hdr->title);

    if (bno >= 0 && bno != currbno)    /* 看板存在 && 要跳到別的看板 */
    {
	tmpbno = currbno; /* 紀錄上一個看板 */

	if (!XoPost(bno))      /* 可以進入才 xover() */
	{
	    p_jump++;
	    xover(XZ_POST);
#ifndef ENHANCED_VISIT
	    time(&brd_visit[currbno]);
#endif
	    p_jump--;

	    XoPost(tmpbno);        /* 跳回原來的看板 */
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

	/* Thor.990204: 為考慮more 傳回值 */
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
			vmsg("本文已被鎖定。");
			return XO_HEAD;
		    }

		    if (do_reply(xo, hdr) == XO_INIT)	/* 有成功地 post 出去了 */
			return post_init(xo);
		}
		break;

	    case 'm':
		if ((bbstate & STAT_BOARD) && !(xmode & POST_MARKED | POST_DELETE))
		{
		    /* hdr->xmode = xmode ^ POST_MARKED; */
		    /* 在 post_browse 時看不到 m 記號，所以限制只能 mark */
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
		if (vget(b_lines, 0, "搜尋：", hunt, sizeof(hunt), DOECHO))
		{
		    more(fpath, FOOTER_POST);
		    goto re_key;
		}
		continue;

	    case 'E':
		return post_edit(xo);

	    case 'C':	/* itoc.000515: post_browse 時可存入暫存檔 */
		{
		    FILE *fp;
		    if (fp = tbf_open())
		    { 
			f_suck(fp, fpath); 
			fclose(fp);
		    }
		}
		break;

	    case 'Q':	/* hrs.091011: 顯示文章資訊 */
		p_fileinfobt(xo);
		break;

	    case 's':
		outl(0, ANSI_COLOR(7) "【選擇看板】" ANSI_RESET);
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
/* 精華區						 */
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

    XoGem(fpath, "精華區", level);
    return post_init(xo);
}


/* ----------------------------------------------------- */
/* 進板畫面						 */
/* ----------------------------------------------------- */


static int
post_memo(xo)
    XO *xo;
{
    char fpath[64];

    brd_fpath(fpath, currboard, fn_note);
    /* Thor.990204: 為考慮more 傳回值 */   
    if (more(fpath, NULL) == -1)
    {
	vmsg("本看板尚無「進板畫面」");
	return XO_FOOT;
    }
 
    return post_head(xo);
}


/* ----------------------------------------------------- */
/* 功能：tag / switch / cross / forward			 */
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
    return xo->pos + 1 + XO_MOVE; /* lkchu.981201: 跳至下一項 */
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
    /* 來源看板 */
    char *dir, *ptr;
    HDR *hdr, xhdr;

    /* 欲轉去的看板 */
    int xbno;
    usint xbattr;
    char xboard[BNLEN + 1], xfolder[64];
    HDR xpost;

    int tag, rc, locus, finish;
    int method;		/* 0:原文轉載 1:從公開看板/精華區/信箱轉錄文章 2:從秘密看板轉錄文章 */
    usint tmpbattr;
    char tmpboard[BNLEN + 1];
    char fpath[64], buf[ANSILINELEN];
    FILE *fpr, *fpw;
    BRD  *brd;

    if (!cuser.userlevel)	/* itoc.000213: 避免 guest 轉錄去 sysop 板 */
	return XO_NONE;

    tag = AskTag("轉錄");
    if (tag < 0)
	return XO_FOOT;

    dir = xo->dir;
    hdr = tag ? &xhdr : (HDR *) xo_pool + (xo->pos - xo->top);    /* lkchu.981201: 整批轉錄 */

    if (!chkrestrict(hdr))
    {
	vmsg("加密文章不得轉錄。");
	return XO_FOOT;
    }
    if (!ask_board(xboard, BRD_W_BIT, "\n\n\033[1;33m請挑選適當的看板，切勿轉錄超過三板。\033[m\n\n") ||
	    (*dir == 'b' && !strcmp(xboard, currboard)))	/* 信箱、精華區中可以轉錄至currboard */
	return XO_HEAD;

    brd = bshm->bcache + currbno;

    if (method && *dir == 'b')    /* 從看板轉出，先檢查此看板是否為秘密板 */
    {
	if (brd->bmode == BMODE_HIDE || brd->bmode == BMODE_PAL)
	    method = 2;
    }

    xbno = brd_bno(xboard);
    brd = bshm->bcache + xbno;
    xbattr = brd->battr;

    int xmethod = 0;
    /* 借用 tmpbattr */
    if (brd->battr & BRD_COSIGN && !(brd_bits[xbno] & BRD_M_BIT))
    {
	vmsg("您無發表權限。");
	return XO_HEAD;
    }

    if (!CheckPostPerm(brd))
    {
	vmsg("對不起，在該看板您沒有發表文章的權限 (大寫I可看限制)");
	return XO_HEAD;
    }

    if (brd->bmode == BMODE_HIDE || brd->bmode == BMODE_PAL)
	xmethod = 2;

    /* 原作者轉錄自己文章時，可以選擇「原文轉載」 */
    method = (HAS_PERM(PERM_ALLBOARD) || (!tag && is_author(hdr))) &&
	(vget(2, 0, "(1)原文轉載 (2)轉錄文章？[1] ", buf, 3, DOECHO) != '2') ? 0 : 1;

    if (!tag)	/* lkchu.981201: 整批轉錄就不要一一詢問 */
    {
	if (method && strncmp(hdr->title, "[轉錄]", 6))
	    sprintf(ve_title, "[轉錄] %.66s", str_ttl(hdr->title));	
	/* 已有 Re:/Fw: 字樣就只要一個 Fw: */
	else
	    strcpy(ve_title, hdr->title);

	if (!vget(2, 0, "標題：", ve_title, TTLEN + 1, GCARRY))
	    return XO_HEAD;
    }

#ifdef HAVE_REFUSEMARK    
    rc = vget(2, 0, "(S)存檔 (L)站內 (X)密封 (Q)取消？[Q] ", buf, 3, LCECHO);
    if (rc != 'l' && rc != 's' && rc != 'x')
#else
	rc = vget(2, 0, "(S)存檔 (L)站內 (Q)取消？[Q] ", buf, 3, LCECHO);
    if (rc != 'l' && rc != 's')
#endif
	return XO_HEAD;

    /* Thor.990111: 在可以轉出前，要檢查有沒有轉出的權力? */
    if ((rc == 's') && (!HAS_PERM(PERM_INTERNET) || (xbattr & BRD_NOTRAN)))
	rc = 'l';

    /* 備份 currboard */
    if (method)
    {
	/* itoc.030325: 一般轉錄呼叫 ve_header，會使用到 currboard、currbattr，先備份起來 */
	strcpy(tmpboard, currboard);
	strcpy(currboard, xboard);
	tmpbattr = currbattr;
	currbattr = xbattr;
    }

    locus = 0;
    do	/* lkchu.981201: 整批轉錄 */
    {
	if (tag)
	{
	    EnumTag(hdr, dir, locus, sizeof(HDR));

	    if (method)
		sprintf(ve_title, "Fw: %.68s", str_ttl(hdr->title));	/* 已有 Re:/Fw: 字樣就只要一個 Fw: */
	    else
		strcpy(ve_title, hdr->title);
	}

	if (hdr->xmode & GEM_FOLDER)	/* 非 plain text 不能轉 */
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

	if (method)		/* 一般轉錄 */
	{
	    /* itoc.030325: 一般轉錄要重新加上 header */
	    fpw = fdopen(hdr_stamp(xfolder, 'A', &xpost, buf), "w");
	    ve_header(fpw);

	    /* itoc.040228: 如果是從精華區轉錄出來的話，會顯示轉錄自 [currboard] 看板，
	       然而 currboard 未必是該精華區的看板。不過不是很重要的問題，所以就不管了 :p */
	    fprintf(fpw, "※ 本文轉錄自 [%s] %s\n\n", 
		    *dir == 'u' ? cuser.userid : method == 2 ? "秘密" : tmpboard, 
		    *dir == 'u' ? "信箱" : "看板");

	    /* Kyo.051117: 若是從秘密看板轉出的文章，刪除文章第一行所記錄的看板名稱 */
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
		fprintf (fpw, CROSS_BANNER, cuser.userid, xmethod == 2 ? "某隱藏" : xboard, Now());
		fclose (fpw);
	    }

	}
	else		/* 原文轉錄 */
	{
	    /* itoc.030325: 原文轉錄直接 copy 即可 */
	    hdr_stamp(xfolder, HDR_COPY | 'A', &xpost, fpath);

	    strcpy(xpost.owner, hdr->owner);
	    strcpy(xpost.nick, hdr->nick);
	    strcpy(xpost.date, hdr->date);	/* 原文轉載保留原日期 */
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

    /* Thor.981205: check 被轉的板有沒有列入紀錄? */
    if (!(xbattr & BRD_NOCOUNT))
	cuser.numposts += tag ? tag : 1;	/* lkchu.981201: 要算 tag */

    /* 復原 currboard、currbattr */
    if (method)
    {
	strcpy(currboard, tmpboard);
	currbattr = tmpbattr;
    }

    vmsg("轉錄完成(加密文章不會被轉錄)");
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

    if (hdr->xmode & GEM_FOLDER)	/* 非 plain text 不能轉 */
	return XO_NONE;

#ifdef HAVE_REFUSEMARK
    if (!chkrestrict(hdr))
	return XO_NONE;
#endif

    if (acct_get("轉達信件給：", &muser) > 0)
    {
	strcpy(quote_user, hdr->owner);
	strcpy(quote_nick, hdr->nick);
	hdr_fpath(quote_file, xo->dir, hdr);
	sprintf(ve_title, "%.64s (fwd)", hdr->title);
	move(1, 0);
	clrtobot();
	prints("轉達給: %s (%s)\n標  題: %s\n", muser.userid, muser.username, ve_title);

	mail_send(muser.userid);
	*quote_file = '\0';
    }
    return XO_HEAD;
}


/* ----------------------------------------------------- */
/* 板主功能：mark / delete / label			 */
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
	if (xmode & POST_DELETE)	/* 待砍的文章不能 mark */
	    return XO_NONE;
#endif

	hdr->xmode = xmode ^ POST_MARKED;
	currchrono = hdr->chrono;
	rec_put(xo->dir, hdr, sizeof(HDR), xo->key == XZ_XPOST ? hdr->xid : pos, cmpchrono);

	move(3 + cur, 0);
	post_item(pos + 1, hdr);
	return xo->pos + 1 + XO_MOVE; /* lkchu.981201: 跳至下一項 */

	/* hrs.080203:板主m完才跳到下一項，其他沒有權限m的就不要動作了 */
    }
    return XO_NONE; 
}

static int          /* qazq.030815: 站方處理完成標記Ｓ*/
post_done_mark(xo)
    XO *xo;
{
    if ((bbstate & STAT_BM) || HAS_PERM(PERM_SYSOP)) /* hrs.060812:板主就可以標記了 */
    {
	HDR *hdr;
	int pos, cur, xmode;

	pos = xo->pos;
	cur = pos - xo->top;
	hdr = (HDR *) xo_pool + cur;
	xmode = hdr->xmode;

#ifdef HAVE_LABELMARK
	if (xmode & POST_DELETE)    /* 待砍的文章不能 mark */
	    return XO_NONE;
#endif

	hdr->xmode = xmode ^ POST_DONE;
	currchrono = hdr->chrono;

	rec_put(xo->dir, hdr, sizeof(HDR), xo->key == XZ_XPOST ?
		hdr->xid : pos, cmpchrono);
	move(3 + cur, 0);
	post_item(pos + 1, hdr);

	return xo->pos + 1 + XO_MOVE; /* lkchu.981201: 跳至下一項 */
	/* hrs.080203:板主l完才跳到下一項，其他沒有權限m的就不要動作了 */
    }
    return XO_NONE; 
}

static int
post_wiki(xo)
    XO *xo;
{
    HDR *hdr;
    int pos, cur, xmode;

    if (!cuser.userlevel) /* itoc.020114: guest 不能對其他 guest 的文章維基 */
	return XO_NONE;

    pos = xo->pos;
    cur = pos - xo->top;
    hdr = (HDR *) xo_pool + cur;

    if (!strcmp(hdr->owner, cuser.userid) || (bbstate & STAT_BOARD))
    {
	xmode = hdr->xmode;

#ifdef HAVE_LABELMARK
	if (xmode & POST_DELETE)    /* 待砍的文章不能 wiki */
	    return XO_NONE;
#endif
#ifdef HAVE_REFUSEMARK
	if (hdr->xmode & POST_RESTRICT)
	    return XO_NONE;
#endif

	if (vansf("確定要%s維基文章 (任何人都可以編輯) ？ [y/N] ", 
		    (hdr->xmode & POST_WIKI) ? "取消" : "標記成" ) != 'y')
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
	    if (vans("確定要取消置底文章？ [y/N] ") == 'y')
	    {
		currchrono = hdr->chrono;

		rec_del(xo->dir, sizeof(HDR), xo->key == XZ_XPOST ? hdr->xid : pos, cmpchrono);
		return XO_LOAD;
	    }
	    else
		return XO_FOOT;
	}

	if (vans("確定要置底文章？ [y/N] ") != 'y')
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
	/* btime_update(currbno); */	/* 不需要，因為置底文章不列入未讀 */

	return post_load(xo);	/* 立刻顯示置底文章 */
    }
    return XO_NONE;
}


#ifdef HAVE_REFUSEMARK
static int
post_refuse(xo)		/* itoc.010602: 文章加密 */
    XO *xo;
{
    HDR *hdr;
    int pos, cur;
    BRD *brd;

    if (!cuser.userlevel)	/* itoc.020114: guest 不能對其他 guest 的文章加密 */
	return XO_NONE;

    pos = xo->pos;
    cur = pos - xo->top;
    hdr = (HDR *) xo_pool + cur;
    brd = bshm->bcache + currbno;

    if (is_author(hdr) || (bbstate & STAT_BM))
    {
	if (!(hdr->xmode & POST_RESTRICT))  /* hrs.090302: 加密文章同時刪除跨板備份 */
	    post_delcrosspost(hdr);
	else /* hrs.090803: 解密時砍除可見名單 */
	{
	    if (vans("解除文章保密會刪除全部可見名單，您確定嗎(Y/N)？[N] ") != 'y')
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

	if (xmode & (POST_MARKED | POST_RESTRICT | POST_DONE | POST_WIKI | POST_BOTTOM))	/* mark 或 加密 或 done 的文章不能待砍 */
	    return XO_NONE;

	hdr->xmode = xmode ^ POST_DELETE;
	currchrono = hdr->chrono;
	rec_put(xo->dir, hdr, sizeof(HDR), xo->key == XZ_XPOST ? hdr->xid : pos, cmpchrono);

	move(3 + cur, 0);
	post_item(pos + 1, hdr);

	return pos + 1 + XO_MOVE;	/* 跳至下一項 */
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

    if (vans("確定要刪除待砍文章嗎(Y/N)？[N] ") != 'y')
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
	    /* 連線砍信 */
	    cancel_post(hdr);

	    /* hrs.090302: 刪除跨板備份 */
	    post_delcrosspost(hdr);

#ifdef HAVE_REFUSEMARK
	    /* hrs.090803: 刪除可見名單 */
	    unlink_canseelist(currboard, hdr);
#endif

	    /* 刪除原文 */
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

    /* hrs.080205:非板主不可以砍連署文章 */
    by_BM = !is_author(hdr);

    if (vans(msg_del_ny) == 'y')
    {
	currchrono = hdr->chrono;

	if (!rec_del(xo->dir, sizeof(HDR), xo->key == XZ_XPOST ? hdr->xid : pos, cmpchrono))
	{

	    /* hrs.090302: 刪除跨板備份 */
	    post_delcrosspost(hdr);

	    /* hrs.090803: 刪除可見名單 */
#ifdef HAVE_REFUSEMARK
	    unlink_canseelist(currboard, hdr);
#endif
	    pos = move_post(hdr, xo->dir, by_BM);

	    if (!by_BM && !(currbattr & BRD_NOCOUNT) && !(hdr->xmode & POST_BOTTOM) && hdr->money)
	    {
		pos = (cuser.money > pos ? pos : cuser.money);

		/* itoc.010830.註解: 漏洞: 若 multi-login 砍不到另一隻的錢 */
		cuser.money -= pos;

		if (cuser.numposts > 0)
		    cuser.numposts--;
		vmsgf("%s，您的文章減為 %d 篇，繳納清潔費 %d 銀", MSG_DEL_OK, cuser.numposts, pos);
	    }

	    if (xo->key == XZ_XPOST)
	    {
		vmsg("原列表經刪除後混亂，請重進串接模式！");
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
	vmsg("原列表經批次刪除後混亂，請重進串接模式！");
	return XO_QUIT;
    }

    return ret;
}


static int
post_copy(xo)	   /* itoc.010924: 取代 gem_gather */
    XO *xo;
{
    int tag;

    tag = AskTag("看板文章拷貝");

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
	    zmsg("檔案標記完成。[注意] 您必須先離開串接模式才能進入精華區。");
	    return XO_FOOT;
	}
	else
#endif
	{
	    zmsg("拷貝完成。[注意] 貼上後才能刪除原文！");
	    return post_gem(xo);	/* 拷貝完直接進精華區 */
	}
    }

    zmsg("檔案標記完成。[注意] 您只能在擔任(小)板主所在或個人精華區貼上。");
    return XO_FOOT;
}


/* ----------------------------------------------------- */
/* 站長功能：edit / title				 */
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
 
    if (!cuser.userlevel)       /* hrs.090928: 不讓 guest 修改文章 */
        return XO_NONE;

    if ((hdr->xmode & POST_MARKED)&& (hdr->xmode & POST_DONE))
    {
	vmsg("本文已被鎖定。");
	return XO_HEAD;
    }

    /* hrs.080205:不可以改連署文章 */
    if (hdr->xmode & POST_COSIGN && !HAS_PERM(PERM_SYSOP)) 
	return XO_HEAD;

    if (chkoutgo(hdr, brd))
    {
	outl(b_lines - 1, ANSI_COLOR(1;31)"★ 本文已經發佈至新聞群組伺服器，您的修改將無法被其他站台閱\讀。 "
		ANSI_RESET);
	if (vans("您還是希望要修改文章嗎？ [Y/n] ") == 'n')
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

	if (HAS_PERM(PERM_ALLBOARD))			/* 站長修改 */
	{ 
	    if(!vedit(fpath, 0))
	    {
		if(vans("是否留下修改文章記錄？[Y/n] ") != 'n')
		    DoRecord = 1;
	    }
	    else
		return XO_HEAD;
	}
	else if (is_author(hdr) && CheckEditPerm(brd) || (bbstate & STAT_BOARD)
		|| hdr->xmode & POST_WIKI)		/* 原作者修改 */
	{
	    if (!vedit(fpath, 0))	/* 若非取消則加上修改資訊 */
	    {
		DoRecord = 1;
	    }
	    else
		return XO_HEAD;
	}
	else		/* itoc.010301: 提供使用者修改(但不能儲存)其他人發表的文章 */
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
	    outs(ANSI_COLOR(1;33) "▲ 檔案已被修改過! ▲" ANSI_RESET "\n\n");
	    outs("進行自動合併 [Smart Merge]...\n");

	    if (AppendTail(genbuf, fpath, oldsz) == 0)
	    {

		oldmt = newmt;
		outs(ANSI_COLOR(1)
			"合併成功\，新修改(或推文)已加入您的文章中。\n"
			ANSI_RESET "\n");
		MergeDone = 1;
	//	vmsg("合併完成");
	    } 
	    else 
	    {
		outs(ANSI_COLOR(31)
			"自動合併失敗。 請改用人工手動編輯合併。" ANSI_RESET);
		vmsg("合併失敗");
	    }
	}
	if (oldmt != newmt)
	{
	    int c = 0;

	    clear();
	    move(b_lines-7, 0);
	    outs(ANSI_COLOR(1;31) "▲ 檔案已被修改過! ▲" ANSI_RESET "\n\n");

	    outs("可能是您在編輯的過程中有人進行推文或修文。\n"
		    "您可以選擇直接覆蓋\檔案(y)、放棄(n)，\n"
		    " 或是" ANSI_COLOR(1)"重新編輯" ANSI_RESET
		    "(新文會被貼到剛編的檔案後面)(e)。\n");
	    c = vans("要直接覆蓋\檔案/取消/重編嗎? [Y/n/e]");

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
		    vmsg("抱歉，檔案已損毀。");
		    if(src) fclose(src);
		    unlink(fpath); // fpath is a temp file
		    return XO_HEAD;
		}

		if(src)
		{
		    int c = 0;

		    fprintf(fp, MSG_SEPERATOR "\n");
		    fprintf(fp, "以下為被修改過的最新內容: ");
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
	/* todo WIKI專文 */
	/* bluesway.070323: 借fpath 來存精華區路徑 */
	gem_fpath(fpath, currboard, fn_dir);
	gem_log(fpath, "Wiki", hdr);
    }

    if (MergeDone)
	vmsg("合併完成");

    return XO_HEAD;	/* itoc.021226: XZ_POST 和 XZ_XPOST 共用 post_edit() */
}


void
header_replace(xo, hdr)		/* itoc.010709: 修改文章標題順便修改內文的標題 */
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

    fgets(buf, sizeof(buf), fpr);		/* 加入作者 */
    fputs(buf, fpw);

    fgets(buf, sizeof(buf), fpr);		/* 加入標題 */
    if (!str_ncmp(buf, "標", 2))		/* 如果有 header 才改 */
    {
	strcpy(buf, buf[2] == ' ' ? "標  題: " : "標題: ");
	strcat(buf, hdr->title);
	strcat(buf, "\n");
    }
    fputs(buf, fpw);

    while(fgets(buf, sizeof(buf), fpr))	/* 加入其他 */
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

    if (!cuser.userlevel)	/* itoc.000213: 避免 guest 在 sysop 板改標題 */
	return XO_NONE;

    pos = xo->pos;
    cur = pos - xo->top;
    fhdr = (HDR *) xo_pool + cur;
    memcpy(&mhdr, fhdr, sizeof(HDR));

    if (!chkrestrict(&mhdr))
	return XO_NONE;

    if (!is_author(&mhdr) && !(bbstate & STAT_BOARD))
	return XO_NONE;

    vget(b_lines, 0, "標題：", mhdr.title, TTLEN + 1, GCARRY);

    if (HAS_PERM(PERM_ALLBOARD))  /* itoc.000213: 原作者只能改標題 */
    {
	vget(b_lines, 0, "作者：", mhdr.owner, 73 /* sizeof(mhdr.owner) */, GCARRY);
	/* Thor.980727: sizeof(mhdr.owner) = 80 會超過一行 */
	vget(b_lines, 0, "暱稱：", mhdr.nick, sizeof(mhdr.nick), GCARRY);
	vget(b_lines, 0, "日期：", mhdr.date, sizeof(mhdr.date), GCARRY);
    }

    if (memcmp(fhdr, &mhdr, sizeof(HDR)) && vans(msg_sure_ny) == 'y')
    {
	memcpy(fhdr, &mhdr, sizeof(HDR));
	currchrono = fhdr->chrono;
	rec_put(xo->dir, fhdr, sizeof(HDR), xo->key == XZ_XPOST ? fhdr->xid : pos, cmpchrono);

	move(3 + cur, 0);
	post_item(++pos, fhdr);

	/* itoc.010709: 修改文章標題順便修改內文的標題 */
	/* hrs.080205: 連署文章則不必了 */
	if (!(fhdr->xmode & POST_COSIGN))
	    header_replace(xo, fhdr);
    }
    return XO_FOOT;
}


/* ----------------------------------------------------- */
/* 額外功能：write / score				 */
/* ----------------------------------------------------- */


int
post_write(xo)			/* itoc.010328: 丟線上作者叉滴 */
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
static time_t next = 0;   /* 下次可評分時間 */
    HDR *hdr;
    int pos, cur, ans, vtlen, maxlen;
    char *dir, userid[IDLEN + 1], *verb, fpath[64], reason[80], vtbuf[12], prompt[256], msg[80];
    FILE *fp;
#ifdef HAVE_ANONYMOUS
    char uid[IDLEN + 1];
#endif
    int msgpos;		    /* hrs.080103:訊息位置 */
    BRD *brd;

    brd = bshm->bcache + currbno;

    if (brd->battr & BRD_NOSCORE)
    {
	vmsg("本看板不得評分。");
	return XO_FOOT;
    }

    if (!cuser.userlevel || !(bbstate & STAT_POST))
    {
	vmsg("權限不足。");
	return XO_FOOT;
    }

    if (!CheckPostPerm(brd))
    {
	vmsg("對不起，您沒有在此推薦文章的權限 (大寫I可看限制)");
	return XO_FOOT;
    }

    pos = xo->pos;
    cur = pos - xo->top;
    hdr = (HDR *) xo_pool + cur;

    if ((hdr->xmode & POST_MARKED)&& (hdr->xmode & POST_DONE))
    {
	vmsg("本文已被鎖定。");
	return XO_HEAD;
    }

    if (hdr->xmode & POST_COSIGN)
	return XO_NONE;
    /* hrs.080205:連署文章不給推 */

#ifdef HAVE_REFUSEMARK
    if (!chkrestrict(hdr))
	return XO_NONE;
#endif

    utmp_mode(M_SCORE);
    if (next < time(NULL))
    {
	switch (ans = vbar(brd->battr & BRD_NOBOO ?
		    ANSI_COLOR(1)  "◎ 評分 "
		    ANSI_COLOR(33) "0)接話 "
		    ANSI_COLOR(31) "1)推文 "
		    ANSI_COLOR(31) "3)自訂推 "
		    ANSI_RESET "[1]? ":

		    ANSI_COLOR(1)  "◎ 評分 "
		    ANSI_COLOR(33) "0)接話 "
		    ANSI_COLOR(31) "1)推文 "
		    ANSI_COLOR(32) "2)唾棄 "
		    ANSI_COLOR(31) "3)自訂推 "
		    ANSI_COLOR(32) "4)自訂呸 "
		    ANSI_RESET "[1]? "))
	{
	    case '0':
		verb = "3m說";
		vtlen = 2;
		break;

	    default:
	    case '1':
		ans = '1';
		verb = "1m推";
		vtlen = 2;
		break;

	    case '2':
		if (brd->battr & BRD_NOBOO)
		    return XO_FOOT;
		verb = "2m呸";
		vtlen = 2;
		break;

	    case '3':
	    case '4':
		if (brd->battr & BRD_NOBOO && ans == '4')
		    return XO_FOOT;
		if (!vget(b_lines, 0, "請輸入動詞：", fpath, 7, DOECHO))
		    return XO_FOOT;
		vtlen = strlen(fpath);
		sprintf(verb = vtbuf, "%cm%s", ans - 2, fpath);
		break;
	}
    }
    else{
	verb = "3m說";
	vtlen = 2;
	ans= '9' ; //magic number
    }

#ifdef HAVE_ANONYMOUS
    if (brd->battr & BRD_ANONYMOUS)
    {
	strcpy(userid, uid);
	if (!vget(b_lines, 0, "請輸入您想用的ID，也可直接按[Enter]，或是按[r]用真名：", userid, IDLEN,
		    DOECHO))
	    strcpy(userid, STR_ANONYMOUS);
	else if (userid[0] == 'r' && userid[1] == '\0')
	    strcpy(userid, cuser.userid);
	else
	    strcat(userid, ".");      /* 自定的話，最後加 '.' */
    }
    else
#endif
	strcpy(userid, cuser.userid);

    maxlen = 64 - strlen(userid) - vtlen;

    if (brd->battr & BRD_SCOREALIGN)
    {
	sprintf(prompt, "→ " ANSI_COLOR(1;33) "%-*s " ANSI_COLOR(1;3%c) "%*s" ANSI_RESET ":",
		                     IDLEN, userid, *verb, 6, verb + 2);
	maxlen -= IDLEN - strlen(userid) + 6 - (strlen(verb) - 2);
    }
    else
	sprintf(prompt,"→ " ANSI_COLOR(1;33) "%s \033[1;3%s" ANSI_RESET ":", userid, verb);

    msgpos = b_lines - 1;

    if(ans=='9')
    {
	sprintf(msg, ANSI_COLOR(1;33) "★ 推文間隔太接近(還有 %ld 秒)，改用「說」： " ANSI_RESET
		, next - time(NULL) > 0 ? next - time(NULL) : 0);
	outl(msgpos--, msg); 
    }

    if(strstr(hdr->owner,"@"))
	outl(msgpos--, ANSI_COLOR(1;31) "★ 這篇文章來自外站轉信板，原作者可能無法看到推文。" ANSI_RESET); 
    if (chkoutgo(hdr, brd))
	outl(msgpos--, ANSI_COLOR(1;31) "★ 本文已經發佈至新聞群組伺服器，您的推文將無法被其他站台閱\讀。" ANSI_RESET);

    if (!vget(b_lines, 0, prompt, reason, maxlen, DOECHO | CTECHO))
	return XO_HEAD;

    while(*reason == '1' && !*(reason + 1) && ans=='9')
    {

	outl(b_lines - 1, ANSI_COLOR(1;31) "★ 由於剛剛評分間隔太接近改用「說」，您可能不小心按錯" ANSI_RESET); 
	if (vans("按錯了? [Y/n] ") == 'n')
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
	    fprintf(fp, "→ " ANSI_COLOR(1;33) "%-*s " ANSI_COLOR(1;3%c) "%*s" ANSI_RESET,
		    IDLEN, userid, *verb, 6, verb + 2);
	else
	    fprintf(fp, "→ " ANSI_COLOR(1;33) "%s \033[1;3%s" ANSI_RESET, userid, verb);

	fprintf(fp, ":%-*s" ANSI_COLOR(1;30) "%02d%02d %02d:%02d" ANSI_RESET "\n",
		maxlen, reason, ptime->tm_mon + 1, ptime->tm_mday, ptime->tm_hour, ptime->tm_min);
	fclose(fp);
    }

#ifdef HAVE_ANONYMOUS
    if (currbattr & BRD_ANONYMOUS && strcmp(userid, cuser.userid) != 0)
    {
	char buf[256];
	sprintf(buf, "%s %-13s(%s)\n%-13s %s 推文:%s\n",
	    Now(), cuser.userid, str_ip(cutmp->in_addr), currboard, hdr->xname, reason);
	f_cat(FN_RUN_ANONYMOUS, buf);
    }
#endif

    ans -= '0';
    curraddscore = 0;
    if (ans == 0 || ans == 9)     /* 說話 */
    {
	curraddscore = 0;
    }
    else if (ans == 1 || ans == 3)     /* 加分 */
    {
	if (hdr->score <= 99)
	    curraddscore = 1;
    }
    else if (ans == 2 || ans == 4)    /* 扣分 */
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
	next = time(NULL) + 10;  /* 板主每 10 秒方可評分一次 */
    else
	next = time(NULL) + 60;  /* 每 60 秒方可評分一次 */

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

	if (vans("確定要將這篇文章的推文數歸零嗎？ [y/N] ") != 'y')
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
    return XO_HEAD;		/* itoc.001029: 與 xpost_help 共用 */
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
    'x', post_cross,		/* 在 post/mbox 中都是小寫 x 轉看板，大寫 X 轉使用者 */
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
    'J', post_jump,     /* 看板跳躍(從 BN_ALLPOST/BN_ALLHIDPOST/BN_LUCKYPOST) */
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

    '~', XoXselect,		/* itoc.001220: 搜尋作者/標題 */
    'S', XoXsearch,		/* itoc.001220: 搜尋相同標題文章 */
    'a', XoXauthor,		/* itoc.001220: 搜尋作者 */
    '/', XoXtitle,		/* itoc.001220: 搜尋標題 */
    'f', XoXfull,			/* itoc.030608: 全文搜尋 */
    'G', XoXmark,			/* itoc.010325: 搜尋 mark 文章 */
    'L', XoXlocal,		/* itoc.010822: 搜尋本地文章 */
    '#', XoXfilename,		/* hrs: 搜尋文章代碼(檔名) */
    'Z', XoXscore,		/* hrs.080419: 搜尋分數 */
#ifdef HAVE_XYNEWS
    'u', XoNews,			/* itoc.010822: 新聞閱讀模式 */
#endif

    'h', post_help
};


KeyFunc xpost_cb[] =
{
    XO_INIT, xpost_init,
    XO_LOAD, xpost_load,
    XO_HEAD, xpost_head,
    XO_BODY, post_body,		/* Thor.980911: 共用即可 */

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
    'd', post_delete,		/* Thor.980911: 方便板主 */
    'E', post_edit,		/* itoc.010716: 提供 XPOST 中可以編輯標題、文章，加密 */
    'T', post_title,
#if defined(HAVE_ALLPOST) || defined(HAVE_LUCKYPOST)
    'J', post_jump,     /* 看板跳躍(從 BN_ALLPOST/BN_ALLHIDPOST/BN_LUCKYPOST) */
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
    'Z', XoXscore,                /* hrs.080419: 搜尋分數 */

    Ctrl('P'), post_add,
    Ctrl('D'), post_prune,
    Ctrl('Q'), xo_uquery,
    Ctrl('O'), xo_usetup,

    'h', post_help		/* itoc.030511: 共用即可 */
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

    'h', post_help		/* itoc.030511: 共用即可 */
};
#endif	/* HAVE_XYNEWS */
