/*-------------------------------------------------------*/
/* util/showMailsize.c  ( NTHU CS MapleBBS Ver 3.00 )    */
/*-------------------------------------------------------*/
/* target : 觀察全站信箱使用狀況                         */
/* create : 03/09/13                                     */
/* update :   /  /                                       */
/* author : itoc.bbs@bbs.tnfsh.tn.edu.tw (util/topusr.c) */
/* modify : qazq.bbs@bbs.cs.nchu.edu.tw                  */
/*-------------------------------------------------------*/

#include "bbs.h"

#define OUTFILE_MAILSIZE    BBSHOME"/gem/@/@-mailsize"

typedef struct
{
  char userid[IDLEN + 1];
  int mailnum;
  int mailsize;
}      DATA;

static int i = 0;  /* 紀錄有幾個 USER */

static DATA *mail;


static int
sort_compare(p1, p2)
  const void *p1;
  const void *p2;
{
  int k;
  DATA *a1, *a2;

  a1 = (DATA *) p1;
  a2 = (DATA *) p2;

  k = a2->mailnum - a1->mailnum;
  return k ? k : str_cmp(a1->userid, a2->userid);

}


static void
write_data(fpath, title, data)
  char *fpath;
  char *title;
  DATA *data;
{
  FILE *fp;
  char buf[256];
  int j;

  if (!(fp = fopen(fpath, "w")))
    return;

  j = 12 - (strlen(title) >> 1);
  sprintf(buf, " \033[1;33m○ ──────────→ "
    "\033[41m%%%ds%%s%%%ds"
    "\033[40m ←────────── ○\033[m\n\n", j, j);
  fprintf(fp, buf, "", title, "");

  for (j = 0; j < i; j++)
  {
    fprintf(fp, "[%3d] %-13s %3d 封   %5d ｋ\n", j + 1,
      data[j].userid, data[j].mailnum, data[j].mailsize);
  }

  fprintf(fp, "\n");
  fclose(fp);
}


static int
mail_size(cuser)
  ACCT cuser;
{
  int fd, fsize,total;
  struct stat st;
  HDR *head, *tail;
  char *base, *folder, fpath[80], mbox_dir[64];


  usr_fpath(mbox_dir, cuser.userid, FN_DIR);

  if ((fd = open(folder = mbox_dir, O_RDWR)) < 0)
    return 0;

  fsize = 0;
  total = 0;

  if (!fstat(fd, &st) && (fsize = st.st_size) >= sizeof(HDR) &&
    (base = (char *) malloc(fsize)))
  {
    f_exlock(fd);

    if ((fsize = read(fd, base, fsize)) >= sizeof(HDR))
    {
      head = (HDR *) base;
      tail = (HDR *) (base + fsize);

      do
      {
        if(head->xid > 0)
        {
          total += head->xid;
        }
        else
        {
          hdr_fpath(fpath, folder, head);
          stat(fpath, &st);
          total += st.st_size;
          head->xid = st.st_size;
        }
      } while (++head < tail);

    }

    lseek(fd, (off_t) 0, SEEK_SET);
    write(fd, base, fsize);
    ftruncate(fd, fsize);
    f_unlock(fd);

    free(base);
  }

  close(fd);

  return total;
}

static inline void
topusr(cuser)
  ACCT cuser;
{
  char buf[64];

  str_ncpy(mail[i].userid, cuser.userid, sizeof(mail[i].userid));

  usr_fpath(buf, cuser.userid, FN_DIR);
  mail[i].mailnum = rec_num(buf, sizeof(HDR));

  mail[i].mailsize = mail_size(cuser) / 1024;

  i++;
}

/*-------------------------------------------------------*/
/* 主程式                                                */
/*-------------------------------------------------------*/


int
main()
{
  int numuser;          /* 總共有幾個註冊帳號 */
  char c;

  chdir(BBSHOME);
  numuser = rec_num(FN_SCHEMA, sizeof(SCHEMA)) + 100;
                                              /* 加 100 以免剛好有人註冊 */
  mail = (DATA *) malloc(sizeof(DATA) * numuser);
  memset(mail, 0, sizeof(DATA) * numuser);

  for (c = 'a'; c <= 'z'; c++)
  {
    char buf[64], fpath[64];
    struct dirent *de;
    DIR *dirp;

    sprintf(buf, BBSHOME "/usr/%c", c);
    chdir(buf);

    if (!(dirp = opendir(".")))
      continue;

    while (de = readdir(dirp))
    {
      ACCT cuser;
      int fd;
      char *fname;

      fname = de->d_name;
      if (*fname <= ' ' || *fname == '.')
        continue;

      sprintf(fpath, "%s/.ACCT", fname);
      if ((fd = open(fpath, O_RDONLY)) < 0)
        continue;

      read(fd, &cuser, sizeof(cuser));
      close(fd);

      chdir(BBSHOME);       /* 換到根目錄，給 usr_fpath() 用 */
      topusr(cuser);
      chdir(buf);           /* 換回到 user 目錄 */
    }

    closedir(dirp);
  }

  qsort(mail, numuser, sizeof(DATA), sort_compare);
  write_data(OUTFILE_MAILSIZE, "信箱容量排行榜", mail);

  return 0;
}
