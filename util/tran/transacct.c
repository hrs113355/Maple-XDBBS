/*-------------------------------------------------------*/
/* util/transacct.c	( NTHU CS MapleBBS Ver 3.10 )	 */
/*-------------------------------------------------------*/
/* target : M3 ACCT �ഫ�{��				 */
/* create : 98/12/15					 */
/* update : 02/04/29					 */
/* author : mat.bbs@fall.twbbs.org			 */
/* modify : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/*-------------------------------------------------------*/
/* syntax : transacct [userid]				 */
/*-------------------------------------------------------*/


/* hrs.080107.todo : �n�O�o��o�� */

#if 0

  �ϥΤ�k�G

  0. For Maple 3.X To Maple 3.X

  1. �ϥΫe�Х��Q�� backupacct.c �ƥ� .ACCT

  2. �Цۦ�� struct NEW �M struct OLD

  3. �̤��P�� NEW�BOLD �ӧ� trans_acct()�C

#endif


#include "bbs.h"

/* ----------------------------------------------------- */
/* (�s��) �ϥΪ̱b�� .ACCT struct			 */
/* ----------------------------------------------------- */


typedef struct			/* �n�M�s���{�� struct �@�� */
{
  int userno;                   /* unique positive code */

  char userid[IDLEN + 1];       /* ID */
  char passwd[PASSLEN + 1];     /* �K�X */
  char realname[RNLEN + 1];     /* �u��m�W */
  char username[UNLEN + 1];     /* �ʺ� */
  char feeling[FLLEN + 1];      /* �߱� */

  usint userlevel;              /* �v�� */
  usint ufo;                    /* habit */
  usint loginview;              /* hrs.080309:�W���e�� */
  uschar signature;             /* �w�]ñ�W�� */

  char year;                    /* �ͤ�(����~) */
  char month;                   /* �ͤ�(��) */
  char day;                     /* �ͤ�(��) */
  char sex;                     /* �ʧO �_��:�k�� ����:�k�� */

  int money;                    /* �ȹ� */
  int mquota;                   /* hrs.080107:�H�c�ʼ� */

  int numlogins;                /* �W������ */
  int numposts;                 /* �o���� */
  int numemails;                /* �H�o Inetrnet E-mail ���� */
  int numbmwtx;                 /* �o�e�w���� */
  int numbmwrx;                 /* ���e�w���� */
  int toquery;                  /* �n�_�� */
  int bequery;                  /* �H��� */

  time_t firstlogin;            /* �Ĥ@���W���ɶ� */
  time_t lastlogin;             /* �W�@���W���ɶ� */
  time_t tcheck;                /* �W�� check �H�c/�B�ͦW�檺�ɶ� */
  time_t tvalid;                /* �Y���v�A���v�������ɶ��F
                                   �Y�����v�B�q�L�{�ҡA�q�L�{�Ҫ��ɶ��F
                                   �Y�����v�B���q�L�{�ҡA�{�Ҩ窺 time-seed */
  time_t staytime;              /* �����`�ɶ� */

  char lasthost[40];            /* �W���n�J�ӷ� */
  char email[60];               /* �ثe�n�O���q�l�H�c */
  char site[40];                /* hrs.080310:���ϥΪ̥i�H����}�s�i */

} PACKSTRUCT NEW;


/* ----------------------------------------------------- */
/* (�ª�) �ϥΪ̱b�� .ACCT struct			 */
/* ----------------------------------------------------- */


typedef struct			/* �n�M�ª��{�� struct �@�� */
{
  int userno;			/* unique positive code */

  char userid[IDLEN + 1];	/* ID */
  char passwd[PASSLEN + 1];	/* �K�X */
  char realname[RNLEN + 1];	/* �u��m�W */
  char username[UNLEN + 1];	/* �ʺ� */
  char feeling[FLLEN + 1];      /* �߱� */

  usint userlevel;		/* �v�� */
  usint ufo;			/* habit */
  usint loginview;		/* hrs.080309:�W���e�� */
  uschar signature;		/* �w�]ñ�W�� */

  char year;			/* �ͤ�(����~) */
  char month;			/* �ͤ�(��) */
  char day;			/* �ͤ�(��) */
  char sex;			/* �ʧO �_��:�k�� ����:�k�� */
  char address[60];		/* hrs.080310:�a�}�s�i�� */
  char phone[20];		/* hrs.080310:�q�ܦs�i�� */

  int money;			/* �ȹ� */
  int mquota;			/* hrs.080107:�H�c�ʼ� */

  int numlogins;		/* �W������ */
  int numposts;			/* �o���� */
  int numemails;		/* �H�o Inetrnet E-mail ���� */
  int numbmwtx;			/* �o�e�w���� */
  int numbmwrx;			/* ���e�w���� */
  int toquery;			/* �n�_�� */
  int bequery;			/* �H��� */

  time_t firstlogin;		/* �Ĥ@���W���ɶ� */
  time_t lastlogin;		/* �W�@���W���ɶ� */
  time_t tcheck;		/* �W�� check �H�c/�B�ͦW�檺�ɶ� */
  time_t tvalid;		/* �Y���v�A���v�������ɶ��F
                                   �Y�����v�B�q�L�{�ҡA�q�L�{�Ҫ��ɶ��F
                                   �Y�����v�B���q�L�{�ҡA�{�Ҩ窺 time-seed */
  time_t staytime;		/* �����`�ɶ� */

  char lasthost[80];		/* �W���n�J�ӷ� */
  char email[80];		/* �ثe�n�O���q�l�H�c */
  char site[40];		/* hrs.080310:���ϥΪ̥i�H����}�s�i */
}	OLD;


/* ----------------------------------------------------- */
/* �ഫ�D�{��						 */
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
      unlink(buf);			/* itoc.010831: �屼��Ӫ� FN_ACCT */

      trans_acct(&old, &new);

      fd = open(buf, O_WRONLY | O_CREAT, 0600);	/* itoc.010831: ���طs�� FN_ACCT */
      write(fd, &new, sizeof(NEW));
      close(fd);
    }

    closedir(dirp);
  }

  return 0;
}
