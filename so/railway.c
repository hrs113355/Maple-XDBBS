/*-------------------------------------------------------*/
/* railway.c    ( NTHU CS MapleBBS Ver 3.10 )		 */
/*-------------------------------------------------------*/
/* target : 台鐵火車時刻表查詢				 */
/* create : 02/05/29					 */
/* update :   /  /					 */
/* author : lp@micro.ee.nthu.edu.tw			 */
/* modify : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/*-------------------------------------------------------*/


#if 0
  1. 需要安裝 lynx、piconv、grep，且請檢查這三者在 http_conn()
     裡面的路徑是否正確。
  2. 程式雖然有轉車站的部分，但是目前沒有用。
  3. 目前程式無法查詢票價。
#endif

#include "bbs.h"

#define mouts(x,y,s)	{ move(x, y); outs(s); }

#define FORM_railway	"\"SBeginStation=%s&SBeginStationName=&SEndStation=%s&SEndStationName=&SClass=%s&SDateTime=%s&STime=%s%s|%s%s&SearchType=0&SBeginCityCode=&SEndCityCode=&BStationIndex=&EStationIndex=\""

#define REF		"http://www.railway.gov.tw/"

#define RAILWAY_XPOS	1
#define RAILWAY_YPOS	0
#define MAX_SITE	216
#define SITE_PER_LINE	13	/* 每列秀 13 個站台 */

static char *siteid[MAX_SITE] =	/* 車站站台 */
{
  "台東", "山里", "鹿野", "瑞源", "瑞和", "月美", "關山", "海端", "池上", "富里", "東竹", "東里", "玉里",
  "三民", "瑞穗", "富源", "大富", "光復", "萬榮", "鳳林", "南平", "溪口", "豐田", "壽豐", "平和", "志學",
  "吉安", "花蓮", "北埔", "景美", "新城", "崇德", "和仁", "和平", "漢本", "武塔", "南澳", "東澳", "永樂",
  "蘇澳","蘇澳新","新馬", "冬山", "羅東", "中里", "二結", "宜蘭", "四城", "礁溪", "頂埔", "頭城", "外澳",
  "龜山", "大溪", "大里", "石城", "福隆", "貢寮", "雙溪", "牡丹","三貂嶺","侯硐", "瑞芳","四腳亭","暖暖",
  "基隆", "三坑", "八堵", "七堵", "百福", "五堵", "汐止", "汐科", "南港", "松山", "台北", "萬華", "板橋",
  "樹林", "山佳", "鶯歌", "桃園", "內壢", "中壢", "埔心", "楊梅", "富岡", "湖口", "新豐", "竹北", "新竹",
  "香山", "崎頂", "竹南", "談文", "大山", "後龍", "龍港","白沙屯","新埔", "通霄", "苑裡", "日南","大甲",
  "台中港","清水","沙鹿", "龍井", "大肚", "追分", "造橋", "豐富", "苗栗", "南勢", "銅鑼", "三義", "泰安",
  "后里", "豐原", "潭子", "太原", "台中", "大慶", "烏日","新烏日","成功\","彰化", "花壇", "大村", "員林",
  "永靖", "社頭", "田中", "二水", "林內", "石榴", "斗六", "斗南", "石龜", "大林", "民雄", "嘉北", "嘉義",
  "水上", "南靖", "後壁", "新營", "柳營","林鳳營","隆田", "拔林", "善化", "新市", "永康", "大橋", "台南",
  "保安", "中洲", "大湖", "路竹", "岡山", "橋頭", "楠梓","新左營","左營", "高雄", "鳳山", "後庄","九曲堂",
  "六塊厝","屏東","歸來", "麟洛", "西勢", "竹田", "潮州", "崁頂", "南州", "鎮安", "林邊", "佳冬", "東海",
  "枋寮", "加祿", "內獅", "枋山", "古莊", "大武", "瀧溪", "金崙","太麻里","知本", "康樂","三貂嶺","大華",
  "十分", "望古", "嶺腳", "平溪", "菁桐", "竹中", "上員", "榮華", "竹東", "橫山","九讚頭","合興", "富貴",
  "內灣", "二水", "源泉", "濁水", "龍泉", "集集", "水里", "車埕"
};


static char *siteno[MAX_SITE] =	/* 車站代碼 */
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
static char fr_no[5], to_no[5], tr_no[4];	/* 起迄 代碼 */
static char fr_tmh[3], to_tmh[3];		/* 起迄轉 時刻 */
static char fr_tms[3], to_tms[3];		/* 起迄轉 時刻 */
static char typer[5];				/* 對號/非對號 */


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
  move(b_lines, 30);	/* 置中 */
  prints("\033[%dm %s \033[m", color, msg);
}


static void
site_drawrow(x)		/* 重繪 x 列 */
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
  outs("\033[1;37;44m◎ 台鐵火車時刻表查詢系統 ◎\033[m");

  for (i = 0; i < MAX_SITE; i++)
  {
    if (i % SITE_PER_LINE == 0)
      move(i / SITE_PER_LINE + RAILWAY_XPOS, RAILWAY_YPOS);
    outs_site(i);
  }

  move(b_lines - 4, 0);
  outs("啟程車站：         到達車站：");
  move(b_lines - 3, 0);
  outs("開車時間：00:00 至 23:59");
  move(b_lines - 2, 0);
  outs("查詢車種：1)對號快車 2)非對號車 3)全部車種");

  cx = cy = 0;
  fr_pos = to_pos = tr_pos = -1;

}


static int		/* 1:成功 */
site_choose(flag)
  int flag;		/* 1:選起站  2:選迄站  4: 選轉站 */
{
  int pos;

  /* 這幾格沒有站台 */
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
  return 1;	/* 選下一個 */
}


static int		/* 1: 已決定起迄站 */
site_menu(flag)		/* 目前在選 1:起 4:迄 2:轉 站 */
  int flag;
{
  for (;;)
  {
    /* itoc.031209: 若遇到三個字的站名，又偵測全形時，那麼會多跳一次，暫時不管了 :p */
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
  outs_info(46, "請輸入查詢時間");
  if (!vget(b_lines - 3, 0, "開車時間：", fr_tmh, 3, DOECHO))
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

  if (!vget(b_lines - 3, 16, "至 ", to_tmh, 3, DOECHO))
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

  outs_info(45, "請選擇查詢車種");

  switch (vget(b_lines - 2, 0, "查詢車種：1)對號快車 2)非對號車 3)全部車種：[3] ", ans, 3, DOECHO))
  {
  case '1':
    strcpy(typer, "0");
    msg = "1)對號快車";
    break;

  case '2':
    strcpy(typer, "1");
    msg = "2)非對號車";
    break;

  default:
    strcpy(typer, "2");
    msg = "3)全部車種";
    break;
  }

  move(b_lines - 2, 10);
  outs(msg);
  clrtoeol();

  refresh();	/* 再次 vget() 會出現前面的控制碼，要 refresh */
}


static char
Tcolor(str)	/* 依車次來決定顏色 */
  char *str;
{
  if (!strncmp(str, "自強", 4))
    return '1';
  if (!strncmp(str, "莒光", 4))
    return '6';
  if (!strncmp(str, "復興", 4))
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
  int tcount;	/* 總共有幾班次 */
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
    "┌───┬──┬─┬───────┬─────┬─────┬──┬─────┐\n"
    "│ 乘坐 │車次│山│始發站->終點站│登車車站  │下車車站  │全票│行駛時間  │\n"
    "│ 車種 │編號│海│              │開車時間  │到達時間  │價錢│          │");

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

    if (!strcmp(result, "\n"))	/* 沒資料了 */
      break;

    fprintf(fpw, "\n├───┼──┼─┼───────┼─────┼─────┼──┼─────┤");

    /* 車型，6 bytes */
    fprintf(fpw, "\n│\033[1;4%c;37m%6.6s\033[m", Tcolor(result), result);	/* 冷氣柴客會超過 6 bytes */

    /* 車次代碼，4 bytes */
    if (!(result = strtok(NULL, "\n")))
      break;
    while (*result == ' ')	/* 解決查某些站的時候，長度會不一的問題 */
      result++;

    fprintf(fpw, "│%4.4s", result);		/* 冷氣柴客會超過 4 bytes */

    /* 山線或海線或空的，2 bytes */
    if (!fgets(data, sizeof(data), fpr) || !(result = strtok(data, "\n")))
      break;
    fprintf(fpw, "│%2s", result + 38);

    /* 啟程站、到達站，6 bytes */
    if (!fgets(data, sizeof(data), fpr) || !(result = strtok(data, "\n")))
      break;
    fprintf(fpw, "│%14s", result + 31);

    /* 開車時間 */
    if (!fgets(data, sizeof(data), fpr) || !(result = strtok(data, "\n")))
      break;
    fprintf(fpw, "│%8s  ", result + 32);

    /* 到達時間 */
    if (!fgets(data, sizeof(data), fpr) || !(result = strtok(data, "\n")))
      break;
    fprintf(fpw, "│%8s  ", result + 32);

#if 0
    /* 全票價格 3 bytes */
    if (!fgets(data, sizeof(data), fpr) || !(result = strtok(data, "\n")))
      break;
    fprintf(fpw, "│%4s", result + 34);
#else
    fprintf(fpw, "│%4s", "");
#endif

    /* 行駛時間 */
    if (!fgets(data, sizeof(data), fpr) || !(result = strtok(data, "\n")) || !(result = strtok( result, " ")))
      break;
    fprintf(fpw, "│%10s│", result);

    /* 跳過備註 */
    if (!fgets(data, sizeof(data), fpr))
      break;

    tcount++;
  }		/* end of while */


  fprintf(fpw,  "\n└───┴──┴─┴───────┴─────┴─────┴──┴─────┘\n\n\n\n");

  if (!tcount)
    fprintf(fpw, "\n\n抱歉！沒有您欲搭乘的班車");

  fclose(fpr);
  fclose(fpw);
}


static int
http_conn(s)
  char *s;
{
  char src[30], dst[30];
  char cmd[768];

  mouts(b_lines - 1, 0, "連接伺服器中，請稍候...");
    refresh();

  sprintf(src, "tmp/%s.rail", cuser.userid);
  sprintf(cmd, "/usr/local/bin/lynx -display_charset=utf-8 -nolist -dump  "
    "http://new.twtraffic.com.tw/twrail/nonscript_search_result.aspx?%s | "
    "/usr/local/bin/piconv -f utf8 -t big5 | /usr/bin/grep -v '圖示' > %s",
    s, src);
  system(cmd);

  sprintf(dst, "tmp/%s.way", cuser.userid);
  html_parser(src, dst);

  /* show message that return from the web server */


  if (more(dst, (char *) -1) >= 0)
  {
    if (vans("您要將查詢結果寄回信箱嗎？[N] ") == 'y')
      mail_self(dst, cuser.userid, "[備 忘 錄] 火車時刻查詢結果", MAIL_READ);
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

  outs_info(41, "請選擇啟程車站");
  if (!site_menu(1))
    return 0;

  outs_info(44, "請選擇到達車站");
  if (!site_menu(4))
    return 0;

  time_query();     /* 決定啟程/到達時間 */

  rail_query();	/* 決定對號/非對號 */

  ch = vans("查詢 0)今天 1)明天 2)後天 3456789)天後的時刻表 Q)離開？[0] ");
  if (ch == 'q')
  {
    vmsg("hmm.....不想查囉...^o^");
    return 0;
  }
  if (ch < '0' || ch > '9')
    ch = 0;
  else
    ch -= '0';

  railway(ch);

  return 0;
}

