/*-------------------------------------------------------*/
/* util/r408_update.c	                                 */
/*-------------------------------------------------------*/
/* target : 轉換程式 for revision 408			 */
/* author : hrs113355.bbs@xdbbs.twbbs.org                */
/*-------------------------------------------------------*/

#include "bbs.h"

/* ----------------------------------------------------- */
/* 轉換主程式						 */
/* ----------------------------------------------------- */

/*
 * 使用前請先備份 .BRD 以及關站清 shm，  *
 * 轉換完畢後請重新啟動                  */

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
      if (!*old.brdname)	/* 此板已被刪除 */
	continue;

      old.ex_maxposts += 2000;
      old.ex_minposts += 2000;
      old.ex_maxtime +=  365;

      rec_add(FN_BRD_TMP, &old, sizeof(BRD));
    }
    close(fd);
  }

  /* 刪除舊的，把新的更名 */
  unlink(FN_BRD);
  rename(FN_BRD_TMP, FN_BRD);

  return 0;
}
