/*-------------------------------------------------------*/
/* bank.c	( NTHU CS MapleBBS Ver 3.10 )		 */
/*-------------------------------------------------------*/
/* target : 銀行、購買權限功能				 */
/* create : 01/07/16					 */
/* update :   /  /  					 */
/* author : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/*-------------------------------------------------------*/


#include "bbs.h"

#ifdef HAVE_BUY

static void
x_give()
{
  int dollar, i;
  int sysopmode = 0;
  char userid[IDLEN + 1], buf[200], passbuf[PASSLEN + 1];
  char folder[64], fpath[64], reason[40];
  HDR hdr;
  FILE *fp;
  time_t now;
  PAYCHECK paycheck;

  if(HAS_PERM(PERM_ALLADMIN))
    sysopmode = vget(13, 0, "您要 1)普通轉帳(扣10%手續費) 2)站長發獎金:", buf, 2, DOECHO) - '1';

  if (sysopmode < 0 || sysopmode > 1)
    return;

  if (!vget(15, 0, "幸運兒的ＩＤ？ ", userid, IDLEN + 1, DOECHO))
    return;

  if (acct_userno(userid) <= 0)
  {
    vmsg(err_uid);
    return;
  }

  if(!str_cmp(userid, cuser.userid) && !sysopmode)
  {
    vmsg("交易取消。");         /* hrs.071226:轉給自己沒意思 */
    return;
  }

  do
  {
    if (!vget(17, 0, "要給多少錢？ ", buf, 9, DOECHO))
    /* 最多轉 99999999 避免溢位 */
      return;

    dollar = atoi(buf);

      if (dollar > cuser.money && !sysopmode)
        dollar = cuser.money;   /* 全轉過去 */

  } while (dollar <= 1);        /* 不能只轉 1，會全變手續費 */

  if (!vget(19, 0, "請輸入原因：", reason, 40, DOECHO))
    strcpy(reason, "招財進寶^_^");

  sprintf(buf, "是否給 %s 銀幣 %d 元 (Y/N)？[N] "
    , userid, dollar);

  if (vget(21, 0, buf, fpath, 3, LCECHO) == 'y')
  {

    i = 3;

    while(i--)
    {
        if (!vget(b_lines - 2, 0, "為了避免被詐騙或是帳號遭到盜用，請輸入您的密碼： "
            , passbuf, PSWDLEN + 1, NOECHO))
        continue;

        if (chkpasswd(cuser.passwd, passbuf))
	{
	    outl(b_lines - 1, ERR_PASSWD);
	    if (!i)
	    {
		vmsg("密碼三次輸入錯誤，取消交易。");
		return;
	    }
	    continue;
	}
	break;
    }

    if (!sysopmode)
    {
        cuser.money -= dollar;

        dollar -= dollar / 10 + ((dollar % 10) ? 1 : 0);        /* 10% 手續費 */
    }

    /* itoc.020831: 加入匯錢記錄 */
    time(&now);
    if (sysopmode)
    {
        sprintf(buf, "%-13s銀幣 %d元", userid, dollar);
        alog("發放獎金",buf);
    }
    else
    {
    sprintf(buf, "%-13s轉給 %-13s計 %d 元 (%s)\n",
    cuser.userid, userid, dollar, Btime(&now));
    f_cat(FN_RUN_BANK_LOG, buf);
    }

    usr_fpath(folder, userid, fn_dir);

    if (fp = fdopen(hdr_stamp(folder, 0, &hdr, fpath), "w"))
    {
      fprintf(fp, "%s %s (%s)\n標題: %s\n時間: %s\n\n",
        str_author1, cuser.userid, cuser.username
        , (sysopmode ? "獎金通知" : "轉帳通知"), Btime(&now));
      fprintf(fp, "%s\n原因是：%s\n\n請您至金融中心將支票兌現", buf, reason);
      fclose(fp);
      sprintf(hdr.title, "%s", (sysopmode ? "獎金通知":"轉帳通知"));
      strcpy(hdr.owner, cuser.userid);
      rec_add(folder, &hdr, sizeof(HDR));
    }

    memset(&paycheck, 0, sizeof(PAYCHECK));
    time(&paycheck.tissue);
    paycheck.money = dollar;

    sprintf(paycheck.reason, "[轉帳] %s", cuser.userid);
    usr_fpath(fpath, userid, FN_PAYCHECK);
    rec_add(fpath, &paycheck, sizeof(PAYCHECK));

    if (!sysopmode)
    vmsgf("交易成功\，餘額 %d 元", cuser.money);
    else
    vmsg(NULL);
  }
  else
  {
    vmsg("取消交易");
  }
}


static void
x_cash()
{
  int fd, money;
  char fpath[64], buf[64];
  FILE *fp;
  PAYCHECK paycheck;

  usr_fpath(fpath, cuser.userid, FN_PAYCHECK);
  if ((fd = open(fpath, O_RDONLY)) < 0)
  {
    vmsg("您目前沒有支票未兌現");
    return;
  }

  usr_fpath(buf, cuser.userid, "cashed");
  fp = fopen(buf, "w");
  fputs("以下是您的支票兌換清單：\n\n", fp);

  money = 0;
  while (read(fd, &paycheck, sizeof(PAYCHECK)) == sizeof(PAYCHECK))
  {
    if (paycheck.money < (INT_MAX - money))	/* 避免溢位 */
      money += paycheck.money;
    else
      money = INT_MAX;

    fprintf(fp, "%s %s %d 銀\n", 
      Btime(&paycheck.tissue), paycheck.reason, paycheck.money);
  }
  close(fd);
  unlink(fpath);

  fprintf(fp, "\n您共兌現 %d 銀\n", money);
  fclose(fp);

  addmoney(money);

  more(buf, NULL);
  unlink(buf);
}


int
x_bank()
{
  char ans[3];

  if (HAS_STATUS(STATUS_COINLOCK))
  {
    vmsg(msg_coinlock);
    return XEASY;
  }

  vs_bar("信託銀行");
  move(2, 0);

  /* itoc.011208: 以防萬一 */
  if (cuser.money < 0)
    cuser.money = 0;

  outs("\033[1;36m  ╭═════════════════════════════╮\n");
  prints("  ║\033[32m您現在有銀幣 \033[33m%12d\033[32m 元                      \033[36m        ║\n", 
    cuser.money);
  outs("  ╠═════════════════════════════╣\n"
    "  ║ 目前銀行提供下列幾項服務：                               ║\n"
    "  ║\033[33m1.\033[37m 轉帳 -- 轉帳給其他人   (抽取 10% 手續費) \033[36m              ║\n"
    "  ║\033[33m2.\033[37m 兌現 -- 支票兌現                         \033[36m              ║\n"
    "  ╰═════════════════════════════╯\033[m");

  vget(11, 0, "請輸入您需要的服務：", ans, 3, DOECHO);
  if (ans[0] == '1')
    x_give();
  else if (ans[0] == '2')
    x_cash();

  return 0;
}

int
b_invis()
{
#ifndef HAVE_FREECLOAK
  if (HAS_STATUS(STATUS_COINLOCK))
  {
    vmsg(msg_coinlock);
    return XEASY;
  }

  if (cuser.ufo & UFO_CLOAK)
  {
    if (vans("是否現身(Y/N)？[N] ") != 'y')
      return XEASY; 
    /* 現身免費 */
  }
  else
  {
    if (HAS_PERM(PERM_CLOAK))
    {
      if (vans("是否隱形(Y/N)？[N] ") != 'y')
	return XEASY;
      /* 有無限隱形權限者免費 */
    }
    else
    {
      if (cuser.money < atoi(FEE_CLOAK))
      {
	vmsg("要 " FEE_CLOAK " 元才能隱形喔");
	return XEASY;
      }
      if (vans("是否花 " FEE_CLOAK " 元隱形(Y/N)？[N] ") != 'y')
	return XEASY;
      cuser.money -= atoi(FEE_CLOAK);
    }
  }
#endif

  cuser.ufo ^= UFO_CLOAK;
  cutmp->ufo ^= UFO_CLOAK;	/* ufo 要同步 */

  pmsg(cuser.ufo & UFO_CLOAK ? "隱形成功\!" : "現身成功\!");
  return XEASY;
}

int
b_mquota()
{
  int fee;
  char buf[5],genbuf[256];

  if (HAS_STATUS(STATUS_COINLOCK))
  {
    vmsg(msg_coinlock);
    return XEASY;
  }

  if (HAS_PERM(PERM_ALLADMIN) || HAS_PERM(PERM_MBOX))
  {
    vmsg("您已經信箱無上限了!");
    return XEASY;
  }
  move (b_lines - 3, 0);
  prints (ANSI_COLOR(1;30) "你曾經購買了 "
	  ANSI_COLOR(37) "%d" ANSI_COLOR(30)" 單位信箱容量\n"
	  , cuser.mquota);
  prints (ANSI_COLOR(1;30) "一個單位會增加 1 封信件配額而且 15 KB 容量限制。\n");
  prints (ANSI_COLOR(1;30) "你現在身上有"
	  ANSI_COLOR(37) " %d " ANSI_COLOR(30) "元", cuser.money);
  prints (ANSI_RESET);
  if (!vget(b_lines, 0, "買一單位信箱容量要 " FEE_MQUOTA " 元，你要買幾單位？ "
       , buf, 4, DOECHO))
    return 0;

  fee = atoi(buf);

  if (fee <= 0)
      return 0;

  if (fee * atoi(FEE_MQUOTA) > cuser.money)
  {
      vmsg("現金不足。");
      return 0;
  }
  else
  {
      cuser.money -= fee * atoi(FEE_MQUOTA);
      cuser.mquota += fee;
  }

  sprintf(genbuf
	  , "您購買了 %d 單位, 購買後配額 %d 單位。 (請立刻照正常程序重新上站)"
	  , fee, cuser.mquota);
  pmsg(genbuf);
  return 0;
}

#if 0

static void
buy_level(userlevel)		/* itoc.010830: 只存 level 欄位，以免變動到在線上更動的認證欄位 */
  usint userlevel;
{
  if (!HAS_STATUS(STATUS_DATALOCK))	/* itoc.010811: 要沒有被站長鎖定，才能寫入 */
  {
    int fd;
    char fpath[80];
    ACCT tuser;

    usr_fpath(fpath, cuser.userid, fn_acct);
    fd = open(fpath, O_RDWR);
    if (fd >= 0)
    {
      if (read(fd, &tuser, sizeof(ACCT)) == sizeof(ACCT))
      {
	tuser.userlevel |= userlevel;
	lseek(fd, (off_t) 0, SEEK_SET);
	write(fd, &tuser, sizeof(ACCT));
	vmsg("您已經獲得權限，請重新上站");
      }
      close(fd);
    }
  }
}


int
b_cloak()
{
  if (HAS_STATUS(STATUS_COINLOCK))
  {
    vmsg(msg_coinlock);
    return XEASY;
  }

  if (HAS_PERM(PERM_CLOAK))
  {
    vmsg("您已經能無限隱形了");
  }
  else
  {
    if (cuser.gold < 1000)
    {
      vmsg("要 1000 金幣才能購買無限隱形權限喔");
    }
    else if (vans("是否花 1000 金幣購買無限隱形權限(Y/N)？[N] ") == 'y')
    {
      cuser.gold -= 1000;
      buy_level(PERM_CLOAK);
    }
  }

  return XEASY;
}


int
b_mbox()
{
  if (HAS_STATUS(STATUS_COINLOCK))
  {
    vmsg(msg_coinlock);
    return XEASY;
  }

  if (HAS_PERM(PERM_MBOX))
  {
    vmsg("您的信箱已經沒有上限了");
  }
  else
  {
    if (cuser.gold < 1000)
    {
      vmsg("要 1000 金幣才能購買信箱無限權限喔");
    }
    else if (vans("是否花 1000 金幣購買信箱無限權限(Y/N)？[N] ") == 'y')
    {
      cuser.gold -= 1000;
      buy_level(PERM_MBOX);
    }
  }

  return XEASY;
}


int
b_xempt()
{
  if (HAS_STATUS(STATUS_COINLOCK))
  {
    vmsg(msg_coinlock);
    return XEASY;
  }

  if (HAS_PERM(PERM_XEMPT))
  {
    vmsg("您的帳號已經永久保留了");
  }
  else
  {
    if (cuser.gold < 1000)
    {
      vmsg("要 1000 金幣才能購買帳號永久保留權限喔");
    }
    else if (vans("是否花 1000 金幣購買帳號永久保留權限(Y/N)？[N] ") == 'y')
    {
      cuser.gold -= 1000;
      buy_level(PERM_XEMPT);
    }
  }

  return XEASY;
}


#if 0	/* 不提供購買自殺功能 */
int
b_purge()
{
  if (HAS_STATUS(STATUS_COINLOCK))
  {
    vmsg(msg_coinlock);
    return XEASY;
  }

  if (HAS_PERM(PERM_PURGE))
  {
    vmsg("系統在下次定期清帳號時，將清除此 ID");
  }
  else
  {
    if (cuser.gold < 1000)
    {
      vmsg("要 1000 金幣才能自殺喔");
    }
    else if (vans("是否花 1000 金幣自殺(Y/N)？[N] ") == 'y')
    {
      cuser.gold -= 1000;
      buy_level(PERM_PURGE);
    }
  }

  return XEASY;
}
#endif

#endif
#endif	/* HAVE_BUY */
