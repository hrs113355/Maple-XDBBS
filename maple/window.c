/*-------------------------------------------------------*/
/* window.c	( NTHU CS MapleBBS Ver 3.10 )		 */
/*-------------------------------------------------------*/
/* target : popup window menu				 */
/* create : 03/02/12					 */
/* update : 03/07/23					 */
/* author : verit.bbs@bbs.yzu.edu.tw			 */
/* modify : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/*-------------------------------------------------------*/


#include "bbs.h"

/* ----------------------------------------------------- */
/* �e��ø�s						 */
/* ----------------------------------------------------- */


static void
draw_line(x, y, msg)	/* �b (x, y) ����m��J msg�A���k���n�L�X��Ӫ��m���r */
  int x, y;
  uschar *msg;
{
  /* hrs.090928: �� terminal �h�B�z */
  move(x, y);
  outstr(msg);
  return;
}


#ifdef HAVE_POPUPMENU

static screen_backup_t old_screen;

#ifdef POPUP_ANSWER

/* ----------------------------------------------------- */
/* �ﶵø�s						 */
/* ----------------------------------------------------- */


static int
draw_item(x, y, desc, hotkey, mode)
  int x, y;
  char *desc;
  char hotkey;
  int mode;		/* 0:�M������  1:�e�W���� */
{
  char buf[128];

  sprintf(buf, " �x%s%c %c%c%c%-25s  \033[m�x ",
    mode ? COLOR4 : "\033[30;47m", mode ? '>' : ' ',
    (hotkey == *desc) ? '[' : '(', *desc,
    (hotkey == *desc) ? ']' : ')', desc + 1);

  draw_line(x, y, buf);
}


static int	/* �^���`�@���X�ӿﶵ */
draw_menu(x, y, title, desc, hotkey, cur)
  int x, y;
  char *title;
  char *desc[];
  char hotkey;
  int *cur;	/* �^�ǹw�]�ȩҦb��m */
{
  int i, meet;
  char buf[128];

  draw_line(x++, y, " �~�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�� ");

  sprintf(buf, " �x" COLOR4 "  %-28s  \033[m�x ", title);
  draw_line(x++, y, buf);

  draw_line(x++, y, " �u�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�t ");

  for (i = 1; desc[i]; i++)
  {
    meet = (desc[i][0] == hotkey);
    draw_item(x++, y, desc[i], hotkey, meet);
    if (meet)
      *cur = i;
  }

  draw_line(x, y, " ���w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�� ");

  /* �קK�b�������k����ΤU�A������|�����G�h��檺���D */
  move(b_lines, 0);

  return i - 1;
}


/* ----------------------------------------------------- */
/* ��ﶵ						 */
/* ----------------------------------------------------- */


static int			/* -1:�䤣�� >=0:�ĴX�ӿﶵ */
find_cur(ch, max, desc)		/* �� ch �o�ӫ���O�ĴX�ӿﶵ */
  int ch, max;
  char *desc[];
{
  int i, cc;

  if (ch >= 'A' && ch <= 'Z')
    ch |= 0x20;		/* ���p�g */

  for (i = 1; i <= max; i++)
  {
    cc = desc[i][0];
    if (cc >= 'A' && cc <= 'Z')
      cc |= 0x20;	/* ���p�g */

    if (ch == cc)
      return i;
  }

  return -1;
}


/*------------------------------------------------------ */
/* �߰ݿﶵ�A�i�ΨӨ��N vans()				 */
/*------------------------------------------------------ */
/* x, y  �O�ۥX�������W���� (x, y) ��m			 */
/* title �O���������D					 */
/* desc  �O�ﶵ���ԭz�G					 */
/*       �Ĥ@�Ӧr�ꥲ������� char			 */
/*         �Ĥ@�Ӧr���N��@�}�l��а�����m		 */
/*         �ĤG�Ӧr���N����U KEY_LEFT ���w�]�^�ǭ�	 */
/*       �������r��O�C�ӿﶵ���ԭz (���r��������)	 */
/*       �̫�@�Ӧr�ꥲ���� NULL			 */
/*------------------------------------------------------ */


int		/* �Ǧ^�p�g�r���μƦr */
pans(x, y, title, desc)
  int x, y;
  char *title;
  char *desc[];
{
  int cur, old_cur, max, ch;
  char hotkey;

  scr_dump(&old_screen);
  grayout(0, b_lines, GRAYOUT_DARK);

  hotkey = desc[0][0];

  /* �e�X��ӿ�� */
  max = draw_menu(x, y, title, desc, hotkey, &cur);
  x += 2;

  /* �@�i�J�A��а��b�w�]�� */
  old_cur = cur;

  while (1)
  {
    switch (ch = vkey())
    {
    case KEY_LEFT:
    case KEY_RIGHT:
    case '\n':
      scr_restore(&old_screen); 
      ch = (ch == KEY_LEFT) ? desc[0][1] : desc[cur][0];
      if (ch >= 'A' && ch <= 'Z')
	ch |= 0x20;		/* �^�Ǥp�g */
      return ch;

    case KEY_UP:
      cur = (cur == 1) ? max : cur - 1;
      break;

    case KEY_DOWN:
      cur = (cur == max) ? 1 : cur + 1;
      break;

    case KEY_HOME:
      cur = 1;
      break;

    case KEY_END:
      cur = max;
      break;

    default:		/* �h��ҫ���O���@�ӿﶵ */
      if ((ch = find_cur(ch, max, desc)) > 0)
	cur = ch;
      break;
    }

    if (old_cur != cur)		/* ����ܰʦ�m�~�ݭn��ø */
    {
      draw_item(x + old_cur, y, desc[old_cur], hotkey, 0);
      draw_item(x + cur, y, desc[cur], hotkey, 1);
      old_cur = cur;
      /* �קK�b�������k����ΤU�A������|�����G�h��檺���D */
      move(b_lines, 0);
    }
  }
}
#endif	/* POPUP_ANSWER */

#endif	/* HAVE_POPUPMENU */


/*------------------------------------------------------ */
/* �ۥX�������T���A�P vmsg() �æs			 */
/*------------------------------------------------------ */


int
pmsg(msg)
  char *msg;		/* ���i�� NULL */
{
  int len, x, y, i;
  char buf[80];

  if (!msg)
      return vmsg(NULL);

  scr_dump(&old_screen);
  grayout(0, b_lines, GRAYOUT_DARK);
  len = strlen(msg);
  if (len < 16)		/* �� msg title �䤤�����̬� len */
    len = 16;
  if (len % 2)		/* �ܦ����� */
    len++;
  x = (b_lines - 4) >> 1;	/* �m�� */
  y = (b_cols - 8 - len) >> 1;

  strcpy(buf, "�~");
  for (i = -4; i < len; i += 2)
    strcat(buf, "�w");
  strcat(buf, "��");
  draw_line(x++, y, buf);

  sprintf(buf, "�x" COLOR4 "  %-*s  \033[m�x", len, "�Ы����N���~��..");
  draw_line(x++, y, buf);

  strcpy(buf, "�u");
  for (i = -4; i < len; i += 2)
    strcat(buf, "�w");
   strcat(buf, "�t");
  draw_line(x++, y, buf);

  sprintf(buf, "�x\033[30;47m  %-*s  \033[m�x", len, msg);
  draw_line(x++, y, buf);

  strcpy(buf, "��");
  for (i = -4; i < len; i += 2)
    strcat(buf, "�w");
  strcat(buf, "��");
  draw_line(x++, y, buf);

  move(b_lines, 0);

  x = vkey();
  scr_restore(&old_screen);
  return x;
}


