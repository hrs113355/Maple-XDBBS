/*-------------------------------------------------------*/
/* util/showRealname.c  ( NTHU CS MapleBBS Ver 3.00 )    */
/*-------------------------------------------------------*/
/* target : �����u��m�W�ƧǦL�X                         */
/* create : 03/08/11                                     */
/* update :   /  /                                       */
/* author : itoc.bbs@bbs.tnfsh.tn.edu.tw (util/topusr.c) */
/* modify : qazq.bbs@bbs.cs.nchu.edu.tw                  */
/*-------------------------------------------------------*/

#include "bbs.h"

#define OUTFILE_REALNAME    BBSHOME"/gem/@/@-realname"

typedef struct
{
  char userid[IDLEN + 1];
  char realname[RNLEN + 1];
  char email[60];
  int num;
}      DATA;


static int i = 0;  /* �������X�� USER */

static DATA *name;


static int
sort_compare(p1, p2)
  const void *p1;
  const void *p2;
{
  DATA *a1, *a2;

  a1 = (DATA *) p1;
  a2 = (DATA *) p2;

  return (strcmp(a2->realname, a1->realname));
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
  sprintf(buf, " \033[1;33m�� �w�w�w�w�w�w�w�w�w�w��"
               " \033[41m%%%ds%%s%%%ds\033[40m "
               "���w�w�w�w�w�w�w�w�w�w ��\033[m\n\n", j, j);
                                //���F�j�a�b�����\Ū��K�A�ڧ�print�_��F

  fprintf(fp, buf, "", title, "");

  for (j = 0; j < i; j++)
  {
    fprintf(fp, "[%3d] %-13s�G%-9s - %s\n", j + 1,
      data[j].userid, data[j].realname, data[j].email);
  }

  fprintf(fp, "\n");
  fclose(fp);
}


static inline void
topusr(cuser)
  ACCT cuser;
{
  str_ncpy(name[i].userid, cuser.userid, IDLEN + 1);
  str_ncpy(name[i].realname, cuser.realname, RNLEN + 1);
  str_ncpy(name[i].email, cuser.email, sizeof(name[i].email));

  i++;
}

/*-------------------------------------------------------*/
/* �D�{��                                                */
/*-------------------------------------------------------*/


int
main()
{
  int numuser;          /* �`�@���X�ӵ��U�b�� */
  char c;

  chdir(BBSHOME);
  numuser = rec_num(FN_SCHEMA, sizeof(SCHEMA)) + 100;
                                              /* �[ 100 �H�K��n���H���U */
  name = (DATA *) malloc(sizeof(DATA) * numuser);
  memset(name, 0, sizeof(DATA) * numuser);

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
      ACCT cuser;
      int fd;
      char *fname;

      fname = de->d_name;
      if (*fname <= ' ' || *fname == '.')
        continue;

      sprintf(buf, "%s/.ACCT", fname);
      if ((fd = open(buf, O_RDONLY)) < 0)
        continue;

      read(fd, &cuser, sizeof(cuser));
      close(fd);

      topusr(cuser);
    }

    closedir(dirp);
  }

  qsort(name, numuser, sizeof(DATA), sort_compare);
  write_data(OUTFILE_REALNAME, "�u��m�W�Ʀ�]", name);
  free(name);

  return 0;
}
