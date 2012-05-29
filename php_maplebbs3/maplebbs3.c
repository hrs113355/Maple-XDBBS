#include "php.h"
#undef HAVE_CALENDAR
#include "bbs.h"


static BCACHE *bshm;

void
init_bshm()
{
      /* itoc.030727: 在開啟 bbsd 之前，應該就要執行過 account，
       *      所以 bshm 應該已設定好 */

      bshm = shm_new(BRDSHM_KEY, sizeof(BCACHE));

      if (bshm->uptime <= 0)        /* bshm 未設定完成 */
	        exit(0);
}

static void
update_btime(brdname)
  char *brdname;
{
    init_bshm();
  BRD *brdp, *bend;

  brdp = bshm->bcache;
  bend = brdp + bshm->number;
  do
  {
    if (!strcmp(brdname, brdp->brdname))
    {
      brdp->btime = -1;
      break;
    }
  } while (++brdp < bend);
}


void
OUTGOPOST(hdr, board)
  HDR *hdr;
  char *board;
{
  bntp_t bntp;

  memset(&bntp, 0, sizeof(bntp_t));

  if (board)            /* 新信 */
  {
    bntp.chrono = hdr->chrono;
  }
  strcpy(bntp.board, board);
  strcpy(bntp.xname, hdr->xname);
  strcpy(bntp.owner, hdr->owner);
  strcpy(bntp.nick, hdr->nick);
  strcpy(bntp.title, hdr->title);
  rec_add("innd/out.bntp", &bntp, sizeof(bntp_t));
}

ZEND_FUNCTION(doPost){
  char *brdname;        /* 欲 post 的看板 */
  char *title;          /* 文章標題 */
  char *owner;          /* 作者id */
  char *nick;           /* 作者暱稱 */
  char *content;        /* 文章內容 */
  int outgo;            /* 是否轉信 */
  int junk;             /* 存垃圾用 */

  HDR hdr;
  char folder[64], fpath[256];
  FILE *fp;

  /* 取得 PHP 傳進來的引數 */
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"sssssl",
	      &brdname,&junk,
	      &title,&junk,
	      &owner,&junk,
	      &nick,&junk,
	      &content,&junk,
	      &outgo)== FAILURE)
	    RETURN_NULL(); // 取得失敗
  chdir(BBSHOME);

  brd_fpath(folder, brdname, FN_DIR);
  if (fp = fdopen(hdr_stamp(folder, 'A', &hdr, fpath), "w")){
      fputs(content, fp);
      fclose(fp);
      strncpy(hdr.owner, owner, sizeof(hdr.owner) - 1);
      strncpy(hdr.nick, nick, sizeof(hdr.nick) - 1);
      strncpy(hdr.title, title, sizeof(hdr.title) - 1);
      rec_bot(folder, &hdr, sizeof(HDR));
      
      if (outgo)
	  OUTGOPOST(&hdr, brdname);
      update_btime(brdname);
      RETURN_TRUE;
  }
  RETURN_FALSE;

}
zend_function_entry maplebbs3_functions[] =
{
    ZEND_FE(doPost, NULL)
    {NULL, NULL, NULL}
};
zend_module_entry maplebbs3_module_entry =
{
    STANDARD_MODULE_HEADER,
    "maplebbs3",
    maplebbs3_functions,
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    NULL,
     NO_VERSION_YET,//PHP_MAPLEBBS3_VERSION,
    STANDARD_MODULE_PROPERTIES
};

#if COMPILE_DL_FIRST_MODULE
ZEND_GET_MODULE(maplebbs3)
#endif


