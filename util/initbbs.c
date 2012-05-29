/*-------------------------------------------------------*/
/* util/initbbs.c	(Maple-xdbbs)	                 */
/*-------------------------------------------------------*/
/* target : do initial work (create boards..)    	 */
/* create : 11/04/09                                     */
/* update :   /  /                                       */
/* author : hrs@xdbbs.twbbs.org                          */
/*-------------------------------------------------------*/
  /* syntax : showBRD [target_board]                       */
/*-------------------------------------------------------*/


#include "bbs.h"
// XXX 一堆參數的寫法真的很爛, 但是包成 brd 的寫法會使得 main 裡面一團亂,
//     先暫時這樣吧.

void create_board(brdname, class, title, readlevel, postlevel, battr, bmode, maxp, minp, maxt)
    char *brdname, *class, *title;
    usint readlevel, postlevel, battr, bmode;
    int maxp, minp, maxt;
{
    BRD newbrd;
    memset(newbrd, 0, sizeof(newbrd));

    str_ncpy(brdname, newbrd.brdname, sizeof(newbrd.brdname));
    str_ncpy(class, newbrd.class, sizeof(newbrd.class));
    str_ncpy(title, newbrd.title, sizeof(newbrd.title));
    
    newbrd.readlevel = readlevel;
    newbrd.postlevel = postlevel;
    newbrd.battr = battr;
    newbrd.bmode = bmode;

    newbrd.bstamp = time(NULL); // now

    newbrd.ex_maxposts = maxp >= 0 ? maxp : DEF_MAXP;
    newbrd.ex_minposts = minp >= 0 ? minp : DEF_MINP;
    newbrd.ex_maxtime = maxt >= 0 ? maxt : DEF_DAYS;
}

int
main(argc, argv)
  int argc;
  char *argv[];
{
  create_board("SYSOP", "[叉滴小站] 報告站長專用版"
  create_board("0announce",
  create_board("admin", 
  int show_allbrd;
  BRD brd;
  FILE *fp;

  if (argc < 2)
    show_allbrd = 1;
  else
    show_allbrd = 0;

  chdir(BBSHOME);

  if (!(fp = fopen(FN_BRD, "r")))
    return -1;

  while (fread(&brd, sizeof(BRD), 1, fp) == 1)
  {
    if (show_allbrd || !str_cmp(brd.brdname, argv[1]))
    {
      printf("看板名稱：%-13s     看板標題：[%s] %s\n", brd.brdname, brd.class, brd.title);
      printf("投票狀態：%-13d     看板板主：%s\n", brd.bvote, brd.BM);
      _bitmsg(MSG_READPERM, STR_PERM, brd.readlevel);
      _bitmsg(MSG_POSTPERM, STR_PERM, brd.postlevel);
      _bitmsg(MSG_BRDATTR, STR_BATTR, brd.battr);
      printf("文章篇數：%d\n", brd.bpost);
      printf("開板時間：%s\n", Btime(&brd.bstamp));
      printf(".DIR時間：%s\n", Btime(&brd.btime));
      printf("最後一篇：%s\n", Btime(&brd.blast));

      if (!show_allbrd)
	break;
    }
  }

  fclose(fp);

  return 0;
}
