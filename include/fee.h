/*-------------------------------------------------------*/
/* fee.h				                 */
/*-------------------------------------------------------*/
/* target : 定義全站各項費用	                         */
/* author : hrs@xdbbs.twbbs.org				 */
/* create : 2008/02/26                                   */
/*-------------------------------------------------------*/

#if 0

hrs : 寫法很偷懶，顯示訊息就直接串起來，比如說 "你要付" FEE_xxx "元嗎"
      
      而使用上用atoi(FEE_xxx)，所以是刻意定義成字串的。

#endif

#define FEE_EXPIRE  "1000"  /* hrs.080226: 每買一單位expire的錢, 是字串 */
#define FEE_CLOAK   "350"   /* hrs.080215: 隱形費用,注意是字串!(銀幣) */
#define FEE_MQUOTA  "1000"  /* hrs.080215: 買信箱容量費用,注意是字串!(銀幣) */

/* post.c LuckyPost 幸運中獎機制 */

#ifdef HAVE_LUCKYPOST
#define LUCKYPOST_REWARD "7777"         /* 一次中獎的獎金(銀幣) */
#endif

/* bbsnet.c bbsnet的費用 */

#ifdef	OPEN_BBSNET
#   define FEE_BBSNET  "50"
#endif

#define FEE_XoAuthor "500"

/* lottery.c 樂透每注的錢 */
#define FEE_LOTTERY "50"
