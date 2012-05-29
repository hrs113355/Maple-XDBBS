/*-------------------------------------------------------*/
/* bank.c	( NTHU CS MapleBBS Ver 3.10 )		 */
/*-------------------------------------------------------*/
/* target : �Ȧ�B�ʶR�v���\��				 */
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
    sysopmode = vget(13, 0, "�z�n 1)���q��b(��10%����O) 2)�����o����:", buf, 2, DOECHO) - '1';

  if (sysopmode < 0 || sysopmode > 1)
    return;

  if (!vget(15, 0, "���B�઺�עҡH ", userid, IDLEN + 1, DOECHO))
    return;

  if (acct_userno(userid) <= 0)
  {
    vmsg(err_uid);
    return;
  }

  if(!str_cmp(userid, cuser.userid) && !sysopmode)
  {
    vmsg("��������C");         /* hrs.071226:�൹�ۤv�S�N�� */
    return;
  }

  do
  {
    if (!vget(17, 0, "�n���h�ֿ��H ", buf, 9, DOECHO))
    /* �̦h�� 99999999 �קK���� */
      return;

    dollar = atoi(buf);

      if (dollar > cuser.money && !sysopmode)
        dollar = cuser.money;   /* ����L�h */

  } while (dollar <= 1);        /* ����u�� 1�A�|���ܤ���O */

  if (!vget(19, 0, "�п�J��]�G", reason, 40, DOECHO))
    strcpy(reason, "�۰]�i�_^_^");

  sprintf(buf, "�O�_�� %s �ȹ� %d �� (Y/N)�H[N] "
    , userid, dollar);

  if (vget(21, 0, buf, fpath, 3, LCECHO) == 'y')
  {

    i = 3;

    while(i--)
    {
        if (!vget(b_lines - 2, 0, "���F�קK�Q�B�F�άO�b���D��s�ΡA�п�J�z���K�X�G "
            , passbuf, PSWDLEN + 1, NOECHO))
        continue;

        if (chkpasswd(cuser.passwd, passbuf))
	{
	    outl(b_lines - 1, ERR_PASSWD);
	    if (!i)
	    {
		vmsg("�K�X�T����J���~�A��������C");
		return;
	    }
	    continue;
	}
	break;
    }

    if (!sysopmode)
    {
        cuser.money -= dollar;

        dollar -= dollar / 10 + ((dollar % 10) ? 1 : 0);        /* 10% ����O */
    }

    /* itoc.020831: �[�J�׿��O�� */
    time(&now);
    if (sysopmode)
    {
        sprintf(buf, "%-13s�ȹ� %d��", userid, dollar);
        alog("�o�����",buf);
    }
    else
    {
    sprintf(buf, "%-13s�൹ %-13s�p %d �� (%s)\n",
    cuser.userid, userid, dollar, Btime(&now));
    f_cat(FN_RUN_BANK_LOG, buf);
    }

    usr_fpath(folder, userid, fn_dir);

    if (fp = fdopen(hdr_stamp(folder, 0, &hdr, fpath), "w"))
    {
      fprintf(fp, "%s %s (%s)\n���D: %s\n�ɶ�: %s\n\n",
        str_author1, cuser.userid, cuser.username
        , (sysopmode ? "�����q��" : "��b�q��"), Btime(&now));
      fprintf(fp, "%s\n��]�O�G%s\n\n�бz�ܪ��Ĥ��߱N�䲼�I�{", buf, reason);
      fclose(fp);
      sprintf(hdr.title, "%s", (sysopmode ? "�����q��":"��b�q��"));
      strcpy(hdr.owner, cuser.userid);
      rec_add(folder, &hdr, sizeof(HDR));
    }

    memset(&paycheck, 0, sizeof(PAYCHECK));
    time(&paycheck.tissue);
    paycheck.money = dollar;

    sprintf(paycheck.reason, "[��b] %s", cuser.userid);
    usr_fpath(fpath, userid, FN_PAYCHECK);
    rec_add(fpath, &paycheck, sizeof(PAYCHECK));

    if (!sysopmode)
    vmsgf("������\\�A�l�B %d ��", cuser.money);
    else
    vmsg(NULL);
  }
  else
  {
    vmsg("�������");
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
    vmsg("�z�ثe�S���䲼���I�{");
    return;
  }

  usr_fpath(buf, cuser.userid, "cashed");
  fp = fopen(buf, "w");
  fputs("�H�U�O�z���䲼�I���M��G\n\n", fp);

  money = 0;
  while (read(fd, &paycheck, sizeof(PAYCHECK)) == sizeof(PAYCHECK))
  {
    if (paycheck.money < (INT_MAX - money))	/* �קK���� */
      money += paycheck.money;
    else
      money = INT_MAX;

    fprintf(fp, "%s %s %d ��\n", 
      Btime(&paycheck.tissue), paycheck.reason, paycheck.money);
  }
  close(fd);
  unlink(fpath);

  fprintf(fp, "\n�z�@�I�{ %d ��\n", money);
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

  vs_bar("�H�U�Ȧ�");
  move(2, 0);

  /* itoc.011208: �H���U�@ */
  if (cuser.money < 0)
    cuser.money = 0;

  outs("\033[1;36m  ��������������������������������������������������������������\n");
  prints("  ��\033[32m�z�{�b���ȹ� \033[33m%12d\033[32m ��                      \033[36m        ��\n", 
    cuser.money);
  outs("  ��������������������������������������������������������������\n"
    "  �� �ثe�Ȧ洣�ѤU�C�X���A�ȡG                               ��\n"
    "  ��\033[33m1.\033[37m ��b -- ��b����L�H   (��� 10% ����O) \033[36m              ��\n"
    "  ��\033[33m2.\033[37m �I�{ -- �䲼�I�{                         \033[36m              ��\n"
    "  ��������������������������������������������������������������\033[m");

  vget(11, 0, "�п�J�z�ݭn���A�ȡG", ans, 3, DOECHO);
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
    if (vans("�O�_�{��(Y/N)�H[N] ") != 'y')
      return XEASY; 
    /* �{���K�O */
  }
  else
  {
    if (HAS_PERM(PERM_CLOAK))
    {
      if (vans("�O�_����(Y/N)�H[N] ") != 'y')
	return XEASY;
      /* ���L�������v���̧K�O */
    }
    else
    {
      if (cuser.money < atoi(FEE_CLOAK))
      {
	vmsg("�n " FEE_CLOAK " ���~�����γ�");
	return XEASY;
      }
      if (vans("�O�_�� " FEE_CLOAK " ������(Y/N)�H[N] ") != 'y')
	return XEASY;
      cuser.money -= atoi(FEE_CLOAK);
    }
  }
#endif

  cuser.ufo ^= UFO_CLOAK;
  cutmp->ufo ^= UFO_CLOAK;	/* ufo �n�P�B */

  pmsg(cuser.ufo & UFO_CLOAK ? "���Φ��\\!" : "�{�����\\!");
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
    vmsg("�z�w�g�H�c�L�W���F!");
    return XEASY;
  }
  move (b_lines - 3, 0);
  prints (ANSI_COLOR(1;30) "�A���g�ʶR�F "
	  ANSI_COLOR(37) "%d" ANSI_COLOR(30)" ���H�c�e�q\n"
	  , cuser.mquota);
  prints (ANSI_COLOR(1;30) "�@�ӳ��|�W�[ 1 �ʫH��t�B�ӥB 15 KB �e�q����C\n");
  prints (ANSI_COLOR(1;30) "�A�{�b���W��"
	  ANSI_COLOR(37) " %d " ANSI_COLOR(30) "��", cuser.money);
  prints (ANSI_RESET);
  if (!vget(b_lines, 0, "�R�@���H�c�e�q�n " FEE_MQUOTA " ���A�A�n�R�X���H "
       , buf, 4, DOECHO))
    return 0;

  fee = atoi(buf);

  if (fee <= 0)
      return 0;

  if (fee * atoi(FEE_MQUOTA) > cuser.money)
  {
      vmsg("�{�������C");
      return 0;
  }
  else
  {
      cuser.money -= fee * atoi(FEE_MQUOTA);
      cuser.mquota += fee;
  }

  sprintf(genbuf
	  , "�z�ʶR�F %d ���, �ʶR��t�B %d ���C (�Хߨ�ӥ��`�{�ǭ��s�W��)"
	  , fee, cuser.mquota);
  pmsg(genbuf);
  return 0;
}

#if 0

static void
buy_level(userlevel)		/* itoc.010830: �u�s level ���A�H�K�ܰʨ�b�u�W��ʪ��{����� */
  usint userlevel;
{
  if (!HAS_STATUS(STATUS_DATALOCK))	/* itoc.010811: �n�S���Q������w�A�~��g�J */
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
	vmsg("�z�w�g��o�v���A�Э��s�W��");
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
    vmsg("�z�w�g��L�����ΤF");
  }
  else
  {
    if (cuser.gold < 1000)
    {
      vmsg("�n 1000 �����~���ʶR�L�������v����");
    }
    else if (vans("�O�_�� 1000 �����ʶR�L�������v��(Y/N)�H[N] ") == 'y')
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
    vmsg("�z���H�c�w�g�S���W���F");
  }
  else
  {
    if (cuser.gold < 1000)
    {
      vmsg("�n 1000 �����~���ʶR�H�c�L���v����");
    }
    else if (vans("�O�_�� 1000 �����ʶR�H�c�L���v��(Y/N)�H[N] ") == 'y')
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
    vmsg("�z���b���w�g�ä[�O�d�F");
  }
  else
  {
    if (cuser.gold < 1000)
    {
      vmsg("�n 1000 �����~���ʶR�b���ä[�O�d�v����");
    }
    else if (vans("�O�_�� 1000 �����ʶR�b���ä[�O�d�v��(Y/N)�H[N] ") == 'y')
    {
      cuser.gold -= 1000;
      buy_level(PERM_XEMPT);
    }
  }

  return XEASY;
}


#if 0	/* �������ʶR�۱��\�� */
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
    vmsg("�t�Φb�U���w���M�b���ɡA�N�M���� ID");
  }
  else
  {
    if (cuser.gold < 1000)
    {
      vmsg("�n 1000 �����~��۱���");
    }
    else if (vans("�O�_�� 1000 �����۱�(Y/N)�H[N] ") == 'y')
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
