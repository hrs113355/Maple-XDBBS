/*-------------------------------------------------------*/
/* util/transacct.c	( NTHU CS MapleBBS Ver 3.10 )	 */
/*-------------------------------------------------------*/
/* target : M3 BRD 轉換程式				 */
/* create : 05/05/19					 */
/* update :   /  /  					 */
/* author : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/*-------------------------------------------------------*/


#if 0

  使用方法：

  0. For Maple 3.X To Maple 3.X

  1. 使用前請先備份 .BRD

  2. 請自行改 struct NEW 和 struct OLD

#endif


#include "bbs.h"


/* ----------------------------------------------------- */
/* (新的) 看板 .BRD struct				 */
/* ----------------------------------------------------- */


typedef struct			/* 要和新版程式 struct 一樣 */
{  
  int  bid;                     /* board id */
  char brdname[BNLEN + 1];      /* board name */
  char class[BCLEN + 1];
  char title[BTLEN + 1];
  char BM[BMLEN + 1];           /* BMs' uid, token '/' */

  char bvote;                   /* 0:無投票 -1:有賭盤(可能有投票) 1:有投票 */

  time_t bstamp;                /* 建立看板的時間, unique */
  usint readlevel;              /* 閱讀文章的權限 */
  usint postlevel;              /* 發表文章的權限 */
  usint battr;                  /* 看板屬性 */
  uschar bmode;                 /* 看板狀態: 公開/隱藏/秘密/封印 */

  time_t btime;                 /* -1:bpost/blast 需要更新 */
  int bpost;                    /* 共有幾篇 post */
  time_t blast;                 /* 最後一篇 post 的時間 */

  int limit_posts;              /* hrs.080226: 連署/發文限制:發表文章數(篇) */
  int limit_logins;             /* hrs.080226: 連署/發文限制:上站次數(次) */
  int limit_regtime;            /* hrs.080226: 連署/發文限制:註冊時間(月) */

  int ex_maxposts;              /* hrs.080226: expire: 文章上限(篇) */
  int ex_minposts;              /* hrs.080226: expire: 文章下限(篇) */
  int ex_maxtime;               /* hrs.080226: expire: 文章保留時間(天) */

  int parent;                   /* hrs.080226: 存上一層的class number */
} PACKSTRUCT NEW;


/* ----------------------------------------------------- */
/* (舊的) 看板 .BRD struct				 */
/* ----------------------------------------------------- */


typedef struct			/* 要和舊版程式 struct 一樣 */
{
  char brdname[BNLEN + 1];	/* board name */
  char class[BCLEN + 1];
  char title[BTLEN + 1];
  char BM[BMLEN + 1];		/* BMs' uid, token '/' */

  char bvote;			/* 0:無投票 -1:有賭盤(可能有投票) 1:有投票 */

  time_t bstamp;		/* 建立看板的時間, unique */
  usint readlevel;		/* 閱讀文章的權限 */
  usint postlevel;		/* 發表文章的權限 */
  usint battr;			/* 看板屬性 */
  time_t btime;			/* -1:bpost/blast 需要更新 */
  int bpost;			/* 共有幾篇 post */
  time_t blast;			/* 最後一篇 post 的時間 */

  int limit_posts;		/* hrs.080226: 連署/發文限制:發表文章數(篇) */
  int limit_logins;		/* hrs.080226: 連署/發文限制:上站次數(次) */
  int limit_regtime;		/* hrs.080226: 連署/發文限制:註冊時間(月) */

  int ex_maxposts;		/* hrs.080226: expire: 文章上限(篇) */
  int ex_minposts;		/* hrs.080226: expire: 文章下限(篇) */
  int ex_maxtime;		/* hrs.080226: expire: 文章保留時間(天) */

  int parent;			/* hrs.080226: 存上一層的class number */
}	OLD;


/* ----------------------------------------------------- */
/* 轉換主程式						 */
/* ----------------------------------------------------- */


#define FN_BRD_TMP	".BRD.tmp"


int
main()
{
  int fd;
  OLD old;
  NEW new;

  chdir(BBSHOME);

  if ((fd = open(FN_BRD, O_RDONLY)) >= 0)
  {
    while (read(fd, &old, sizeof(OLD)) == sizeof(OLD))
    {
      if (!*old.brdname)	/* 此板已被刪除 */
	continue;

      memset(&new, 0, sizeof(NEW));

      /* 轉換的動作在此 */
      str_ncpy(new.brdname, old.brdname, sizeof(new.brdname));
      str_ncpy(new.class, old.class, sizeof(new.class));
      str_ncpy(new.title, old.title, sizeof(new.title));
      str_ncpy(new.BM, old.BM, sizeof(new.BM));
      new.bvote = old.bvote;
      new.bstamp = old.bstamp;
      new.readlevel = old.readlevel;
      new.postlevel = old.postlevel;
      new.bmode = new.readlevel == PERM_SYSOP ? BMODE_HIDE :
                  new.readlevel == PERM_BOARD ? BMODE_PAL : 0;
      if (new.readlevel == PERM_BOARD || new.readlevel == PERM_SYSOP)
	  new.readlevel = 0;
      new.battr = old.battr;
      if (!strcmp(new.class, "站務"))
      new.battr |= BRD_SERVICE;
      else if (!strcmp(new.class, "個人"))
      new.battr |= BRD_PERSONAL;

      new.btime = old.btime;
      new.bpost = old.bpost;
      new.blast = old.blast;

      new.limit_posts = old.limit_posts;
      new.limit_logins = old.limit_logins;
      new.limit_regtime = old.limit_regtime;

      new.ex_maxposts = old.ex_maxposts;
      new.ex_minposts = old.ex_minposts;
      new.ex_maxtime = old.ex_maxtime;

      rec_add(FN_BRD_TMP, &new, sizeof(NEW));
    }
    close(fd);
  }

  /* 刪除舊的，把新的更名 */
  unlink(FN_BRD);
  rename(FN_BRD_TMP, FN_BRD);

  return 0;
}
