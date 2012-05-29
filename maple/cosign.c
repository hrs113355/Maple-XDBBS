/*-------------------------------------------------------*/
/* cosign.c					         */
/*-------------------------------------------------------*/
/* target : cosign functions		                 */
/* Author : hrs@xdbbs.twbbs.org				 */
/* create : 2008/02/05                                   */
/*-------------------------------------------------------*/

#if 0

創意參考openPtt的voteboard

(reply的部份 code大大的參考voteboard)

做出連署(看板、板主、群組、活動)功能

希望結合openPtt和M3的優點做修正

作者是叉滴小站的hrs (hrs@xdbbs.twbbs.org)

#endif

#include "bbs.h"

#define LINEBAR "----------"
extern char xo_pool[];

int
do_cosign(void)
{
    time_t now;
    int op,fd;
    char brdname[BNLEN + 1],brdclass[BCLEN + 1],brdtitle[BTLEN + 1];
    char brdbm[BMLEN + 1],classname[14 + 1],classtitle[CH_TTLEN + 1],classbm[20 + 1];
    char actname[40 + 1],acttitle[60 + 1];
    char fpath[64],apath[64];
    char buf[80];
    register int i;
    static char * option[8] = {"申請新板", "廢除看板", "連署板主", "罷免板主"
			 ,"連署群組長", "罷免群組長", "連署活動", "廢除活動" };
    FILE *fp;
    BRD * brd;

    clear();
    vs_head("連署系統", NULL);
    
    move(2,0);
    outs("歡迎使用連署機，請勿用來測試或試圖做出欺騙連署機的事，違者將會被列入黑名單。\n");
    for (i = 1;i < 9;i++)
    {
	prints("%d)%s ", i, option[i - 1]);
	if (i == 4 || i == 8)
	    outc('\n');
    }
    op = vans("請選擇連署模式：[Q] ") - '0';
    if (op < 1 || op > 8) return XO_HEAD;

    if (op == 1)
    {
         if (!vget(6, 0, "英文板名：", brdname, BNLEN + 1, DOECHO))
	    return XO_HEAD;
         if (brd_bno(brdname) >= 0 || !valid_brdname(brdname))
         {
            vmsg("已有此板或板名不合法");
            return XO_HEAD;
         }
	
         if (!vget(7, 0, "看板分類：", brdclass, BCLEN + 1, DOECHO) ||
             !vget(8, 0, "看板主題：", brdtitle, BTLEN + 1, DOECHO))
	    return XO_HEAD;
    }

    else if (op > 1 && op < 5)
    {

	 if (brd = ask_board(brdname, BRD_R_BIT | BRD_W_BIT, NULL))
         {
	    if (brd->battr & BRD_PERSONAL)
	    {
		/* hrs.080206: 不准罷免個人板板主或廢除個人板:P */
		vmsg("這個板是個人板。");
		return XO_HEAD;
	    }
	 }
	 else
	 {
	    vmsg(err_bid);
	    return XO_HEAD;
	 }

    }

    else if (op > 4 && op < 7)
    {
	if (!vget(6, 0, "群組英文名稱：", classname, 15, DOECHO))
            return XO_HEAD;
	
	if (!vget(7, 0, "群組中文名稱：", classtitle, CH_TTLEN + 1, DOECHO))
            return XO_HEAD;
    }

    else 
    {
	if (!vget(6, 0, "活動名稱：", actname, 41, DOECHO))
            return XO_HEAD;

        if (!vget(7, 0, "簡介(一句話)：", acttitle, 61, DOECHO))
            return XO_HEAD;
    }

    if (op < 5 && op != 2)
    {
	if (!vget(9, 0, "看板板主：", brdbm, BMLEN + 1, DOECHO))
	    return XO_HEAD;
    }

    else if (op > 4 && op < 7)
    {
	if (!vget(9, 0, "群組長：", classbm, 21, DOECHO))
            return XO_HEAD;
    }
    
    vmsg("開始編輯 [看板說明與板主抱負或連署原因] (請務必確實填寫)");

    sprintf(apath, "tmp/newbrd.%s.%ld", cuser.userid, time(NULL));   /* 連署原因的暫存檔案 */
    strcpy(fpath, apath);
    strcat(fpath, "2");

    if (fd = vedit(apath, 0))
    {
       unlink(apath);
       vmsg(msg_cancel);
       return XO_HEAD;
    }
    
    time(&now);

    fp = fopen(fpath, "a");

    fprintf(fp, "作者: %s (%s) 站內: 連署系統\n", cuser.userid, cuser.username);
    sprintf(buf, "[%s] %s", option[op - 1], (op > 4 ? (op > 6 ? actname : classname) :brdname));
    fprintf(fp, "標題: %s\n", buf);
    fprintf(fp, "時間: %s\n\n", Btime(&now));

    fprintf(fp, "%s\n\n", option[op - 1]);

    if (op < 5)
	fprintf(fp, "英文板名：%s\n", brdname);
    if (op == 1)
    {
	fprintf(fp, "看板分類：%s\n", brdclass);
	fprintf(fp, "看板主題：%s\n", brdtitle);
    }
    if (op < 5 && op != 2)
	fprintf(fp, "板主名稱：%s\n", brdbm);
    
    if (op > 4 && op < 7)
    {
	fprintf(fp, "群組英文名稱：%s\n", classname);
	fprintf(fp, "群組中文名稱：%s\n", classtitle);
	fprintf(fp, "群   組   長：%s\n", classbm);
    }

    if (op > 6)
    {
        fprintf(fp, "活動名稱：%s\n", actname);
        fprintf(fp, "活動簡介：%s\n", acttitle);

    }

    fprintf(fp, "申 請 人：%s\n", cuser.userid);
    fprintf(fp, "上站次數：%d\t發表文章：%d\n", cuser.numlogins, cuser.numposts);
    fprintf(fp, "電子信箱：%s\n", cuser.email);
    fprintf(fp, "連署說明：\n");
    f_suck(fp, apath);
    unlink(apath);
    now += 86400 * 15;
    fprintf(fp, "連署結束時間：(%ld) %s\n", now, Btime(&now));
    fprintf(fp, LINEBAR "支持" LINEBAR "\n");
    fprintf(fp, LINEBAR "反對" LINEBAR "\n");
    fclose(fp);
    
    add_post(currboard, fpath, buf, cuser.userid, cuser.username, POST_COSIGN);
    unlink(fpath);
    
    vmsg("連署開始了!");
    return XO_INIT;
}

int
cosign_reply(xo, hdr)
XO *xo;   
HDR *hdr;
{
    char apath[64], fpath[64], buf[256],reason[40] = "", op[5];
    char *ptr;
    FILE *fp,*fw;
    int yes,no,fd,len;
    time_t end;

    if (hdr->xmode & (POST_MARKED | POST_DONE))
    {
	vmsg("本篇連署已處理完畢。");
	return XO_FOOT;
    }
    yes = no = 0;
    hdr_fpath(apath, xo->dir, hdr);
    sprintf(fpath, "tmp/reply.%s.%ld", cuser.userid, time(NULL));

    clear();
    vs_head("連署系統", NULL);


    if (!(fp = fopen(apath, "r")))
    {
	vmsg("無法讀取檔案。");
	return XO_HEAD;
    }

    while (fgets(buf, 256, fp))
    {
        if (yes >= 0)
        {
            if (!strncmp(buf, LINEBAR, 10))
	    {
		yes=-1; 
		continue;
	    }
            else
               yes++;
        }

        if (yes > 3) outs(buf);	    /* 文章正文 */

	if (!strncmp(buf, "連署結束時間", 12))    
	{            
	    ptr = strchr(buf, '(');
	    if (!ptr)
		return XO_HEAD;
	    sscanf(ptr + 1, "%ld", &end);
	    if (end < time(NULL)) 
	    {
	        vmsg("連署時間已過");
	        fclose(fp);
	        return XO_HEAD;
	    }
	}

	if(yes >= 0) continue;

        if (!strncmp(buf + 4, cuser.userid, strlen(cuser.userid))) 
	{
	    vget(17, 0, "要修改您之前的連署嗎？(Y/N) [N]", op, 4, LCECHO);
            if (op[0] != 'y') 
	    {
		fclose(fp);
	        return XO_HEAD;
	    }
	 }
    }
	fclose(fp);

        do 
	{
           if (!vget(19, 0, "請問您 (Y)支持 (N)反對 這個議題：", op, 4, LCECHO)) 
             return XO_HEAD;				        
        } while (op[0] != 'y' && op[0] != 'n');

	if (!vget(20, 0, "您的理由是：", reason, 35, GCARRY)) 
		return XO_HEAD;
	if ((fd = open(apath, O_RDONLY)) == -1)
        	return XO_HEAD;
	f_exlock(fd);

	if(!(fp = fopen(apath, "r")))
        {
	    f_unlock(fd); 
	    close(fd); 
	    return XO_HEAD;
	}
       if(!(fw = fopen(fpath, "w")))
       {
        f_unlock(fd);
        close(fd);
        fclose(fp);
        return XO_HEAD;
       }

    while (fgets(buf, 256, fp)) 
    {
        if (!strncmp(LINEBAR, buf, 10))
            break;
        fputs(buf, fw);
    }

    if (!end) 
    {
	end = time(NULL) + 14 * 86400;
        fprintf(fw, "連署結束時間: (%ld)%s\n", end, Btime(&end));
    }

    fputs(buf, fw);
    len = strlen(cuser.userid);

    for(yes = 0; fgets(buf, 256, fp);) 
    {
        if (!strncmp(LINEBAR, buf, 10))
            break;
        if (strlen(buf)<30 || (buf[4+len]==' ' && !strncmp(buf + 4, cuser.userid, len)))
            continue;
        fprintf(fw, "%3d.%s", ++yes, buf + 4);
    }

    if (op[0] == 'y')
        fprintf(fw, "%3d.%-15s%-34s 來源:%-20.20s\n", ++yes, cuser.userid, reason, str_ip(cutmp->in_addr));
    fputs(buf, fw);

    for(no = 0; fgets(buf, 256, fp);) 
    {
        if (!strncmp(LINEBAR, buf, 10))
            break;
        if (strlen(buf)<30 || (buf[4+len]==' ' && !strncmp(buf + 4, cuser.userid, len)))
            continue;
        fprintf(fw, "%3d.%s", ++no, buf + 4);
    }

    if (op[0] == 'n')
        fprintf(fw, "%3d.%-15s%-34s 來源:%-20.20s\n", ++no, cuser.userid, reason, str_ip(cutmp->in_addr));
    fprintf(fw, LINEBAR "總計" LINEBAR "\n");
    fprintf(fw, "支持人數:%-9d反對人數:%-9d\n", yes, no);

    f_unlock(fd);
    close(fd);
    fclose(fp);
    fclose(fw);
    unlink(apath);
    f_mv(fpath, apath);
    return XO_HEAD;
}
