/*-------------------------------------------------------*/
/* fee.h				                 */
/*-------------------------------------------------------*/
/* target : �w�q�����U���O��	                         */
/* author : hrs@xdbbs.twbbs.org				 */
/* create : 2008/02/26                                   */
/*-------------------------------------------------------*/

#if 0

hrs : �g�k�ܰ��i�A��ܰT���N������_�ӡA��p�� "�A�n�I" FEE_xxx "����"
      
      �ӨϥΤW��atoi(FEE_xxx)�A�ҥH�O��N�w�q���r�ꪺ�C

#endif

#define FEE_EXPIRE  "1000"  /* hrs.080226: �C�R�@���expire����, �O�r�� */
#define FEE_CLOAK   "350"   /* hrs.080215: ���ζO��,�`�N�O�r��!(�ȹ�) */
#define FEE_MQUOTA  "1000"  /* hrs.080215: �R�H�c�e�q�O��,�`�N�O�r��!(�ȹ�) */

/* post.c LuckyPost ���B�������� */

#ifdef HAVE_LUCKYPOST
#define LUCKYPOST_REWARD "7777"         /* �@������������(�ȹ�) */
#endif

/* bbsnet.c bbsnet���O�� */

#ifdef	OPEN_BBSNET
#   define FEE_BBSNET  "50"
#endif

#define FEE_XoAuthor "500"

/* lottery.c �ֳz�C�`���� */
#define FEE_LOTTERY "50"
