/*-------------------------------------------------------*/
/* util/economy         ( NTHU CS MapleBBS Ver 3.10 )    */
/*-------------------------------------------------------*/
/* target : 經濟概況                                     */
/* create : 04/01/23                                     */
/* update :   /  /                                       */
/* author : BioStar.bbs@micro.bio.ncue.edu.tw            */
/*-------------------------------------------------------*/


#include "bbs.h"


#define OUTFILE_ECONOMY "gem/@/@-economy"
#define FN_ECONOMY      "run/economy"


typedef struct
{
  long long int totalmoney;
  time_t last_time;
} ECONOMY;


/*-------------------------------------------------------*/
/* 主程式                                                */
/*-------------------------------------------------------*/


int
main()
{
  char c;
  int usernum = 0, avgmoney = 0;
  long long int totalmoney = 0;
  FILE *fp;
  ECONOMY economy;
  long long int moneydiff = 0;
  double avgmoneydiff, days;
  time_t now_time, interval;

  chdir(BBSHOME);

  /* 第一次統計 */
  if (rec_get(FN_ECONOMY, &economy, sizeof(ECONOMY), 0))
    memset(&economy, 0, sizeof(ECONOMY));

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
      ACCT acct;
      int fd;
      char *fname;

      fname = de->d_name;
      if (*fname <= ' ' || *fname == '.')
        continue;

      sprintf(buf, "%s/.ACCT", fname);
      if ((fd = open(buf, O_RDONLY)) < 0)
        continue;

      read(fd, &acct, sizeof(ACCT));
      close(fd);

      totalmoney += acct.money;
      usernum++;
    }

    closedir(dirp);
  }

  chdir(BBSHOME);

  avgmoney = totalmoney / usernum;

  now_time = time(0);
  interval = now_time - economy.last_time;
  days = (double) interval / (double) 86400;

  moneydiff = totalmoney - economy.totalmoney;
  avgmoneydiff = ((double) moneydiff / (double) usernum) / days;

  if (fp = fopen(OUTFILE_ECONOMY, "w"))
  {
    fprintf(fp, "\n\n\t\t\t\t\t本站經濟概況\n");
    fprintf(fp, "\n\t\t╭─┤\033[1;32;41m %s  \033[m├─╮\n", Now());
    fprintf(fp, "\t\t│本站目前總人口：%14d 人 │\n", usernum);
    fprintf(fp, "\t\t│\033[1m本站人民總銀幣：%14lld 元\033[m │\n", totalmoney);
    fprintf(fp, "\t\t│\033[1m平均每人有銀幣：%14d 元\033[m │\n", avgmoney);
    fprintf(fp, "\t\t│\033[1m銀幣日生產毛額：%14lld 元\033[m │\n", moneydiff);
    fprintf(fp, "\t\t│\033[1m平均銀幣日產額：%14.3f 元\033[m │\n", avgmoneydiff);
    fprintf(fp, "\t\t╰─────────────────╯\n");

    fclose(fp);
  }

  economy.totalmoney = totalmoney;
  economy.last_time = now_time;
  rec_put(FN_ECONOMY, &economy, sizeof(ECONOMY), 0, NULL);
}

