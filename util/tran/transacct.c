/*-------------------------------------------------------*/
/* util/transacct.c	( NTHU CS MapleBBS Ver 3.10 )	 */
/*-------------------------------------------------------*/
/* target : M3 ACCT 轉換程式				 */
/* create : 98/12/15					 */
/* update : 02/04/29					 */
/* author : mat.bbs@fall.twbbs.org			 */
/* modify : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/*-------------------------------------------------------*/
/* syntax : transacct [userid]				 */
/*-------------------------------------------------------*/


/* hrs.080107.todo : 要記得改這裡 */

#if 0

  使用方法：

  0. For Maple 3.X To Maple 3.X

  1. 使用前請先利用 backupacct.c 備份 .ACCT

  2. 請自行改 struct NEW 和 struct OLD

  3. 依不同的 NEW、OLD 來改 trans_acct()。

#endif


#include "bbs.h"

/* ----------------------------------------------------- */
/* (新的) 使用者帳號 .ACCT struct			 */
/* ----------------------------------------------------- */


typedef struct			/* 要和新版程式 struct 一樣 */
{
  int userno;                   /* unique positive code */

  char userid[IDLEN + 1];       /* ID */
  char passwd[PASSLEN + 1];     /* 密碼 */
  char realname[RNLEN + 1];     /* 真實姓名 */
  char username[UNLEN + 1];     /* 暱稱 */
  char feeling[FLLEN + 1];      /* 心情 */

  usint userlevel;              /* 權限 */
  usint ufo;                    /* habit */
  usint loginview;              /* hrs.080309:上站畫面 */
  uschar signature;             /* 預設簽名檔 */

  char year;                    /* 生日(民國年) */
  char month;                   /* 生日(月) */
  char day;                     /* 生日(日) */
  char sex;                     /* 性別 奇數:男性 偶數:女性 */

  int money;                    /* 銀幣 */
  int mquota;                   /* hrs.080107:信箱封數 */

  int numlogins;                /* 上站次數 */
  int numposts;                 /* 發表次數 */
  int numemails;                /* 寄發 Inetrnet E-mail 次數 */
  int numbmwtx;                 /* 發叉滴次數 */
  int numbmwrx;                 /* 收叉滴次數 */
  int toquery;                  /* 好奇度 */
  int bequery;                  /* 人氣度 */

  time_t firstlogin;            /* 第一次上站時間 */
  time_t lastlogin;             /* 上一次上站時間 */
  time_t tcheck;                /* 上次 check 信箱/朋友名單的時間 */
  time_t tvalid;                /* 若停權，停權期滿的時間；
                                   若未停權且通過認證，通過認證的時間；
                                   若未停權且未通過認證，認證函的 time-seed */
  time_t staytime;              /* 掛站總時間 */

  char lasthost[40];            /* 上次登入來源 */
  char email[60];               /* 目前登記的電子信箱 */
  char site[40];                /* hrs.080310:讓使用者可以放網址廣告 */

} PACKSTRUCT NEW;


/* ----------------------------------------------------- */
/* (舊的) 使用者帳號 .ACCT struct			 */
/* ----------------------------------------------------- */


typedef struct			/* 要和舊版程式 struct 一樣 */
{
  int userno;			/* unique positive code */

  char userid[IDLEN + 1];	/* ID */
  char passwd[PASSLEN + 1];	/* 密碼 */
  char realname[RNLEN + 1];	/* 真實姓名 */
  char username[UNLEN + 1];	/* 暱稱 */
  char feeling[FLLEN + 1];      /* 心情 */

  usint userlevel;		/* 權限 */
  usint ufo;			/* habit */
  usint loginview;		/* hrs.080309:上站畫面 */
  uschar signature;		/* 預設簽名檔 */

  char year;			/* 生日(民國年) */
  char month;			/* 生日(月) */
  char day;			/* 生日(日) */
  char sex;			/* 性別 奇數:男性 偶數:女性 */
  char address[60];		/* hrs.080310:地址存進來 */
  char phone[20];		/* hrs.080310:電話存進來 */

  int money;			/* 銀幣 */
  int mquota;			/* hrs.080107:信箱封數 */

  int numlogins;		/* 上站次數 */
  int numposts;			/* 發表次數 */
  int numemails;		/* 寄發 Inetrnet E-mail 次數 */
  int numbmwtx;			/* 發叉滴次數 */
  int numbmwrx;			/* 收叉滴次數 */
  int toquery;			/* 好奇度 */
  int bequery;			/* 人氣度 */

  time_t firstlogin;		/* 第一次上站時間 */
  time_t lastlogin;		/* 上一次上站時間 */
  time_t tcheck;		/* 上次 check 信箱/朋友名單的時間 */
  time_t tvalid;		/* 若停權，停權期滿的時間；
                                   若未停權且通過認證，通過認證的時間；
                                   若未停權且未通過認證，認證函的 time-seed */
  time_t staytime;		/* 掛站總時間 */

  char lasthost[80];		/* 上次登入來源 */
  char email[80];		/* 目前登記的電子信箱 */
  char site[40];		/* hrs.080310:讓使用者可以放網址廣告 */
}	OLD;


/* ----------------------------------------------------- */
/* 轉換主程式						 */
/* ----------------------------------------------------- */

static void
trans_acct(old, new)
  OLD *old;
  NEW *new;
{
  memset(new, 0, sizeof(NEW));

  new->userno = old->userno;

  str_ncpy(new->userid, old->userid, sizeof(new->userid));
  str_ncpy(new->passwd, old->passwd, sizeof(new->passwd));
  str_ncpy(new->realname, old->realname, sizeof(new->realname));
  str_ncpy(new->username, old->username, sizeof(new->username));
  str_ncpy(new->feeling, old->feeling, sizeof(new->feeling));
  str_ncpy(new->site, old->site, sizeof(new->site));

  new->userlevel = old->userlevel;
  new->ufo = old->ufo;
  new->loginview = old->loginview;
  new->signature = old->signature;

  new->year = old->year;
  new->month = old->month;
  new->day = old->day;
  new->sex = old->sex;
  new->money = old->money;
  new->mquota = old->mquota;

  new->numlogins = old->numlogins;
  new->numposts = old->numposts;
  new->numemails = old->numemails;

  new->firstlogin = old->firstlogin;
  new->lastlogin = old->lastlogin;
  new->tcheck = old->tcheck;
  new->tvalid = old->tvalid;
  
  new->staytime = old->staytime;
  new->toquery = old->toquery;
  new->bequery = old->bequery;
  new->numbmwrx = old->numbmwrx;
  new->numbmwtx = old->numbmwtx;

  str_ncpy(new->lasthost, old->lasthost, sizeof(new->lasthost));
  str_ncpy(new->email, old->email, sizeof(new->email));
}


int
main(argc, argv)
  int argc;
  char *argv[];
{
  NEW new;
  char c;

  if (argc > 2)
  {
    printf("Usage: %s [userid]\n", argv[0]);
    return -1;
  }

  for (c = 'a'; c <= 'z'; c++)
  {
    char buf[64];
    struct dirent *de;
    DIR *dirp;

    sprintf(buf, BBSHOME "/usr/%c", c);
    chdir(buf);

    if (!(dirp = opendir(".")))
      continue;

    while (de = readdir(dirp))
    {
      OLD old;
      int fd;
      char *str;

      str = de->d_name;
      if (*str <= ' ' || *str == '.')
	continue;

      if ((argc == 2) && str_cmp(str, argv[1]))
	continue;

      sprintf(buf, "%s/" FN_ACCT, str);
      if ((fd = open(buf, O_RDONLY)) < 0)
	continue;

      read(fd, &old, sizeof(OLD));
      close(fd);
      unlink(buf);			/* itoc.010831: 砍掉原來的 FN_ACCT */

      trans_acct(&old, &new);

      fd = open(buf, O_WRONLY | O_CREAT, 0600);	/* itoc.010831: 重建新的 FN_ACCT */
      write(fd, &new, sizeof(NEW));
      close(fd);
    }

    closedir(dirp);
  }

  return 0;
}
