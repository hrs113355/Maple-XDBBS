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
#define LV_SONGTOP	BFLAG(1)	/* 點歌次數統計 */
#define LV_DAYTOP	BFLAG(2)	/* 本日十大熱門話題 */
#define LV_WEEKTOP	BFLAG(3)	/* 本週五十大熱門話題 */
#define LV_MONTHTOP	BFLAG(4)	/* 本月百大熱門話題 */
#define LV_YEARTOP	BFLAG(5)	/* 本年度百大熱門話題 */
#define LV_TOPTOQUERY	BFLAG(6)	/* 好奇指數排行榜 */
#define LV_TOPBEQUERY	BFLAG(7)	/* 人氣指數排行榜 */
#define LV_TOPSTAYTIME	BFLAG(8)	/* 掛站留戀分鐘榜 */
#define LV_TOPPOST	BFLAG(9)	/* 灌水篇數英雄榜 */
#define LV_TOPMONEY	BFLAG(10)	/* 銀幣富翁封神榜 */
#define LV_TOPLOGIN	BFLAG(11)	/* 上站次數排行榜 */
#define LV_BIRTHDAY	BFLAG(12)	/* 本日壽星 */
#define LV_ECONOMY	BFLAG(13)	/* 全站經濟統計 */ 
#define LV_TOPSCORE	BFLAG(14)	/* 文章評分排行榜 */
#define LV_NOTEPAD      BFLAG(15)	/* 酸甜苦辣留言板 */
#define LV_TODO		BFLAG(16)	/* 個人行事曆 */


#define LV_DEFAULT_NEW         (LV_DAYTOP | LV_TOPMONEY | LV_BIRTHDAY | LV_NOTEPAD | LV_TODO)
#define LV_DEFAULT_GUEST       (LV_SONGTOP | LV_DAYTOP | LV_WEEKTOP | LV_MONTHTOP | LV_YEARTOP | LV_TOPTOQUERY \
				| LV_TOPBEQUERY | LV_TOPSTAYTIME | LV_TOPPOST | LV_TOPMONEY \
				| LV_TOPLOGIN | LV_BIRTHDAY | LV_ECONOMY | LV_TOPSCORE | LV_NOTEPAD)


/* ----------------------------------------------------- */
/* 上站畫面的中文意義					 */
/* ----------------------------------------------------- */

#define NUMLVS		17

#ifdef _ADMIN_C_

char *lv_tbl[NUMLVS] =
{
  "保留",
  "點歌次數統計",
  "本日十大熱門話題",
  "本週五十大熱門話題",
  "本月百大熱門話題",
  "本年度百大熱門話題",
  "好奇指數排行榜",
  "人氣指數排行榜",
  "掛站留戀分鐘榜",
  "灌水篇數英雄榜",
  "銀幣富翁封神榜",
  "上站次數排行榜",
  "本日壽星",
  "全站經濟統計",
  "文章評分排行榜",
  "酸甜苦辣留言板",
  "個人行事曆"
};
#endif

#endif				/* _LV_H_ */
