/*-------------------------------------------------------*/
/* loginview.h						 */
/*-------------------------------------------------------*/
/* target : User Flag Option				 */
/* create : 2008/03/11				 	 */
/*-------------------------------------------------------*/


#ifndef	_LV_H_
#define	_LV_H_


/* ----------------------------------------------------- */
/* User Flag Option : flags in ACCT.loginview		 */
/* ----------------------------------------------------- */


#define BFLAG(n)	(1 << n)	/* 32 bit-wise flag */

#define LV_XXX		BFLAG(0)	/* reserved */
#define LV_SONGTOP	BFLAG(1)	/* �I�q���Ʋέp */
#define LV_DAYTOP	BFLAG(2)	/* ����Q�j�������D */
#define LV_WEEKTOP	BFLAG(3)	/* ���g���Q�j�������D */
#define LV_MONTHTOP	BFLAG(4)	/* ����ʤj�������D */
#define LV_YEARTOP	BFLAG(5)	/* ���~�צʤj�������D */
#define LV_TOPTOQUERY	BFLAG(6)	/* �n�_���ƱƦ�] */
#define LV_TOPBEQUERY	BFLAG(7)	/* �H����ƱƦ�] */
#define LV_TOPSTAYTIME	BFLAG(8)	/* �����d�ʤ����] */
#define LV_TOPPOST	BFLAG(9)	/* ����g�ƭ^���] */
#define LV_TOPMONEY	BFLAG(10)	/* �ȹ��I�Ϋʯ��] */
#define LV_TOPLOGIN	BFLAG(11)	/* �W�����ƱƦ�] */
#define LV_BIRTHDAY	BFLAG(12)	/* ����جP */
#define LV_ECONOMY	BFLAG(13)	/* �����g�ٲέp */ 
#define LV_TOPSCORE	BFLAG(14)	/* �峹�����Ʀ�] */
#define LV_NOTEPAD      BFLAG(15)	/* �Ĳ��W���d���O */
#define LV_TODO		BFLAG(16)	/* �ӤH��ƾ� */


#define LV_DEFAULT_NEW         (LV_DAYTOP | LV_TOPMONEY | LV_BIRTHDAY | LV_NOTEPAD | LV_TODO)
#define LV_DEFAULT_GUEST       (LV_SONGTOP | LV_DAYTOP | LV_WEEKTOP | LV_MONTHTOP | LV_YEARTOP | LV_TOPTOQUERY \
				| LV_TOPBEQUERY | LV_TOPSTAYTIME | LV_TOPPOST | LV_TOPMONEY \
				| LV_TOPLOGIN | LV_BIRTHDAY | LV_ECONOMY | LV_TOPSCORE | LV_NOTEPAD)


/* ----------------------------------------------------- */
/* �W���e��������N�q					 */
/* ----------------------------------------------------- */

#define NUMLVS		17

#ifdef _ADMIN_C_

char *lv_tbl[NUMLVS] =
{
  "�O�d",
  "�I�q���Ʋέp",
  "����Q�j�������D",
  "���g���Q�j�������D",
  "����ʤj�������D",
  "���~�צʤj�������D",
  "�n�_���ƱƦ�]",
  "�H����ƱƦ�]",
  "�����d�ʤ����]",
  "����g�ƭ^���]",
  "�ȹ��I�Ϋʯ��]",
  "�W�����ƱƦ�]",
  "����جP",
  "�����g�ٲέp",
  "�峹�����Ʀ�]",
  "�Ĳ��W���d���O",
  "�ӤH��ƾ�"
};
#endif

#endif				/* _LV_H_ */
