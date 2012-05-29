/*-------------------------------------------------------*/
/* railway.c    ( NTHU CS MapleBBS Ver 3.10 )		 */
/*-------------------------------------------------------*/
/* target : �x�K�����ɨ��d��				 */
/* create : 02/05/29					 */
/* update :   /  /					 */
/* author : lp@micro.ee.nthu.edu.tw			 */
/* modify : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/*-------------------------------------------------------*/


#if 0
  1. �ݭn�w�� lynx�Bpiconv�Bgrep�A�B���ˬd�o�T�̦b http_conn()
     �̭������|�O�_���T�C
  2. �{�����M���ਮ���������A���O�ثe�S���ΡC
  3. �ثe�{���L�k�d�߲����C
#endif

#include "bbs.h"

#define mouts(x,y,s)	{ move(x, y); outs(s); }

#define FORM_railway	"\"SBeginStation=%s&SBeginStationName=&SEndStation=%s&SEndStationName=&SClass=%s&SDateTime=%s&STime=%s%s|%s%s&SearchType=0&SBeginCityCode=&SEndCityCode=&BStationIndex=&EStationIndex=\""

#define REF		"http://www.railway.gov.tw/"

#define RAILWAY_XPOS	1
#define RAILWAY_YPOS	0
#define MAX_SITE	216
#define SITE_PER_LINE	13	/* �C�C�q 13 �ӯ��x */

static char *siteid[MAX_SITE] =	/* �������x */
{
  "�x�F", "�s��", "����", "�緽", "��M", "���", "���s", "����", "���W", "�I��", "�F��", "�F��", "�ɨ�",
  "�T��", "���J", "�I��", "�j�I", "���_", "�U�a", "��L", "�n��", "�ˤf", "�ץ�", "����", "���M", "�Ӿ�",
  "�N�w", "�Ὤ", "�_�H", "����", "�s��", "�R�w", "�M��", "�M��", "�~��", "�Z��", "�n�D", "�F�D", "�ü�",
  "Ĭ�D","Ĭ�D�s","�s��", "�V�s", "ù�F", "����", "�G��", "�y��", "�|��", "�G��", "���H", "�Y��", "�~�D",
  "�t�s", "�j��", "�j��", "�۫�", "�ֶ�", "�^�d", "����", "�d��","�T�I��","�Jֻ", "���","�|�}�F","�x�x",
  "��", "�T�|", "�K��", "�C��", "�ʺ�", "����", "����", "����", "�n��", "�Q�s", "�x�_", "�U��", "�O��",
  "��L", "�s��", "�a�q", "���", "���c", "���c", "�H��", "����", "�I��", "��f", "�s��", "�˥_", "�s��",
  "���s", "�T��", "�˫n", "�ͤ�", "�j�s", "���s", "�s��","�ըF��","�s�H", "�q�]", "�b��", "��n","�j��",
  "�x����","�M��","�F��", "�s��", "�j�{", "�l��", "�y��", "�״I", "�]��", "�n��", "���r", "�T�q", "���w",
  "�Z��", "�׭�", "��l", "�ӭ�", "�x��", "�j�y", "�Q��","�s�Q��","���\\","����", "���", "�j��", "���L",
  "�ùt", "���Y", "�Ф�", "�G��", "�L��", "�ۺh", "�椻", "��n", "���t", "�j�L", "����", "�ť_", "�Ÿq",
  "���W", "�n�t", "���", "�s��", "�h��","�L����","����", "�ުL", "����", "�s��", "�ñd", "�j��", "�x�n",
  "�O�w", "���w", "�j��", "����", "���s", "���Y", "����","�s����","����", "����", "��s", "����","�E����",
  "������","�̪F","�k��", "�ﬥ", "���", "�˥�", "��{", "�r��", "�n�{", "��w", "�L��", "�ΥV", "�F��",
  "�D�d", "�[�S", "����", "�D�s", "�j��", "�j�Z", "�]��", "���[","�ӳ¨�","����", "�d��","�T�I��","�j��",
  "�Q��", "��j", "���}", "����", "�׮�", "�ˤ�", "�W��", "�a��", "�˪F", "��s","�E�g�Y","�X��", "�I�Q",
  "���W", "�G��", "���u", "�B��", "�s�u", "����", "����", "���L"
};


static char *siteno[MAX_SITE] =	/* �����N�X */
{
  "1632", "1631", "1630", "1629", "1628", "1627", "1626", "1625", "1624", "1623", "1622", "1621", "1619",
  "1617", "1616", "1614", "1613", "1612", "1611", "1610", "1609", "1608", "1607", "1606", "1605", "1604",
  "1602", "1715", "1714", "1713", "1712", "1711", "1710", "1709", "1708", "1706", "1705", "1704", "1703",
  "1827", "1826", "1825", "1824", "1823", "1822", "1821", "1820", "1819", "1818", "1817", "1816", "1815",
  "1814", "1813", "1812", "1811", "1810", "1809", "1808", "1807", "1806", "1805", "1804", "1803", "1802",
  "1001", "1029", "1002", "1003", "1030", "1004", "1005", "1031", "1006", "1007", "1008", "1009", "1011",
  "1012", "1013", "1014", "1015", "1016", "1017", "1018", "1019", "1020", "1021", "1022", "1023", "1025",
  "1026", "1027", "1028", "1102", "1104", "1105", "1106", "1107", "1108", "1109", "1110", "1111", "1112",
  "1113", "1114", "1115", "1116", "1117", "1118", "1302", "1304", "1305", "1307", "1308", "1310", "1314",
  "1315", "1317", "1318", "1323", "1319", "1322", "1320", "1324", "1321", "1120", "1202", "1240", "1203",
  "1204", "1205", "1206", "1207", "1208", "1209", "1210", "1211", "1212", "1213", "1214", "1241", "1215",
  "1217", "1218", "1219", "1220", "1221", "1222", "1223", "1224", "1225", "1226", "1227", "1239", "1228",
  "1229", "1230", "1231", "1232", "1233", "1234", "1235", "1242", "1236", "1238", "1402", "1403", "1404",
  "1405", "1406", "1407", "1408", "1409", "1410", "1411", "1412", "1413", "1414", "1415", "1416", "1417",
  "1418", "1502", "1503", "1504", "1507", "1508", "1510", "1512", "1514", "1516", "1517", "1806", "1903",
  "1904", "1905", "1906", "1907", "1908", "2203", "2204", "2211", "2205", "2206", "2207", "2208", "2209",
  "2210", "1207", "2702", "2703", "2704", "2705", "2706", "2707"
};


static int cx, cy;
static int fr_pos, to_pos, tr_pos;
static char fr_no[5], to_no[5], tr_no[4];	/* �_�� �N�X */
static char fr_tmh[3], to_tmh[3];		/* �_���� �ɨ� */
static char fr_tms[3], to_tms[3];		/* �_���� �ɨ� */
static char typer[5];				/* �︹/�D�︹ */


static void
outs_site(pos)
  int pos;
{
  char *site;

  site = siteid[pos];
  if (site[4] == '\0')
    prints(" %s ", site);
  else
    outs(site);
}


static void
outs_info(color, msg)
  int color;
  char *msg;		/* strlen(msg) == 14 */
{
  move(b_lines, 0);
  clrtoeol();
  move(b_lines, 30);	/* �m�� */
  prints("\033[%dm %s \033[m", color, msg);
}


static void
site_drawrow(x)		/* ��ø x �C */
  int x;
{
  int i;

  move(x + RAILWAY_XPOS, RAILWAY_YPOS);
  i = x * SITE_PER_LINE;
  x = (x + 1) * SITE_PER_LINE;
  if (x > MAX_SITE)
    x = MAX_SITE;
  for (; i < x; i++)
  {
    if (i == fr_pos)
      outs("\033[41m");
    else if (i == to_pos)
      outs("\033[44m");
    else if (i == tr_pos)
      outs("\033[42m");
    outs_site(i);
    if (i == fr_pos || i == to_pos || i == tr_pos)
      outs("\033[m");
  }
}


static void
site_show()
{
  int i;

  clear();
  move(0, 23);
  outs("\033[1;37;44m�� �x�K�����ɨ��d�ߨt�� ��\033[m");

  for (i = 0; i < MAX_SITE; i++)
  {
    if (i % SITE_PER_LINE == 0)
      move(i / SITE_PER_LINE + RAILWAY_XPOS, RAILWAY_YPOS);
    outs_site(i);
  }

  move(b_lines - 4, 0);
  outs("�ҵ{�����G         ��F�����G");
  move(b_lines - 3, 0);
  outs("�}���ɶ��G00:00 �� 23:59");
  move(b_lines - 2, 0);
  outs("�d�ߨ��ءG1)�︹�֨� 2)�D�︹�� 3)��������");

  cx = cy = 0;
  fr_pos = to_pos = tr_pos = -1;

}


static int		/* 1:���\ */
site_choose(flag)
  int flag;		/* 1:��_��  2:�勤��  4: ���௸ */
{
  int pos;

  /* �o�X��S�����x */
  if (cx == MAX_SITE / SITE_PER_LINE && cy >= MAX_SITE % SITE_PER_LINE)
    return 0;

  pos = cx * SITE_PER_LINE + cy;

  if (flag == 1)
  {
    fr_pos = pos;
    strcpy(fr_no, siteno[fr_pos]);
    move(b_lines - 4, 10);
    prints("%-6s", siteid[fr_pos]);
  }
  else if (flag == 4)
  {
    if (fr_pos == pos)
      return 0;
    to_pos = pos;
    strcpy(to_no, siteno[to_pos]);
    move(b_lines - 4, 29);
    outs(siteid[to_pos]);
  }
  else /* if (flag == 2) */
  {
    if (fr_pos == pos || to_pos == pos)
      return 0;
    tr_pos = pos;
    strcpy(tr_no, siteno[tr_pos]);
    move(b_lines - 4, 48);
    outs(siteid[tr_pos]);
  }

  site_drawrow(cx);
  return 1;	/* ��U�@�� */
}


static int		/* 1: �w�M�w�_���� */
site_menu(flag)		/* �ثe�b�� 1:�_ 4:�� 2:�� �� */
  int flag;
{
  for (;;)
  {
    /* itoc.031209: �Y�J��T�Ӧr�����W�A�S�������ήɡA����|�h���@���A�Ȯɤ��ޤF :p */
    move(cx + RAILWAY_XPOS, cy * 6 + RAILWAY_YPOS + 2);
    switch (vkey())
    {
    case KEY_RIGHT:
      cy++;
      if (cy > 12)
        cy = 0;
      break;

    case KEY_LEFT:
      cy--;
      if (cy < 0)
        cy = 12;
      break;

    case KEY_DOWN:
      cx++;
      if (cx > 16)
        cx = 0;
      break;

    case KEY_UP:
      cx--;
      if (cx < 0)
        cx = 16;
      break;

    case KEY_ENTER:
    case ' ':
      if (site_choose(flag))
        return 1;
      break;

    case 'q':
      return 0;
    }
  }
}


static void
time_query()
{
  int i;
  outs_info(46, "�п�J�d�߮ɶ�");
  if (!vget(b_lines - 3, 0, "�}���ɶ��G", fr_tmh, 3, DOECHO))
  {
    strcpy(fr_tmh, "00");
    move(b_lines - 3, 10);
    outs("00");
  }
  i = atoi(fr_tmh);
  if (i >= 0 && i <= 9)
    sprintf(fr_tmh, "0%d", i);

  if (!vget(b_lines - 3, 12, ":", fr_tms, 3, DOECHO))
  {
    strcpy(fr_tms, "00");
    move(b_lines - 3, 13);
    outs("00 ");
  }
  i = atoi(fr_tms);
  if (i >= 0 && i <= 9)
    sprintf(fr_tms, "0%d", i);

  if (!vget(b_lines - 3, 16, "�� ", to_tmh, 3, DOECHO))
  {
    strcpy(to_tmh, "23");
    move(b_lines - 3, 19);
    outs("23");
  }
  i = atoi(to_tmh);
  if (i >= 0 && i <= 9)
    sprintf(to_tmh, "0%d", i);

  if (!vget(b_lines - 3, 21, ":", to_tms, 3, DOECHO))
  {
    strcpy(to_tms, "59");
    move(b_lines - 3, 22);
    outs("59");
  }
  i = atoi(to_tms);
  if (i >= 0 && i <= 9)
    sprintf(to_tms, "0%d", i);
}


static void
rail_query()
{
  char ans[3], *msg;

  outs_info(45, "�п�ܬd�ߨ���");

  switch (vget(b_lines - 2, 0, "�d�ߨ��ءG1)�︹�֨� 2)�D�︹�� 3)�������ءG[3] ", ans, 3, DOECHO))
  {
  case '1':
    strcpy(typer, "0");
    msg = "1)�︹�֨�";
    break;

  case '2':
    strcpy(typer, "1");
    msg = "2)�D�︹��";
    break;

  default:
    strcpy(typer, "2");
    msg = "3)��������";
    break;
  }

  move(b_lines - 2, 10);
  outs(msg);
  clrtoeol();

  refresh();	/* �A�� vget() �|�X�{�e��������X�A�n refresh */
}


static char
Tcolor(str)	/* �̨����ӨM�w�C�� */
  char *str;
{
  if (!strncmp(str, "�۱j", 4))
    return '1';
  if (!strncmp(str, "����", 4))
    return '6';
  if (!strncmp(str, "�_��", 4))
    return '4';
  return '0';
}


static void
html_parser(src, dst)
  char *src, *dst;
{
  FILE *fpr, *fpw;
  char data[256];
  char *result;
  int line_num;
  int tcount;	/* �`�@���X�Z�� */
  struct stat st;

  if ((!stat(src, &st) && !st.st_size) || !(fpr = fopen(src, "r")))
    return;

  fpw = fopen(dst, "w");

  line_num = 0;
  while (fgets(data, sizeof(data), fpr))
  {
    if (++line_num >= 5)
    {
      fprintf(fpw, "%s", data + 16);
      break;
    }
  }

  fprintf(fpw, "\n"
    "�z�w�w�w�s�w�w�s�w�s�w�w�w�w�w�w�w�s�w�w�w�w�w�s�w�w�w�w�w�s�w�w�s�w�w�w�w�w�{\n"
    "�x ���� �x�����x�s�x�l�o��->���I���x�n������  �x�U������  �x�����x��p�ɶ�  �x\n"
    "�x ���� �x�s���x���x              �x�}���ɶ�  �x��F�ɶ�  �x�����x          �x");

  while (fgets(data, sizeof(data), fpr))
  {
    if (++line_num >= 28)
      break;
  }

  tcount = 0;
  while (1)
  {
    if (!fgets(data, sizeof(data), fpr) || !(result = strtok(data, " ")))
      break;

    if (!strcmp(result, "\n"))	/* �S��ƤF */
      break;

    fprintf(fpw, "\n�u�w�w�w�q�w�w�q�w�q�w�w�w�w�w�w�w�q�w�w�w�w�w�q�w�w�w�w�w�q�w�w�q�w�w�w�w�w�t");

    /* �����A6 bytes */
    fprintf(fpw, "\n�x\033[1;4%c;37m%6.6s\033[m", Tcolor(result), result);	/* �N���ȷ|�W�L 6 bytes */

    /* �����N�X�A4 bytes */
    if (!(result = strtok(NULL, "\n")))
      break;
    while (*result == ' ')	/* �ѨM�d�Y�ǯ����ɭԡA���׷|���@�����D */
      result++;

    fprintf(fpw, "�x%4.4s", result);		/* �N���ȷ|�W�L 4 bytes */

    /* �s�u�ή��u�ΪŪ��A2 bytes */
    if (!fgets(data, sizeof(data), fpr) || !(result = strtok(data, "\n")))
      break;
    fprintf(fpw, "�x%2s", result + 38);

    /* �ҵ{���B��F���A6 bytes */
    if (!fgets(data, sizeof(data), fpr) || !(result = strtok(data, "\n")))
      break;
    fprintf(fpw, "�x%14s", result + 31);

    /* �}���ɶ� */
    if (!fgets(data, sizeof(data), fpr) || !(result = strtok(data, "\n")))
      break;
    fprintf(fpw, "�x%8s  ", result + 32);

    /* ��F�ɶ� */
    if (!fgets(data, sizeof(data), fpr) || !(result = strtok(data, "\n")))
      break;
    fprintf(fpw, "�x%8s  ", result + 32);

#if 0
    /* �������� 3 bytes */
    if (!fgets(data, sizeof(data), fpr) || !(result = strtok(data, "\n")))
      break;
    fprintf(fpw, "�x%4s", result + 34);
#else
    fprintf(fpw, "�x%4s", "");
#endif

    /* ��p�ɶ� */
    if (!fgets(data, sizeof(data), fpr) || !(result = strtok(data, "\n")) || !(result = strtok( result, " ")))
      break;
    fprintf(fpw, "�x%10s�x", result);

    /* ���L�Ƶ� */
    if (!fgets(data, sizeof(data), fpr))
      break;

    tcount++;
  }		/* end of while */


  fprintf(fpw,  "\n�|�w�w�w�r�w�w�r�w�r�w�w�w�w�w�w�w�r�w�w�w�w�w�r�w�w�w�w�w�r�w�w�r�w�w�w�w�w�}\n\n\n\n");

  if (!tcount)
    fprintf(fpw, "\n\n��p�I�S���z���f�����Z��");

  fclose(fpr);
  fclose(fpw);
}


static int
http_conn(s)
  char *s;
{
  char src[30], dst[30];
  char cmd[768];

  mouts(b_lines - 1, 0, "�s�����A�����A�еy��...");
    refresh();

  sprintf(src, "tmp/%s.rail", cuser.userid);
  sprintf(cmd, "/usr/local/bin/lynx -display_charset=utf-8 -nolist -dump  "
    "http://new.twtraffic.com.tw/twrail/nonscript_search_result.aspx?%s | "
    "/usr/local/bin/piconv -f utf8 -t big5 | /usr/bin/grep -v '�ϥ�' > %s",
    s, src);
  system(cmd);

  sprintf(dst, "tmp/%s.way", cuser.userid);
  html_parser(src, dst);

  /* show message that return from the web server */


  if (more(dst, (char *) -1) >= 0)
  {
    if (vans("�z�n�N�d�ߵ��G�H�^�H�c�ܡH[N] ") == 'y')
      mail_self(dst, cuser.userid, "[�� �� ��] �����ɨ�d�ߵ��G", MAIL_READ);
  }
  unlink(dst);
  unlink(src);

  return 0;
}



static void
railway(delay)
  int delay;
{
  char atrn[512];
  char day[20];
  struct tm *ptime;
  time_t now;

  time(&now);
  now += 86400 * delay;
  ptime = localtime(&now);
  sprintf(day, "%02d/%d/%d",
    (ptime->tm_year - 11 ) % 100 , ptime->tm_mon + 1, ptime->tm_mday);

  sprintf(atrn, FORM_railway, fr_no, to_no, typer, day, fr_tmh, fr_tms, to_tmh, to_tms);
  http_conn(atrn);
}


int
main_railway()
{
  int ch;

  site_show();

  outs_info(41, "�п�ܱҵ{����");
  if (!site_menu(1))
    return 0;

  outs_info(44, "�п�ܨ�F����");
  if (!site_menu(4))
    return 0;

  time_query();     /* �M�w�ҵ{/��F�ɶ� */

  rail_query();	/* �M�w�︹/�D�︹ */

  ch = vans("�d�� 0)���� 1)���� 2)��� 3456789)�ѫ᪺�ɨ�� Q)���}�H[0] ");
  if (ch == 'q')
  {
    vmsg("hmm.....���Q�d�o...^o^");
    return 0;
  }
  if (ch < '0' || ch > '9')
    ch = 0;
  else
    ch -= '0';

  railway(ch);

  return 0;
}

