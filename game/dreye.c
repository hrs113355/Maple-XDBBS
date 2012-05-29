/*-------------------------------------------------------*/
/* dreye.c    ( YZU WindTopBBS Ver 3.02 )                */
/*-------------------------------------------------------*/
/* target : Dreye Ķ��q�u�W�r��                         */
/* create : 01/07/09                                     */
/* update :   /  /                                       */
/* author : statue.bbs@bbs.yzu.edu.tw                    */
/*-------------------------------------------------------*/


#if 0

          ���q http://www.dreye.com/tw/dict/dict.phtml?w=hello&d=010300
        �ܤƧ� http://www.dreye.com/tw/dict/dict.phtml?w=hello&d=010301
 �P�q�r/�ϸq�r http://www.dreye.com/tw/dict/dict.phtml?w=hello&d=010304

#endif


#include "bbs.h"

#ifdef HAVE_NETTOOL

#define mouts(x,y,s)    { move(x, y); outs(s); }

#define HTTP_PORT       80
#define SERVER_dreye    "67.15.182.28"
#define CGI_dreye       "/tw/dict/dict.phtml"
#define REF             "http://www.dreye.com"


static void
url_encode(dst, src)    /* URL encoding */
  unsigned char *dst;   /* Thor.990331: �n src ���T���Ŷ� */
  unsigned char *src;
{
  for (; *src; src++)
  {
    if (*src == ' ')
      *dst++ = '+';
    else if (is_alnum(*src))
      *dst++ = *src;
    else
    {
      register cc = *src;
      *dst++ = '%';
      *dst++ = radix32[cc >> 4];
      *dst++ = radix32[cc & 0xf];
    }
  }
  *dst = '\0';
}


static int
http_conn(server, s)
  char *server, *s;
{
  int sockfd, start_show;
  int cc, tlen;
  char *xhead, *xtail, tag[10], fpath[50];
  static char pool[2048];
  FILE *fp;

  mouts(b_lines - 1, 0, "�s�����A�����A�еy��.............");
  refresh();

  if ((sockfd = dns_open(server, HTTP_PORT)) < 0)
  {
    vmsg("�L�k�P���A�����o�s���A�d�ߥ���");
    return -1;
  }

  write(sockfd, s, strlen(s));
  shutdown(sockfd, 1);

  /* parser return message from web server */
  xhead = pool;
  xtail = pool;
  tlen = 0;     /* tag length */
  start_show = 0;

  sprintf(fpath, "tmp/%s.dreye", cuser.userid);

  if (!(fp = fopen(fpath, "w")))
    return -1;

  fprintf(fp, "%-24s\033[1;37;44m  Dr.eye \033[31mi\033[37mDictionary "
    "�u�W�r��  \033[m\n%s", "", MSG_SEPERATOR);

  for (;;)
  {
    if (xhead >= xtail)
    {
      xhead = pool;
      cc = read(sockfd, xhead, sizeof(pool));
      if (cc <= 0)
        break;
      xtail = xhead + cc;
    }
    cc = *xhead++;

    /* �q </table> �U���}�l�L */
    if ((tlen == 7) && (!str_ncmp(tag, "/table", 6)))
      start_show = 1;

    /* �L�� <hr width ... ���� */
    if ((tlen == 3) && (!str_ncmp(tag, "hr", 2)))
      start_show = 0;

    if ((tlen == 3) && (!str_ncmp(tag, "td", 2)))
      fputc(' ', fp);

    if ((tlen == 4) && (!str_ncmp(tag, "div", 3)))
      fputc('\n', fp);

    if (cc == '<')
    {
      tlen = 1;
      continue;
    }

    if (tlen)
    {
      /* support<br>and<P>and</P> */

      if (cc == '>')
      {
        if ((tlen == 3) && (!str_ncmp(tag, "tr", 2)))
        {
          fputc('\n', fp);
        }
        else if ((tlen == 2) && (!str_ncmp(tag, "P", 1)))
        {
          fputc('\n', fp);
        }
        else if ((tlen == 3) && (!str_ncmp(tag, "br", 2)))
        {
          fputc('\n', fp);
        }
        tlen = 0;
        continue;
      }

      if (tlen <= 6)
      {
        tag[tlen - 1] = cc;
      }

      tlen++;
      continue;
    }

    if (start_show)
    {
      if (cc != '\r' && cc != '\n')
        fputc(cc, fp);
    }
  }
  close(sockfd);

  fprintf(fp, "\n\n%s\n%-26s\033[1;36;40mDr.eye \033[37miDictionary "
    "�u�W�r��\033[m\n", MSG_SEPERATOR, "");

  fclose(fp);

  more(fpath, NULL);
  unlink(fpath);
  return 0;
}


static int
dreye(word, ans)
  char *word, *ans;
{
  char atrn[256], sendform[512];
  char ue_word[90];

  url_encode(ue_word, word);

  sprintf(atrn, "w=%s&d=%d",
    ue_word, (ans[0] == '3') ? 10304 : (ans[0] == '2') ? 10301 : 10300);
  sprintf(sendform, "GET %s?%s HTTP/1.0\n\n", CGI_dreye, atrn);
  http_conn(SERVER_dreye, sendform);

  return 0;
}


int
main_dreye()
{
  char ans[3];
  char word[30];

  while (1)
  {
    clear();

    move(0, 25);
    outs("\033[1;37;44m�� DreyeĶ��q�u�W�r�� ��\033[m");
    move(3, 0);
    outs("���r��ӷ��� Dreye Ķ��q�u�W�r��C\n\n");
    outs("WWW: http://www.dreye.com/");

    if (!vget(8, 0, "�d�ߦr�J�G", word, 30, DOECHO))
      break;

    if (vget(10, 0, "1)�N�q 2)�ܤƧ� 3)�P�q�r/�ϸq�r Q)���} [1] ",
      ans, 3, LCECHO) == 'q')
      break;

    dreye(word, ans);
  }

  return 0;
}
#endif  /* HAVE_NETTOOL */
