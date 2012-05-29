/*-------------------------------------------------------*/
/* util/r408_update.c	                                 */
/*-------------------------------------------------------*/
/* target : �ഫ�{�� for revision 408			 */
/* author : hrs113355.bbs@xdbbs.twbbs.org                */
/*-------------------------------------------------------*/

#include "bbs.h"

/* ----------------------------------------------------- */
/* �ഫ�D�{��						 */
/* ----------------------------------------------------- */

/*
 * �ϥΫe�Х��ƥ� .BRD �H�������M shm�A  *
 * �ഫ������Э��s�Ұ�                  */

#define FN_BRD_TMP	".BRD.tmp"

int
main()
{
  int fd;
  BRD old;

  chdir(BBSHOME);

  if ((fd = open(FN_BRD, O_RDONLY)) >= 0)
  {
    while (read(fd, &old, sizeof(BRD)) == sizeof(BRD))
    {
      if (!*old.brdname)	/* ���O�w�Q�R�� */
	continue;

      old.ex_maxposts += 2000;
      old.ex_minposts += 2000;
      old.ex_maxtime +=  365;

      rec_add(FN_BRD_TMP, &old, sizeof(BRD));
    }
    close(fd);
  }

  /* �R���ª��A��s����W */
  unlink(FN_BRD);
  rename(FN_BRD_TMP, FN_BRD);

  return 0;
}
