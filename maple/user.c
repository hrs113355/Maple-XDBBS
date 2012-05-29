/*-------------------------------------------------------*/
/* user.c	( NTHU CS MapleBBS Ver 3.00 )		 */
/*-------------------------------------------------------*/
/* target : account / user routines		 	 */
/* create : 95/03/29				 	 */
/* update : 96/04/05				 	 */
/*-------------------------------------------------------*/


#include "bbs.h"


extern char *ufo_tbl[];
extern char *lv_tbl[];

/* ----------------------------------------------------- */
/* 認證用函式						 */
/* ----------------------------------------------------- */


void
justify_log(userid, justify)	/* itoc.010822: 拿掉 .ACCT 中 justify 這欄位，改記錄在 FN_JUSTIFY */
  char *userid;
  char *justify;	/* 認證資料 RPY:email-reply  KEY:認證碼  POP:pop3認證  REG:註冊單 */
{
  char fpath[64];
  FILE *fp;

  usr_fpath(fpath, userid, FN_JUSTIFY);
  if (fp = fopen(fpath, "a"))		/* 用附加檔案，可以保存歷次認證記錄 */
  {
    fprintf(fp, "%s\n", justify);
    fclose(fp);
  }
}


static int
ban_addr(addr)
  char *addr;
{
  char *host;
  char foo[128];	/* SoC: 放置待檢查的 email address */

  /* Thor.991112: 記錄用來認證的email */
  sprintf(foo, "%s # %s (%s)\n", addr, cuser.userid, Now());
  f_cat(FN_RUN_EMAILREG, foo);

  /* SoC: 保持原 email 的大小寫 */
  str_lower(foo, addr);

  /* check for acl (lower case filter) */

  host = (char *) strchr(foo, '@');
  *host = '\0';

  /* *.bbs@xx.yy.zz、*.brd@xx.yy.zz 一律不接受 */
  if (host > foo + 4 && (!str_cmp(host - 4, ".bbs") || !str_cmp(host - 4, ".brd")))
    return 1;

  /* 不在白名單上或在黑名單上 */
  return (!acl_has(TRUST_ACLFILE, foo, host + 1) ||
    acl_has(UNTRUST_ACLFILE, foo, host + 1) > 0);
}


/* ----------------------------------------------------- */
/* POP3 認證						 */
/* ----------------------------------------------------- */


#ifdef HAVE_POP3_CHECK

static int		/* >=0:socket -1:連線失敗 */
Get_Socket(site)	/* site for hostname */
  char *site;
{
  int sock;
  struct sockaddr_in sin;
  struct hostent *host;

  sock = 110;

  /* Getting remote-site data */

  memset((char *) &sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_port = htons(sock);

  if (!(host = gethostbyname(site)))
    sin.sin_addr.s_addr = inet_addr(site);
  else
    memcpy(&sin.sin_addr.s_addr, host->h_addr, host->h_length);

  /* Getting a socket */

  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    return -1;
  }

  /* perform connecting */

  if (connect(sock, (struct sockaddr *) & sin, sizeof(sin)) < 0)
  {
    close(sock);
    return -1;
  }

  return sock;
}


static int		/* 0:成功 */
POP3_Check(sock, account, passwd)
  int sock;
  char *account, *passwd;
{
  FILE *fsock;
  char buf[512];

  if (!(fsock = fdopen(sock, "r+")))
  {
    outs("\n傳回錯誤值，請重試幾次看看\n");
    return -1;
  }

  sock = 1;

  while (1)
  {
    switch (sock)
    {
    case 1:		/* Welcome Message */
      fgets(buf, sizeof(buf), fsock);
      break;

    case 2:		/* Verify Account */
      fprintf(fsock, "user %s\r\n", account);
      fflush(fsock);
      fgets(buf, sizeof(buf), fsock);
      break;

    case 3:		/* Verify Password */
      fprintf(fsock, "pass %s\r\n", passwd);
      fflush(fsock);
      fgets(buf, sizeof(buf), fsock);
      sock = -1;
      break;

    default:		/* 0:Successful 4:Failure  */
      fprintf(fsock, "quit\r\n");
      fclose(fsock);
      return sock;
    }

    if (!strncmp(buf, "+OK", 3))
    {
      sock++;
    }
    else
    {
      outs("\n遠端系統傳回錯誤訊息如下：\n");
      prints("%s\n", buf);
      sock = -1;
    }
  }
}


static int		/* -1:不支援 0:密碼錯誤 1:成功 */
do_pop3(addr)		/* itoc.010821: 改寫一下 :) */
  char *addr;
{
  int sock, i;
  char *ptr, *str, buf[80], username[80];
  char *alias[] = {"", "pop3.", "mail.", NULL};
  ACCT acct;

  strcpy(username, addr);
  *(ptr = strchr(username, '@')) = '\0';
  ptr++;

  clear();
  move(2, 0);
  prints("主機: %s\n帳號: %s\n", ptr, username);
  outs("\033[1;5;36m連線遠端主機中...請稍候\033[m\n");
  refresh();

  for (i = 0; str = alias[i]; i++)
  {
    sprintf(buf, "%s%s", str, ptr);	/* itoc.020120: 主機名稱加上 pop3. 試試看 */
    if ((sock = Get_Socket(buf)) >= 0)	/* 找到這機器且對方支援 POP3 */
      break;
  }

  if (sock < 0)
  {
    outs("您的電子郵件系統不支援 POP3 認證，使用認證信函身分確認\n\n\033[1;36;5m系統送信中...\033[m");
    return -1;
  }

  if (vget(15, 0, "請輸入以上所列出之工作站帳號的密碼：", buf, 20, NOECHO))
  {
    move(17, 0);
    outs("\033[5;37m身分確認中...請稍候\033[m\n");

    if (!POP3_Check(sock, username, buf))	/* POP3 認證成功 */
    {
      /* 提升權限 */
      sprintf(buf, "POP: %s", addr);
      justify_log(cuser.userid, buf);
      strcpy(cuser.email, addr);
      if (acct_load(&acct, cuser.userid) >= 0)
      {
	time(&acct.tvalid);
	acct_setperm(&acct, PERM_VALID, 0);
      }

      /* 寄信通知使用者 */
      mail_self(FN_ETC_JUSTIFIED, str_sysop, msg_reg_valid, 0);
      cutmp->status |= STATUS_BIFF;
      vmsg(msg_reg_valid);

      close(sock);
      return 1;
    }
  }

  close(sock);

  /* POP3 認證失敗 */
  outs("您的密碼或許\打錯了，使用認證信函身分確認\n\n\033[1;36;5m系統送信中...\033[m");
  return 0;
}
#endif


/* ----------------------------------------------------- */
/* 設定 E-mail address					 */
/* ----------------------------------------------------- */


int
u_addr()
{
  char *msg, addr[64];

  if (HAS_STATUS(STATUS_COINLOCK))
  {
    vmsg(msg_coinlock);
    return XEASY;
  }

  /* itoc.050405: 不能讓停權者重新認證，因為會改掉他的 tvalid (停權到期時間) */
  if (HAS_PERM(PERM_ALLDENY))
  {
    vmsg("您被停權，無法改信箱");
    return XEASY;
  }

  film_out(FILM_EMAIL, 0);

  if (vget(b_lines - 2, 0, "E-mail 地址：", addr, sizeof(cuser.email), DOECHO))
  {
    if (not_addr(addr))
    {
      msg = err_email;
    }
    else if (ban_addr(addr))
    {
      msg = "本站不接受您的信箱做為認證地址";
    }
    else
    {
#ifdef EMAIL_JUSTIFY
      if (vans("修改 E-mail 要重新認證，確定要修改嗎(Y/N)？[Y] ") == 'n')
	return 0;

#  ifdef HAVE_POP3_CHECK
      if (vans("是否使用 POP3 認證(Y/N)？[N] ") == 'y')
      {
	if (do_pop3(addr) > 0)	/* 若 POP3 認證成功，則離開，否則以認證信寄出 */
	  return 0;
      }
#  endif

      if (bsmtp(NULL, NULL, addr, MQ_JUSTIFY) < 0)
      {
	msg = "身分認證信函無法寄出，請正確填寫 E-mail address";
      }
      else
      {
	ACCT acct;

	strcpy(cuser.email, addr);
	cuser.userlevel &= ~PERM_ALLVALID;
	if (acct_load(&acct, cuser.userid) >= 0)
	{
	  strcpy(acct.email, addr);
	  acct_setperm(&acct, 0, PERM_ALLVALID);
	}

	film_out(FILM_JUSTIFY, 0);
	prints("\n%s(%s)您好，由於您更新 E-mail address 的設定，\n\n"
	  "請您儘快到 \033[44m%s\033[m 所在的工作站回覆『身分認證信函』。",
	  cuser.userid, cuser.username, addr);
	msg = NULL;
      }
#else
      msg = NULL;
#endif

    }
    vmsg(msg);
  }

  return 0;
}


/* ----------------------------------------------------- */
/* 填寫註冊單						 */
/* ----------------------------------------------------- */


#ifdef HAVE_REGISTER_FORM

static void
getfield(line, len, buf, desc, hint)
  int line, len;
  char *hint, *desc, *buf;
{
  move(line, 0);
  prints("%s%s", desc, hint);
  vget(line + 1, 0, desc, buf, len, GCARRY);
}


int
u_register()
{
  FILE *fn;
  int ans;
  RFORM rform;

#ifdef JUSTIFY_PERIODICAL
  if (HAS_PERM(PERM_VALID) && cuser.tvalid + VALID_PERIOD - INVALID_NOTICE_PERIOD >= ap_start)
#else
  if (HAS_PERM(PERM_VALID))
#endif
  {
    zmsg("您的身分確認已經完成，不需填寫申請表");
    return XEASY;
  }

  if (fn = fopen(FN_RUN_RFORM, "rb"))
  {
    while (fread(&rform, sizeof(RFORM), 1, fn))
    {
      if ((rform.userno == cuser.userno) && !strcmp(rform.userid, cuser.userid))
      {
	fclose(fn);
	zmsg("您的註冊申請單尚在處理中，請耐心等候");
	return XEASY;
      }
    }
    fclose(fn);
  }

  if (vans("您確定要填寫註冊單嗎(Y/N)？[N] ") != 'y')
    return XEASY;

  move(1, 0);
  clrtobot();
  prints("\n%s(%s) 您好，請據實填寫以下的資料：\n(按 [Enter] 接受初始設定)",
    cuser.userid, cuser.username);

  memset(&rform, 0, sizeof(RFORM));
  for (;;)
  {
    move (7, 4);
    outs(ANSI_COLOR(1) "例如：建國中學97級/台灣大學資工系101級/中華電信"  ANSI_RESET);

    getfield(5, 50, rform.career, "服務單位："
	    , "請詳細填寫" ANSI_COLOR(1;31) "學校" ANSI_COLOR(33) "科系級別"
	      ANSI_RESET "或" ANSI_COLOR(1;31) "工作單位" ANSI_COLOR(33) "職稱" ANSI_RESET);

    move (11, 4);
    outs(ANSI_COLOR(1) "例如：臺北市羅斯福路四段一號" ANSI_RESET);

    getfield(9, 60, rform.address, "目前住址：", "包括寢室或門牌號碼");

    move (15, 4);
    outs(ANSI_COLOR(1) "例如：(02) 3456-7890" ANSI_RESET);

    getfield(13, 20, rform.phone, "連絡電話：", "包括長途撥號區域碼");

    ans = vans("以上資料是否正確(Y/N/Q)？[N] ");
    if (ans == 'q')
      return 0;
    if (ans == 'y')
      break;
  }

  rform.userno = cuser.userno;
  strcpy(rform.userid, cuser.userid);

  time(&rform.rtime);
  rec_add(FN_RUN_RFORM, &rform, sizeof(RFORM));
  return 0;
}
#endif


/* ----------------------------------------------------- */
/* 填寫註認碼						 */
/* ----------------------------------------------------- */


#ifdef HAVE_REGKEY_CHECK
int
u_verify()
{
  char buf[80], key[10];
  ACCT acct;
  
  if (HAS_PERM(PERM_VALID))
  {
    zmsg("您的身分確認已經完成，不需填寫認證碼");
  }
  else
  {
    if (vget(b_lines, 0, "請輸入認證碼：", buf, 8, DOECHO))
    {
      archiv32(str_hash(cuser.email, cuser.tvalid), key);	/* itoc.010825: 不用開檔了，直接拿 tvalid 來比就是了 */

      if (str_ncmp(key, buf, 7))
      {
	zmsg("抱歉，您的認證碼錯誤");      
      }
      else
      {
	/* 提升權限 */
	sprintf(buf, "KEY: %s", cuser.email);
	justify_log(cuser.userid, buf);
	if (acct_load(&acct, cuser.userid) >= 0)
	{
	  time(&acct.tvalid);
	  acct_setperm(&acct, PERM_VALID, 0);
	}

	/* 寄信通知使用者 */
	mail_self(FN_ETC_JUSTIFIED, str_sysop, msg_reg_valid, 0);
	cutmp->status |= STATUS_BIFF;
	vmsg(msg_reg_valid);
      }
    }
  }

  return XEASY;
}
#endif


/* ----------------------------------------------------- */
/* 恢復權限						 */
/* ----------------------------------------------------- */


int
u_deny()
{
  ACCT acct;
  time_t diff;
  struct tm *ptime;
  char msg[80];

  if (!HAS_PERM(PERM_ALLDENY))
  {
    zmsg("您沒被停權，不需復權");
  }
  else
  {
    if ((diff = cuser.tvalid - time(0)) < 0)      /* 停權時間到了 */
    {
      if (acct_load(&acct, cuser.userid) >= 0)
      {
	time(&acct.tvalid);
#ifdef JUSTIFY_PERIODICAL
	/* xeon.050112: 在認證快到期前時 Cross-Post，然後 tvalid 就會被設定到未來時間，
	   等復權時間到了去復權，這樣就可以避過重新認證，所以復權後要重新認證。 */
	acct_setperm(&acct, 0, PERM_ALLVALID | PERM_ALLDENY);
#else
	acct_setperm(&acct, 0, PERM_ALLDENY);
#endif
	vmsg("下次請勿再犯，請重新上站");
      }
    }
    else
    {
      ptime = gmtime(&diff);
      sprintf(msg, "您還要等 %d 年 %d 天 %d 時 %d 分 %d 秒才能申請復權",
	ptime->tm_year - 70, ptime->tm_yday, ptime->tm_hour, ptime->tm_min, ptime->tm_sec);
      vmsg(msg);
    }
  }

  return XEASY;
}

/* ----------------------------------------------------- */
/* 檢查是否已滿 18 歲 (內容分級)			 */
/* ----------------------------------------------------- */

int 
over18(ACCT * u)
{
    struct tm then;

    then.tm_hour = then.tm_min = then.tm_sec = 0;
    then.tm_year = (int)u->year + 1911 - 1900;
    then.tm_mon  = (int)u->month - 1;
    then.tm_mday = (int)u->day;

    if (mktime(&then) == -1)
        return 0;
    if (time(NULL) - mktime(&then) >= 18 * 365 * 86400)
	return 1;
    else 
	return 0;
}

/* ----------------------------------------------------- */
/* 個人工具						 */
/* ----------------------------------------------------- */


int
u_info()
{
  char *str, username[UNLEN + 1];
  char *ptr, feeling[FLLEN + 1];

  if (HAS_STATUS(STATUS_COINLOCK))
  {
    vmsg(msg_coinlock);
    return XEASY;
  }

  move(1, 0); 
  strcpy(username, str = cuser.username);
  strcpy(feeling, ptr = cuser.feeling);
  acct_setup(&cuser, 0);
  if (strcmp(username, str))
    memcpy(cutmp->username, str, UNLEN + 1);
  if (strcmp(feeling, ptr))
    memcpy(cutmp->feeling, ptr, FLLEN + 1);
  return 0;
}


int
u_setup()
{
  usint ulevel;
  int len;

  /* itoc.000320: 增減項目要更改 len 大小, 也別忘了改 ufo.h 的旗標 STR_UFO */

  ulevel = cuser.userlevel;
  if (!ulevel)
    len = NUMUFOS_GUEST;
  else if (ulevel & PERM_ALLADMIN)
    len = NUMUFOS;		/* ADMIN 除了可用 acl，還順便也可以用隱身術 */
  else if (ulevel & PERM_CLOAK)
    len = NUMUFOS - 2;		/* 不能用紫隱、acl */
  else
    len = NUMUFOS_USER;

  cuser.ufo = cutmp->ufo = bitset(cuser.ufo, len, len, MSG_USERUFO, ufo_tbl);
  vmsg("儲存設定。");
  return 0;
}

int
u_lv()
{

  cuser.loginview = bitset(cuser.loginview, NUMLVS, NUMLVS
	  , "請選擇上站時想要看的項目", lv_tbl);
  vmsg("儲存設定。");
  return 0;
}


int
u_lock()
{
  char buf[PSWDLEN + 1];

  switch (vans("是否進入螢幕鎖定狀態，將不能傳送/接收水球(Y/N/C)？[N] "))
  {
  case 'c':		/* itoc.011226: 可自行輸入發呆的理由 */
    if (vget(b_lines, 0, "請輸入發呆的理由：", cutmp->mateid, IDLEN + 1, DOECHO))
      break;

  case 'y':
    strcpy(cutmp->mateid, "掛站");
    break;

  default:
    return XEASY;
  }

  bbstate |= STAT_LOCK;		/* lkchu.990513: 鎖定時不可回水球 */
  cutmp->status |= STATUS_REJECT;	/* 鎖定時不收水球 */

  clear();
  move(5, 0);
  prints(ANSI_COLOR(1) BBSNAME ANSI_RESET "\n"
         ANSI_COLOR(1;33) "[%s] %s" ANSI_RESET "\n\n"
         ANSI_COLOR(1;30)"鎖定螢幕: %s" ANSI_RESET "\n",
	 cuser.userid, cuser.username, cutmp->mateid);

  do
  {
    vget(b_lines - 1, 0, "◆ 請輸入密碼，以解除螢幕鎖定：", buf, PSWDLEN + 1, NOECHO);
  } while (chkpasswd(cuser.passwd, buf));

  cutmp->status ^= STATUS_REJECT;
  bbstate ^= STAT_LOCK;

  return 0;
}


int
u_log()
{
  char fpath[64];

  usr_fpath(fpath, cuser.userid, FN_LOG);
  more(fpath, FOOTER_MORE);
  return 0;
}


/* ----------------------------------------------------- */
/* 設定檔案						 */
/* ----------------------------------------------------- */


/* static */			/* itoc.010110: 給 a_xfile() 用 */
void
x_file(mode, xlist, flist)
  int mode;			/* M_XFILES / M_UFILES */
  char *xlist[];		/* description list */
  char *flist[];		/* filename list */
{
  int n, i;
  char *fpath, *desc;
  char buf[64], fname[128];

  while (1)
  {
      clear();
      vs_head("個人檔案設定", str_site);
      move(MENU_XPOS - 1, 0);
      outs_n("─", b_cols / 2);
      n = 0;
      while (desc = xlist[n])
      {
	  n++;
	  if (n <= 9)			/* itoc.020123: 分二欄，一欄九個 */
	  {
	      move(n + MENU_XPOS - 1, 0);
	      clrtoeol();
	      move(n + MENU_XPOS - 1, 2);
	  }
	  else
	  {
	      move(n + MENU_XPOS - 10, 40);
	  }
	  prints("(%d) %s", n, desc);

	  if (mode == M_XFILES)	/* Thor.980806.註解: 印出檔名 */
	  {
	      if (n <= 9)
		  move(n + MENU_XPOS - 1, 22);
	      else
		  move(n + MENU_XPOS - 10, 62);
	      outs(flist[n - 1]);
	  }
      }

      vget(b_lines, 0, "請選擇檔案編號，或按 [0] 取消：", buf, 3, DOECHO);
      i = atoi(buf);
      if (i <= 0 || i > n)
	  return;

      fpath = flist[--i];
      if (mode == M_UFILES)
	  usr_fpath(fname, cuser.userid, fpath);
      else			/* M_XFILES */
	  strcpy(fname, fpath);

      // preview
      {
	  char str[ANSILINELEN + 1];
	  FILE *fp = fopen(fname, "r");
	  int pos = 1;

	  if (fp)
	  {
	      while (fgets(str, sizeof(str) - 1, fp) && pos < MENU_XPOS - 1)
		  outlx(pos++, str);

	      fclose(fp);
	  }
      }
	  
      n = vget(b_lines, 36, "(D)刪除 (E)編輯 [Q]取消？", buf, 3, LCECHO);
      if (n != 'd' && n != 'e')
	  continue;

      if (n == 'd')
      {
	  if (vans(msg_sure_ny) == 'y')
	      unlink(fname);
      }
      else
      {
	  char lock[70], ch, prompt[80];
	  struct stat st;
	  int ve = 0;

	  sprintf(lock, "%s.lock", fname);

	  if (!stat(lock, &st) && S_ISREG(st.st_mode))
	  {
	      outl(b_lines - 2, NULL);
	      outl(b_lines - 1, ANSI_COLOR(1;33) 
		      "已有其他人(帳號)正在編輯此檔案，或先前不正常斷線: ");
	      if ( (ch = vmsg("請問還是要進入編輯嗎? [y/N]")) != 'Y' &&
		      ch != 'y')
		  return;
	      else
		  unlink(lock);
	  }
	  f_cat(lock, "the lock file.");

	  ve = vedit(fname, 0);
	  unlink(lock);

	  sprintf(prompt, "%s%s", ve ? "原封不動" : "更新完畢"
		  , (mode == M_UFILES && !ve) ? "(自定游標要重新上站才會生效)" : "");

	  vmsg(prompt);
	  /* Thor.981020: 注意被talk的問題  */
      }
  }
}


int
u_xfile()
{
  int i;

  static char *desc[] = 
  {
    "上站地點設定檔",
    "名片檔",
    "簽名檔.1",
    "簽名檔.2",
    "簽名檔.3",
    "簽名檔.4",
    "簽名檔.5",
    "簽名檔.6",
    "自定游標",
    "暫存檔.1",
    "暫存檔.2",
    "暫存檔.3",
    "暫存檔.4",
    "暫存檔.5",
    NULL
  };

  static char *path[] = 
  {
    "acl",
    "plans",
    FN_SIGN ".1",
    FN_SIGN ".2",
    FN_SIGN ".3",
    FN_SIGN ".4",
    FN_SIGN ".5",
    FN_SIGN ".6",
    "cursor",		
    "buf.1",
    "buf.2",
    "buf.3",
    "buf.4",
    "buf.5"
  };

  i = HAS_PERM(PERM_ALLADMIN) ? 0 : 1;
  x_file(M_UFILES, &desc[i], &path[i]);
  return 0;
}
