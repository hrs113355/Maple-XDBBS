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


#define BRD_NOZAP	0x000001	/* 不可 zap */
#define BRD_NOTRAN	0x000002	/* 不轉信 */
#define BRD_NOCOUNT	0x000004	/* 不計文章發表篇數 */
#define BRD_NOSTAT	0x000008	/* 不納入熱門話題統計 */
#define BRD_NOVOTE	0x000010	/* 不公佈投票結果於 [record] 板 */
#define BRD_ANONYMOUS	0x000020	/* 匿名看板 */
#define BRD_NOSCORE	0x000040	/* 不評分看板 */
#define BRD_COSIGN	0x000080	/* 連署機 */
#define BRD_18X		0x000100	/* 18禁看板 */
#define BRD_NOREPLY	0x000200	/* 不可回覆文章 */
#define BRD_NOBOO	0x000400	/* 不可以扣分 */
#define BRD_LOCAL	0x000800	/* hrs.080323: 預設站內信看板 */
#define BRD_SCOREALIGN	0x001000	/* hrs.080510: 推文自動對齊 */
#define BRD_SERVICE	0x002000	/* hrs.080514: 站務性質看板 */
#define BRD_PERSONAL	0x004000	/* hrs.080514: 個人板 */
#define BRD_HIDEALLPOST	0x008000	/* hrs.090717: 把文章備份到BN_ALLHIDPOST 而非 BN_ALLPOST */

/* ----------------------------------------------------- */
/* 各種旗標的中文意義					 */
/* ----------------------------------------------------- */


#define NUMBATTRS	16

#define STR_BATTR	"zTcsvA%V8ybLS!Pa"   /* itoc: 新增旗標的時候別忘了改這裡啊 */


#ifdef _ADMIN_C_
char *battr_tbl[NUMBATTRS] =
{
  "不可 Zap",			/* BRD_NOZAP */
  "不轉信出去",			/* BRD_NOTRAN */
  "不記錄篇數",			/* BRD_NOCOUNT */
  "不做熱門話題統計",		/* BRD_NOSTAT */
  "不公開投票結果",		/* BRD_NOVOTE */
  "匿名看板",			/* BRD_ANONYMOUS */
  "不評分看板",			/* BRD_NOSCORE */
  "連署機",			/* BRD_VOTEBRD */
  "18禁看板",			/* BRD_18X */
  "不可回覆文章",		/* BRD_NOREPLY */
  "不可以扣分",			/* BRD_NOBOO */
  "發表預設存站內",		/* BRD_LOCAL */
  "推文自動對齊",		/* BRD_SCOREALIGN */
  "站務性質看板",		/* BRD_SERVICE */
  "個人板",			/* BRD_PERSONAL */
  "文章不進ALLPOST板"		/* BRD_HIDEALLPOST */
};

#endif

#endif				/* _BATTR_H_ */
