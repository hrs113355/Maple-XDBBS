/*-------------------------------------------------------*/
/* util/economy         ( NTHU CS MapleBBS Ver 3.10 )    */
/*-------------------------------------------------------*/
/* target : �g�ٷ��p                                     */
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
/* �D�{��                                                */
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

  /* �Ĥ@���έp */
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
    fprintf(fp, "\n\n\t\t\t\t\t�����g�ٷ��p\n");
    fprintf(fp, "\n\t\t�~�w�t\033[1;32;41m %s  \033[m�u�w��\n", Now());
    fprintf(fp, "\t\t�x�����ثe�`�H�f�G%14d �H �x\n", usernum);
    fprintf(fp, "\t\t�x\033[1m�����H���`�ȹ��G%14lld ��\033[m �x\n", totalmoney);
    fprintf(fp, "\t\t�x\033[1m�����C�H���ȹ��G%14d ��\033[m �x\n", avgmoney);
    fprintf(fp, "\t\t�x\033[1m�ȹ���Ͳ����B�G%14lld ��\033[m �x\n", moneydiff);
    fprintf(fp, "\t\t�x\033[1m�����ȹ��鲣�B�G%14.3f ��\033[m �x\n", avgmoneydiff);
    fprintf(fp, "\t\t���w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w��\n");

    fclose(fp);
  }

  economy.totalmoney = totalmoney;
  economy.last_time = now_time;
  rec_put(FN_ECONOMY, &economy, sizeof(ECONOMY), 0, NULL);
}

