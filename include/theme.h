/*-------------------------------------------------------*/
/* theme.h	( NTHU CS MapleBBS Ver 3.10 )		 */
/*-------------------------------------------------------*/
/* target : custom theme				 */
/* create : 02/08/17				 	 */
/* update :   /  /  				 	 */
/*-------------------------------------------------------*/


#ifndef	_THEME_H_
#define	_THEME_H_


/* ----------------------------------------------------- */
/* ���C��w�q�A�H�Q�����ק�				 */
/* ----------------------------------------------------- */

#define COLOR1      ANSI_COLOR(1;37;41)		/* footer/feeter ���e�q�C�� */
#define COLOR2      ANSI_COLOR(1;33;44)		/* footer/feeter ����q�C�� */
#define COLOR3      ANSI_COLOR(1;33;44)		/* neck ���C�� */
#define COLOR4      ANSI_COLOR(1;33;44)		/* ���� ���C�� */
#define COLOR5      ANSI_COLOR(1;37;41)		/* more ���Y�����D�C�� */ /* ĵ�i����X���C�� */
#define COLOR6      ANSI_COLOR(1;33;44)		/* more ���Y�����e�C�� */
#define COLOR7      ANSI_COLOR(1;37)		/* �@�̦b�u�W���C�� */

#define FILEDFG	    (4)		/* ��J�ت��e���C�� (�H 0 - 7 ���) */
#define FILEDBG	    (7)		/* ��J�ت��I���C�� (�H 0 - 7 ���) */

/* ----------------------------------------------------- */
/* �ϥΪ̦W���C��					 */
/* ----------------------------------------------------- */


#define COLOR_NORMAL	""			/* �@��ϥΪ� */
#define COLOR_MYBAD	ANSI_COLOR(1;31)	/* �a�H */
#define COLOR_MYGOOD	ANSI_COLOR(1;32)	/* �ڪ��n�� */
#define COLOR_OGOOD	ANSI_COLOR(1;33)	/* �P�ڬ��� */
#define COLOR_CLOAK	ANSI_COLOR(1;35)	/* ���� */	
#define COLOR_BIRTHDAY	ANSI_COLOR(1;35)	/* �جPID�ܦ� */
#define COLOR_SELF	ANSI_COLOR(1;36)	/* �ۤv */
#define COLOR_BOTHGOOD	ANSI_COLOR(1;37)	/* ���]�n�� */
#define COLOR_BRDMATE	ANSI_COLOR(36)		/* �O�� */
#define COLOR_MYMATE	ANSI_COLOR(1;35)	/* �¦N */

/* ----------------------------------------------------- */
/* �ݪO�C��B�ڪ��̷R�C��B�����s�զC��                  */
/* ----------------------------------------------------- */

#define COLOR_NINFAVOR	""			/* ���b�ڪ��̷R�����q�ݪO�Τ��� */
#define COLOR_INFAVOR	ANSI_COLOR(1;36)	/* �b�ڪ��̷R�����ݪO�Τ��� */
#define COLOR_CANTREAD	ANSI_COLOR(1;30)	/* �i�C�X���L�k�i�J���ݪO */

/* ----------------------------------------------------- */
/* ����m						 */
/* ----------------------------------------------------- */

/* itoc.����: �`�N MENU_XPOS �n >= MENU_XNOTE + MOVIE_LINES */

#define MENU_XNOTE	2		/* �ʺA�ݪO�� (2, 0) �}�l */
#define MOVIE_LINES	11		/* �ʵe�̦h�� 12 �C */

#define MENU_XPOS	13		/* ���}�l�� (x, y) �y�� */
#define MENU_YPOS	((d_cols >> 1) + 18)


/* ----------------------------------------------------- */
/* �T���r��G*_neck() �ɪ� necker ����X�өw�q�b�o	 */
/* ----------------------------------------------------- */

/* necker ����Ƴ��O�G��A�q (1, 0) �� (2, 80) */

/* �Ҧ��� XZ_* ���� necker�A�u�O���Ǧb *_neck()�A�����æb *_head() */

/* ulist_neck() �� xpost_head() ���Ĥ@�����S�O�A���b���w�q */

#define NECKER_CLASS	"[��]�D��� [��]�\\Ū [����]��� [c]�g�� [y]���J [/?]�j�M [s]�ݪO [h]����\n" \
			COLOR3 "  %s   ��  �O       ���O��H��   ��   ��   �z%*s              �H�� �O    �D%*s    " ANSI_RESET
#define NECKER_CLASS2	COLOR3 "       ��  ��            ��   ��   ��   �z%*s                   �O    �D%*s         " ANSI_RESET

#define NECKER_ULIST	"\n" \
			COLOR3 "  �s��   �N��         %s%*s               %-*s             �ʺA       �߱� %s " ANSI_RESET

#define NECKER_PAL	"[��]���} [a]�s�W [c]�ק� [d]�R�� [m]�H�H [w]�e�w [s]��z [��]�d�� [h]����\n" \
			COLOR3 "  �s��    �N ��         %s%*s                                           " ANSI_RESET

#define NECKER_ALOHA	"[��]���} [a]�s�W [d]�R�� [D]�Ϭq�R�� [m]�H�H [w]�e�w [s]���� [f]�ޤJ [h]����\n" \
			COLOR3 "  �s��   �W �� �q �� �W ��%*s                                                    " ANSI_RESET

#define NECKER_VOTE	"[��]���} [R]���G [^P]�|�� [E]�ק� [V]�w�� [^Q]��� [o]�W�� [h]����\n" \
			COLOR3 "  �s��      �}����   �D��H       ��  ��  �v  ��%*s                              " ANSI_RESET

#define NECKER_BMW	"[��]���} [d]�R�� [D]�Ϭq�R�� [m]�H�H [M]�x�s [w]�e�w [s]��s [��]�d�� [h]����\n" \
			COLOR3 "  �s�� �N  ��       ��       �e%*s                                          �ɶ� " ANSI_RESET

#define NECKER_MF	"[��]���} [��]�i�J [^P]�s�W [d]�R�� [c]���� [C]�ƻs [^V]�K�W [m]���� [h]����\n" \
			COLOR3 "  %s   ��  �O       ���O��H��   ��   ��   �z%*s              �H�� �O    �D%*s    " ANSI_RESET

#define NECKER_COSIGN	"[��]���} [��]�\\Ū [^P]�ӽ� [d]�R�� [o]�}�O [h]����\n" \
			COLOR3 "  �s��   �� ��  �|��H       ��  �O  ��  �D%*s                                   " ANSI_RESET

#define NECKER_SONG	"[��]���} [��]�s�� [o]�I�q��ݪO [m]�I�q��H�c [Enter]�s�� [h]����\n" \
			COLOR3 "  �s��     �D              �D%*s                            [�s      ��] [��  ��]" ANSI_RESET

#define NECKER_NEWS	"[��]���} [��]�\\Ū [h]����\n" \
			COLOR3 "  �s��    �� �� �@  ��       �s  �D  ��  �D%*s                                   " ANSI_RESET

#define NECKER_XPOST	"\n" \
			COLOR3 "  �s��    �� �� �@  ��       ��  ��  ��  �D%*s                                   " ANSI_RESET

#define NECKER_MBOX	"[��]���} [��,r]Ū�H [d]�R�� [R,y](�s��)�^�H [s]�H�H [x]��� [X]��F [h]����\n" \
			COLOR3 "  �s��   �� �� �@  ��       �H  ��  ��  �D     (�e�q:%4d/%s %4d/%s)%*s" ANSI_RESET

#define NECKER_POST	"[��]���} [��]�\\Ū [^P]�o�� [b]�i�O�e�� [d]�R�� [V]�벼 [TAB]��ذ� [h]����\n" \
			COLOR3 "  �s��     %s �@  ��       ��  ��  ��  �D%*s                       �H��:%-4d  " ANSI_RESET

#define NECKER_GEM	"[��]���} [��]�s�� [B]�Ҧ� [C]�Ȧs [F]��H [d]�R�� [h]����  %s\n" \
			COLOR3 "  �s��     �D              �D%*s                            [�s      ��] [��  ��]" ANSI_RESET

#define NECKER_RSS	"[��]���} [^P]�s�W [d]�R�� [H]�s����} [T]�s����� [m]���� [��]�d�� [h]����\n" \
			COLOR3 "  �s��  �� �� ��  ��          ��               �z    %*s                         " ANSI_RESET

/* �H�U�o�ǫh�O�@���� XZ_* ���c�� necker */

#define NECKER_VOTEALL	"[��/��]�W�U [PgUp/PgDn]�W�U�� [Home/End]���� [��]�벼 [��][q]���}\n" \
			COLOR3 "  �s��   ��  �O       ���O��H��   ��   ��   �z%*s                  �O    �D%*s     " ANSI_RESET

#define NECKER_CREDIT	"[��]���} [C]���� [1]�s�W [2]�R�� [3]���R [4]�`�p\n" \
			COLOR3 "  �s��   ��  ��   ����  ��  �B  ����     ��  ��%*s                               " ANSI_RESET

#define NECKER_HELP	"[��]���} [��]�\\Ū [^P]�s�W [d]�R�� [T]���D [E]�s�� [m]����\n" \
			COLOR3 "  �s��    �� ��         ��       �D%*s                                           " ANSI_RESET

#define NECKER_INNBBS	"[��]���} [^P]�s�W [d]�R�� [E]�s�� [/]�j�M [Enter]�Բ�\n" \
			COLOR3 "  �s��            ��         �e%*s                                               " ANSI_RESET

#define NECKER_RSSM	"[��]���} [d]�R�� [E]�s�� [/]�j�M [Enter]�Բ�\n" \
			COLOR3 "  �s��  �ӷ����}               %*s                                      �ݪO��   " ANSI_RESET

#define NECKER_RSSB	"[��]���} [d]�R�� [E]�s�� [/]�j�M [Enter]�Բ�\n" \
			COLOR3 "  �s��  �q�\\�H        �q�\\�ݪO      �ɶ� %*s                                     " ANSI_RESET
/* ----------------------------------------------------- */
/* �T���r��Gmore() �ɪ� footer ����X�өw�q�b�o	 */
/* ----------------------------------------------------- */

/* itoc.010914.����: ��@�g�A�ҥH�s FOOTER�A���O 78 char */

/* itoc.010821: �`�N \\ �O \�A�̫�O�|�F�@�Ӫť��� :p */

#define FOOTER_POST	\
COLOR1 " �峹��Ū " COLOR2 " (ry)�^�� (=\\[]<>-+;'`)�D�D (|?QA)�j�M���D�@�� (kj)�W�U�g (C)�Ȧs   "

#define FOOTER_MAILER	\
COLOR1 " �������� " COLOR2 " (ry)�^�H/�s�� (X)��F (d)�R�� (m)�аO (C)�Ȧs (=\\[]<>-+;'`|?QAkj)  "

#define FOOTER_GEM	\
COLOR1 " ��ؿ�Ū " COLOR2 " (=\\[]<>-+;'`)�D�D (|?QA)�j�M���D�@�� (kj)�W�U�g (������)�W�U���}   "

#ifdef HAVE_GAME
#define FOOTER_TALK	\
COLOR1 " ��ͼҦ� " COLOR2 " (^O)�﫳�Ҧ� (^C,^D)������� (^T)�����I�s�� (^Z)�ֱ��C�� (^G)�͹�  "
#else
#define FOOTER_TALK	\
COLOR1 " ��ͼҦ� " COLOR2 " (^C,^D)������� (^T)�����I�s�� (^Z)�ֱ��C�� (^G)�͹� (^Y)�M��      "
#endif

#define FOOTER_COSIGN	\
COLOR1 " �s�p���� " COLOR2 " (ry)�[�J�s�p (kj)�W�U�g (������)�W�U���} (h)����                   " 

#define FOOTER_MORE	\
COLOR1 " �s�� P.%d (%d%%) " COLOR2 " (h)���� [PgUp][PgDn][0][$]���� (/n)�j�M (C)�Ȧs (��q)���� "

#define FOOTER_MORE2	\
COLOR1 " �s�� P.%d (%d%%) |        ���������e�t������X,��@�̥����o���z����ơ�        "

#define FOOTER_MORE3     \
COLOR1 "     �峹��Ū     |        ���������e�t������X,��@�̥����o���z����ơ�        "



#define FOOTER_VEDIT	\
COLOR1 " %s " COLOR2 " (^Z)���� (^W)�Ÿ� (^L)��ø (^X)�ɮ׳B�z ��%s�x%s��%5d:%3d    " ANSI_RESET


/* ----------------------------------------------------- */
/* �T���r��Gxo_foot() �ɪ� feeter ����X�өw�q�b�o      */
/* ----------------------------------------------------- */


/* itoc.010914.����: �C��h�g�A�ҥH�s FEETER�A���O 78 char */

#define FEETER_CLASS	\
COLOR1 " �ݪO��� " COLOR2 " (c)�s�峹 (vV)�аO�wŪ��Ū (y)�����C�X (z)��q (A)����j�M (S)�Ƨ� "

#define FEETER_ULIST	\
COLOR1 " ���ͦC�� " COLOR2 " (f)�n�� (t)��� (q)�d�� (ad)��� (m)�H�H (w)�e�w (s)��s (TAB)���� "

#define FEETER_PAL	\
COLOR1 " �I�B�ަ� " COLOR2 " (a)�s�W (d)�R�� (c)�ͽ� (m)�H�H (f)�ޤJ�n�� (r^Q)�d�� (s)��s      "

#define FEETER_PBAD	\
COLOR1 " ����W�� " COLOR2 " (a)�s�W (d)�R�� (c)�c�� (m)�H�H (f)�ޤJ�n�� (r^Q)�d�� (s)��s      "

#define FEETER_ALOHA	\
COLOR1 " �W���q�� " COLOR2 " (a)�s�W (d)�R�� (D)�Ϭq�R�� (f)�ޤJ�n�� (r^Q)�d�� (s)��s          "

#define FEETER_VOTE	\
COLOR1 " �ݪO�벼 " COLOR2 " (��/r/v)�벼 (R)���G (^P)�s�W�벼 (E)�ק� (V)�w�� (b)�}�� (o)�W��  "

#define FEETER_BMW	\
COLOR1 " �e�w�^�U " COLOR2 " (d)�R�� (D)�Ϭq�R�� (m)�H�H (w)�e�w (^R)�^�T (^Q)�d�� (s)��s      "

#define FEETER_MF	\
COLOR1 " �ڪ��̷R " COLOR2 " (^P/d)�s�W/�R�� (Cg/p^V)�ƻs/�K�W (vV)�wŪ/��Ū (K)�ƥ� (y)�C�X���� "
		 
#define FEETER_COSIGN	\
COLOR1 " �s�p�p�� " COLOR2 " (r)Ū�� (y)�^�� (^P)�o�� (d)�R�� (o)�}�O (c)���� (E)�s�� (B)�]�w   "

#define FEETER_SONG	\
COLOR1 " �I�q�t�� " COLOR2 " (r)Ū�� (o)�I�q��ݪO (m)�I�q��H�c (E)�s���ɮ� (T)�s����D        "

#define FEETER_NEWS	\
COLOR1 " �s�D�I�� " COLOR2 " (��/��)�W�U (PgUp/PgDn)�W�U�� (Home/End)���� (��r)��� (��)(q)���} "

#define FEETER_XPOST	\
COLOR1 " ��C�j�M " COLOR2 " (y)�^�� (x)��� (m)�аO (d)�R�� (^P)�o�� (^Q)�d�ߧ@�� (t)����      "

#define FEETER_MBOX	\
COLOR1 " �H�H�۱� " COLOR2 " (y)�^�H (F/X/x)��H/��F/��� (d)�R�� (D)�Ϭq�R�� (m)�аO (E)�s��  "

#define FEETER_POST	\
COLOR1 " �峹�C�� " COLOR2 " (ry)�^�� (S/a)�j�M/���D/�@�� (~G)��C�j�M (x)��� (QI)�d�� (u)�s�D  "

#define FEETER_GEM	\
COLOR1 " �ݪO��� " COLOR2 " (^P/a/f)�s�W/�峹/�ؿ� (E)�s�� (T)���D (m)���� (c)�ƻs (p^V)�K�W    "

#define FEETER_RSS	\
COLOR1 " ���Ᾱ " COLOR2 " (U)�k�s��s�ɶ� (s)��s�C�� (E)�s�� (D)�Ϭq�R (K)�Ȱ� (o)�]�w�L�o  "

#define FEETER_VOTEALL	\
COLOR1 " �벼���� " COLOR2 " (��/��)�W�U (PgUp/PgDn)�W�U�� (Home/End)���� (��)�벼 (��)(q)���}  "

#define FEETER_HELP	\
COLOR1 " ������� " COLOR2 " (��/��)�W�U (PgUp/PgDn)�W�U�� (Home/End)���� (��r)�s�� (��)(q)���} "

#define FEETER_INNBBS	\
COLOR1 " ��H�]�w " COLOR2 " (��/��)�W�U (PgUp/PgDn)�W�U�� (Home/End)���� (��)(q)���}           "

#define FEETER_RSSM	\
COLOR1 " ���� RSS �]�w " COLOR2 " (��/��)�W�U (PgUp/PgDn)�W�U�� (Home/End)���� (��)(q)���}      "

#define FEETER_RSSB	\
COLOR1 " �ӷ��ݪO�C�� " COLOR2 " (��/��)�W�U (PgUp/PgDn)�W�U�� (Home/End)���� (��)(q)���}       "
/* ----------------------------------------------------- */
/* ���x�ӷ�ñ�W						 */
/* ----------------------------------------------------- */

/* itoc: ��ĳ banner ���n�W�L�T��A�L������ñ�i��|�y���Y�ǨϥΪ̪��ϷP */
#define EDIT_BANNER "\n--\n" \
    ANSI_COLOR(32)   " �� �o�H��: " BBSNAME "(" MYHOSTNAME ")" ANSI_RESET "\n" \
    ANSI_COLOR(1;30) " �� �@��: %s" ANSI_RESET "\n"\
    ANSI_COLOR(1;30) " �� �G�m: %s (%s)" ANSI_RESET "\n"

#define MODIFY_BANNER   ANSI_COLOR(32) " �� �s��: %-13s  �Ӧ� %-20s  �� %s" ANSI_RESET "\n"
#define CROSS_BANNER	ANSI_COLOR(32) " �� ���: %-13s  ��� [%s] �ݪO  �� %s" ANSI_RESET "\n"
#define DELETE_BANNER	ANSI_COLOR(32) " �� �R��: %-13s  �Ӧ� %-20s  �� %s" ANSI_RESET "\n"
#define LUCKY_BANNER	ANSI_COLOR(32) " �� ����: �峹����," BN_LUCKYPOST "�O���å�,���O�峹�|�[�K�B�z,��@�̥i�H�R��" ANSI_RESET "\n"


/* ----------------------------------------------------- */
/* ��L�T���r��						 */
/* ----------------------------------------------------- */
#define VMSG_NULL   ANSI_COLOR(1;34;44) " �e�e�e�e�e�e�e�e�e�e�e�e�e�e" ANSI_RESET ANSI_COLOR(1;37;44) \
		    " �� �Ы����N���~�� �� " ANSI_RESET \
		    ANSI_COLOR(1;34;44) "�e�e�e�e�e�e�e�e�e�e�e�e�e�e " ANSI_RESET

#define ICON_UNREAD_BRD		ANSI_COLOR(1;35) "��"	/* ��Ū�ݪO */
#define ICON_UNREAD_BRD2        ANSI_COLOR(1;37) "��"   /* ��Ū (���s����/�פ�) */
#define ICON_READ_BRD		"  "			/* �wŪ�ݪO */

#define ICON_GAMBLED_BRD	ANSI_COLOR(1;31) "��" ANSI_RESET	
/* �|���L�����ݪO */
#define ICON_VOTED_BRD		ANSI_COLOR(1;33) "��" ANSI_RESET	
/* �|��벼�����ݪO */
#define ICON_NOTRAN_BRD		"��"			/* ����H�O */
#define ICON_TRAN_BRD		"��"			/* ��H�O */

#define TOKEN_ZAP_BRD		'-'			/* zap �O */
#define TOKEN_FRIEND_BRD	'.'			/* �n�ͪO */
#define TOKEN_SECRET_BRD	')'			/* ���K�O */
#define TOKEN_SEAL_BRD		']'			/* �ʦL�O */
#endif				/* _THEME_H_ */
