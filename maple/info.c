/*-------------------------------------------------------*/
/* info.c                                                */
/*-------------------------------------------------------*/
/* target : show informations.                           */
/* create : 2008/02/25                                   */
/*-------------------------------------------------------*/

/* hrs.080225: ��bmenu�����έp��T�W�ߥX�� */
/* hrs.080225.todo: �U���@����v_xxx()�]�\�i�H�ΦX�_�� */

#include "bbs.h"

#define INFOLEVEL_ADMIN	    (1)  /* information level Admin */
#define INFOLEVEL_BM	    (6)	 /* information level BM    */

extern BCACHE *bshm;
extern UCACHE *ushm;
extern char xo_pool[];
extern char brd_bits[];

extern char * compile_time;
extern int money_mode;

/* login views */
int v_topmoney(void);
int v_topsong(void);
int v_birthday(void);
int v_counter(void);
int v_topscore(void);
int v_toquery(void);
int v_bequery(void);
int v_topstaytime(void);

/* fileheader information */
static int info_showfileinfo(XO *xo, int y);
int p_fileinfo(XO *xo);
int p_fileinfobt(XO *xo);

/* boardheader information */
int cmpbrdbno(BRD *brd);
static void info_showattr(int ypos, char symbol, int level);
static int info_editattr(BRD *brd, usint attr, int level);
int p_brdinfo(void);

/* site's information */
int x_siteinfo(void);

/*-------------------------------------------------------*/
/* login views                                           */
/*-------------------------------------------------------*/

int v_topmoney(void)
{
      more("gem/@/@-topmoney", NULL);
      return 0;
}


int v_topsong(void)
{
      more("gem/@/@-topsong", NULL);
      return 0;
}


int v_birthday(void)
{
      more("gem/@/@-birthday", NULL);
      return 0;
}


int v_counter(void)
{
      more("gem/@/@-counter", FOOTER_MORE);
      return 0;
}


int v_topscore(void)
{
      more("gem/@/@-topscore", FOOTER_MORE);
      return 0;
}


int v_toquery(void)
{
      more("gem/@/@-toquery", NULL);
      return 0;
}


int v_bequery(void)
{
      more("gem/@/@-bequery", NULL);
      return 0;
}


int v_topstaytime(void)
{
      more("gem/@/@-topstaytime", NULL);
      return 0;
}


 
/*-------------------------------------------------------*/
/* show one fileheader's information                     */
/*-------------------------------------------------------*/

static int info_showfileinfo(XO *xo, int y)
{
    HDR *fhdr;
    int ystart, barlen; 
    char buf[256]; 

    fhdr = (HDR *) xo_pool + xo->pos - xo->top;

    if (y > 0)
    {
	grayout(0, y - 1, GRAYOUT_DARK);
	grayout(y, y, GRAYOUT_BOLD);
	grayout(y + 1, b_lines - 1, GRAYOUT_DARK);

	if (y > b_lines - 6)
	    ystart = y - 4;
	else
	    ystart = y + 1;

    }
    else  // y < 0
    {
	grayout(0, b_lines - 1, GRAYOUT_DARK);
	ystart = b_lines - 4;
    }

    barlen = (b_cols - 4)/2;
    move (ystart++, 0); clrtoeol();
    outs("�z"); outs_n("�w", barlen); outs("�{");

    move (ystart, 0); clrtoeol();
    snprintf(buf, sizeof(buf), "�x �峹�N�X(AID): #%s (%s) [" BBSNAME "] %s", fhdr->xname, currboard, (chkrestrict(fhdr) ? fhdr->title : ""));
    if (strlen(buf) > barlen * 2 + 2)
    {
	if (is_zhc_low(buf, barlen * 2 + 2))
	    buf[barlen * 2 + 1] = '\0';
	else
	    buf[barlen * 2 + 2] = '\0';
    }
    prints("%-*s", barlen * 2 + 2, buf);
    move (ystart++, barlen * 2 + 2); outs("�x");

    move (ystart, 0); clrtoeol();
    if (POST_SPECIAL(fhdr->xmode))
	prints("�x �S��峹�S������O��");
    else
	prints("�x ���峹���� %d ��", fhdr->money);
    move (ystart++, barlen * 2 + 2); outs("�x");

    move (ystart++, 0); clrtoeol();
    outs("�|"); outs_n("�w", barlen); outs("�}");

    if (vmsg(NULL) == 'Q')
    {
	vmsg(!money_mode ? "�}�Ҥ峹������ܼҦ�" : "��_�峹�����ܼҦ�");
	money_mode = money_mode ? 0 : 1;
    }
    return XO_HEAD;
}


int p_fileinfo(XO *xo)
{
    int y;

    /* hrs.090929: �e���O��Ф��n�]��O�� */
    gety(&y);
    return info_showfileinfo(xo, y);
}


int p_fileinfobt(XO *xo)  /* show one fileheader's information at bottom */
{
    return info_showfileinfo(xo, -1);
}

/*-------------------------------------------------------*/
/* show one board's information                          */
/*-------------------------------------------------------*/

int currbno = -1;
int cmpbrdbno(BRD *brd)
{
    int bno = brd_bno(brd->brdname);
    
    if (bno < 0)
	return 0;

    return (bno == currbno);
}

int p_brdinfo(void)
{
    BRD *brd, newbrd;
    BPAL *pbad;

    int limit, ypos = b_lines - 20, ch, exit = 0, touch = 0;

    if (ibno < 0)
	ibno = currbno;

    brd = bshm->bcache + ibno;
    pbad = bshm->pbad + ibno;
    memcpy(&newbrd, brd, sizeof(newbrd));

    if (!(brd_bits[ibno] & BRD_L_BIT))
	return XO_NONE;

    grayout(0, ypos - 1, GRAYOUT_DARK);

    do
    {
	ypos = b_lines - 21;

	move(ypos++, 0);
	clrtobot();

	/* seperating Bar */
	outs(ANSI_COLOR(1;34));
	outs_n("�b", b_cols / 2 + 1);
	outs(ANSI_RESET);

	move(ypos++, 0);
	prints(ANSI_COLOR(1;33;44) "�� " ANSI_COLOR(37) "%-13s �ݪO��T %*s" ANSI_RESET, brd->brdname, b_cols - 25, "");

	ypos++; // reserve for a blank line

	move(ypos++, 0);
	/* Board Name */
	prints(INFO_MARK ANSI_COLOR(37)"�ݪO�W�١G %s",  brd->brdname);
	/* show board property */
	if (brd->bmode == BMODE_HIDE)
	    outs (ANSI_COLOR(1;31) " (���êO)\n");
	else if (brd->bmode == BMODE_PAL)
	    outs (ANSI_COLOR(1;31) " (�n�ͪO)\n");
	else if (brd->bmode == BMODE_SEAL)
	    outs (ANSI_COLOR(1;35) " (�ʦL�O)\n");

	/* Board class/title */
	move(ypos++, 0);
	prints(INFO_MARK ANSI_COLOR(37)"����ԭz�G "ANSI_COLOR(3%d)"[%s]" ANSI_COLOR(37) " %s\n"
		, brd->class[3] & 7, brd->class, brd->title);
	info_showattr(ypos - 1, 't', INFOLEVEL_BM);

	/* BM list */
	move(ypos++, 0);
	prints(INFO_MARK ANSI_COLOR(37)"�O�D�W��G %s\n", 
		!brd->BM[0] ? brd->battr & BRD_SERVICE ? str_operators : "�x�D��" : brd->BM);
	info_showattr(ypos - 1, 'm', INFOLEVEL_BM);

	/* Creating time */
	move(ypos++, 0);
	prints(INFO_MARK ANSI_COLOR(37)"�}�O�ɶ��G %s\n", Btime(&brd->bstamp));

	/* Post/Cosign Limit */
	{
	    limit = 0;

	    move(ypos++, 0);
	    prints(INFO_MARK ANSI_COLOR(37)"%s����G ", brd->battr & BRD_COSIGN ? "�s�p" : "�o��");

	    if (brd->limit_posts)
	    {
		prints(ANSI_COLOR(3%c) "�o��峹 %d �g�H�W", cuser.numposts >= brd->limit_posts ? '0' : '1'
			, brd->limit_posts);
		limit++;
	    }

	    if (brd->limit_logins)
	    {
		prints("%s" ANSI_COLOR(3%c) "�W�� %d ���H�W" ANSI_COLOR(30), limit ? "/" : ""
			, cuser.numlogins >= brd->limit_logins ? '0' : '1', brd->limit_logins);
		limit++;
	    }

	    if (brd->limit_regtime)
	    {
		prints("%s" ANSI_COLOR(3%c) "���U %d �Ӥ�H�W", limit ? "/" : ""
			, (time(NULL) - cuser.firstlogin) / (86400 * 30) >= brd->limit_regtime ? '0' : '1'
			, brd->limit_regtime);
		limit++;
	    }

	    if (!limit)
	    {
		if (brd->postlevel > PERM_POST)
		    outs(ANSI_COLOR(30) "����S���v��");
		else
		    outs(ANSI_COLOR(30) "�L�S��W�w");
	    }

	    if (is_bgood(pbad))
		outs(ANSI_COLOR(1;31) " (�ϥΪ̤���)" ANSI_RESET);
	    else if (!CheckPostPerm(brd))
	    {
		outs(ANSI_COLOR(1;31) " (�z�S���o���v��)" ANSI_RESET);
	    }

	    info_showattr(ypos - 1, 's', INFOLEVEL_ADMIN);
	} // Post/Cosign Limit

	/* Expire limit */
	move(ypos++, 0);
	prints(INFO_MARK ANSI_COLOR(37) "�峹�����G �W�� %d �g/�U�� %d �g/�O�d�ɶ� %d ��\n"
		,brd->ex_maxposts, brd->ex_minposts, brd->ex_maxtime);
	info_showattr(ypos - 1, 'e', INFOLEVEL_ADMIN);

	/* Other attributes */
	move(ypos++, 0);
	prints(INFO_MARK ANSI_COLOR(37) "��H�ݩʡG " ANSI_COLOR(33) "%s" ANSI_RESET "��H�O�A�o��w�]" ANSI_COLOR(1;36) "%s" ANSI_RESET, brd->battr & BRD_NOTRAN ? "���O" : "�O", brd->battr & BRD_LOCAL ? "�s����" : "��X");
	info_showattr(ypos - 1, 'f', INFOLEVEL_BM);

	/* statstic */
	move(ypos++, 0);
	prints(INFO_MARK ANSI_COLOR(37) "�έp�ݩʡG " ANSI_COLOR(33) "%s" ANSI_RESET "�C�J�������D�έp����"
		, brd->battr & BRD_NOSTAT ? "���|" : "�|");
	info_showattr(ypos - 1, 'r', INFOLEVEL_BM);

	/* vote result */
	move(ypos++, 14);
	prints(ANSI_COLOR(1;33) "%s" ANSI_RESET "���}�벼���G�� " BN_RECORD " �O"
		, brd->battr & BRD_NOVOTE ? "���|" : "�|");
	info_showattr(ypos - 1, 'v', INFOLEVEL_BM);

	/* count posts */
	move(ypos++, 14);
	prints("�o��峹" ANSI_COLOR(1;33) "%s" ANSI_RESET "�O���g��"
		, brd->battr & BRD_NOCOUNT ? "���|" : "�|");
	info_showattr(ypos - 1, 'c', INFOLEVEL_ADMIN);

#ifdef HAVE_ANONYMOUS
	/* anonymous board */
	move(ypos++, 0);
	prints(INFO_MARK ANSI_COLOR(37) "�ΦW�ݩʡG " ANSI_COLOR(1;33) "%s" ANSI_RESET "�ΦW�O"
		, brd->battr & BRD_ANONYMOUS ? "�O" : "���O");
	info_showattr(ypos - 1, 'a', INFOLEVEL_ADMIN);
#endif

	/* restriction to the age */
	move(ypos++, 0);
	prints(INFO_MARK ANSI_COLOR(37) "�~�֭���G " ANSI_COLOR(1;31) "%s" ANSI_RESET "�Q�K���H�U�ϥΪ̶i�J"
		, brd->battr & BRD_18X ? "�T��" : "�}��");
	info_showattr(ypos - 1, '8', INFOLEVEL_BM);

#ifdef HAVE_SCORE
	/* score */
	move(ypos++, 0);
	prints(INFO_MARK ANSI_COLOR(37) "����]�w�G " ANSI_COLOR(1;33) "%s" ANSI_RESET "����"
		, brd->battr & BRD_NOSCORE ? "���i�H" : "�i�H");
	info_showattr(ypos - 1, '%', INFOLEVEL_BM);

	/* boo */
	move(ypos++, 14);
	prints(ANSI_COLOR(1;33) "%s" ANSI_RESET "����"
		, brd->battr & BRD_NOSCORE ? "���i�H" : brd->battr & BRD_NOBOO ? "���i�H" : "�i�H");
	info_showattr(ypos - 1, 'b', INFOLEVEL_BM);

	/* score alignment */
	move(ypos++, 14);
	prints(ANSI_COLOR(1;33) "%s" ANSI_RESET "�۰ʹ��"
		, brd->battr & BRD_SCOREALIGN?"�|" : "���|");
	info_showattr(ypos - 1, 'i', INFOLEVEL_BM);
#endif

	/* reply */
	move(ypos++, 0);
	prints(INFO_MARK ANSI_COLOR(37) "�^�г]�w�G " ANSI_COLOR(1;33) "%s" ANSI_RESET "�^�Ф峹"
		, brd->battr & BRD_NOREPLY ? "���i�H" : "�i�H");
	info_showattr(ypos - 1, 'y', INFOLEVEL_BM);

#ifdef HAVE_ALLPOST
	/* ALLPOST setting */
	move(ypos++, 0);
	prints(INFO_MARK ANSI_COLOR(37) "���p�]�w�G " ANSI_RESET "���}�O���A�峹" ANSI_COLOR(1;31) "%s" ANSI_RESET "�i " BN_ALLPOST " �O"
		, brd->battr & BRD_HIDEALLPOST ? "���|" : "�|");
	info_showattr(ypos - 1, 'z', (brd->battr & BRD_PERSONAL ? INFOLEVEL_BM : INFOLEVEL_ADMIN));
#endif

	if (!(brd_bits[ibno] & BRD_M_BIT) && !HAS_PERM(PERM_ALLBOARD))
	    break;
	else
	    ch = vans("�Ы�����������������ק�]�w�A��L�����}: ");

	switch (ch)
	{
	    case 't':
		brd_title(ibno);
		touch = 1;
		break;

	    case 'm':
		brd_changeBM(brd);
		rec_put(FN_BRD, brd, sizeof(BRD), currbno, NULL); // TODO: check is needed.
		sprintf(currBM, "�O�D�G%s",  *(brd->BM) <= ' '  ? brd->battr & BRD_SERVICE ? 
			str_operators : "�x�D��" : brd->BM);
		touch = 1;
		break;

	    case 's':

		if (HAS_PERM(PERM_ALLBOARD))
		{
		    brd_setlimit(brd, b_lines);
		    touch = 1;
		}
		else
		    exit = 1;
		break;

	    case 'e': 

		if (HAS_PERM(PERM_ALLBOARD))
		{
		    brd_setexpire(brd, b_lines);
		    touch = 1;
		}
		else
		    exit = 1;
		break;

	    case 'f':
		if (info_editattr(brd, BRD_LOCAL, INFOLEVEL_BM))
		    touch = 1;
		break;

	    case 'r':
		if (info_editattr(brd, BRD_NOSTAT, INFOLEVEL_BM))
		    touch = 1;
		break;

	    case 'v':
		if (info_editattr(brd, BRD_NOVOTE, INFOLEVEL_BM))
		    touch = 1;
		break;

	    case 'c':
		if (info_editattr(brd, BRD_NOCOUNT, INFOLEVEL_ADMIN))
		    touch = 1;
		else
		    exit = 1;
		break;

	    case 'a':
		if (info_editattr(brd, BRD_ANONYMOUS, INFOLEVEL_ADMIN))
		    touch = 1;
		else
		    exit = 1;
		break;

	    case '8':
		if (info_editattr(brd, BRD_18X, INFOLEVEL_BM))
		    touch = 1;
		break;

	    case '%':
		if (info_editattr(brd, BRD_NOSCORE, INFOLEVEL_BM))
		    touch = 1;
		break;

	    case 'b':
		if (info_editattr(brd, BRD_NOBOO, INFOLEVEL_BM))
		    touch = 1;
		break;

	    case 'i':
		if (info_editattr(brd, BRD_SCOREALIGN, INFOLEVEL_BM))
		    touch = 1;
		break;

	    case 'y':
		if (info_editattr(brd, BRD_NOREPLY, INFOLEVEL_BM))
		    touch = 1;
		break;

	    case 'z':
		if (info_editattr(brd, BRD_HIDEALLPOST, (brd->battr & BRD_PERSONAL ? INFOLEVEL_BM : INFOLEVEL_ADMIN)))
		    touch = 1;
		else
		    exit = 1;
		break;

	    default:
		exit = 1;
		break;
	}
    } while (exit != 1);

    if (touch)
	rec_put(FN_BRD, brd, sizeof(BRD), ibno, cmpbrdbno);

    vmsg(NULL);
    ibno = -1; /* hrs: reload for next time */
    return XO_HEAD;
}


static void info_showattr(int ypos, char symbol, int level)
{
    if (HAS_PERM(PERM_ALLBOARD) || (level == INFOLEVEL_BM && brd_bits[ibno] & BRD_M_BIT))
	{
	    move (ypos, 72);
	    prints (ANSI_COLOR(0;3%d) "(%c)" ANSI_RESET, level, symbol);
	}
}

static int info_editattr(BRD* brd, usint attr, int level)
{
    int ibno = brd_bno(brd->brdname);

    if (ibno < 0)
	return 0;

    if (level == INFOLEVEL_BM && !brd_bits[ibno])
	return 0;

    if (level == INFOLEVEL_ADMIN && !HAS_PERM(PERM_ALLBOARD))
	return 0;

    brd->battr ^= attr;
    return 1;
}

/*-------------------------------------------------------*/
/* show the BBS site's information                       */
/*-------------------------------------------------------*/


int x_siteinfo(void)
{
    double load[3];
    FILE *fp;
    char buf[STRLEN], *ptr;

    getloadavg(load, 3);

    clear();
    vs_head("�t�θ�T", NULL);

    move(2, 0);
    prints(INFO_MARK ANSI_COLOR(37) "���W�G %s - %s (%s)\n"
	    , BBSNAME, MYHOSTNAME, MYIPADDR);
    prints(INFO_MARK ANSI_COLOR(37) "�{�������G %s\n", compile_time);
    prints(INFO_MARK ANSI_COLOR(37) "���W�H�ơG %d / %d  ���W�ݪO�G %d / %d\n"
	    , ushm->count, MAXACTIVE, bshm->number, MAXACTIVE);
    prints(INFO_MARK ANSI_COLOR(37) "�t�έt���G %.2f %.2f %.2f "ANSI_COLOR(37;44)"[%s]\n"
	    , load[0], load[1], load[2], load[0] > 7 ? "�L��" : load[0] > 3 ? "����" : "���`");
    prints(ANSI_RESET);
    prints("\n\n");

// ---------------------------------------------------------------------------------

    if (HAS_PERM(PERM_SYSOP))
    {
	/* dirty hack */
	if (system("uptime > " BBSHOME "/tmp/uptime") == 0 &&
	    (fp = fopen (BBSHOME "/tmp/uptime", "r")))
	{
	    fgets (buf, sizeof(buf) - 1, fp);
	    ptr = strtok (buf, ",");
	    if (ptr)
		prints (INFO_MARK ANSI_COLOR(37) "�W�u�ɶ��G%s\n", ptr);
	    fclose (fp);
	}
	prints(INFO_MARK ANSI_COLOR(37) "���c�j�p�G BRD: %d / ACCT:%d / HDR:%d\n",
		 sizeof(BRD), sizeof(ACCT), sizeof(HDR));

    }

    prints(ANSI_RESET);
    vmsg(NULL);
    return 0;
}
