/*-------------------------------------------------------*/
/* manage.c	( NTHU CS MapleBBS Ver 3.10 )		 */
/*-------------------------------------------------------*/
/* target : �ݪO�޲z				 	 */
/* create : 95/03/29				 	 */
/* update : 96/04/05				 	 */
/*-------------------------------------------------------*/


#include "bbs.h"


extern BCACHE *bshm;
extern char *battr_tbl[];

extern XZ xz[];


#ifdef HAVE_TERMINATOR
/* ----------------------------------------------------- */
/* �����\�� : �ط�������				 */
/* ----------------------------------------------------- */


extern char xo_pool[];


#define MSG_TERMINATOR	"�m�ط������١n"

int
post_terminator(xo)		/* Thor.980521: �׷��峹�R���j�k */
    XO *xo;
{
    int mode, type;
    HDR *hdr;
    char keyOwner[80], keyTitle[TTLEN + 1], buf[80];

    if (!HAS_PERM(PERM_ALLBOARD))
	return XO_FOOT;

    mode = vans(MSG_TERMINATOR "�R�� (1)����@�� (2)������D (3)�۩w�H[Q] ") - '0';

    if (mode == 1)
    {
	hdr = (HDR *) xo_pool + (xo->pos - xo->top);
	strcpy(keyOwner, hdr->owner);
    }
    else if (mode == 2)
    {
	hdr = (HDR *) xo_pool + (xo->pos - xo->top);
	strcpy(keyTitle, str_ttl(hdr->title));		/* ���� Re: */
    }
    else if (mode == 3)
    {
	if (!vget(b_lines, 0, "�@�̡G", keyOwner, 73, DOECHO))
	    mode ^= 1;
	if (!vget(b_lines, 0, "���D�G", keyTitle, TTLEN + 1, DOECHO))
	    mode ^= 2;
    }
    else
    {
	return XO_FOOT;
    }

    type = vans(MSG_TERMINATOR "�R�� (1)��H�O (2)�D��H�O (3)�Ҧ��ݪO�H[Q] ");
    if (type < '1' || type > '3')
	return XO_FOOT;

    sprintf(buf, "�R��%s�G%.35s ��%s�O�A�T�w��(Y/N)�H[N] ", 
	    mode == 1 ? "�@��" : mode == 2 ? "���D" : "����", 
	    mode == 1 ? keyOwner : mode == 2 ? keyTitle : "�۩w", 
	    type == '1' ? "��H" : type == '2' ? "�D��H" : "�Ҧ���");

    if (vans(buf) == 'y')
    {
	BRD *bhdr, *head, *tail;
	char tmpboard[BNLEN + 1];

	/* Thor.980616: �O�U currboard�A�H�K�_�� */
	strcpy(tmpboard, currboard);

	head = bhdr = bshm->bcache;
	tail = bhdr + bshm->number;
	do				/* �ܤ֦� note �@�O */
	{
	    int fdr, fsize, xmode;
	    FILE *fpw;
	    char fpath[64], fnew[64], fold[64];
	    HDR *hdr;

	    xmode = head->battr;
	    if ((type == '1' && (xmode & BRD_NOTRAN)) || (type == '2' && !(xmode & BRD_NOTRAN)))
		continue;

	    /* Thor.980616: ��� currboard�A�H cancel post */
	    strcpy(currboard, head->brdname);

	    sprintf(buf, MSG_TERMINATOR "�ݪO�G%s \033[5m...\033[m", currboard);
	    outz(buf);
	    refresh();

	    brd_fpath(fpath, currboard, fn_dir);

	    if ((fdr = open(fpath, O_RDONLY)) < 0)
		continue;

	    if (!(fpw = f_new(fpath, fnew)))
	    {
		close(fdr);
		continue;
	    }

	    fsize = 0;
	    mgets(-1);
	    while (hdr = mread(fdr, sizeof(HDR)))
	    {
		xmode = hdr->xmode;

		if ((xmode & POST_MARKED) || 
			((mode & 1) && strcmp(keyOwner, hdr->owner)) ||
			((mode & 2) && strcmp(keyTitle, str_ttl(hdr->title))))
		{
		    if ((fwrite(hdr, sizeof(HDR), 1, fpw) != 1))
		    {
			fclose(fpw);
			close(fdr);
			goto contWhileOuter;
		    }
		    fsize++;
		}
		else
		{
		    /* ���ós�u��H */

		    cancel_post(hdr);
		    hdr_fpath(fold, fpath, hdr);
		    unlink(fold);
		}
	    }
	    close(fdr);
	    fclose(fpw);

	    sprintf(fold, "%s.o", fpath);
	    rename(fpath, fold);
	    if (fsize)
		rename(fnew, fpath);
	    else
		contWhileOuter:
		    unlink(fnew);

	    btime_update(brd_bno(currboard));
	} while (++head < tail);

	/* �٭� currboard */
	strcpy(currboard, tmpboard);
	return XO_LOAD;
    }

    return XO_FOOT;
}
#endif	/* HAVE_TERMINATOR */


/* ----------------------------------------------------- */
/* �O�D�\�� : �ק�O�W					 */
/* ----------------------------------------------------- */


static int
post_brdtitle(xo)
    XO *xo;
{
    BRD *oldbrd, newbrd;

    oldbrd = bshm->bcache + currbno;
    memcpy(&newbrd, oldbrd, sizeof(BRD));

    /* itoc.����: ���I�s brd_title(bno) �N�i�H�F�A�S�t�A�Z�F�@�U�n�F :p */
    vget(b_lines, 0, "�ݪO�D�D�G", newbrd.title, BTLEN + 1, GCARRY);

    if (memcmp(&newbrd, oldbrd, sizeof(BRD)) && vans(msg_sure_ny) == 'y')
    {
	memcpy(oldbrd, &newbrd, sizeof(BRD));
	rec_put(FN_BRD, &newbrd, sizeof(BRD), currbno, NULL);
	brd_classchange("gem/@/@"CLASS_INIFILE, oldbrd->brdname, &newbrd);
    }

    return XO_HEAD;
}


/* ----------------------------------------------------- */
/* �O�D�\�� : �ק�i�O�e��				 */
/* ----------------------------------------------------- */


static int
post_memo_edit(xo)
    XO *xo;
{
    int mode;
    char fpath[64];

    mode = vans("�i�O�e�� (D)�R�� (E)�ק� (Q)�����H[E] ");

    if (mode != 'q')
    {
	brd_fpath(fpath, currboard, fn_note);

	if (mode == 'd')
	{
	    unlink(fpath);
	}
	else
	{
	    if (vedit(fpath, 0))	/* Thor.981020: �`�N�Qtalk�����D */
		vmsg(msg_cancel);
	}
    }
    return XO_HEAD;
}

/* ----------------------------------------------------- */
/* �O�D�\�� : ���ذϵe��                               */
/* ----------------------------------------------------- */


static int
post_gemmemo_edit(xo)
    XO *xo;
{
    int mode;
    char fpath[64];

    mode = vans("��ذ��w��e�� (D)�R�� (E)�ק� (Q)�����H[E] ");

    if (mode != 'q')
    {
	brd_fpath(fpath, currboard, fn_gemnote);

	if (mode == 'd')
	{
	    unlink(fpath);
	}
	else
	{
	    if (vedit(fpath, 0))      /* Thor.981020: �`�N�Qtalk�����D */
		vmsg(msg_cancel);
	}
    }
    return XO_HEAD;
}

/* ----------------------------------------------------- */
/* �O�D�\�� : �ק�ݪO����                               */
/* ----------------------------------------------------- */


#ifdef HAVE_BRD_EXPLAIN
static int
post_explain_edit(xo)
    XO *xo;
{
    int mode;
    char fpath[64];

    mode = vans("�ݪO���� (D)�R�� (E)�ק� (Q)�����H[E] ");

    if (mode != 'q')
    {
	brd_fpath(fpath, currboard, fn_explain);
	if (mode == 'd')
	    unlink(fpath);
	else
	{
	    if (vedit(fpath, 0)) /* Thor.981020: �`�N�Qtalk�����D */
		vmsg(msg_cancel);

	    return XO_HEAD;
	}
    }

    return XO_FOOT;
}
#endif


/* ----------------------------------------------------- */
/* �O�D�\�� : �ק�o�����                               */
/* ----------------------------------------------------- */


static int
post_postlaw_edit(xo)       /* �O�D�۩w�峹�o����� */
    XO *xo;
{
    int mode;
    char fpath[64];

    mode = vans("�峹�o����� (D)�R�� (E)�ק� (Q)�����H[E] ");

    if (mode != 'q')
    {
	brd_fpath(fpath, currboard, FN_POSTLAW);

	if (mode == 'd')
	{
	    unlink(fpath);
	    return XO_FOOT;
	}

	if (vedit(fpath, 0))      /* Thor.981020: �`�N�Qtalk�����D */
	    vmsg(msg_cancel);
	return XO_HEAD;
    }
    return XO_FOOT;
}

/* ----------------------------------------------------- */
/* �O�D�\�� : ����q�\�\��                             */
/* ----------------------------------------------------- */

static int post_rss(XO *xo)
{
    XO *xt;
    char fpath[256];

    if (!(bbstate & STAT_BOARD) && !HAS_PERM(PERM_SYSOP))
	return XO_NONE;

    pmsg("RSS�q�\\���W�u����, ��������D�Ш�XDBug�O�^��");
    brd_fpath(fpath, currboard, fn_rssconf); 
    xz[XZ_RSS - XO_ZONE].xo = xt = xo_new(fpath);
    strcpy(xt->dir, fpath);
    xover(XZ_RSS);        
    free(xt);

    return XO_INIT;
}

#ifdef POST_PREFIX
/* ----------------------------------------------------- */
/* �O�D�\�� : �ק�o�����O                               */
/* ----------------------------------------------------- */


static int
post_prefix_edit(xo)
    XO *xo;
{
    int i;
    FILE *fp;
    char mybuf[40];
    char fpath[64], buf[20], prefix[NUM_PREFIX][20], *menu[NUM_PREFIX + 4];
    char *prefix_def[NUM_PREFIX] =   /* �w�]�����O */
    {
	"[���i]","[�s�D]","[����]","[���]","[�Ч@]",
	"[����]","[��L]","[���D]","[�K��]","[����]"
    };

    if (!(bbstate & STAT_BOARD))
	return XO_NONE;
    i = vans("���O (D)�R�� (E)�ק� (Q)�����H[E] ");

    if (i == 'q')
	return XO_FOOT;

    brd_fpath(fpath, currboard, FN_PREFIX);

    if (i == 'd')
    {
	unlink(fpath);
	return XO_FOOT;
    }

    i = 0;

    if (fp = fopen(fpath, "r"))
    {
	for (; i < NUM_PREFIX; i++)
	{
	    char *ptr;
	    if (fgets(buf, 14, fp) == NULL)
		break;

	    ptr = strchr(buf, '\n');
	    if (ptr)
		*ptr = '\0';
	    sprintf(prefix[i], "%d.%s", i, buf);
	}
	fclose(fp);
    }
    else
    {
	/* �񺡦� NUM_PREFIX �� */
	for (; i < NUM_PREFIX; i++)
	    sprintf(prefix[i], "%d.%s", i, prefix_def[i]);
    }

    if (!i)
	return XO_FOOT;

    int j;
    menu[0] = "0Q";
    for (j = 1; j <= i; j++)
	menu[j] = prefix[j - 1];
    menu[j++] = "C.�ܧ����O�ƶq";
    menu[j++] = "Q.���}";
    menu[j++] = NULL;

    do
    {
	/* �b popupmenu �̭��� ���� ���} */
	j = pans(3, 20, "�峹���O", menu) - '0';
	if (j >= 0 && j < i)
	{
	    strcpy(buf, prefix[j] + 2);
	    if (vget(b_lines, 0, "���O�G", buf, 13, GCARRY))
		strcpy(prefix[j] + 2, buf);
	}
	else if (j == 'c' - '0')
	{
	    char buf[3];
	    sprintf(buf, "%d", i);
	    if (vget(b_lines, 0, "�п�J�s�����O�ƶq(1 - 10)�G", buf, 3, GCARRY))
	    {
		int num = atoi(buf);
		if (num <= 0 || num >NUM_PREFIX)
		    continue;
		if (num > i)
		{
		    int k;
		    for (k = i ; k < num; k++)
		    {
			sprintf(prefix[k], "%d.%s", k, prefix_def[k]);
			menu[k+1] = prefix[k];
		    }
		}
		i = num;
		menu[i+1] = "C.�ܧ����O�ƶq";
		menu[i+2] = "Q.���}";
		menu[i+3] = NULL;
	    }
	}
    } while (j != 'q' - '0');

    if (vans(msg_sure_ny) != 'y')
    {
	pmsg("�����]�w");
	return XO_HEAD;
    }

    if (fp = fopen(fpath, "w"))
    {
	for (j = 0; j < i; j++)
	{
	    char *ptr;
	    sprintf(mybuf, "%s", prefix[j] + 2);
	    ptr = strchr(mybuf, '\n');
	    if (ptr)
		*ptr = '\0';
	    fprintf(fp, "%s\n", mybuf);
	}
	fclose(fp);
    }

    return XO_FOOT;

}
#endif      /* POST_PREFIX */


/* ----------------------------------------------------- */
/* �O�D�\�� : �ק�O�D�W��				 */
/* ----------------------------------------------------- */


static int
post_changeBM(void)
{
    BRD *brd = bshm->bcache + currbno;
    int ret = brd_changeBM(brd);
    rec_put(FN_BRD, brd, sizeof(BRD), currbno, NULL); // TODO: check is needed.
    sprintf(currBM, "�O�D�G%s", *(brd->BM) <= ' '  ? brd->battr & BRD_SERVICE ?
	            str_operators : "�x�D��" : brd->BM);
    return ret;
}


#ifdef HAVE_MODERATED_BOARD
/* ----------------------------------------------------- */
/* �O�D�\�� : �ݪO�v��					 */
/* ----------------------------------------------------- */


static int
post_brdlevel(xo)
    XO *xo;
{
    BRD *oldbrd, newbrd;

    oldbrd = bshm->bcache + currbno;
    memcpy(&newbrd, oldbrd, sizeof(BRD));

    switch (vansf("1)���}�ݪO 2)���K�ݪO 3)�n�ͬݪO %s�H[Q] ",
		HAS_PERM(PERM_ALLBOARD) ? "4)�ʦL�ݪO" : ""))
    {
	case '1':				/* ���}�ݪO */
	    newbrd.bmode = BMODE_OPEN;
	    newbrd.postlevel = PERM_POST;
	    newbrd.battr &= ~(BRD_NOSTAT | BRD_NOVOTE);
	    break;

	case '2':				/* ���K�ݪO */
	    newbrd.postlevel = 0;
	    newbrd.bmode = BMODE_HIDE;
	    newbrd.battr |= (BRD_NOSTAT | BRD_NOVOTE);
	    break;

	case '3':				/* �n�ͬݪO */
	    newbrd.bmode = BMODE_PAL;
	    newbrd.postlevel = 0;
	    newbrd.battr |= (BRD_NOSTAT | BRD_NOVOTE);
	    break;

	case '4':				/* �ʦL�O */
	    if (!HAS_PERM(PERM_ALLBOARD))
		return XO_HEAD;
	    newbrd.bmode = BMODE_SEAL;
	    newbrd.battr |= (BRD_NOSTAT | BRD_NOVOTE);
	    break;

	default:
	    return XO_HEAD;
    }

    if (memcmp(&newbrd, oldbrd, sizeof(BRD)) && vans(msg_sure_ny) == 'y')
    {
	memcpy(oldbrd, &newbrd, sizeof(BRD));
	rec_put(FN_BRD, &newbrd, sizeof(BRD), currbno, NULL);
    }

    return XO_HEAD;
}
#endif	/* HAVE_MODERATED_BOARD */


#ifdef HAVE_MODERATED_BOARD
/* ----------------------------------------------------- */
/* �O�ͦW��Gmoderated board				 */
/* ----------------------------------------------------- */


static void
bpal_cache(char * fpath, int type) /* hrs.080412 : type = 1 �i��/type = 2 ���� */
{
    BPAL *bpal;

    bpal = type - 1 ? bshm->pbad + currbno : bshm->pcache + currbno;
    bpal->pal_max = image_pal(fpath, bpal->pal_spool);
}




static int
XoBM(xo)
    XO *xo;
{
    XO *xt;
    char fpath[64];
    char op;

    outl(b_lines - 5, NULL);
    outl(b_lines - 4, "���ެO ���}/�n��/���� �O�G");
    outl(b_lines - 3, "1) �b�i���W�椤���b���A�@�ߥi�i�J���ݪO�æ��\\Ū�εo���v���C");
    outl(b_lines - 2, "2) �b����W�椤���b���A�@�ߤ��i�o��A�W�椤���a�H�L�k�ݨ����O�C");
    outl(b_lines - 1, ANSI_COLOR(1;31)"�Ъ`�N: �W��]�w������A�n�b���}��������~�|�M�ΡC" 
	    ANSI_RESET);
    op = vans("�A�n�]�w 1) �i���W�� 2) ����W�� Q) ���} [Q] ");

    if (op != '1' && op != '2')
	return XO_HEAD;
    op -= '0';
    brd_fpath(fpath, currboard, op - 1 ? fn_bad : fn_pal);
    xz[XZ_PAL - XO_ZONE].xo = xt = xo_new(fpath);
    xt->key =  op - 1 ? PALTYPE_PBAD : PALTYPE_BPAL;
    xover(XZ_PAL);		/* Thor: �ixover�e, pal_xo �@�w�n ready */

    /* build userno image to speed up, maybe upgreade to shm */

    bpal_cache(fpath, op);

    free(xt);

    vmsg("�W���s�����C");

    return XO_INIT;
}
#endif	/* HAVE_MODERATED_BOARD */

/* ----------------------------------------------------- */
/* �ݪO�\Ū����                                          */
/* ----------------------------------------------------- */

static int
post_usies(xo)
    XO *xo;
{
    char fpath[64];

    brd_fpath(fpath, currboard, "usies");
    if (more(fpath, FOOTER_POST) >= 0 &&
	    vans("�аݬO�_�R���o�ǬݪO�\\Ū�O��(Y/N)�H[N] ") == 'y')
	unlink(fpath);

    return XO_HEAD;
}

/* ----------------------------------------------------- */
/* �O�D�ʶR�峹�W��(expire�W��)                          */
/* ----------------------------------------------------- */

static int
m_expire()
{
    BRD *oldbrd, newbrd;
    char buf[80];
    int pay;
    uschar op;

    if (HAS_STATUS(STATUS_COINLOCK))
    {
	vmsg(msg_coinlock);
	return XEASY;
    }

    oldbrd = bshm->bcache + currbno;
    memcpy(&newbrd, oldbrd, sizeof(BRD));

    move(b_lines - 3, 0);
    clrtobot();
    prints("�ثe�ݪO���峹�W���� %d �g/�U�� %d �g/�O�s %d ��\n"
	    , oldbrd->ex_maxposts, oldbrd->ex_minposts, oldbrd->ex_maxtime);
    prints("�@�ӳ�쬰 50 �g�峹�� 20 �� , �@�ӳ��� " FEE_EXPIRE " ��\n");
    prints("�A�{�b���W�� %d ���C", cuser.money);

    op = vans("�A�n�R���O�@��� 1) 50 �g�峹 2) 20 �� [Q]");
    if (!op)
	return XO_HEAD;

    if (!vget(b_lines, 0, "�A�n�R�X�ӳ��O? ", buf, 4, DOECHO))
	return XO_HEAD;
    pay = atoi(buf);
    if (pay <= 0)
	return XO_HEAD;

    if (pay * atoi(FEE_EXPIRE) > cuser.money)
    {
	vmsg("�l�B�����C");
	return XO_HEAD;
    }

    newbrd.ex_maxposts += op - '1' ? 0 : 50 * pay; 
    newbrd.ex_minposts += op - '1' ? 0 : 50 * pay;
    newbrd.ex_maxtime  += op - '1' ? 20 * pay : 0;

    cuser.money -= atoi(FEE_EXPIRE) * pay;
    if (memcmp(&newbrd, oldbrd, sizeof(BRD)) && vans(msg_sure_ny) == 'y')
    {
	memcpy(oldbrd, &newbrd, sizeof(BRD));
	rec_put(FN_BRD, &newbrd, sizeof(BRD), currbno, NULL);
	vmsgf("�z�R�F %d �ӳ��A�l�B %d ��", pay, cuser.money);
    }

    return XO_HEAD;
}

/* ----------------------------------------------------- */
/* �۩w�峹�d��                                          */
/* ----------------------------------------------------- */

static int
m_samplepost(void)
{
    int mode;
    char fpath[64], prefix[NUM_PREFIX][20], *menu[NUM_PREFIX + 3], buf[30];
    register int i, j;
    FILE *fp;

    brd_fpath(fpath, currboard, FN_PREFIX);

    if (fp = fopen(fpath, "r"))
    {
	for (i = 0; i < NUM_PREFIX; i++)
	{
	    char *ptr;
	    if (fgets(buf, 14, fp) == NULL)
		break;
	    ptr = strchr(buf, '\n');
	    if (ptr)
		*ptr = '\0';
	    sprintf(prefix[i], "%d.%s", i, buf);
	}
	fclose(fp);
    }
    else
    {
	vmsg("�Х��]�w�n�峹���O�C");
	return XO_HEAD;
    }

    menu[0] = "0Q";
    for (j = 1; j <= i; j++)
	menu[j] = prefix[j - 1];
    menu[j++] = "Q.���}";
    menu[j++] = NULL;

    do
    {
	clear();
	j = pans(3, 20, "�峹�d���۹������峹���O", menu) - '0';
	if (j >= 0 && j < i)
	{
	    sprintf(buf, FN_SAMPLE ".%d", j);
	    brd_fpath(fpath, currboard, buf);
	    mode = vans("�峹�d�� (D)�R�� (E)�ק� (Q)�����H[E] ");

	    if (mode != 'q')
	    {

		if (mode == 'd')
		    unlink(fpath);
		else
		{
		    if (vedit(fpath, 0))
			vmsg(msg_cancel);
		    else
			vmsg("�x�s�]�w�C");
		}
	    }
	}
    } while (j != 'q' - '0');

    return XO_HEAD;
}


/* ----------------------------------------------------- */
/* �O�D���						 */
/* ----------------------------------------------------- */


int
post_manage(xo)
    XO *xo;
{

#ifdef POPUP_ANSWER
    char *menu[] = 
    {
	"BQ",
	"BTitle  �ק�ݪO���D",
	"WMemo   �s��i�O�e��",
	"GemMemo ���ذϵe��",
#ifdef HAVE_BRD_EXPLAIN
	"TExplain�s��ݪO����",
#endif
	"PostLaw �s��o�����",
	"ZRSS    ���Ᾱ�]�w",
#ifdef POST_PREFIX
	"RPrefix �s��峹���O",
#endif
	"Expire  �ʶR�峹�W��",
	"ASample �s��峹�d��",
	"Manager �W��ƪO�D",
	"Usies   �[��ݪO�\\Ū�O��",

#  ifdef HAVE_MODERATED_BOARD
	"Level   ���}/�n��/���K",
	"OPal    �]�w�i��/����W��",
#  endif
	NULL
    };
#else
    char *menu = "�� �O�D��� (B)���D (W)�i�O (G)��� (P)���� (R)���O (E)�W�� (A)�d�� (M)�ƪO (U)�O�� (Z)RSS"
#ifdef HAVE_BRD_EXPLAIN
	" (T)����"
#endif
#  ifdef HAVE_MODERATED_BOARD
	" (L)�v�� (O)�W��"
#  endif
	"�H[Q] ";
#endif

    if (!(bbstate & STAT_BOARD))
	return XO_NONE;

    utmp_mode(M_MBOARD);

#ifdef POPUP_ANSWER
    switch (pans(3, 20, "�O�D���", menu))
#else
	switch (vans(menu))
#endif
	{
	    case 'b':
		return post_brdtitle(xo);

	    case 'w':
		return post_memo_edit(xo);

	    case 'g':
		return post_gemmemo_edit(xo);

	    case 'p':
		return post_postlaw_edit(xo);

	    case 'z':
		return post_rss(xo);

#ifdef POST_PREFIX
	    case 'r':
		return post_prefix_edit(xo);
#endif
	    case 'e':
		return m_expire();

	    case 'a':
		return m_samplepost();

	    case 'm':
		return post_changeBM();

	    case 'u':
		return post_usies(xo);

#ifdef HAVE_BRD_EXPLAIN
	    case 't':
		return post_explain_edit(xo);
#endif

#ifdef HAVE_MODERATED_BOARD
	    case 'l':
		return post_brdlevel(xo);

	    case 'o':
		return XoBM(xo);
#endif
	}

    return XO_HEAD;
}
