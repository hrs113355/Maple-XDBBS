/*-------------------------------------------------------*/
/* cosign.c					         */
/*-------------------------------------------------------*/
/* target : cosign functions		                 */
/* Author : hrs@xdbbs.twbbs.org				 */
/* create : 2008/02/05                                   */
/*-------------------------------------------------------*/

#if 0

�зN�Ѧ�openPtt��voteboard

(reply������ code�j�j���Ѧ�voteboard)

���X�s�p(�ݪO�B�O�D�B�s�աB����)�\��

�Ʊ浲�XopenPtt�MM3���u�I���ץ�

�@�̬O�e�w�p����hrs (hrs@xdbbs.twbbs.org)

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
    static char * option[8] = {"�ӽзs�O", "�o���ݪO", "�s�p�O�D", "�}�K�O�D"
			 ,"�s�p�s�ժ�", "�}�K�s�ժ�", "�s�p����", "�o������" };
    FILE *fp;
    BRD * brd;

    clear();
    vs_head("�s�p�t��", NULL);
    
    move(2,0);
    outs("�w��ϥγs�p���A�ФťΨӴ��թθչϰ��X���F�s�p�����ơA�H�̱N�|�Q�C�J�¦W��C\n");
    for (i = 1;i < 9;i++)
    {
	prints("%d)%s ", i, option[i - 1]);
	if (i == 4 || i == 8)
	    outc('\n');
    }
    op = vans("�п�ܳs�p�Ҧ��G[Q] ") - '0';
    if (op < 1 || op > 8) return XO_HEAD;

    if (op == 1)
    {
         if (!vget(6, 0, "�^��O�W�G", brdname, BNLEN + 1, DOECHO))
	    return XO_HEAD;
         if (brd_bno(brdname) >= 0 || !valid_brdname(brdname))
         {
            vmsg("�w�����O�ΪO�W���X�k");
            return XO_HEAD;
         }
	
         if (!vget(7, 0, "�ݪO�����G", brdclass, BCLEN + 1, DOECHO) ||
             !vget(8, 0, "�ݪO�D�D�G", brdtitle, BTLEN + 1, DOECHO))
	    return XO_HEAD;
    }

    else if (op > 1 && op < 5)
    {

	 if (brd = ask_board(brdname, BRD_R_BIT | BRD_W_BIT, NULL))
         {
	    if (brd->battr & BRD_PERSONAL)
	    {
		/* hrs.080206: ����}�K�ӤH�O�O�D�μo���ӤH�O:P */
		vmsg("�o�ӪO�O�ӤH�O�C");
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
	if (!vget(6, 0, "�s�խ^��W�١G", classname, 15, DOECHO))
            return XO_HEAD;
	
	if (!vget(7, 0, "�s�դ���W�١G", classtitle, CH_TTLEN + 1, DOECHO))
            return XO_HEAD;
    }

    else 
    {
	if (!vget(6, 0, "���ʦW�١G", actname, 41, DOECHO))
            return XO_HEAD;

        if (!vget(7, 0, "²��(�@�y��)�G", acttitle, 61, DOECHO))
            return XO_HEAD;
    }

    if (op < 5 && op != 2)
    {
	if (!vget(9, 0, "�ݪO�O�D�G", brdbm, BMLEN + 1, DOECHO))
	    return XO_HEAD;
    }

    else if (op > 4 && op < 7)
    {
	if (!vget(9, 0, "�s�ժ��G", classbm, 21, DOECHO))
            return XO_HEAD;
    }
    
    vmsg("�}�l�s�� [�ݪO�����P�O�D��t�γs�p��]] (�аȥ��T���g)");

    sprintf(apath, "tmp/newbrd.%s.%ld", cuser.userid, time(NULL));   /* �s�p��]���Ȧs�ɮ� */
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

    fprintf(fp, "�@��: %s (%s) ����: �s�p�t��\n", cuser.userid, cuser.username);
    sprintf(buf, "[%s] %s", option[op - 1], (op > 4 ? (op > 6 ? actname : classname) :brdname));
    fprintf(fp, "���D: %s\n", buf);
    fprintf(fp, "�ɶ�: %s\n\n", Btime(&now));

    fprintf(fp, "%s\n\n", option[op - 1]);

    if (op < 5)
	fprintf(fp, "�^��O�W�G%s\n", brdname);
    if (op == 1)
    {
	fprintf(fp, "�ݪO�����G%s\n", brdclass);
	fprintf(fp, "�ݪO�D�D�G%s\n", brdtitle);
    }
    if (op < 5 && op != 2)
	fprintf(fp, "�O�D�W�١G%s\n", brdbm);
    
    if (op > 4 && op < 7)
    {
	fprintf(fp, "�s�խ^��W�١G%s\n", classname);
	fprintf(fp, "�s�դ���W�١G%s\n", classtitle);
	fprintf(fp, "�s   ��   ���G%s\n", classbm);
    }

    if (op > 6)
    {
        fprintf(fp, "���ʦW�١G%s\n", actname);
        fprintf(fp, "����²���G%s\n", acttitle);

    }

    fprintf(fp, "�� �� �H�G%s\n", cuser.userid);
    fprintf(fp, "�W�����ơG%d\t�o��峹�G%d\n", cuser.numlogins, cuser.numposts);
    fprintf(fp, "�q�l�H�c�G%s\n", cuser.email);
    fprintf(fp, "�s�p�����G\n");
    f_suck(fp, apath);
    unlink(apath);
    now += 86400 * 15;
    fprintf(fp, "�s�p�����ɶ��G(%ld) %s\n", now, Btime(&now));
    fprintf(fp, LINEBAR "���" LINEBAR "\n");
    fprintf(fp, LINEBAR "�Ϲ�" LINEBAR "\n");
    fclose(fp);
    
    add_post(currboard, fpath, buf, cuser.userid, cuser.username, POST_COSIGN);
    unlink(fpath);
    
    vmsg("�s�p�}�l�F!");
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
	vmsg("���g�s�p�w�B�z�����C");
	return XO_FOOT;
    }
    yes = no = 0;
    hdr_fpath(apath, xo->dir, hdr);
    sprintf(fpath, "tmp/reply.%s.%ld", cuser.userid, time(NULL));

    clear();
    vs_head("�s�p�t��", NULL);


    if (!(fp = fopen(apath, "r")))
    {
	vmsg("�L�kŪ���ɮסC");
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

        if (yes > 3) outs(buf);	    /* �峹���� */

	if (!strncmp(buf, "�s�p�����ɶ�", 12))    
	{            
	    ptr = strchr(buf, '(');
	    if (!ptr)
		return XO_HEAD;
	    sscanf(ptr + 1, "%ld", &end);
	    if (end < time(NULL)) 
	    {
	        vmsg("�s�p�ɶ��w�L");
	        fclose(fp);
	        return XO_HEAD;
	    }
	}

	if(yes >= 0) continue;

        if (!strncmp(buf + 4, cuser.userid, strlen(cuser.userid))) 
	{
	    vget(17, 0, "�n�ק�z���e���s�p�ܡH(Y/N) [N]", op, 4, LCECHO);
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
           if (!vget(19, 0, "�аݱz (Y)��� (N)�Ϲ� �o��ĳ�D�G", op, 4, LCECHO)) 
             return XO_HEAD;				        
        } while (op[0] != 'y' && op[0] != 'n');

	if (!vget(20, 0, "�z���z�ѬO�G", reason, 35, GCARRY)) 
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
        fprintf(fw, "�s�p�����ɶ�: (%ld)%s\n", end, Btime(&end));
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
        fprintf(fw, "%3d.%-15s%-34s �ӷ�:%-20.20s\n", ++yes, cuser.userid, reason, str_ip(cutmp->in_addr));
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
        fprintf(fw, "%3d.%-15s%-34s �ӷ�:%-20.20s\n", ++no, cuser.userid, reason, str_ip(cutmp->in_addr));
    fprintf(fw, LINEBAR "�`�p" LINEBAR "\n");
    fprintf(fw, "����H��:%-9d�Ϲ�H��:%-9d\n", yes, no);

    f_unlock(fd);
    close(fd);
    fclose(fp);
    fclose(fw);
    unlink(apath);
    f_mv(fpath, apath);
    return XO_HEAD;
}
