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
// XXX �@��Ѽƪ��g�k�u������, ���O�]�� brd ���g�k�|�ϱo main �̭��@�ζ�,
//     ���Ȯɳo�˧a.

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
  create_board("SYSOP", "[�e�w�p��] ���i�����M�Ϊ�"
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
      printf("�ݪO�W�١G%-13s     �ݪO���D�G[%s] %s\n", brd.brdname, brd.class, brd.title);
      printf("�벼���A�G%-13d     �ݪO�O�D�G%s\n", brd.bvote, brd.BM);
      _bitmsg(MSG_READPERM, STR_PERM, brd.readlevel);
      _bitmsg(MSG_POSTPERM, STR_PERM, brd.postlevel);
      _bitmsg(MSG_BRDATTR, STR_BATTR, brd.battr);
      printf("�峹�g�ơG%d\n", brd.bpost);
      printf("�}�O�ɶ��G%s\n", Btime(&brd.bstamp));
      printf(".DIR�ɶ��G%s\n", Btime(&brd.btime));
      printf("�̫�@�g�G%s\n", Btime(&brd.blast));

      if (!show_allbrd)
	break;
    }
  }

  fclose(fp);

  return 0;
}
