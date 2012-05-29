/*-------------------------------------------------------*/
/* battr.h	( NTHU CS MapleBBS Ver 2.36 )		 */
/*-------------------------------------------------------*/
/* target : Board Attribution				 */
/* create : 95/03/29				 	 */
/* update : 95/12/15				 	 */
/*-------------------------------------------------------*/


#ifndef	_BATTR_H_
#define	_BATTR_H_


/* ----------------------------------------------------- */
/* Board Attribution : flags in BRD.battr		 */
/* ----------------------------------------------------- */


#define BRD_NOZAP	0x000001	/* ���i zap */
#define BRD_NOTRAN	0x000002	/* ����H */
#define BRD_NOCOUNT	0x000004	/* ���p�峹�o��g�� */
#define BRD_NOSTAT	0x000008	/* ���ǤJ�������D�έp */
#define BRD_NOVOTE	0x000010	/* �����G�벼���G�� [record] �O */
#define BRD_ANONYMOUS	0x000020	/* �ΦW�ݪO */
#define BRD_NOSCORE	0x000040	/* �������ݪO */
#define BRD_COSIGN	0x000080	/* �s�p�� */
#define BRD_18X		0x000100	/* 18�T�ݪO */
#define BRD_NOREPLY	0x000200	/* ���i�^�Ф峹 */
#define BRD_NOBOO	0x000400	/* ���i�H���� */
#define BRD_LOCAL	0x000800	/* hrs.080323: �w�]�����H�ݪO */
#define BRD_SCOREALIGN	0x001000	/* hrs.080510: ����۰ʹ�� */
#define BRD_SERVICE	0x002000	/* hrs.080514: ���ȩʽ�ݪO */
#define BRD_PERSONAL	0x004000	/* hrs.080514: �ӤH�O */
#define BRD_HIDEALLPOST	0x008000	/* hrs.090717: ��峹�ƥ���BN_ALLHIDPOST �ӫD BN_ALLPOST */

/* ----------------------------------------------------- */
/* �U�غX�Ъ�����N�q					 */
/* ----------------------------------------------------- */


#define NUMBATTRS	16

#define STR_BATTR	"zTcsvA%V8ybLS!Pa"   /* itoc: �s�W�X�Ъ��ɭԧO�ѤF��o�̰� */


#ifdef _ADMIN_C_
char *battr_tbl[NUMBATTRS] =
{
  "���i Zap",			/* BRD_NOZAP */
  "����H�X�h",			/* BRD_NOTRAN */
  "���O���g��",			/* BRD_NOCOUNT */
  "�����������D�έp",		/* BRD_NOSTAT */
  "�����}�벼���G",		/* BRD_NOVOTE */
  "�ΦW�ݪO",			/* BRD_ANONYMOUS */
  "�������ݪO",			/* BRD_NOSCORE */
  "�s�p��",			/* BRD_VOTEBRD */
  "18�T�ݪO",			/* BRD_18X */
  "���i�^�Ф峹",		/* BRD_NOREPLY */
  "���i�H����",			/* BRD_NOBOO */
  "�o��w�]�s����",		/* BRD_LOCAL */
  "����۰ʹ��",		/* BRD_SCOREALIGN */
  "���ȩʽ�ݪO",		/* BRD_SERVICE */
  "�ӤH�O",			/* BRD_PERSONAL */
  "�峹���iALLPOST�O"		/* BRD_HIDEALLPOST */
};

#endif

#endif				/* _BATTR_H_ */
