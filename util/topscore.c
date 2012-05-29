/*-------------------------------------------------------*/
/* util/topscore.c      ( NTHU CS MapleBBS Ver 3.10 )    */
/*-------------------------------------------------------*/
/* target : 文章評分排名                                 */
/* create : 03/04/26                                     */
/* update :   /  /                                       */
/* author : itoc.bbs@bbs.tnfsh.tn.edu.tw                 */
/*-------------------------------------------------------*/
/* syntax : topscore                                     */
/*-------------------------------------------------------*/


#include "bbs.h"


#define FN_RUN_SCOREUSIES       "run/score_usies"       /* 評分記錄 */
#define OUTFILE_TOPSCORE        "gem/@/@-topscore"


typedef struct
{
  char brdname[BNLEN + 1];
  char owner[IDLEN + 1];
  char title[TTLEN + 1];
  char score;
} SCOREDATA;


/*-------------------------------------------------------*/
/* 把所有被評分的文章找出來                              */
/*-------------------------------------------------------*/


static void
do_find(brdname)
  char *brdname;
{
  int i, size, fd;
  char fpath[64];
  struct stat st;
  HDR *hdr;
  SCOREDATA score;
  time_t due;

  brd_fpath(fpath, brdname, FN_DIR);
  if ((fd = open(fpath, O_RDONLY)) >= 0)
  {
    fstat(fd, &st);
    size = st.st_size;
    hdr = (HDR *) malloc(size);
    read(fd, hdr, size);
    close(fd);

    due = time(NULL) - 86400 * 10;      /* 最多統計到十天前的文章 */

    size /= sizeof(HDR);
    for (i = 0; i < size; i++)
    {
      if (hdr[i].score > 0 && hdr[i].chrono >= due)
      {
        strcpy(score.brdname, brdname);
        strcpy(score.owner, hdr[i].owner);
        strcpy(score.title, hdr[i].title);
        score.score = hdr[i].score;
        rec_add(FN_RUN_SCOREUSIES, &score, sizeof(SCOREDATA));
      }
    }

    free(hdr);
  }
}


/*-------------------------------------------------------*/
/* 製作排名                                              */
/*-------------------------------------------------------*/


static void
write_data(score, num)
  SCOREDATA *score;
  int num;
{
  int n;
  FILE *fp;

  if (!(fp = fopen(OUTFILE_TOPSCORE, "w")))
    return;

  fprintf(fp, "\033[1;33m名次\033[36m─\033[33m看板\033[36m────"
    "\033[33m作者\033[36m─────\033[33m標  題\033[36m────"
    "─────────────\033[33m分數\033[36m─\033[m\n");

  for (n = 0; n < 50 && n < num; n++)           /* 只取前 50 名 */
  {
    fprintf(fp, "\033[1;32m%3d\033[31m. \033[37m%-13.13s\033[33m%-13.13s\033[31m%-38.38s\033[33m %4d \033[37m次\033[m\n",
      n + 1, score[n].brdname, score[n].owner, score[n].title, score[n].score);
  }

  fclose(fp);
}


static int
count_cmp(b, a)
  SCOREDATA *a, *b;
{
  return (a->score - b->score);
}


int
main()
{
  int fd, size;
  struct stat st;
  SCOREDATA *score;
  BRD brd;

  chdir(BBSHOME);

  unlink(FN_RUN_SCOREUSIES);

  size = 0;
  while (!rec_get(FN_BRD, &brd, sizeof(BRD), size))
  {
    if ((brd.readlevel | brd.postlevel) < (PERM_VALID << 1)
	    && brd.bmode != BMODE_HIDE && brd.bmode != BMODE_HIDE) /* 隱藏板不統計 */
      do_find(brd.brdname);
    size++;
  }

  if ((fd = open(FN_RUN_SCOREUSIES, O_RDWR)) < 0)
    return 0;

  if (!fstat(fd, &st) && (size = st.st_size) >= sizeof(SCOREDATA))
  {
    score = (SCOREDATA *) malloc(size);
    size = read(fd, score, size);

    qsort(score, size / sizeof(SCOREDATA), sizeof(SCOREDATA), count_cmp);

    lseek(fd, 0, SEEK_SET);
    write(fd, score, size);
    ftruncate(fd, size);

    write_data(score, size / sizeof(SCOREDATA));
    free(score);
  }

  close(fd);
  return 0;
}

