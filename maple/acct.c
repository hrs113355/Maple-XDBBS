/*-------------------------------------------------------*/
/* acct.c	( NTHU CS MapleBBS Ver 3.00 )		 */
/*-------------------------------------------------------*/
/* target : account / administration routines	 	 */
/* create : 95/03/29				 	 */
/* update : 96/04/05				 	 */
/*-------------------------------------------------------*/


#define	_ADMIN_C_


#include "bbs.h"


extern BCACHE *bshm;
extern char brd_bits[];


/* ----------------------------------------------------- */
/* (.ACCT) 使用者帳號 (account) subroutines		 */
/* ----------------------------------------------------- */


int
acct_load(acct, userid)
  ACCT *acct;
  char *userid;
{
  int fd;

  usr_fpath((char *) acct, userid, fn_acct);
  fd = open((char *) acct, O_RDONLY);
  if (fd >= 0)
  {
    /* Thor.990416: 特別注意, 有時 .ACCT的長度會是0 */
    read(fd, acct, sizeof(ACCT));
    close(fd);
  }
  return fd;
}


/* static */	/* itoc.010408: 給其他程式用 */
void
acct_save(acct)
  ACCT *acct;
{
  int fd;
  char fpath[64];

  /* itoc.010811: 若被站長鎖定，就不能寫回自己的檔案 */
  if ((acct->userno == cuser.userno) && HAS_STATUS(STATUS_DATALOCK) && !HAS_PERM(PERM_ALLACCT))
    return;

  usr_fpath(fpath, acct->userid, fn_acct);
  fd = open(fpath, O_WRONLY, 0600);	/* fpath 必須已經存在 */
  if (fd >= 0)
  {
    write(fd, acct, sizeof(ACCT));
    close(fd);
  }
}


int
acct_userno(userid)
  char *userid;
{
  int fd;
  int userno;
  char fpath[64];

  usr_fpath(fpath, userid, fn_acct);
  fd = open(fpath, O_RDONLY);
  if (fd >= 0)
  {
    read(fd, &userno, sizeof(userno));
    close(fd);
    return userno;
  }
  return 0;
}


/* ----------------------------------------------------- */
/* name complete for user ID				 */
/* ----------------------------------------------------- */
/* return value :					 */
/* 0 : 使用直接按 enter ==> cancel			 */
/* -1 : bad user id					 */
/* ow.: 傳回該 userid 之 userno				 */
/* ----------------------------------------------------- */


int
acct_get(msg, acct)
  char *msg;
  ACCT *acct;
{
  outz("★ 輸入首字母後，可以按空白鍵自動搜尋");
  
  if (!vget(1, 0, msg, acct->userid, IDLEN + 1, GET_USER))
    return 0;

  if (acct_load(acct, acct->userid) >= 0)
    return acct->userno;

  vmsg(err_uid);
  return -1;
}


/* ----------------------------------------------------- */
/* bit-wise display and setup				 */
/* ----------------------------------------------------- */


#define BIT_ON		"★"
#define BIT_OFF		"☆"


void
bitmsg(msg, str, level)
  char *msg, *str;
  int level;
{
  int cc;

  outs(msg);
  while (cc = *str)
  {
    outc((level & 1) ? cc : '-');
    level >>= 1;
    str++;
  }

  outc('\n');
}


usint
bitset(pbits, count, maxon, msg, perms)
  usint pbits;
  int count;			/* 共有幾個選項 */
  int maxon;			/* 最多可以 enable 幾項 */
  char *msg;
  char *perms[];
{
  int i, j, on;

  move(1, 0);
  clrtobot();
  move(3, 0);
  outs(msg);

  for (i = on = 0, j = 1; i < count; i++)
  {
    msg = BIT_OFF;
    if (pbits & j)
    {
      on++;
      msg = BIT_ON;
    }
    move(5 + (i & 15), (i < 16 ? 0 : 40));
    prints("%c %s %s", radix32[i], msg, perms[i]);
    j <<= 1;
  }

  while (i = vmsg("請按鍵切換設定，或按 [Return] 結束"))
  {
    if(i == KEY_ENTER) break;

    if(i >=65 && i<=90) i+= 32;

    i -= '0';
    if (i >= 10)
      i -= 'a' - '0' - 10;

    if (i >= 0 && i < count)
    {
      j = 1 << i;
      if (pbits & j)
      {
	on--;
	msg = BIT_OFF;
      }
      else
      {
	if (on >= maxon)
	  continue;
	on++;
	msg = BIT_ON;
      }

      pbits ^= j;
      move(5 + (i & 15), (i < 16 ? 2 : 42));
      outs(msg);
    }
  }
  return (pbits);
}


static usint
setperm(level)
  usint level;
{
  if (HAS_PERM(PERM_SYSOP))
    return bitset(level, NUMPERMS, NUMPERMS, MSG_USERPERM, perm_tbl);

  /* [帳號管理員] 不能管 PERM_SYSOP */
  if (level & PERM_SYSOP)
    return level;

  /* [帳號管理員] 不能更改權限 PERM_ACCOUNTS CHATROOM BOARD SYSOP */
  return bitset(level, NUMPERMS - 4, NUMPERMS - 4, MSG_USERPERM, perm_tbl);
}


/* ----------------------------------------------------- */
/* 帳號管理						 */
/* ----------------------------------------------------- */


static void
bm_list(userid)			/* 顯示 userid 是哪些板的板主 */
  char *userid;
{
  int len;
  char *list;
  BRD *bhead, *btail;

  len = strlen(userid);
  outs(ANSI_COLOR(32) "  擔任板主：" ANSI_COLOR(37));		/* itoc.010922: 換 user info 版面 */

  bhead = bshm->bcache;
  btail = bhead + bshm->number;

  do
  {
    list = bhead->BM;
    if (str_has(list, userid, len))
    {
      outs(bhead->brdname);
      outc(' ');
    }
  } while (++bhead < btail);

  outc('\n');
}


static void
adm_log(old, new)
  ACCT *old, *new;
{
  int i;
  usint bit, oldl, newl;
  char *userid, buf[80];

  userid = new->userid;
  alog("異動資料", userid);

  if (strcmp(old->passwd, new->passwd))
    alog("異動密碼", userid);

  if (old->money != new->money)
  {
    sprintf(buf, "%-13s銀%d→%d", userid, old->money, new->money);
    alog("異動錢幣", buf);
  }

  /* Thor.990405: log permission modify */
  oldl = old->userlevel;
  newl = new->userlevel;
  for (i = 0, bit = 1; i < NUMPERMS; i++, bit <<= 1)
  {
    if ((newl & bit) != (oldl & bit))
    {
      sprintf(buf, "%-13s%s %s", userid, (newl & bit) ? BIT_ON : BIT_OFF, perm_tbl[i]);
      alog("異動權限", buf);
    }
  }
}

static int          /* 1:找到  0:沒找到 */
find_rform(userno, userid, address, phone)
  int userno;               /* 傳入 userno/userid */
  char *userid;
  char *address, *phone;    /* 回傳 address/phone */
{
  RFORM *rform;
  int fd, ret = 0;

  if ((fd = open(FN_RUN_RFORM_LOG, O_RDONLY)) >= 0)
  {
    mgets(-1);

    while (rform = mread(fd, sizeof(RFORM)))
    {
      if (userno == rform->userno && !strcmp(userid, rform->userid))
      {
        strcpy(address, rform->address);
        strcpy(phone, rform->phone);
        ret = 1;
        break;
      }
    }
    close(fd);
  }
  return ret;
}

void
acct_show(u, adm)
  ACCT *u;
  int adm;			/* 0: user info  1: admin  2: reg-form */
{
  int diff;
  usint ulevel;
  char *uid, buf[80];

  clrtobot();

  /* itoc.010922: 換 user info 版面 */
  if (adm == 0)
  {
    film_out(FILM_USERINFO, 2);
    move (6, 0);
    clrtobot();
  }

  uid = u->userid;

  outs("\n" ANSI_COLOR(1));

  /* itoc.010408: 新增金錢/生日/性別欄位 */

  /* show user's information */
  ulevel = u->userlevel;
  usr_fpath(buf, uid, fn_dir);

  if (adm != 2)
      prints(ANSI_COLOR(32) "  英文代號：" ANSI_COLOR(37) "%-35s" ANSI_COLOR(32) "用戶編號：" ANSI_COLOR(37) "%d\n", uid, u->userno);

  prints(ANSI_COLOR(32) "  我的暱稱：" ANSI_COLOR(37) "%-35s" ANSI_COLOR(32) "擁有銀幣：" ANSI_COLOR(37) "%d\n", u->username, u->money);

  prints(ANSI_COLOR(32) "  真實姓名：" ANSI_COLOR(37) "%-35s" ANSI_COLOR(32) "我的性別：" ANSI_COLOR(37) "%.2s\n", u->realname, "？♂♀" + (u->sex << 1));

  prints(ANSI_COLOR(32) "  上站次數：" ANSI_COLOR(37) "%-35d" ANSI_COLOR(32) "文章篇數：" ANSI_COLOR(37) "%d\n", u->numlogins, u->numposts);

  prints(ANSI_COLOR(32) "  人氣指數：" ANSI_COLOR(37) "%-35d" ANSI_COLOR(32) "好奇指數：" ANSI_COLOR(37) "%d\n", u->bequery,u->toquery);

  prints(ANSI_COLOR(32) "  發送水球：" ANSI_COLOR(37) "%-35d" ANSI_COLOR(32) "接收水球：" ANSI_COLOR(37) "%d\n", u->numbmwtx, u->numbmwrx);

  prints(ANSI_COLOR(32) "  郵件信箱：" ANSI_COLOR(37) "%-35s" ANSI_COLOR(32) "個人信件：" ANSI_COLOR(37) "%d 封\n", u->email, rec_num(buf, sizeof(HDR)));

  if (ulevel & (PERM_MBOX | PERM_ALLADMIN))
      prints(ANSI_COLOR(32) "  我的心情：" ANSI_COLOR(37) "%-35s" ANSI_COLOR(32) "信箱配額：" ANSI_COLOR(37) "信箱無上限\n", u->feeling );
  else
      prints(ANSI_COLOR(32) "  我的心情：" ANSI_COLOR(37) "%-35s" ANSI_COLOR(32) "信箱配額：" ANSI_COLOR(37) "+%d 單位\n", u->feeling ,u->mquota);

  prints(ANSI_COLOR(32) "  出生日期：" ANSI_COLOR(37) "民國 %d 年 %02d 月 %02d 日 %s\n", u->year, u->month, u->day, (over18(u) ? "(已滿 18 歲)" : ""));

  prints(ANSI_COLOR(32) "  註冊日期：" ANSI_COLOR(37) "%s (已滿 %d 天)\n", Btime(&u->firstlogin), (time(NULL) - u->firstlogin) / 86400);

  prints(ANSI_COLOR(32) "  光臨日期：" ANSI_COLOR(37) "%-35s\n", Btime(&u->lastlogin), u->numbmwtx, u->numbmwrx);

  if (ulevel & PERM_ALLDENY)
  {
      /* yiting: 顯示停權天數 */
      outs(ANSI_COLOR(32) "  停權天數：" ANSI_COLOR(37) );
      if ((diff = u->tvalid - time(0)) < 0)
      {
	  outs("停權期限已到，可自行申請復權\n");
      }
      else
      {
	  /* 不滿一小時的部份加一小時計算，這樣顯示0小時就表示可以去復權了 */
	  diff += 3600;
	  prints("還有 %d 天 %d 小時\n", diff / 86400, (diff % 86400) / 3600);
      }
  }
  else
  {
      prints(ANSI_COLOR(32) "  身分認證：" ANSI_COLOR(37) "%s\n", (ulevel & PERM_VALID) ? Btime(&u->tvalid) : "請參考本站公佈欄進行確認，以提昇權限");
  }

  diff = u->staytime / 60;
  prints(ANSI_COLOR(32) "  掛站時數：" ANSI_COLOR(37) "%d 日 %d 時 %d 分\n", diff / 1440 , (diff % 1440) / 60, diff % 60);

  if (adm)
  {
    prints(ANSI_COLOR(32) "  上站地點：" ANSI_COLOR(37) "%-35s\n", u->lasthost);
    bitmsg(ANSI_COLOR(32) "  權限等級：" ANSI_COLOR(37), STR_PERM, ulevel);
    bitmsg(ANSI_COLOR(32) "  習慣旗標：" ANSI_COLOR(37), STR_UFO, u->ufo);
  }
  else
  {
    diff = (time(0) - ap_start) / 60;
    prints(ANSI_COLOR(32) "  停留期間：" ANSI_COLOR(37) "%d 小時 %d 分\n", diff / 60, diff % 60);
  }

  if (adm != 2)
  {
      /* Thor: 想看看這個 user 是那些板的板主 */
      if (ulevel & PERM_BM)
	  bm_list(uid);

      char address[60], phone[20];
      if (find_rform(u->userno, u->userid, address, phone))
      {
	  prints(ANSI_COLOR(32) "  註冊住址：" ANSI_COLOR(1;37) "%s\n", address);
	  prints(ANSI_COLOR(32) "  註冊電話：" ANSI_COLOR(1;37) "%s\n", phone);
      }

#ifdef NEWUSER_LIMIT
      if (u->lastlogin - u->firstlogin < 3 * 86400)
	  outs("\n  " ANSI_COLOR(36) "新手上路：三天後開放權限\n");
#endif
  }
  outs(ANSI_RESET);
}

void
acct_setup(u, adm)
  ACCT *u;
  int adm;
{
  ACCT x;
  int i, num;
  char *str, buf[80], pass[PSWDLEN + 1];
  char op, fpath[256];
  FILE *fp;
  UTMP *ui;
  pid_t pid;
  BMW bmw;

  acct_show(u, adm);
  memcpy(&x, u, sizeof(ACCT));

  if (adm)
  {
    op = vans("設定 1)資料 2)權限 3)習慣 4)上站畫面 5)退註 Q)取消 [Q] ");

    if (op > '5' || op < '1')
      return;
  }
  else
  {
    if (vans("修改資料(Y/N)？[N] ") != 'y')
      return;
    op = '1';
  }

  move(i = 3, 0);
  clrtobot();
  
  switch(op)
  {
    case '1':

    if (adm)
    {
	str = x.userid;
	for (;;)
	{
	/* itoc.010804.註解: 改使用者代號時請確定該 user 不在站上 */
	vget(i, 0, "使用者代號(不改請按 Enter)：", str, IDLEN + 1, GCARRY);
	if (!str_cmp(str, u->userid) || !acct_userno(str))
	    break;
	vmsg("錯誤！已有相同 ID 的使用者");
	}
    }
    else
    {
	vget(i, 0, "請確認密碼：", buf, PSWDLEN + 1, NOECHO);
	if (chkpasswd(u->passwd, buf))
	{
	vmsg("密碼錯誤");
	return;
	}
    }  

    /* itoc.030223: 只有 PERM_SYSOP 能變更其他站務的密碼 */
    if (!adm || !(u->userlevel & PERM_ALLADMIN) || HAS_PERM(PERM_SYSOP))
    { 
	i++;
	for (;;)
	{
	    if (!vget(i, 0, "設定新密碼(不改請按 Enter)：", buf, PSWDLEN + 1, NOECHO))
		break;

	    strcpy(pass, buf);
	    vget(i + 1, 0, "檢查新密碼：", buf, PSWDLEN + 1, NOECHO);
	    if (!strcmp(buf, pass))
	    {
		str_ncpy(x.passwd, genpasswd(buf), sizeof(x.passwd));
		break;
	    }
	}
    }

    i++;
    str = x.username;
    while (1)
    {
	if (vget(i, 0, "暱    稱：", str, UNLEN + 1, GCARRY))
	break;
    };

    i++;
    str = x.feeling;
    while (1)
    {
	if (vget(i, 0, "心    情：", str, FLLEN + 1, GCARRY))
	break;
    };

    i++;
    do
    {
	sprintf(buf, "生日－民國 %d 年：", u->year);
	if (!vget(i, 0, buf, buf, 4, DOECHO))
	break;
	x.year = atoi(buf);
    } while (x.year < 0 || x.year > 125);

    do
    {
	sprintf(buf, "生日－ %02d 月：", u->month);
	if (!vget(i, 0, buf, buf, 3, DOECHO))
	break;
	x.month = atoi(buf);
    } while (x.month < 0 || x.month > 12);
    do
    {
	sprintf(buf, "生日－ %02d 日：", u->day);
	if (!vget(i, 0, buf, buf, 3, DOECHO))
	break;
	x.day = atoi(buf);
    } while (x.day < 0 || x.day > 31);

    i++;
    sprintf(buf, "性別 (0)中性 (1)男性 (2)女性：[%d] ", u->sex);
    if (vget(i, 0, buf, buf, 3, DOECHO))
	x.sex = (*buf - '0') & 3;

    if (adm)
    {
	/* itoc.010317: 不讓 user 改姓名 */
	i++;
	str = x.realname;
	do
	{
	vget(i, 0, "真實姓名：", str, RNLEN + 1, GCARRY);
	} while (strlen(str) < 4);

	sprintf(buf, "%d", u->userno);
	vget(++i, 0, "用戶編號：", buf, 10, GCARRY);
	if ((num = atoi(buf)) > 0)
	x.userno = num;

	sprintf(buf, "%d", u->numlogins);
	vget(++i, 0, "上線次數：", buf, 10, GCARRY);
	if ((num = atoi(buf)) >= 0)
	x.numlogins = num;

	sprintf(buf, "%d", u->numposts);
	vget(++i, 0, "文章篇數：", buf, 10, GCARRY);
	if ((num = atoi(buf)) >= 0)
	x.numposts = num;

	/* itoc.010408: 新增金錢欄位 */
	sprintf(buf, "%d", u->money);
	vget(++i, 0, "銀    幣：", buf, 10, GCARRY);
	if ((num = atoi(buf)) >= 0)
	x.money = num;

	/* hrs.080115: mail quota... */
	sprintf(buf, "%d", u->mquota);
	vget(++i, 0, "信箱配額：", buf, 10, GCARRY);
	if ((num = atoi(buf)) >= 0)
	x.mquota = num;

	sprintf(buf, "%d", u->toquery);
	vget(++i, 0, "好 奇 度：", buf, 10, GCARRY);
	if ((num = atoi(buf)) >= 0)
	x.toquery = num;

	sprintf(buf, "%d", u->bequery);
	vget(++i, 0, "人 氣 度：", buf, 10, GCARRY);
	if ((num = atoi(buf)) >= 0)
	x.bequery = num;

	sprintf(buf, "%d", u->numbmwtx);
	vget(++i, 0, "傳水球數：", buf, 10, GCARRY);
	if ((num = atoi(buf)) >= 0)
	x.numbmwtx = num;
 
	sprintf(buf, "%d", u->numbmwrx);
	vget(++i, 0, "收水球數：", buf, 10, GCARRY);
	if ((num = atoi(buf)) >= 0)
	x.numbmwrx = num;

	sprintf(buf, "%d", u->numemails);
	vget(++i, 0, "發信次數：", buf, 10, GCARRY);
	if ((num = atoi(buf)) >= 0)
	x.numemails = num;

	vget(++i, 0, "上站地點：", x.lasthost, sizeof(x.lasthost), GCARRY);
	vget(++i, 0, "郵件信箱：", x.email, sizeof(x.email), GCARRY);
    }
    break;

    case '2':

	    i = setperm(num = x.userlevel);

	    if (i != num) 
	    {
		x.userlevel = i;

		/* itoc.011120: 站長放水加上認證通過權限，要附加改認證時間 */
		if ((i & PERM_VALID) && !(num & PERM_VALID))
		time(&x.tvalid);

		/* itoc.050413: 如果站長手動停權，就要由站長才能來復權 */
		if ((i & PERM_ALLDENY) && (i & PERM_ALLDENY) != (num & PERM_ALLDENY))
		x.tvalid = INT_MAX;
	    }
    break;

    case '3':

	x.ufo = bitset(x.ufo, NUMUFOS, NUMUFOS, MSG_USERUFO, ufo_tbl);

    break;

    case '4':

        x.loginview = bitset(x.loginview, NUMLVS, NUMLVS, "設定上站畫面", lv_tbl);

    break;

    case '5':

	if (!(x.userlevel & PERM_VALID))
	{
	    vmsg("使用者尚未通過認證");
	    break;
	}

        if (vans("確定要退回註冊狀態嗎? [y/N]") == 'y')
	{
	    if (!vget(b_lines, 0, "請輸入退回註冊原因: ", buf, 60, DOECHO))
	    {
    		vmsg("取消。");
		break;
	    }

	    x.userlevel &= ~PERM_ALLVALID;
	}

    break; 

  } // switch(op)

  if (!memcmp(&x, u, sizeof(ACCT)) || vans(msg_sure_ny) != 'y')
  {
    vmsg("取消修改。");
    return;
  }

  if (op == '5')
  {
            sprintf(fpath, "tmp/reject.%s", x.userid);
            if (fp = fopen(fpath, "w"))
            {
                /* 文章檔頭 */
                fprintf(fp, "%s %s (站務站長)\n",
                str_author1, cuser.userid);
                fprintf(fp, "標題: 您的註冊狀態被取消，請重新註冊\n時間: %s\n\n", Now());

                /* 文章內容 */
                fprintf(fp, "雖然您之前已經已經通過註冊，但經站務人員重新確認後\n\n"
                            "因為下列理由: %s，取消您的註冊狀態，請重新註冊\n", buf);
                fclose(fp);

                mail_him(fpath, x.userid, "您的註冊狀態被取消，請重新註冊", 0);
                unlink(fpath);
            }
            log_register(&x, (RFORM *) NULL, RFORM_CANCELREG, buf);

   }

  if (adm)
  {
    if (str_cmp(u->userid, x.userid))
    { /* Thor: 980806: 特別注意如果 usr每個字母不在同一partition的話會有問題 */
      char dst[80];

      usr_fpath(buf, u->userid, NULL);
      usr_fpath(dst, x.userid, NULL);
      rename(buf, dst);
      /* Thor.990416: 特別注意! .USR並未一併更新, 可能有部分問題 */
    }

    /* itoc.010811: 動態設定線上使用者 */
    /* 被站長改過資料的線上使用者(包括站長自己)，其 cutmp->status 會被加上 STATUS_DATALOCK
       這個旗標，就無法 acct_save()，於是站長便可以修改線上使用者資料 */
    /* 在站長修改過才上線的 ID 因為其 cutmp->status 沒有 STATUS_DATALOCK 的旗標，
       所以將可以繼續存取，所以線上如果同時有修改前、修改後的同一隻 ID multi-login，也是無妨。 */
    utmp_admset(x.userno, STATUS_DATALOCK | STATUS_COINLOCK);

    /* lkchu.981201: security log */
    adm_log(u, &x);
  }
  else
  {
    /* itoc.010804.註解: 線上的 userlevel/tvalid 是舊的，.ACCT 裡才是新的 */
    if (acct_load(u, x.userid) >= 0)
    {
      x.userlevel = u->userlevel;
      x.tvalid = u->tvalid;
    }
  }

  memcpy(u, &x, sizeof(ACCT));
  acct_save(u);
  if (adm && vans("是否要將被改資料的使用者踢下線，強制重新上站？ [Y/n] ") != 'n')
  {
    /* hrs.080514: 送水球通知被改資料的人要被踢下線了 */
    if (!(ui = (UTMP *) utmp_find(x.userno)))
            return;              /* user isn't logged in */
    strcpy(bmw.msg, "您的資料已受過修改，請重新上站");
    bmw.caller = cutmp;
    bmw.sender = 0;                  /* 不可回覆 */
    bmw.recver = x.userno;
    strcpy(bmw.userid, "系統通知");
    bmw_send(ui, &bmw);
  
    sleep(2);

    /* hrs.080514: 把被修改資料的人踢下線 */
    pid = ui->pid;
    if ((kill(pid, SIGTERM) == -1) && (errno == ESRCH))
      utmp_free(ui);
    else
      sleep(3);                 /* 被踢的人這時候正在自我了斷 */
    blog("RESET", cuser.userid);
  }
}


#if 0	/* itoc.010805.註解 */

  認證成功只加上 PERM_VALID，讓 user 在下次進站才自動得到 PERM_POST | PERM_PAGE | PERM_CHAT
  以免新手上路、停權的功能失效

  但重填 email 拿掉認證者需拿掉 PERM_VALID | PERM_POST | PERM_PAGE | PERM_CHAT
  否則 user 可以在下次進站前任意使用 bbs_post

#endif

#if 0	/* itoc.010831.註解 */

  因為線上 cuser.userlevel 並不是最新的，使用者如果在線上認證或是被停權，
  硬碟中的 .ACCT 寫的才是正確的 userlevel，
  所以要先讀出 .ACCT，加入 level 後再蓋回去。

  使用 acct_seperm(&acct, adm) 之前要先 acct_load(&acct, userid)，
  其中 &acct 不能是 &cuser。
  使用者要重新上站才會換成新的權限。

#endif

void
acct_setperm(u, levelup, leveldown)	/* itoc.000219: 加/減權限程式 */
  ACCT *u;
  usint levelup;		/* 加權限 */
  usint leveldown;		/* 減權限 */
{
  u->userlevel |= levelup;
  u->userlevel &= ~leveldown;

  acct_save(u);
}


/* ----------------------------------------------------- */
/* 增加金銀幣						 */
/* ----------------------------------------------------- */


void
addmoney(addend)
  int addend;
{
  if (addend < (INT_MAX - cuser.money))	/* 避免溢位 */
    cuser.money += addend;
  else
    cuser.money = INT_MAX;
}



void
acct_bequery(acct)
  ACCT *acct;
{
  acct->bequery++;
  acct_save(acct);
}


/* ----------------------------------------------------- */
/* 看板管理						 */
/* ----------------------------------------------------- */


#ifndef HAVE_COSIGN
static
#endif
int			/* 1:合法的板名 */
valid_brdname(brd)
  char *brd;
{
  int ch;

  if (!is_alnum(*brd))
    return 0;

  while (ch = *++brd)
  {
    if (!is_alnum(ch) && ch != '.' && ch != '-' && ch != '_')
      return 0;
  }
  return 1;
}


/* itoc.010212: 開新板/修改看板自動加上板主權限. */
/* 目前的作法是一輸入完 id 就加入板主權限，即使最後選擇不變動，
   如果因此多加了板主權限，在 reaper.c 中拿下 */

int brd_changeBM(BRD *oldbrd)
{
    char buf[80], userid[IDLEN + 2], *blist;
    ACCT acct;
    int BMlen, len, adm;

    blist = oldbrd->BM;
    adm = HAS_PERM(PERM_ALLBOARD);

    if (!adm && is_bm(blist, cuser.userid) != 1)	/* 只有板務總管和第一位板主可以設定名單 */
    {
	pmsg("只有板務總管和第一位板主可以設定名單");
	return XO_HEAD;
    }

    if (!(oldbrd->battr & BRD_PERSONAL) && !adm)
    {
	pmsg("非個人看板更動板主請至相關看板提出申請。");
	return XO_HEAD;
    }

    move(3, 0);
    clrtobot();

    move(8, 0);
    prints("目前板主為 %s\n請輸入新的板主名單，或按 [Return] 不改", oldbrd->BM);

    strcpy(buf, oldbrd->BM);
    BMlen = strlen(buf);

    while (vget(10, 0, "請輸入副板主，結束請按 Enter，清掉所有副板主請打「無」：", userid, IDLEN + 1, DOECHO))
    {
	if (!strcmp(userid, "無"))
	{
	    if (adm)
	    {
		*buf = '\0';
		BMlen = 0;
	    }
	    else
	    {
		strcpy(buf, cuser.userid);
		BMlen = strlen(buf);
	    }
	}
	else if (is_bm(buf, userid))	/* 刪除舊有的板主 */
	{
	    len = strlen(userid);
	    if (!str_cmp(cuser.userid, userid) && !adm)
	    {
		vmsg("不可以將自己移出板主名單");
		continue;
	    }
	    else if (BMlen == len)
	    {
		*buf = '\0';
	    }
	    else if (!str_cmp(buf + BMlen - len, userid) && buf[BMlen - len - 1] == '/')	
		/* 名單上最後一位，ID 後面不接 '/' */
	    {
		buf[BMlen - len - 1] = '\0';			/* 刪除 ID 及前面的 '/' */
		len++;
	    }
	    else						/* ID 後面會接 '/' */
	    {
		str_lower(userid, userid);
		strcat(userid, "/");
		len++;
		blist = str_str(buf, userid);
		strcpy(blist, blist + len);
	    }
	    BMlen -= len;
	}
	else if (acct_load(&acct, userid) >= 0 && !is_bm(buf, userid))	/* 輸入新板主 */
	{
	    len = strlen(userid) + 1;	/* '/' + userid */
	    if (BMlen + len > BMLEN)
	    {
		vmsg("板主名單過長，無法將這 ID 設為板主");
		continue;
	    }
	    if (BMlen)
	    {
		sprintf(buf + BMlen, "/%s", acct.userid);
		BMlen += len;
	    }
	    else
	    {
		strcpy(buf, acct.userid);
		BMlen = strlen(acct.userid);
	    }

	    acct_setperm(&acct, PERM_BM, 0);
	}
	else
	    continue;

	move(8, 0);
	prints("目前板主為 %s", buf);
	clrtoeol();
    }
    strcpy(oldbrd->BM, buf);
    return XO_HEAD;
}


int brd_setexpire(BRD *brd, int y)
{
    char buf[10];

    if (vget(y, 0, "設定文章上下限(Y/N)？[N] ", buf, 3, LCECHO) == 'y')
    {
	sprintf(buf, "%d", brd->ex_maxposts);
	vget(y, 0, "文章上限(篇): ", buf, 6, GCARRY);
	brd->ex_maxposts = ((atoi(buf)) > 0 ? atoi(buf) : DEF_MAXP);

	sprintf(buf, "%d", brd->ex_minposts);
	vget(y, 0, "文章下限(篇): ", buf, 6, GCARRY);
	brd->ex_minposts = (atoi(buf) > 0 ? atoi(buf) : DEF_MINP);

	sprintf(buf, "%d", brd->ex_maxtime);
	vget(y, 0, "文章保留天數(天): ", buf, 6, GCARRY);
	brd->ex_maxtime = (atoi(buf) > 0 ? atoi(buf) : DEF_DAYS);

	return 0;
    }
    else
	return -1;
}


int brd_setlimit(BRD *brd, int y)
{
    char buf[10];

    if (vget(y, 0, "設定連署/發文門檻(Y/N)？[N] ", buf, 3, LCECHO) == 'y')
    {
	sprintf(buf, "%d", brd->limit_posts);
	vget(y, 0, "發表文章(篇): ", buf, 5, GCARRY);
	brd->limit_posts = ((atoi(buf)) > 0 ? atoi(buf) : 0);

	sprintf(buf, "%d", brd->limit_logins);
	vget(y, 0, "上站次數(次): ", buf, 5, GCARRY);
	brd->limit_logins = (atoi(buf) > 0 ? atoi(buf) : 0);

	sprintf(buf, "%d", brd->limit_regtime);
	vget(y, 0, "註冊時間(月): ", buf, 4, GCARRY);
	brd->limit_regtime = (atoi(buf) > 0 ? atoi(buf) : 0);

	return 0;
    }
    else
	return -1;
}


static int
brd_set(brd, row)
  BRD *brd;
  int row;
{
  int i;
  char *brdname, buf[80];

  i = row;
  brdname = brd->brdname;
  strcpy(buf, brdname);

  for (;;)
  {
    if (!vget(i, 0, MSG_BID, brdname, BNLEN + 1, GCARRY))
    {
      if (i == 1)	/* 開新板若無輸入板名表示離開 */
	return -1;

      strcpy(brdname, buf);	/* Thor: 若是清空則設為原名稱 */
      continue;
    }

    if (!valid_brdname(brdname))
      continue;

    if (!str_cmp(buf, brdname))	/* Thor: 與舊板原名相同則跳過 */
      break;

    if (brd_bno(brdname) >= 0)
      outs("\n錯誤！板名雷同");
    else
      break;
  }

  vget(++i, 0, "看板分類：", brd->class, BCLEN + 1, GCARRY);
  vget(++i, 0, "看板主題：", brd->title, BTLEN + 1, GCARRY);

  /* vget(++i, 0, "板主名單：", brd->BM, BMLEN + 1, GCARRY); */

  move(++i, 0);
  prints("目前板主為：%s", *brd->BM < ' ' ? "(無)" : brd->BM);

  if (vget(++i, 0, "設定板主名單嗎？ [y/N] ", buf, 3, LCECHO) == 'y')
  {
    brd_changeBM(brd);
    clrregion(8, 10);
  }

#ifdef HAVE_MODERATED_BOARD
  /* itoc.011208: 改用較便利的看板權限設定 */
  switch (vget(++i, 0, "看板權限 A)一般 B)自定 C)秘密 D)好友 E)封印？[Q] "
	      , buf, 3, LCECHO))
  {
  case 'c':
    brd->bmode = BMODE_HIDE;
    brd->readlevel = 0;	
    brd->postlevel = PERM_POST;
    brd->battr |= (BRD_NOSTAT | BRD_NOVOTE);
    break;

  case 'd':
    brd->bmode = BMODE_PAL;
    brd->readlevel = 0;	
    brd->postlevel = PERM_POST;
    brd->battr |= (BRD_NOSTAT | BRD_NOVOTE);
    break;
#else
  switch (vget(++i, 0, "看板權限 A)一般 B)自定？[Q] ", buf, 3, LCECHO))
  {
#endif

  case 'e':
    brd->bmode = BMODE_SEAL;
    brd->readlevel = 0; 
    brd->postlevel = PERM_POST;
    brd->battr |= (BRD_NOSTAT | BRD_NOVOTE);
    break;

  case 'a':
    brd->bmode = BMODE_OPEN;
    brd->readlevel = 0;
    brd->postlevel = PERM_POST;		/* 一般看板發表權限為 PERM_POST */
    brd->battr &= ~(BRD_NOSTAT | BRD_NOVOTE);	/* 拿掉好友＆秘密板屬性 */
    break;

  case 'b':
    if (vget(++i, 0, "閱\讀權限(Y/N)？[N] ", buf, 3, LCECHO) == 'y')
    {
      brd->readlevel = bitset(brd->readlevel, NUMPERMS, NUMPERMS, MSG_READPERM, perm_tbl);
      move(2, 0);
      clrtobot();
      i = 1;
    }

    if (vget(++i, 0, "發表權限(Y/N)？[N] ", buf, 3, LCECHO) == 'y')
    {
      brd->postlevel = bitset(brd->postlevel, NUMPERMS, NUMPERMS, MSG_POSTPERM, perm_tbl);
      move(2, 0);
      clrtobot();
      i = 1;
    }
    break;

  default:	/* 預設不變動 */
    break;
  }

  if (brd_setexpire(brd, ++i) < 0 && row == 1)
  {
      brd->ex_maxposts = DEF_MAXP;
      brd->ex_minposts = DEF_MINP;
      brd->ex_maxtime = DEF_DAYS;
  }

  if (brd_setlimit(brd, ++i) < 0 && row == 1)
  {
      brd->limit_posts = brd->limit_logins = brd->limit_regtime = 0;
  }

  vmsg("設定屬性");

  brd->battr = bitset(brd->battr, NUMBATTRS, NUMBATTRS, MSG_BRDATTR, battr_tbl);

  return 0;
}

int			/* 0:開板成功 -1:開板失敗 */
brd_new(brd)
  BRD *brd;
{
  int bno;
  char fpath[64], folder[256];
  HDR hdr;

  vs_bar("建立新板");

  if (brd_set(brd, 1))
    return -1;

  if (vans(msg_sure_ny) != 'y')
    return -1;

  if (brd_bno(brd->brdname) >= 0)
  {
    vmsg("錯誤！板名雷同，可能有其他站務剛開啟此板");
    return -1;
  }

  time(&brd->bstamp);
  if ((bno = brd_bno("")) >= 0)
  {
    rec_put(FN_BRD, brd, sizeof(BRD), bno, NULL);
  }
  /* Thor.981102: 防止超過shm看板個數 */
  else if (bshm->number >= MAXBOARD)
  {
    vmsg("超過系統所能容納看板個數，請調整系統參數");
    return -1;
  }
  else if (rec_add(FN_BRD, brd, sizeof(BRD)) < 0)
  {
    vmsg("無法建立新板");
    return -1;
  }

  gem_fpath(fpath, brd->brdname, NULL);
  mak_dirs(fpath);
  mak_dirs(fpath + 4);

  bshm_reload();		/* force reload of bcache */

  brd_fpath(folder, brd->brdname, fn_dir);
  hdr_stamp(folder, HDR_COPY | 'A', &hdr, "etc/newboard");
  strcpy(hdr.owner, str_sysop);
  strcpy(hdr.nick, SYSOPNICK);
  strcpy(hdr.title, "[公告] 看板成立");
  hdr.xmode = POST_MARKED;
  rec_add(folder, &hdr, sizeof(HDR));

  ann_newbrd(brd->brdname);
  log_newbrd(brd->brdname);

  bshm_reload();                /* force reload of bcache */
  system("bin/account -nokeeplog");

  brh_save();
  board_main();			/* reload brd_bits[] */

  return 0;
}


void
brd_classchange(folder, oldname, newbrd)	/* itoc.020117: 異動 @Class 中的看板 */
  char *folder;
  char *oldname;
  BRD *newbrd;		/* 若為 NULL，表示要刪除看板 */
{
  int pos, xmode;
  char fpath[64];
  HDR hdr;

  pos = 0;
  while (!rec_get(folder, &hdr, sizeof(HDR), pos))
  {
    xmode = hdr.xmode & (GEM_BOARD | GEM_FOLDER);

    if (xmode == (GEM_BOARD | GEM_FOLDER))	/* 看板精華區捷徑 */
    {
      if (!strcmp(hdr.xname, oldname))
      {
	if (newbrd)	/* 看板更名 */
	{
	  brd2gem(newbrd, &hdr);
	  rec_put(folder, &hdr, sizeof(HDR), pos, NULL);
	}
	else		/* 看板刪除 */
	{
	  rec_del(folder, sizeof(HDR), pos, NULL);
	  continue;	/* rec_del 以後不需要 pos++ */
	}
      }
    }
    else if (xmode == GEM_FOLDER)		/* 分類 recursive 進去砍 */
    {
      hdr_fpath(fpath, folder, &hdr);
      brd_classchange(fpath, oldname, newbrd);
    }
    pos++;
  }
}


void
brd_edit(bno)
  int bno;
{
  BRD *bhdr, newbh;
  char *bname, src[64], dst[64], oldname[BNLEN + 1];
  const char *bmodemsg[4] = {"公開", "隱藏", "好友", "封印"};

  vs_bar("看板設定");
  bhdr = bshm->bcache + bno;
  memcpy(&newbh, bhdr, sizeof(BRD));
  prints("看板名稱：%s\n看板說明：[%s] %s\n板主名單：%s\n",
    newbh.brdname, newbh.class, newbh.title, newbh.BM);

  bitmsg(MSG_READPERM, STR_PERM, newbh.readlevel);
  bitmsg(MSG_POSTPERM, STR_PERM, newbh.postlevel);
  bitmsg(MSG_BRDATTR, STR_BATTR, newbh.battr);
  prints("看板狀態： %d (%s板)", (int)bhdr->bmode, bmodemsg[bhdr->bmode]);

  switch (vget(8, 0, "(D)刪除 (E)設定 (S)直接封印 (U)解除封印 (Q)取消？[Q] ", src, 3, LCECHO))
  {
  case 'd':

    if (vget(9, 0, msg_sure_ny, src, 3, LCECHO) != 'y')
    {
      vmsg(MSG_DEL_CANCEL);
    }
    else
    {
      bname = bhdr->brdname;
      if (*bname)	/* itoc.000512: 同時砍除同一個看板會造成精華區、看板全毀 */
      {
	alog("刪除看板", bname);

	gem_fpath(src, bname, NULL);
	f_rm(src);
	f_rm(src + 4);
	brd_classchange("gem/@/@"CLASS_INIFILE, bname, NULL);	/* itoc.020117: 刪除 @Class 中的看板精華區捷徑 */
	memset(&newbh, 0, sizeof(BRD));
	sprintf(newbh.title, "[%s] deleted by %s", bname, cuser.userid);
	memcpy(bhdr, &newbh, sizeof(BRD));
	rec_put(FN_BRD, &newbh, sizeof(BRD), bno, NULL);

	/* itoc.050531: 砍板會造成看板不是按字母排序，所以要修正 numberOld */
	if (bshm->numberOld > bno)
	  bshm->numberOld = bno;

	vmsg("刪板完畢");
      }
    }
    break;

  case 'e':

    move(9, 0);
    outs("直接按 [Return] 不修改該項設定");

    if (!brd_set(&newbh, 11))
    {
      if (memcmp(&newbh, bhdr, sizeof(BRD)) && vans(msg_sure_ny) == 'y')
      {
	strcpy (oldname, bhdr->brdname);
	if (strcmp(oldname, newbh.brdname))	/* 看板更名要移目錄 */
	{
	  /* Thor.980806: 特別注意如果看板不在同一partition裡的話會有問題 */
	  gem_fpath(src, oldname, NULL);
	  gem_fpath(dst, newbh.brdname, NULL);
	  rename(src, dst);
	  rename(src + 4, dst + 4);

	  /* itoc.050520: 改了板名會造成看板不是按字母排序，所以要修正 numberOld */
	  if (bshm->numberOld > bno)
	    bshm->numberOld = bno;
	}
	memcpy(bhdr, &newbh, sizeof(BRD));
	rec_put(FN_BRD, &newbh, sizeof(BRD), bno, NULL);
      }
      brd_classchange("gem/@/@"CLASS_INIFILE, oldname, &newbh);
    }
    vmsg("設定完畢");
    break;

  case 's':
        if (vans(msg_sure_ny) == 'y')
	{
	     if (newbh.bmode == BMODE_SEAL)
	     {
		 vmsg("已經封印了!!");
		 break;
	     }
	     newbh.bmode = BMODE_SEAL;
	     newbh.battr |= (BRD_NOSTAT | BRD_NOVOTE);

	     memcpy(bhdr, &newbh, sizeof(BRD));
	     rec_put(FN_BRD, &newbh, sizeof(BRD), bno, NULL);
	}
	break;

  case 'u':

	if (vans(msg_sure_ny) == 'y')
	{
	    if (newbh.bmode != BMODE_SEAL)
	    {
	        vmsg("本來就沒封印!!");
                break;
            }
            newbh.bmode = vans("把看板屬性改成 (0)公開 (1)隱藏 (2)好友 ") - '0';
	    if (newbh.bmode > BMODE_PAL)
	    {
		vmsg("取消。");
		break;
	    }
	    if (newbh.bmode == BMODE_OPEN)
		newbh.battr &= ~(BRD_NOSTAT | BRD_NOVOTE);

             memcpy(bhdr, &newbh, sizeof(BRD));
             rec_put(FN_BRD, &newbh, sizeof(BRD), bno, NULL);
	}
	break;
  }
}


void
brd_title(bno)		/* itoc.000312: 板主修改中文敘述 */
  int bno;
{
  BRD *bhdr, newbh;

  bhdr = bshm->bcache + bno;
  memcpy(&newbh, bhdr, sizeof(BRD));

  if (brd_bits[bno] & BRD_M_BIT || HAS_PERM(PERM_ALLBOARD))
  {
    if (vans("是否修改中文板名敘述(Y/N)？[N] ") == 'y')
    {
      vget(b_lines, 0, "看板主題：", newbh.title, BTLEN + 1, GCARRY);
      memcpy(bhdr, &newbh, sizeof(BRD));
      rec_put(FN_BRD, &newbh, sizeof(BRD), bno, NULL);
      brd_classchange("gem/@/@"CLASS_INIFILE, bhdr->brdname, &newbh);
    }
  }
}
