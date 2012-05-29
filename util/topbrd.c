/*-------------------------------------------------------*/
/* topbrd.c ( NTHU CS MapleBBS Ver 3.10 )                */
/*-------------------------------------------------------*/
/* target : �����ݪO�Ʀ�]                               */
/* create : 03/07/03                                     */
/* update : 03/07/03                                     */
/* author : itoc.bbs@bbs.tnfsh.tn.edu.tw                 */
/* modify : qazq.bbs@bbs.cs.nchu.edu.tw                  */
/*-------------------------------------------------------*/



#include "bbs.h"


#define BRD_TIMES   "run/brd_times.log"
#define TOPBRD      "gem/@/@-topbrd"

/*-------------------------------------------------------*/
/* BRD shm �������P cache.c �ۮe                         */
/*-------------------------------------------------------*/


static BCACHE *bshm;


static void
init_bshm()
{
  /* itoc.030727: �b�}�� bbsd ���e�A���ӴN�n����L account�A
     �ҥH bshm ���Ӥw�]�w�n */

  bshm = shm_new(BRDSHM_KEY, sizeof(BCACHE));

  if (bshm->uptime <= 0)        /* bshm ���]�w���� */
    exit(0);
}


static BRD *
brd_get(brdname)
  char *brdname;
{
  BRD *bhdr, *tail;

  bhdr = bshm->bcache;
  tail = bhdr + bshm->number;
  do
  {
    if (!strcmp(brdname, bhdr->brdname))
      return bhdr;
  } while (++bhdr < tail);
  return NULL;
}


typedef struct
{
  int times;            /* �i�J�ݪO�\Ū���� */
  char brdname[BNLEN + 1];  /* �O�W */
}   BRDDATA;


static int
int_cmp(a, b)
  BRDDATA *a, *b;
{
  return (b->times - a->times); /* �Ѥj�ƨ�p */
}


int
main()
{
  time_t now;
  struct tm *ptime;
  BRDDATA board[MAXBOARD];
  FILE *fp;
  int locus, i, m;
  BRD *bhdr;

  chdir(BBSHOME);

  if (!(fp = fopen(BRD_TIMES, "r")))
    return;

  locus = 0;
  while (!feof(fp))
  {
    fscanf(fp, "%s%d", board[i].brdname, &(board[i].times));
    locus++;
  }
  fclose(fp);

  qsort(board, locus, sizeof(BRDDATA), int_cmp);

  init_bshm();

  fp = fopen(TOPBRD, "w");

  // �U���O��ܵe���A�i�H�ۤv�e....�ݰ_�өM�L�W�p�������I��...(�k...:P)

  time(&now);
  ptime = localtime(&now);
  fprintf(fp, "        �t\033[1;41m        �����ݪO�Ʀ�]        \033[m�u"
    "        \033[33m�έp���: \033[36m%d �� %d ��\033[m\n\n",
    ptime->tm_mon + 1, ptime->tm_mday);

  m = 1;
  for (i = 0; i < locus; i++)
  {
    if (board[i].times == 0)
      break;

    if (!(bhdr = brd_get(board[i].brdname)))   /* ���O�w��W�γQ�� */
      continue;

    /* ���L���C�J�Ʀ�]���ݪO */
    /* (BASIC + ... + VALID) < (VALID << 1) */
    if ((bhdr->readlevel | bhdr->postlevel) >= (PERM_VALID << 1)
	    || bhdr->bmode == BMODE_HIDE || bhdr->bmode == BMODE_PAL)
      continue;

    fprintf(fp, "\033[1;31m%4d. \033[33m�ݪO: \033[32m%-13s  "
      "\033[1;3%dm%5.5s\033[m %-34.33s  \033[1;35m�H�� \033[36m%-3d\033[m\n",
      m, board[i].brdname, bhdr->class[3] & 7, bhdr->class,
      bhdr->title, board[i].times);

    m++;
  }

  fclose(fp);
}
