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
/* 畫面繪製						 */
/* ----------------------------------------------------- */


static void
draw_line(x, y, msg)	/* 在 (x, y) 的位置塞入 msg，左右仍要印出原來的彩色文字 */
  int x, y;
  uschar *msg;
{
  /* hrs.090928: 讓 terminal 去處理 */
  move(x, y);
  outstr(msg);
  return;
}


#ifdef HAVE_POPUPMENU

static screen_backup_t old_screen;

#ifdef POPUP_ANSWER

/* ----------------------------------------------------- */
/* 選項繪製						 */
/* ----------------------------------------------------- */


static int
draw_item(x, y, desc, hotkey, mode)
  int x, y;
  char *desc;
  char hotkey;
  int mode;		/* 0:清除光棒  1:畫上光棒 */
{
  char buf[128];

  sprintf(buf, " │%s%c %c%c%c%-25s  \033[m│ ",
    mode ? COLOR4 : "\033[30;47m", mode ? '>' : ' ',
    (hotkey == *desc) ? '[' : '(', *desc,
    (hotkey == *desc) ? ']' : ')', desc + 1);

  draw_line(x, y, buf);
}


static int	/* 回傳總共有幾個選項 */
draw_menu(x, y, title, desc, hotkey, cur)
  int x, y;
  char *title;
  char *desc[];
  char hotkey;
  int *cur;	/* 回傳預設值所在位置 */
{
  int i, meet;
  char buf[128];

  draw_line(x++, y, " ╭────────────────╮ ");

  sprintf(buf, " │" COLOR4 "  %-28s  \033[m│ ", title);
  draw_line(x++, y, buf);

  draw_line(x++, y, " ├────────────────┤ ");

  for (i = 1; desc[i]; i++)
  {
    meet = (desc[i][0] == hotkey);
    draw_item(x++, y, desc[i], hotkey, meet);
    if (meet)
      *cur = i;
  }

  draw_line(x, y, " ╰────────────────╯ ");

  /* 避免在偵測左右鍵全形下，按左鍵會跳離二層選單的問題 */
  move(b_lines, 0);

  return i - 1;
}


/* ----------------------------------------------------- */
/* 找選項						 */
/* ----------------------------------------------------- */


static int			/* -1:找不到 >=0:第幾個選項 */
find_cur(ch, max, desc)		/* 找 ch 這個按鍵是第幾個選項 */
  int ch, max;
  char *desc[];
{
  int i, cc;

  if (ch >= 'A' && ch <= 'Z')
    ch |= 0x20;		/* 換小寫 */

  for (i = 1; i <= max; i++)
  {
    cc = desc[i][0];
    if (cc >= 'A' && cc <= 'Z')
      cc |= 0x20;	/* 換小寫 */

    if (ch == cc)
      return i;
  }

  return -1;
}


/*------------------------------------------------------ */
/* 詢問選項，可用來取代 vans()				 */
/*------------------------------------------------------ */
/* x, y  是蹦出視窗左上角的 (x, y) 位置			 */
/* title 是視窗的標題					 */
/* desc  是選項的敘述：					 */
/*       第一個字串必須為兩個 char			 */
/*         第一個字元代表一開始游標停的位置		 */
/*         第二個字元代表按下 KEY_LEFT 的預設回傳值	 */
/*       中間的字串是每個選項的敘述 (首字母為熱鍵)	 */
/*       最後一個字串必須為 NULL			 */
/*------------------------------------------------------ */


int		/* 傳回小寫字母或數字 */
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

  /* 畫出整個選單 */
  max = draw_menu(x, y, title, desc, hotkey, &cur);
  x += 2;

  /* 一進入，游標停在預設值 */
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
	ch |= 0x20;		/* 回傳小寫 */
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

    default:		/* 去找所按鍵是哪一個選項 */
      if ((ch = find_cur(ch, max, desc)) > 0)
	cur = ch;
      break;
    }

    if (old_cur != cur)		/* 游標變動位置才需要重繪 */
    {
      draw_item(x + old_cur, y, desc[old_cur], hotkey, 0);
      draw_item(x + cur, y, desc[cur], hotkey, 1);
      old_cur = cur;
      /* 避免在偵測左右鍵全形下，按左鍵會跳離二層選單的問題 */
      move(b_lines, 0);
    }
  }
}
#endif	/* POPUP_ANSWER */

#endif	/* HAVE_POPUPMENU */


/*------------------------------------------------------ */
/* 蹦出式視窗訊息，與 vmsg() 並存			 */
/*------------------------------------------------------ */


int
pmsg(msg)
  char *msg;		/* 不可為 NULL */
{
  int len, x, y, i;
  char buf[80];

  if (!msg)
      return vmsg(NULL);

  scr_dump(&old_screen);
  grayout(0, b_lines, GRAYOUT_DARK);
  len = strlen(msg);
  if (len < 16)		/* 取 msg title 其中較長者為 len */
    len = 16;
  if (len % 2)		/* 變成偶數 */
    len++;
  x = (b_lines - 4) >> 1;	/* 置中 */
  y = (b_cols - 8 - len) >> 1;

  strcpy(buf, "╭");
  for (i = -4; i < len; i += 2)
    strcat(buf, "─");
  strcat(buf, "╮");
  draw_line(x++, y, buf);

  sprintf(buf, "│" COLOR4 "  %-*s  \033[m│", len, "請按任意鍵繼續..");
  draw_line(x++, y, buf);

  strcpy(buf, "├");
  for (i = -4; i < len; i += 2)
    strcat(buf, "─");
   strcat(buf, "┤");
  draw_line(x++, y, buf);

  sprintf(buf, "│\033[30;47m  %-*s  \033[m│", len, msg);
  draw_line(x++, y, buf);

  strcpy(buf, "╰");
  for (i = -4; i < len; i += 2)
    strcat(buf, "─");
  strcat(buf, "╯");
  draw_line(x++, y, buf);

  move(b_lines, 0);

  x = vkey();
  scr_restore(&old_screen);
  return x;
}


