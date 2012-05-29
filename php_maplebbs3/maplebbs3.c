#include "php.h"
#undef HAVE_CALENDAR
#include "bbs.h"


static BCACHE *bshm;

void
init_bshm()
{
      /* itoc.030727: �b�}�� bbsd ���e�A���ӴN�n����L account�A
       *      �ҥH bshm ���Ӥw�]�w�n */

      bshm = shm_new(BRDSHM_KEY, sizeof(BCACHE));

      if (bshm->uptime <= 0)        /* bshm ���]�w���� */
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

  if (board)            /* �s�H */
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
  char *brdname;        /* �� post ���ݪO */
  char *title;          /* �峹���D */
  char *owner;          /* �@��id */
  char *nick;           /* �@�̼ʺ� */
  char *content;        /* �峹���e */
  int outgo;            /* �O�_��H */
  int junk;             /* �s�U���� */

  HDR hdr;
  char folder[64], fpath[256];
  FILE *fp;

  /* ���o PHP �Ƕi�Ӫ��޼� */
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"sssssl",
	      &brdname,&junk,
	      &title,&junk,
	      &owner,&junk,
	      &nick,&junk,
	      &content,&junk,
	      &outgo)== FAILURE)
	    RETURN_NULL(); // ���o����
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


