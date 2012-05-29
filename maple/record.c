/*-------------------------------------------------------*/
/* record.c                                              */
/*-------------------------------------------------------*/
/* target : log and record functions                     */
/* Author : hrs@xdbbs.twbbs.org                          */
/* create : 2008/05/14                                   */
/*-------------------------------------------------------*/

#include "bbs.h"

void
log_newbrd(bname)
  char *bname;
{
  char fpath[64], title[TTLEN + 1];
  FILE *fp;

  sprintf(fpath, "tmp/log_newbrd.%s", cuser.userid);   /* 暫存檔 */
  if (fp = fopen(fpath, "w"))
  {
    sprintf(title, "站長 %s 開啟看板 %s ", cuser.userid, bname);

    /* 文章檔頭 */
    fprintf(fp, "%s %s (%s) %s %s\n",
      str_author1, cuser.userid, cuser.username,
      str_post2, BN_SECURITY);
    fprintf(fp, "標題: %s\n時間: %s\n\n", title, Now());

    /* 文章內容 */
    fprintf(fp, "\n\n站長 %s 在 %s 開啟了看板 %s\n",
     cuser.userid, Now(),bname);
    fclose(fp);

    add_post(BN_SECURITY, fpath, title, "[板務紀錄]", "[系統紀錄]", 0);
    unlink(fpath);
  }
}

void
ann_newbrd(bname)
  char *bname;
{
  char fpath[64], title[TTLEN + 1];
  FILE *fp;

  sprintf(fpath, "tmp/ann_newbrd.%s", cuser.userid);   /* 暫存檔 */
  if (fp = fopen(fpath, "w"))
  {
    sprintf(title, "[新板成立] %s", bname);

    /* 文章檔頭 */
    fprintf(fp, "%s %s (%s) %s %s\n",
      str_author1, "SYSOP", "站長",
      str_post2, "XDNewBoards");
    fprintf(fp, "標題: %s\n時間: %s\n\n", title, Now());

    /* 文章內容 */
    fprintf(fp, "\033[1m英文版名:\033[36m%s\033[m\n\n", bname);
    fprintf(fp, "\033[1m時間    :\033[33m%s\033[m\n\n", Now());
    fprintf(fp, "\033[1m由%s開版\033[m\n\n",cuser.userid);
    fprintf(fp, "\033[1m請多多光臨該版^^~\033[m\n\n");

    fclose(fp);

    add_post(BN_RECORD, fpath, title, "[板務公告]", "[板務公告]", 0);
    unlink(fpath);
  }
}

/* hrs.080514: 把log_register搬過來 */

/* pass: 註冊單審核狀態
 * RFORM_PASS: 通過
 * RFORM_NOPASS: 不通過
 * RFORM_DEL: 刪除註冊單
 * RFORM_CANCELREG: 取消註冊狀態 (退註)
 */

void
log_register(ACCT *acct, RFORM *rform, int pass, char * reason)
{
  char fpath[64], title[TTLEN + 1];
  FILE *fp;

  sprintf(fpath, "tmp/log_register.%s", cuser.userid);   /* 暫存檔 */

  if (fp = fopen(fpath, "w"))
  {
    switch(pass)
    {
	case RFORM_PASS:
	    sprintf(title, "站長 %s 審核 %s 的註冊單通過",
		    cuser.userid, rform->userid);
	    break;

	case RFORM_NOPASS:
	    sprintf(title, "站長 %s 審核 %s 的註冊單不通過",
		    cuser.userid, rform->userid);
	    break;

	case RFORM_DEL:
	    sprintf(title, "站長 %s 刪除 %s 的註冊單",
		    cuser.userid, rform->userid);
	    break;
	
	case RFORM_CANCELREG:
	    sprintf(title, "站長 %s 取消 %s 的註冊狀態",
		    cuser.userid, acct->userid);
	    break;
    }

    /* 文章檔頭 */
    fprintf(fp, "%s %s (%s) %s %s\n",
      str_author1, cuser.userid, cuser.username,
      str_post2, BN_SECURITY);
    fprintf(fp, "標題: %s\n時間: %s\n\n", title, Now());

    /* 文章內容 */
    fprintf(fp, "%s\n\n", title);
    fprintf(fp, "申請代號: %s ", acct->userid);

    if (rform != (RFORM *) NULL)
        fprintf(fp, "(申請時間：%s)\n", Btime(&(rform->rtime)));
    else
        fprintf(fp, "\n");

    fprintf(fp, "使用暱稱: %s\n", acct->username);
    fprintf(fp, "真實姓名: %s\n", acct->realname);
    fprintf(fp, "出生日期: 民國 %d 年 %d 月 %d 日          "
            , acct->year, acct->month, acct->day);
    fprintf(fp, "性別： %s\n", (acct->sex % 2 ? "男" : "女"));
    fprintf(fp, "上次上站： %s\n\n", acct->lasthost);

    if (rform != (RFORM *) NULL)
    {
        fprintf(fp, "服務單位: %s\n", rform->career);
        fprintf(fp, "目前住址: %s\n", rform->address);
        fprintf(fp, "連絡電話: %s\n\n", rform->phone);
    }

    if (pass == RFORM_NOPASS)
        fprintf(fp, "未通過原因: %s\n", reason);
    else if (pass == RFORM_CANCELREG)
        fprintf(fp, "退註原因: %s\n", reason);
    else if (pass == RFORM_DEL)
	fprintf(fp, "刪除原因: %s\n", reason);

    fclose(fp);

    add_post(BN_SECURITY, fpath, title, "[帳務紀錄]", "[系統紀錄]", 0);
    unlink(fpath);
  }
}

void
alog(mode, msg)         /* Admin 行為記錄 */
  char *mode, *msg;
{
  // XXX: buffer overflow
  char buf[512];

  sprintf(buf, "%s %s %-13s%s\n", Now(), mode, cuser.userid, msg);
  f_cat(FN_RUN_ADMIN, buf);
}


void
blog(mode, msg)         /* BBS 一般記錄 */
  char *mode, *msg;
{
  // XXX: buffer overflow
  char buf[512];

  sprintf(buf, "%s %s %-13s%s\n", Now(), mode, cuser.userid, msg);
  f_cat(FN_RUN_USIES, buf);
}

