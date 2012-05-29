/*-------------------------------------------------------*/
/* rss.c                                                 */
/*-------------------------------------------------------*/
/* target : RSS Reader for BM                            */
/* create : 2009/04/08                                   */
/* author : hrs113355.bbs@xdbbs.twbbs.org                */
/*-------------------------------------------------------*/

#include "bbs.h"


extern int TagNum;

extern BCACHE *bshm;
extern XZ xz[];

extern char xo_pool[];

#if 0

hrs.090408:

RSS的設定檔被放在三個地方 (以Test板 http://www.abc.com/rss.xml為例)

1. brd/Test/rss.conf
    
    這是各看板的設定，每個看板有多個 (has-many) 訂閱的網址，
    以 rssfeed_t 格式儲存，欄位詳見 include/struct.h

2. etc/rss/http:#%slash%##%slash%#www.abc.com#%slash%#rss.xml

    這是各個 feed 網址，訂閱它的看板列表，針對每個 feed 網址，
    紀錄有哪些看板訂閱它，並且紀錄部份設定。
    以 rssbrdlist_t 格式儲存，欄位詳見 include/struct.h

3. etc/rss/feed.list

    紀錄 etc/rss/ 下有哪些網址的設定檔，
    以 rssfeedlist_t 格式儲存，欄位詳見 include/struct.h

#endif

//todo 考慮用同一個chrono作識別

int rss_cmpchrono(rssfeed_t *feed)
{
    return feed->chrono == currchrono;
}

int rss_fcmpchrono(rssfeedlist_t *feedlist)
{
    return feedlist->chrono == currchrono;
}

int rss_bcmpchrono(rssbrdlist_t *brdlist)
{
    return brdlist->chrono == currchrono;
}

#define ESCAPE_SLASH "#%slash%#"

// todo: check all path length
char slashbuf[512];
char * escape_slash(const char * src)
{
    char *ptr;
    const char *copy;

    memset(slashbuf, 0, sizeof(slashbuf));
    copy = src;
    ptr = slashbuf;

    while (*copy)
    {
	if (*copy == '/')
	{
	    strcat(ptr, ESCAPE_SLASH);
	    ptr+= strlen(ESCAPE_SLASH);
	}
	else
	    *ptr++ = *copy;

	copy++;
    }
    *ptr = '\0';
    return slashbuf;
}

int ValidURL(const char * s)
{
    // if the url...
    if ((strncmp(s, "http://", 7)        // doesn't start with 'http://'
	    && strncmp(s, "https://", 8)) // doen't start with 'https://'
		|| strstr(s, ESCAPE_SLASH) // has had escape_slash inside
		    || strchr(s, '\"'))     // has had quotation mark inside
	return 0; // invalid
    else
	return 1; // valid
}

/* ----------------------------------------------------- */
/* 新增/刪除/編輯                                        */
/* ----------------------------------------------------- */

// declarations
static void rss_brdlist_del(rssfeed_t *);
static void rss_feedlist_del(rssfeed_t *);

static void rss_brdlist_add(rssfeed_t *);
static void rss_feedlist_add(rssfeed_t *);

static void rss_brdlist_update(rssfeed_t *, rssfeed_t *);
static void rss_feedlist_update(rssfeed_t *, rssfeed_t *);

static void rss_item(int, rssfeed_t *);


static void rss_brdlist_del(rssfeed_t *feed)
{
    char fpath[512];
    register int i, size;
    rssbrdlist_t brdlist;

    sprintf(fpath, FN_RSS_FOLDER "%s", escape_slash(feed->feedurl));
    size = rec_num(fpath, sizeof(rssbrdlist_t));

    for (i = 1; i < size; i++)
	if (rec_get(fpath, &brdlist, sizeof(brdlist), i) >= 0)
	{
	    if (!strcmp(brdlist.brdname, currboard)
		    && !strcmp(brdlist.owner, feed->owner))
	    {
		currchrono = brdlist.chrono;
		rec_del(fpath, sizeof(rssbrdlist_t), i, rss_bcmpchrono);
		break;
	    }
	}

    if (rec_num(fpath, sizeof(rssbrdlist_t)) <= 1)
    {
	unlink(fpath);
	rss_feedlist_del(feed);
    }
}

static void rss_feedlist_del(rssfeed_t *feed)
{
    char *fpath = FN_RSS_LIST;
    register int i, size;
    rssfeedlist_t feedlist;

    size = rec_num(fpath, sizeof(rssfeedlist_t));
    for (i = 0; i < size; i++)
	if (rec_get(fpath, &feedlist, sizeof(feedlist), i) >= 0)
	{
	    if (!strcmp(feedlist.feedurl, feed->feedurl))
	    {
		currchrono = feedlist.chrono;						
		rec_del(fpath, sizeof(rssfeedlist_t), i, rss_fcmpchrono);
		break;
	    }
	}
}

static void rss_brdlist_add(rssfeed_t *feed)
{
    char fpath[512];
    rssbrdlist_t brdlist;

    memset(&brdlist, 0, sizeof(brdlist));
    str_ncpy(brdlist.brdname, currboard, sizeof(brdlist.brdname));
    str_ncpy(brdlist.owner, feed->owner, sizeof(brdlist.owner));
    str_ncpy(brdlist.prefix, feed->prefix, sizeof(brdlist.prefix));
    brdlist.chrono = time(NULL);
    brdlist.attr = feed->attr;

    sprintf(fpath, FN_RSS_FOLDER "%s", escape_slash(feed->feedurl));
    if (rec_num(fpath, sizeof(brdlist)) <= 0)
    {
	rssbrdlist_t update;
	memset(&update, 0, sizeof(update));
	rec_add(fpath, &update, sizeof(update));
	rss_feedlist_add(feed);
    }
    rec_add(fpath, &brdlist, sizeof(brdlist));
}

static void rss_feedlist_add(rssfeed_t *feed)
{
    char *fpath = FN_RSS_LIST;
    rssfeedlist_t feedlist;
    register int i, size;

    size = rec_num(fpath, sizeof(rssfeedlist_t));
    for (i = 0; i < size; i++)
	if (rec_get(fpath, &feedlist, sizeof(rssfeedlist_t), i) >= 0 &&
		!strcmp(feedlist.feedurl, feed->feedurl))
	    return;

    memset(&feedlist, 0, sizeof(feedlist));
    str_ncpy(feedlist.feedurl, feed->feedurl, sizeof(feedlist.feedurl));
    feedlist.chrono = time(NULL);
    rec_add(fpath, &feedlist, sizeof(feedlist));
}

static void rss_brdlist_update(rssfeed_t *feed, rssfeed_t *mfeed) //old & new
{
    int changeURL = strcmp(feed->feedurl, mfeed->feedurl), size, i;
    char fpath[512];
    rssbrdlist_t brdlist;

    sprintf(fpath, FN_RSS_FOLDER "%s", escape_slash(feed->feedurl));
    size = rec_num(fpath, sizeof(rssbrdlist_t));

    if (!changeURL)
    {
	for (i = 1; i < size; i++)
	    if (rec_get(fpath, &brdlist, sizeof(brdlist), i) >= 0)
	    {
		if (!strcmp(brdlist.brdname, currboard)
			&& !strcmp(brdlist.owner, feed->owner))
		{
		    currchrono = brdlist.chrono;
		    str_ncpy(brdlist.owner, mfeed->owner, sizeof(brdlist.owner));
		    str_ncpy(brdlist.prefix, mfeed->prefix, sizeof(brdlist.prefix));
		    brdlist.attr = mfeed->attr;
		    rec_put(fpath, &brdlist, sizeof(rssbrdlist_t), i, rss_bcmpchrono);
		}
	    }
    }
    else
    {
	rss_brdlist_del(feed);
	rss_brdlist_add(mfeed);
	rss_feedlist_update(feed, mfeed);
    }
}

static void rss_feedlist_update(rssfeed_t *feed, rssfeed_t *mfeed)
{
    if (!strcmp(feed->feedurl, mfeed->feedurl))
	return;

    char *fpath = FN_RSS_LIST;
    register int i, size;
    rssfeedlist_t feedlist;

    size = rec_num(fpath, sizeof(rssfeedlist_t));
    for (i = 0; i < size; i++)
        if (rec_get(fpath, &feedlist, sizeof(feedlist), i) >= 0)
        {
            if (!strcmp(feedlist.feedurl, feed->feedurl))
            {
                currchrono = feedlist.chrono;
		str_ncpy(feedlist.feedurl, mfeed->feedurl, sizeof(feedlist.feedurl));
                rec_put(fpath, &feedlist, sizeof(rssfeedlist_t), i, rss_fcmpchrono);
		break;
            }
        }
}

static int rss_set(rssfeed_t *feed)
{
    char buf[2];
    int i = 0;
    BRD *brd = bshm->bcache + currbno;

    move(3, 0);
    clrtobot();

    if (!vget(4, 0, "標籤：", feed->prefix, sizeof(feed->prefix), GCARRY | CTECHO))
        return -1;

    if (!*(feed->feedurl))
	strcpy(feed->feedurl, "http://");
    do
    {
	if (i++)
	    pmsg("請輸入以http://或https://開頭網址，並且請不要輸入特殊字元");
	outl(6, "來源網址：");
        if (!vget(7, 0, "", feed->feedurl, 79, GCARRY))
            return -1; // todo: expand length
    } while(!ValidURL(feed->feedurl));

    outl(9, "請輸入相關說明：");
    if (!vget(10, 0, "", feed->note, sizeof(feed->note), GCARRY | CTECHO))
        return -1; // todo: expand length

    feed->attr = 0;
    if (!(brd->battr & BRD_NOTRAN) &&
            vget(11, 0, "請問要把抓取的文章站際存檔嗎？ [y/N] ", buf, 2, LCECHO) &&
            *buf == 'y')
        feed->attr |= RSS_OUTGO;

    vget(13, 0, "請問當文章更新時要重抓一份嗎 (有可能會得到重複內容的文章)？ [y/N] "
	    , buf, 2, LCECHO);
    if (*buf == 'y')
	feed->attr |= RSS_GETUPDATE;

    vget(15, 0, "請問文章生成時是否要把標籤名稱作為文章類別？ [Y/n] ", buf, 2, LCECHO);
    if (*buf != 'n')
	feed->attr |= RSS_USELABEL;

    strncpy(feed->owner, cuser.userid, sizeof(feed->owner));
    return 0;
}

static int rss_add(xo)
    XO *xo;
{
    rssfeed_t feed, check;
    register int i, size;

    memset(&feed, 0, sizeof(feed));
    if (rss_set(&feed) < 0 || vans(msg_sure_ny) != 'y')
	return XO_HEAD;

    // check repeat
    size = rec_num (xo->dir, sizeof(rssfeed_t));
    for (i = 0; i < size; i++)
	if (rec_get(xo->dir, &check, sizeof(rssfeed_t), i) >= 0 &&
	    !strcmp(feed.feedurl, check.feedurl))
	{
	    pmsg("已有相同網址");
	    return XO_LOAD;
	}

    feed.chrono = time(NULL);
    str_stamp(feed.date, &feed.chrono);

    rss_brdlist_add(&feed);
    rec_add(xo->dir, &feed, sizeof(feed));

    return XO_LOAD;
}

static int rss_delete(xo)
    XO *xo;
{
    int pos, cur;
    rssfeed_t *feed;

    pos = xo->pos;
    cur = pos - xo->top;
    feed = (rssfeed_t *) xo_pool + cur;

    if (vans(msg_del_ny) == 'y')
    {
	currchrono = feed->chrono;

	if (!rec_del(xo->dir, sizeof(rssfeed_t), pos, rss_cmpchrono))
	{
	    rss_brdlist_del(feed);
	    return XO_LOAD;
	}
    }
    return XO_FOOT;
}

//todo: tag delete
static int vfyfeed(feed, pos)
  rssfeed_t *feed;
  int pos;
{
      return Tagger(feed->chrono, pos, TAG_NIN);
}


static void delfeed(xo, feed)
  XO *xo;
  rssfeed_t *feed;
{
    rss_brdlist_del(feed);
}

static int
rss_prune(xo)
  XO *xo;
{
  return xo_prune(xo, sizeof(rssfeed_t), vfyfeed, delfeed);
}

static int rss_rangedel(xo)
    XO *xo;
{
    return xo_rangedel(xo, sizeof(rssfeed_t), 0, delfeed);
}

static int rss_edit(XO *xo)
{
    rssfeed_t *feed, mfeed;
    int pos, cur;

    pos = xo->pos;
    cur = pos - xo->top;
    feed = (rssfeed_t *) xo_pool + cur;
    memcpy(&mfeed, feed, sizeof(mfeed));

    if (rss_set(&mfeed) < 0)
	return XO_HEAD;

    if (memcmp(feed, &mfeed, sizeof(rssfeed_t)) && vans(msg_sure_ny) == 'y')
    {
	rss_brdlist_update(feed, &mfeed);
	rss_feedlist_update(feed, &mfeed);

        memcpy(feed, &mfeed, sizeof(rssfeed_t));
        currchrono = feed->chrono;
        rec_put(xo->dir, feed, sizeof(rssfeed_t), pos, rss_cmpchrono);
    }
    return XO_HEAD;
}

static int rss_edit_feedurl(XO *xo)
{
    rssfeed_t *feed, mfeed;
    int pos, cur;

    pos = xo->pos;
    cur = pos - xo->top;
    feed = (rssfeed_t *) xo_pool + cur;
    memcpy(&mfeed, feed, sizeof(mfeed));

    vget(b_lines, 0, "來源網址：", mfeed.feedurl, 70, GCARRY | CTECHO);
    // todo: expand the length

    if (memcmp(&mfeed,feed, sizeof(rssfeed_t)) && vans(msg_sure_ny) == 'y')
    {
        memcpy(feed, &mfeed, sizeof(rssfeed_t));
	currchrono = feed->chrono;
	rec_put(xo->dir, feed, sizeof(rssfeed_t), pos, rss_cmpchrono);
	rss_brdlist_update(feed, &mfeed);
	rss_feedlist_update(feed, &mfeed);
    }
    return XO_HEAD;
}

static int rss_edit_prefix(XO *xo)
{    
    rssfeed_t *feed, mfeed;
    int pos, cur;

    pos = xo->pos;
    cur = pos - xo->top;
    feed = (rssfeed_t *) xo_pool + cur;
    memcpy(&mfeed, feed, sizeof(mfeed));

    vget(b_lines, 0, "標籤：", mfeed.prefix, sizeof(mfeed.prefix), GCARRY | CTECHO);
    if (memcmp(&mfeed,feed, sizeof(rssfeed_t)) && vans(msg_sure_ny) == 'y')
    {
        memcpy(feed, &mfeed, sizeof(rssfeed_t));
	currchrono = feed->chrono;
	rec_put(xo->dir, feed, sizeof(rssfeed_t), pos, rss_cmpchrono);
	rss_brdlist_update(feed, &mfeed);
	rss_feedlist_update(feed, &mfeed);
    }
    return XO_HEAD;
}

static int rss_edit_filter(XO *xo)
{
    rssbrdlist_t brdlist, newbrdlist;
    rssfeed_t *feed;
    register int pos, cur, i, size;
    char fpath[512];

    pos = xo->pos;
    cur = pos - xo->top;
    feed = (rssfeed_t *) xo_pool + cur;
    sprintf (fpath, FN_RSS_FOLDER "%s", escape_slash(feed->feedurl));
    size = rec_num(fpath, sizeof(rssbrdlist_t));

    // todo: error control
    for (i = 1; i < size; i++)
	if (rec_get(fpath, &brdlist, sizeof(brdlist), i) >= 0)
	    if (!strcmp(brdlist.brdname, currboard) &&
		    !strcmp(brdlist.owner, feed->owner))
		break;
    memcpy(&newbrdlist, &brdlist, sizeof(brdlist));

    move(3, 0);
    clrtobot();

    outl(4, "編輯白名單: 以斜線分隔白名單過濾關鍵字");
    outl(5, ANSI_COLOR(1;30) "例如: 教育/政治/經濟/外交");
    vget(6, 0, "", newbrdlist.white, sizeof(newbrdlist.white), GCARRY);

    outl(8, "編輯黑名單: 以斜線分隔黑名單過濾關鍵字");
    outl(9, ANSI_COLOR(1;30) "例如: 賺錢方法/廣告/減肥/spam/foo/bar");
    vget(10, 0, "", newbrdlist.black, sizeof(newbrdlist.black), GCARRY);

    if (memcmp(&brdlist, &newbrdlist, sizeof(brdlist)) && vans(msg_sure_ny) == 'y')
    {
	rec_put(fpath, &newbrdlist, sizeof(newbrdlist), i, NULL);
    }
    return XO_HEAD;
}

static int rssb_info(rssfeed_t * feed)
{
    rssbrdlist_t brdlist;
    register int size, i;
    char fpath[256];
    time_t update;

    sprintf (fpath, FN_RSS_FOLDER "%s", escape_slash(feed->feedurl));
    size = rec_num(fpath, sizeof(rssbrdlist_t));
    
    if (rec_get(fpath, &brdlist, sizeof(brdlist), 0) >= 0)
	update = brdlist.update;
    else
	update = 0;

    brdlist.update = 0;
    for (i = 1; i < size; i++)
            if (rec_get(fpath, &brdlist, sizeof(brdlist), i) >= 0)
                if (!strcmp(brdlist.brdname, currboard)
                        && !strcmp(brdlist.owner, feed->owner))
			break;

    move(3, 0);
    clrtobot();

    move(4, 0);

    if (feed->attr & RSS_STOP)
	prints(ANSI_COLOR(1;31) "[目前為停止抓取狀態]" ANSI_RESET "\n");
    prints(INFO_MARK ANSI_COLOR(37)"標籤: %s" ANSI_RESET "\n", feed->prefix);
    prints(INFO_MARK ANSI_COLOR(37)"訂閱\人: %s" ANSI_RESET "\n", feed->owner);
    prints(INFO_MARK ANSI_COLOR(37)"訂閱\來源: %s" ANSI_RESET "\n", feed->feedurl);
    prints(INFO_MARK ANSI_COLOR(37)"建立日期: %s" ANSI_RESET "\n", Btime(&(feed->chrono)));
    prints(INFO_MARK ANSI_COLOR(37)"最新文章: %s" ANSI_RESET "\n", 
	    (brdlist.update <= 0 ? "尚未開始" : Btime(&(brdlist.update))));
    prints(INFO_MARK ANSI_COLOR(37)"最近抓取: %s" ANSI_RESET "\n",
	    (update <= 0 ? "尚未開始" : Btime(&(update))));
    prints(INFO_MARK ANSI_COLOR(37)"站際存檔: %s" ANSI_RESET "\n", (feed->attr & RSS_OUTGO ? "○" : "╳"));
    prints(INFO_MARK ANSI_COLOR(37)"重複抓取更新: %s" ANSI_RESET "\n", (feed->attr & RSS_GETUPDATE ? "○" : "╳"));
    prints(INFO_MARK ANSI_COLOR(37)"使用標籤名作為文章類別: %s" ANSI_RESET "\n", (feed->attr & RSS_USELABEL ? "○" : "╳"));
    prints(INFO_MARK ANSI_COLOR(37)"敘述說明: %s" ANSI_RESET "\n\n", feed->note);

    if (*(brdlist.white))
	prints(INFO_MARK ANSI_COLOR(37)"過濾白名單:\n" ANSI_COLOR(0;30;47) "%-*s" ANSI_RESET "\n", sizeof(brdlist.white), brdlist.white);

    if (*(brdlist.black))
	prints(INFO_MARK ANSI_COLOR(37)"過濾黑名單:\n" ANSI_COLOR(0;30;47) "%-*s" ANSI_RESET "\n", sizeof(brdlist.black), brdlist.black);

    vmsg(NULL);
    return XO_HEAD;
}

static int rss_query(XO *xo)
{
    rssfeed_t *feed;
    register int pos, cur;

    pos = xo->pos;
    cur = pos - xo->top;
    feed = (rssfeed_t *) xo_pool + cur;

    return rssb_info(feed);
}

/* ----------------------------------------------------- */
/* 將更新時間歸零                                        */
/* ----------------------------------------------------- */

static int rss_reset(XO *xo)
{
    rssbrdlist_t brdlist;
    rssfeed_t *feed;
    register int pos, cur, size, i;
    char fpath[512];

    if (vans("確定要將更新時間歸零，重抓ＲＳＳ嗎 [y/N]？ ") != 'y')
	return XO_FOOT;

    pos = xo->pos;
    cur = pos - xo->top;
    feed = (rssfeed_t *) xo_pool + cur;

    sprintf (fpath, FN_RSS_FOLDER "%s", escape_slash(feed->feedurl));
    size = rec_num(fpath, sizeof(rssbrdlist_t));

    for (i = 1; i < size; i++)
            if (rec_get(fpath, &brdlist, sizeof(brdlist), i) >= 0)
            {
                if (!strcmp(brdlist.brdname, currboard)
                        && !strcmp(brdlist.owner, feed->owner))
                {
                    currchrono = brdlist.chrono;
                    brdlist.update = 0;
                    rec_put(fpath, &brdlist, sizeof(rssbrdlist_t), i, rss_bcmpchrono);
		    break;
                }
            }
    vmsg("歸零成功\!");
    return XO_FOOT;
}

/* ----------------------------------------------------- */
/* 暫停抓取                                              */
/* ----------------------------------------------------- */

static int rss_stop(XO *xo)
{
    rssfeed_t *feed, mfeed;
    register int pos, cur;

    pos = xo->pos;
    cur = pos - xo->top;
    feed = (rssfeed_t *) xo_pool + cur;

    memcpy(&mfeed, feed, sizeof(mfeed));

    if (mfeed.attr & RSS_STOP && vans("現在是暫停抓取狀態，要恢復抓取嗎 [y/N]？ ") == 'y')
	mfeed.attr &= ~RSS_STOP;
    else if (!(mfeed.attr & RSS_STOP) && vans("要暫停抓取這個 feed 嗎 [y/N]？ ") == 'y')
	mfeed.attr |= RSS_STOP;

    if (memcmp(&mfeed, feed, sizeof(mfeed)))
    {
	memcpy(feed, &mfeed, sizeof(rssfeed_t));
	currchrono = feed->chrono;
	rec_put(xo->dir, feed, sizeof(rssfeed_t), pos, rss_cmpchrono);
	rss_brdlist_update(feed, &mfeed);
    }

    return XO_HEAD;
}


/* ----------------------------------------------------- */
/* 移動項目                                              */
/* ----------------------------------------------------- */

static int rss_move(XO *xo)
{
  rssfeed_t *feed;
  char buf[40];
  int pos, newOrder;

  pos = xo->pos;
  sprintf(buf, "請輸入第 %d 選項的新位置：", pos + 1);
  if (!vget(b_lines, 0, buf, buf, 5, DOECHO))
    return XO_FOOT;

  feed = (rssfeed_t *) xo_pool + pos - xo->top;

  newOrder = atoi(buf) - 1;
  if (newOrder < 0)
    newOrder = 0;
  else if (newOrder >= xo->max)
    newOrder = xo->max - 1;

  if (newOrder != pos)
  {
    if (!rec_del(xo->dir, sizeof(rssfeed_t), pos, NULL))
    {
      rec_ins(xo->dir, feed, sizeof(rssfeed_t), newOrder, 1);
      xo->pos = newOrder;
      return XO_LOAD;
    }
  }
  return XO_FOOT;
}


/* ----------------------------------------------------- */
/* 新增Tag                                               */
/* ----------------------------------------------------- */

static int
rss_tag(xo)
  XO *xo;
{
  rssfeed_t *feed;
  int tag, pos, cur;

  pos = xo->pos;
  cur = pos - xo->top;
  feed = (rssfeed_t *) xo_pool + cur;

  if (tag = Tagger(feed->chrono, pos, TAG_TOGGLE))
  {
     move(3 + cur, 0);
     rss_item(pos + 1, feed);
  }

  /* return XO_NONE; */
  return xo->pos + 1 + XO_MOVE; 
}


/* ----------------------------------------------------- */
/* 看板功能表						 */
/* ----------------------------------------------------- */


static int rss_body();
static int rss_head();

static int
rss_init(xo)
    XO *xo;
{
#ifdef HAVE_BRD_EXPLAIN
    XO_TALL = b_lines - 3;
#endif
    xo_load(xo, sizeof(rssfeed_t));
    return rss_head(xo);
}

static int
rss_load(xo)
    XO *xo;
{
    xo_load(xo, sizeof(rssfeed_t));
    return rss_body(xo);
}


static void
rss_item(int num, rssfeed_t *feed)
{
    clrtoeol();
    prints("%6d%c "
	    ANSI_COLOR(1;3%d) "%s" ANSI_RESET
	    "%s %-15.15s %-48.48s" ANSI_RESET "\n"
	    , num, (TagNum && !Tagger(feed->chrono, 0, TAG_NIN) ? '*' : ' ')
	    , (*(feed->date+6) + *(feed->date+7)) % 8
	    , feed->date+3, ( feed->attr & RSS_STOP ? ANSI_COLOR(31) : "" )
	    , feed->prefix, feed->note);
}

static int
rss_body(xo)
    XO *xo;
{
    rssfeed_t *feed;
    int num, max, tail;

    max = xo->max;
    if (max <= 0)
    {
	if (vans("要新增ＲＳＳ資料嗎(Y/N)？[N] ") == 'y')
	    return rss_add(xo);
	return XO_QUIT;
    }

    feed = (rssfeed_t *) xo_pool;
    num = xo->top;
    tail = num + XO_TALL;
    if (max > tail)
	max = tail;

    move(3, 0);
    do
    {
	rss_item(++num, feed++);
    } while (num < max);
    clrtobot();

    return XO_FOOT;	/* itoc.010403: 把 b_lines 填上 feeter */
}


static int
rss_head(xo)
    XO *xo;
{ 
    vs_head("ＲＳＳ訂閱\器", str_site);
    prints(NECKER_RSS, d_cols, "");  
    return rss_body(xo);
}


static int
rss_help(xo)
    XO *xo;
{
    xo_help("rss");
    return rss_head(xo);
}


KeyFunc rss_cb[] =
{
    XO_INIT, rss_init,
    XO_LOAD, rss_load,
    XO_HEAD, rss_head,
    XO_BODY, rss_body,

    'r', rss_query,

    'd', rss_delete,
    't', rss_tag,
    'E', rss_edit,
    'H', rss_edit_feedurl,
    'T', rss_edit_prefix,
    'o', rss_edit_filter,
    'D', rss_rangedel,
    Ctrl('D'), rss_prune,

    'U', rss_reset,
    'K', rss_stop,
    's', rss_load,
    'm', rss_move,

    Ctrl('P'), rss_add,

    'h', rss_help
};

/* ------------------------------------------------------ */
/* 管理全站 ＲＳＳ                                        */
/* ------------------------------------------------------ */

#define RSSM_FEEDLIST	(1)
#define RSSM_BRDLIST	(2)

// rssm_*: rss manager 子函式
static void rssm_head(void);
int rssm_xover(int);

// rssu_*: rss feed list manager 設定全站 feed 來源列表的子函式
static void rssf_item(int, rssfeedlist_t *);
static void rssf_query(rssfeedlist_t *);
static int rssf_cmp(rssfeedlist_t *, rssfeedlist_t *);
static int rssf_search(rssfeedlist_t *, char * key);

// rssb_*: rss board list manager 設定同一個 feed 來源的所有訂閱看板的子函式
static void rssb_head(void);
static void rssb_item(int, rssbrdlist_t *);
static void rssb_query(rssbrdlist_t *);
static int rssb_cmp(rssbrdlist_t *, rssbrdlist_t *);
static int rssb_search(rssbrdlist_t *, char * key);

rssfeedlist_t * currrf = NULL;

/* ----------------------------------------------------- */
/* ＲＳＳ設定主函式					 */
/* ----------------------------------------------------- */

// rssm = rss manager

int 
rssm_main()
{
#ifdef HAVE_BRD_EXPLAIN
        XO_TALL = b_lines - 3;
#endif
    if (more("etc/rssm.hlp", (char *) -1) != -1) 
	vmsg(NULL);
    return rssm_xover(RSSM_FEEDLIST);
}

void
rssm_head(void)
{
    vs_head("全站ＲＳＳ設定", str_site);
    prints(NECKER_RSSM, d_cols, "");
}

void 
rssb_head(void)
{
    vs_head("來源看板列表", str_site);
    prints(NECKER_RSSB, d_cols, ""); 
}

int rssm_xover(int type)
{
    int num, pageno, pagemax, redraw, reload;
    int ch, cur, i, dirty;
    struct stat st;
    char *data;
    int recsiz;
    char fpath[512];
    char buf[40];
    void (*item_func)(), (*query_func)(), (*head_func)();
    int (*sync_func)(), (*search_func)();

    if (type == RSSM_FEEDLIST)
    {
	recsiz = sizeof(rssfeedlist_t);
	item_func = rssf_item;
	query_func = rssf_query;
	sync_func = rssf_cmp;
	search_func = rssf_search;
	head_func = rssm_head;  // rssm_head is rssf_head
	strcpy(fpath, FN_RSS_LIST);
    }
    else if (type == RSSM_BRDLIST)
    {
	if (!currrf)
	    return 0;

	recsiz = sizeof(rssbrdlist_t);
	item_func = rssb_item;
	query_func = rssb_query;
	sync_func = rssb_cmp;
	search_func = rssb_search;
	head_func = rssb_head;
	sprintf(fpath, FN_RSS_FOLDER "%s", escape_slash(currrf->feedurl));
    }
    else
    {
	return 0;
    }

    /* initialize */
    dirty = 0;  /* 1:有新增/刪除資料 */
    reload = 1;
    pageno = 0;
    cur = 0;
    data = NULL;

    do
    {
	if (reload)
	{
	    if (stat(fpath, &st) == -1)
		return 0;

	    i = st.st_size;
	    num = (i / recsiz) - 1;
	    if (num < 0)
		    return 0;

	    if ((ch = open(fpath, O_RDONLY)) >= 0)
	    {
		data = data ? (char *) realloc(data, i) : (char *) malloc(i);
		read(ch, data, i);
		close(ch);
	    }

	    pagemax = num / XO_TALL;
	    reload = 0;
	    redraw = 1;
	}

	if (redraw)
	{
	    head_func();

	    i = pageno * XO_TALL;
	    ch = BMIN(num, i + XO_TALL - 1);
	    move(3, 0);
	    do
	    {
		item_func(i + 1, data + i * recsiz);
		i++;
	    } while (i <= ch);


	    outf((type == RSSM_FEEDLIST ? FEETER_RSSM : FEETER_RSSB));
	    move(3 + cur, 0);
	    outc('>');
	    redraw = 0;
	}

	ch = vkey();
	switch (ch)
	{
	    case KEY_RIGHT:
	    case '\n':
	    case 'r':
		i = cur + pageno * XO_TALL;
		if (!i && type == RSSM_BRDLIST)
		    break;

		query_func(data + i * recsiz);
		redraw = 1;
		break;

	    case 'd':
		outl(b_lines - 1, ANSI_COLOR(1;31) "警告: 刪除本紀錄可能破壞與看板設定檔之間的連結" ANSI_RESET);
		if (vans(msg_del_ny) == 'y')
		{
		    dirty = 1;
		    i = cur + pageno * XO_TALL;
		    rec_del(fpath, recsiz, i, NULL);
		    cur = i ? ((i - 1) % XO_TALL) : 0;	/* 游標放在砍掉的前一篇 */
		    reload = 1;
		}
		redraw = 1;
		break;

	    case 'E':
#if 0
		i = cur + pageno * XO_TALL;
		if (add_func(fpath, data + i * recsiz, i))
		{
		    dirty = 1;
		    reload = 1;
		}
		redraw = 1;

		temporarily disable it.
#endif
		break;

	    case '/':
		if (vget(b_lines, 0, "關鍵字：", buf, sizeof(buf), DOECHO))
		{
		    str_lower(buf, buf);
		    for (i = pageno * XO_TALL + cur + 1; i <= num; i++)	/* 從游標下一個開始找 */
		    {
			if (search_func(data + i * recsiz, buf))
			{
			    pageno = i / XO_TALL;
			    cur = i % XO_TALL;
			    break;
			}
		    }
		}
		redraw = 1;
		break;

	    case 'e':
		ch = 'q';
		break;

	    case ' ':
		ch = KEY_PGDN;
		// no break, let xo_cursor handle it.

	    default:
		ch = xo_cursor(ch, pagemax, num, &pageno, &cur, &redraw);
		break;
	}
    } while (ch != 'q');

    free(data);

    if (dirty)
	rec_sync(fpath, recsiz, sync_func, NULL);
    return 0;
}


static void
rssf_item(int num, rssfeedlist_t *rssfeed)
{
    int size;
    char fpath[512];

    sprintf(fpath, FN_RSS_FOLDER "%s", escape_slash(rssfeed->feedurl));
    size = rec_num(fpath, sizeof(rssbrdlist_t)) - 1;

    prints("%6d  %-60.60s (%d)\n", num, rssfeed->feedurl, size);
}

static void
rssf_query(rssfeedlist_t *rf)
{
    char fpath[512];
    int size;

    move (3, 0);
    clrtobot();
    move (5, 0);
    outs(INFO_MARK ANSI_COLOR(37) "來源網址: " ANSI_RESET "\n");
    prints(ANSI_COLOR(7) "%s" ANSI_RESET "\n\n", rf->feedurl);
    outs(INFO_MARK ANSI_COLOR(37) "最早訂閱\時間: " ANSI_RESET);
    prints("%s\n", Btime(&(rf->chrono)));
    outs(INFO_MARK ANSI_COLOR(37) "訂閱\看板數: " ANSI_RESET);

    sprintf(fpath, FN_RSS_FOLDER "%s", escape_slash(rf->feedurl));
    size = rec_num(fpath, sizeof(rssbrdlist_t)) - 1;
    prints("%d\n", size);

    if (size > 0)
    {
	outs(INFO_MARK ANSI_COLOR(37) "看板列表: " ANSI_RESET "\n");
	int i;
	rssbrdlist_t rb;
	for (i = 1; i <= size; i++)
	{
	    if (rec_get(fpath, &rb, sizeof(rssbrdlist_t), i) >= 0)
		prints (" %s", rb.brdname);
	}
    }

    if (vans("是否進入查看訂閱\看板列表？ [y/N] ") == 'y')
    {
	currrf = rf;
	rssm_xover(RSSM_BRDLIST);
    }
    return;
}

static int
rssf_cmp(a, b)
    rssfeedlist_t *a, *b;
{
    // compare with chrono
    return (a->chrono == b->chrono);
}

static int rssf_search(rssfeedlist_t * rf, char * key)
{
    return (int)(str_str(rf->feedurl, key));
}

static void
rssb_item(int num, rssbrdlist_t *rssbrd)
{
    if (num == 1)
	prints("    " INFO_MARK ANSI_COLOR(37) " 最後抓取此來源時間:         %s" ANSI_RESET "\n"
		, Btime(&(rssbrd->update)));
    else
	prints("%6d  %-13.13s %-13.13s %s\n"
	    , num, rssbrd->owner, rssbrd->brdname, Btime(&(rssbrd->chrono)));
}

static void
rssb_query(rssbrdlist_t *rb)
{
    char fpath[512];
    int i;
    rssfeed_t rf;
    brd_fpath(fpath, rb->brdname, fn_rssconf);

    for (i = 0; rec_get(fpath, &rf, sizeof(rssfeed_t), i) >= 0; i++)
	if (!strcmp(rf.feedurl, currrf->feedurl))
	{
	    rssb_info(&rf);
	    return;
	}

    vmsg("失去與看板的連結");
}

static int
rssb_cmp(a, b)
    rssbrdlist_t *a, *b;
{
    // compare with chrono
    return (a->chrono == b->chrono);
}

static int rssb_search(rssbrdlist_t * rb, char * key)
{
    return (int)(str_str(rb->brdname, key) || str_str(rb->owner, key));
}

