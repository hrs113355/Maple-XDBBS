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

  sprintf(fpath, "tmp/log_newbrd.%s", cuser.userid);   /* �Ȧs�� */
  if (fp = fopen(fpath, "w"))
  {
    sprintf(title, "���� %s �}�ҬݪO %s ", cuser.userid, bname);

    /* �峹���Y */
    fprintf(fp, "%s %s (%s) %s %s\n",
      str_author1, cuser.userid, cuser.username,
      str_post2, BN_SECURITY);
    fprintf(fp, "���D: %s\n�ɶ�: %s\n\n", title, Now());

    /* �峹���e */
    fprintf(fp, "\n\n���� %s �b %s �}�ҤF�ݪO %s\n",
     cuser.userid, Now(),bname);
    fclose(fp);

    add_post(BN_SECURITY, fpath, title, "[�O�Ȭ���]", "[�t�ά���]", 0);
    unlink(fpath);
  }
}

void
ann_newbrd(bname)
  char *bname;
{
  char fpath[64], title[TTLEN + 1];
  FILE *fp;

  sprintf(fpath, "tmp/ann_newbrd.%s", cuser.userid);   /* �Ȧs�� */
  if (fp = fopen(fpath, "w"))
  {
    sprintf(title, "[�s�O����] %s", bname);

    /* �峹���Y */
    fprintf(fp, "%s %s (%s) %s %s\n",
      str_author1, "SYSOP", "����",
      str_post2, "XDNewBoards");
    fprintf(fp, "���D: %s\n�ɶ�: %s\n\n", title, Now());

    /* �峹���e */
    fprintf(fp, "\033[1m�^�媩�W:\033[36m%s\033[m\n\n", bname);
    fprintf(fp, "\033[1m�ɶ�    :\033[33m%s\033[m\n\n", Now());
    fprintf(fp, "\033[1m��%s�}��\033[m\n\n",cuser.userid);
    fprintf(fp, "\033[1m�Цh�h���{�Ӫ�^^~\033[m\n\n");

    fclose(fp);

    add_post(BN_RECORD, fpath, title, "[�O�Ȥ��i]", "[�O�Ȥ��i]", 0);
    unlink(fpath);
  }
}

/* hrs.080514: ��log_register�h�L�� */

/* pass: ���U��f�֪��A
 * RFORM_PASS: �q�L
 * RFORM_NOPASS: ���q�L
 * RFORM_DEL: �R�����U��
 * RFORM_CANCELREG: �������U���A (�h��)
 */

void
log_register(ACCT *acct, RFORM *rform, int pass, char * reason)
{
  char fpath[64], title[TTLEN + 1];
  FILE *fp;

  sprintf(fpath, "tmp/log_register.%s", cuser.userid);   /* �Ȧs�� */

  if (fp = fopen(fpath, "w"))
  {
    switch(pass)
    {
	case RFORM_PASS:
	    sprintf(title, "���� %s �f�� %s �����U��q�L",
		    cuser.userid, rform->userid);
	    break;

	case RFORM_NOPASS:
	    sprintf(title, "���� %s �f�� %s �����U�椣�q�L",
		    cuser.userid, rform->userid);
	    break;

	case RFORM_DEL:
	    sprintf(title, "���� %s �R�� %s �����U��",
		    cuser.userid, rform->userid);
	    break;
	
	case RFORM_CANCELREG:
	    sprintf(title, "���� %s ���� %s �����U���A",
		    cuser.userid, acct->userid);
	    break;
    }

    /* �峹���Y */
    fprintf(fp, "%s %s (%s) %s %s\n",
      str_author1, cuser.userid, cuser.username,
      str_post2, BN_SECURITY);
    fprintf(fp, "���D: %s\n�ɶ�: %s\n\n", title, Now());

    /* �峹���e */
    fprintf(fp, "%s\n\n", title);
    fprintf(fp, "�ӽХN��: %s ", acct->userid);

    if (rform != (RFORM *) NULL)
        fprintf(fp, "(�ӽЮɶ��G%s)\n", Btime(&(rform->rtime)));
    else
        fprintf(fp, "\n");

    fprintf(fp, "�ϥμʺ�: %s\n", acct->username);
    fprintf(fp, "�u��m�W: %s\n", acct->realname);
    fprintf(fp, "�X�ͤ��: ���� %d �~ %d �� %d ��          "
            , acct->year, acct->month, acct->day);
    fprintf(fp, "�ʧO�G %s\n", (acct->sex % 2 ? "�k" : "�k"));
    fprintf(fp, "�W���W���G %s\n\n", acct->lasthost);

    if (rform != (RFORM *) NULL)
    {
        fprintf(fp, "�A�ȳ��: %s\n", rform->career);
        fprintf(fp, "�ثe��}: %s\n", rform->address);
        fprintf(fp, "�s���q��: %s\n\n", rform->phone);
    }

    if (pass == RFORM_NOPASS)
        fprintf(fp, "���q�L��]: %s\n", reason);
    else if (pass == RFORM_CANCELREG)
        fprintf(fp, "�h����]: %s\n", reason);
    else if (pass == RFORM_DEL)
	fprintf(fp, "�R����]: %s\n", reason);

    fclose(fp);

    add_post(BN_SECURITY, fpath, title, "[�b�Ȭ���]", "[�t�ά���]", 0);
    unlink(fpath);
  }
}

void
alog(mode, msg)         /* Admin �欰�O�� */
  char *mode, *msg;
{
  // XXX: buffer overflow
  char buf[512];

  sprintf(buf, "%s %s %-13s%s\n", Now(), mode, cuser.userid, msg);
  f_cat(FN_RUN_ADMIN, buf);
}


void
blog(mode, msg)         /* BBS �@��O�� */
  char *mode, *msg;
{
  // XXX: buffer overflow
  char buf[512];

  sprintf(buf, "%s %s %-13s%s\n", Now(), mode, cuser.userid, msg);
  f_cat(FN_RUN_USIES, buf);
}

